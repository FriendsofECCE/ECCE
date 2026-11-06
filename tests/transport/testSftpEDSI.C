/*
 * testSftpEDSI ops|twowriters|dead <sftp-url-of-an-existing-folder>
 * testSftpEDSI local <copy-of-that-folder>
 *
 * SftpEDSI against a real sshd (tests/transport/sshd/sftpedsi_test.sh):
 * projects, calculations, metadata, data, listing, move, copy and remove;
 * two processes updating one directory's metadata; a dead server.
 * PASS/FAIL line per check; exit 1 if any failed.
 */
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
using namespace std;

#include <sys/wait.h>
#include <unistd.h>

#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "util/EcceURL.H"

static int failures = 0;

static void check(bool ok, const string& name, const string& why = "")
{
  if (ok) cout << "PASS " << name << endl;
  else { cout << "FAIL " << name << ": " << why << endl; failures++; }
}

static EDSI *edsi(const string& url) { return EDSIFactory::getEDSI(EcceURL(url)); }

static MetaDataResult prop(const string& n, const string& v)
{
  MetaDataResult m; m.name = n; m.type = "string"; m.value = v; return m;
}

static string getProp(const string& url, const string& name)
{
  EDSI *e = edsi(url);
  vector<MetaDataRequest> r(1);
  r[0].name = name;
  vector<MetaDataResult> out;
  e->getMetaData(r, out);
  delete e;
  return out.empty() ? "" : out[0].value;
}

static bool setProp(const string& url, const string& n, const string& v)
{
  EDSI *e = edsi(url);
  vector<MetaDataResult> r(1, prop(n, v));
  bool ok = e->putMetaData(r);
  delete e;
  return ok;
}

static string getData(const string& url)
{
  EDSI *e = edsi(url);
  ostringstream os;
  bool ok = e->getDataSet(os);
  delete e;
  return ok ? os.str() : string("<<failed>>");
}

static bool putData(const string& url, const string& d)
{
  EDSI *e = edsi(url);
  istringstream in(d);
  bool ok = e->putDataSet(in);
  delete e;
  return ok;
}

static string make(const string& parent, const string& name, bool dir)
{
  EDSI *e = edsi(parent);
  EcceURL *u = dir ? e->makeCollection(name) : e->makeDataSet(name);
  string r = u ? u->toString(false) : "";
  delete u;
  delete e;
  return r;
}

static bool exists(const string& url)
{
  EDSI *e = edsi(url);
  bool r = e->exists();
  delete e;
  return r;
}

static vector<ResourceMetaDataResult> list(const string& url)
{
  EDSI *e = edsi(url);
  vector<ResourceMetaDataResult> r;
  e->listCollection(r);
  delete e;
  return r;
}

static bool listed(const vector<ResourceMetaDataResult>& l, const string& url)
{
  for (size_t i = 0; i < l.size(); i++) if (l[i].url.toString(false) == url) return true;
  return false;
}

static int ops(const string& base)
{
  string proj = make(base, "proj", true);
  check(proj == base + "/proj", "makeCollection url", proj);
  string calc = make(proj, "calc", true);
  check(calc == proj + "/calc", "makeCollection nested", calc);
  check(make(proj, "calc", true) == proj + "/calc-1", "makeCollection unique name");
  check(exists(calc) && !exists(proj + "/nothing"), "exists");

  check(setProp(calc, "ecce:state", "created"), "putMetaData on a directory");
  check(getProp(calc, "ecce:state") == "created", "getMetaData on a directory");
  check(setProp(calc, "ecce:state", "completed") && getProp(calc, "ecce:state") == "completed",
        "metadata update");
  check(setProp(proj, "ecce:note", "two\nlines\twith tab"), "metadata with newline");
  check(getProp(proj, "ecce:note") == "two\nlines\twith tab", "metadata value escaped");

  string input = make(calc, "input.nw", false);
  check(input == calc + "/input.nw", "makeDataSet", input);
  check(putData(input, "geometry\n O 0 0 0\nend\n") && getData(input) == "geometry\n O 0 0 0\nend\n",
        "put and get text");
  check(setProp(input, "ecce:kind", "input") && getProp(input, "ecce:kind") == "input",
        "metadata on a file");
  string bin;
  for (int i = 0; i < 70000; i++) bin += (char)(i * 7 % 256);
  bin += "\r\n\r\n";
  string bf = calc + "/data.bin";
  check(putData(bf, bin) && getData(bf) == bin, "binary with NUL and CRLF, 70 KB");
  {
    EDSI *e = edsi(bf);
    check(e->getDataSetSize() == bin.size(), "getDataSetSize");
    ostringstream os;
    e->getDataSubSet(os, 10, 20);
    check(os.str() == bin.substr(10, 20), "getDataSubSet");
    delete e;
  }
  {
    string lg = calc + "/run.log";
    putData(lg, "line1\nline2\n");
    EDSI *e = edsi(lg);
    e->appendDataSet("tail\n", 0);
    delete e;
    check(getData(lg) == "line1\nline2\ntail\n", "append");
    e = edsi(lg);
    e->appendDataSet("XY\n", 5);
    delete e;
    check(getData(lg) == "line1\nline2\nXY\n", "append over the last 5 bytes");
  }
  {
    vector<ResourceMetaDataResult> l = list(calc);
    check(listed(l, input) && listed(l, bf), "listCollection urls", "");
    bool sized = false;
    for (size_t i = 0; i < l.size(); i++)
      if (l[i].url.toString(false) == bf)
        for (size_t m = 0; m < l[i].metaData.size(); m++)
          if (l[i].metaData[m].name == "DAV:getcontentlength")
            sized = l[i].metaData[m].value == to_string(bin.size());
    check(sized, "listing carries sizes");
    check(!listed(l, calc + "/.ecce-meta") && l.size() == 3, "listing hides dot files");
    vector<ResourceResult> rr;
    EDSI *e = edsi(proj);
    e->listCollection(rr);
    delete e;
    check(rr.size() == 2 && rr[0].url.toString(false).compare(0, 7, "sftp://") == 0, "list ResourceResult");
  }

  string calc2 = proj + "/calc-copy";
  {
    EDSI *e = edsi(calc);
    EcceURL t(calc2);
    check(e->copyResource(t), "copyResource directory");
    delete e;
  }
  check(getProp(calc2, "ecce:state") == "completed" && getData(calc2 + "/data.bin") == bin &&
        getProp(calc2 + "/input.nw", "ecce:kind") == "input", "copy carries data and metadata");
  {
    EDSI *e = edsi(calc2);
    EcceURL t(calc2);
    check(!e->copyResource(t), "copy onto an existing name refused");
    delete e;
  }
  {
    EDSI *e = edsi(input);
    EcceURL t(calc + "/in2.nw");
    check(e->moveResource(t), "moveResource file");
    delete e;
  }
  check(!exists(input) && getData(calc + "/in2.nw").compare(0, 8, "geometry") == 0 &&
        getProp(calc + "/in2.nw", "ecce:kind") == "input" && getProp(input, "ecce:kind") == "",
        "move carries the file's metadata");
  {
    EDSI *e = edsi(calc + "/in2.nw");
    EcceURL t(calc2 + "/in3.nw");
    check(e->copyResource(t), "copyResource file across directories");
    delete e;
    check(getProp(calc2 + "/in3.nw", "ecce:kind") == "input", "file copy carries metadata");
  }
  {
    EDSI *e = edsi(calc2);
    EcceURL t(proj + "/calc-moved");
    check(e->moveResource(t) && getProp(t.toString(false), "ecce:state") == "completed" &&
          !exists(calc2), "moveResource directory");
    delete e;
  }
  {
    EDSI *e = edsi(calc + "/in2.nw");
    check(e->removeResource() && !exists(calc + "/in2.nw") &&
          getProp(calc + "/in2.nw", "ecce:kind") == "", "removeResource file and its metadata");
    delete e;
    e = edsi(proj + "/calc-moved");
    check(e->removeResource() && !exists(proj + "/calc-moved"), "removeResource directory");
    delete e;
    e = edsi(proj + "/gone");
    check(!e->removeResource(), "removeResource of nothing fails");
    delete e;
  }
  {
    EDSI *e = edsi(proj);
    vector<EcceURL> m;
    e->efind("", "calc", EcceURL(proj), m);
    check(m.size() >= 2 && m[0].toString(false).compare(0, 7, "sftp://") == 0, "efind");
    check(e->isWritable(), "isWritable");
    delete e;
  }
  return failures ? 1 : 0;
}

// Two processes, one directory's metadata, no lost update.
static int twowriters(const string& base)
{
  string dir = base + "/shared";
  const int N = 12;
  pid_t pids[2];
  for (int w = 0; w < 2; w++) {
    pids[w] = fork();
    if (pids[w] == 0) {
      int bad = 0;
      for (int i = 0; i < N; i++)
        if (!setProp(dir, "ecce:w" + to_string(w) + "_" + to_string(i), "v")) bad++;
      _exit(bad ? 1 : 0);
    }
  }
  int fails = 0;
  for (int w = 0; w < 2; w++) { int st = 0; waitpid(pids[w], &st, 0); if (!WIFEXITED(st) || WEXITSTATUS(st)) fails++; }
  check(fails == 0, "both writers finished");
  int found = 0;
  for (int w = 0; w < 2; w++)
    for (int i = 0; i < N; i++)
      if (getProp(dir, "ecce:w" + to_string(w) + "_" + to_string(i)) == "v") found++;
  check(found == 2 * N, "no lost update", to_string(found) + " of " + to_string(2 * N));
  return failures ? 1 : 0;
}

static int dead(const string& base)
{
  time_t t0 = time(0);
  EDSI *e = edsi(base);
  bool ok = e->exists();
  string msg = e->m_msgStack.getMessage();
  bool put = e->putMetaData(vector<MetaDataResult>(1, prop("ecce:x", "y")));
  delete e;
  long secs = (long)(time(0) - t0);
  check(!ok && !put, "operations on a dead server fail");
  check(!msg.empty(), "the failure carries a message", msg);
  cout << "message: " << msg << endl;
  check(secs < 90, "bounded wait", to_string(secs) + " s");
  return failures ? 1 : 0;
}

int main(int argc, char **argv)
{
  if (argc < 3) { cerr << "usage: testSftpEDSI ops|twowriters|dead|local url" << endl; return 2; }
  string mode = argv[1], url = argv[2];
  if (mode == "ops") return ops(url);
  if (mode == "twowriters") return twowriters(url);
  if (mode == "dead") return dead(url);
  if (mode == "local") {
    // The folder copied out of the server reads as a local data folder.
    string calc = "file://" + url + "/proj/calc";
    check(getProp(calc, "ecce:state") == "completed", "local read of the server's metadata");
    check(getProp("file://" + url + "/proj", "ecce:note") == "two\nlines\twith tab", "local read of a value with escapes");
    return failures ? 1 : 0;
  }
  return 2;
}
