/*
 * filedsiTest <scratch-dir>
 *
 * FileEDSI against a directory, through EDSIFactory with a file:// URL and
 * no data server: metadata in the per-directory sidecar, append and ranged
 * reads, and the sidecar following a copy, move or remove.  Prints one
 * "PASS name" or "FAIL name: why" line per check; exit 1 if any failed.
 */
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

#include <unistd.h>
#include <sys/stat.h>

#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "util/EcceURL.H"

static int failures = 0;

static void check(bool ok, const string& name, const string& why = "")
{
  if (ok) cout << "PASS " << name << endl;
  else { cout << "FAIL " << name << ": " << why << endl; failures++; }
}

static EDSI *edsi(const string& path)
{
  return EDSIFactory::getEDSI(EcceURL("file://" + path));
}

static string slurp(const string& path)
{
  ifstream in(path.c_str(), ios::binary);
  ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

static bool put(const string& path, const string& text)
{
  EDSI *e = edsi(path);
  bool ok = e->putDataSet(text.c_str());
  delete e;
  return ok;
}

static MetaDataResult prop(const string& n, const string& v)
{
  MetaDataResult m;
  m.name = n;
  m.type = "string";
  m.value = v;
  return m;
}

static MetaDataRequest req(const string& n)
{
  MetaDataRequest r;
  r.name = n;
  return r;
}

// "" when the property is absent.
static string getProp(const string& path, const string& name, bool *found = 0)
{
  EDSI *e = edsi(path);
  vector<MetaDataRequest> rq(1, req(name));
  vector<MetaDataResult> res;
  e->getMetaData(rq, res);
  delete e;
  if (found) *found = !res.empty();
  return res.empty() ? string() : res[0].value;
}

int main(int argc, char **argv)
{
  if (argc < 2) { cerr << "usage: filedsiTest <scratch-dir>" << endl; return 2; }
  string root = argv[1];
  string NS = "ecce:";
  string tricky = "line one\nline two\t<a href=\"x\">&amp; 1 < 2 & \\ end</a>\r\n";

  // Collection and document.
  EDSI *top = edsi(root);
  EcceURL *coll = top->makeCollection("proj");
  check(coll != 0, "makeCollection");
  if (!coll) return 1;
  string proj = coll->getPath();
  EDSI *pe = edsi(proj);
  EcceURL *doc = pe->makeDataSet("calc.nw");
  check(doc != 0, "makeDataSet");
  if (!doc) return 1;
  string file = doc->getPath();
  delete top; delete pe;

  // putMetaData / getMetaData round trip.
  {
    EDSI *e = edsi(file);
    vector<MetaDataResult> in;
    in.push_back(prop(NS + "state", "created"));
    in.push_back(prop(NS + "annotation", tricky));
    check(e->putMetaData(in), "putMetaData file");
    delete e;
    check(getProp(file, NS + "state") == "created", "get state");
    check(getProp(file, NS + "annotation") == tricky, "get value with newlines, < and &",
          getProp(file, NS + "annotation"));
    bool found = true;
    getProp(file, NS + "nosuch", &found);
    check(!found, "unset property is absent");
  }

  // The collection's own properties live apart from the child's.
  {
    EDSI *e = edsi(proj);
    vector<MetaDataResult> in(1, prop(NS + "name", "My Project"));
    check(e->putMetaData(in), "putMetaData collection");
    delete e;
    check(getProp(proj, NS + "name") == "My Project", "get collection property");
    bool found = true;
    getProp(file, NS + "name", &found);
    check(!found, "collection property not on child");
  }

  // Stored value overrides the built-in default (application is "").
  {
    check(getProp(file, "application") == "", "application default empty");
    EDSI *e = edsi(file);
    vector<MetaDataResult> in(1, prop("application", "NWChem"));
    e->putMetaData(in);
    delete e;
    check(getProp(file, "application") == "NWChem", "stored overrides default");
  }

  // listCollection with requests returns the stored property; the sidecar
  // itself is not listed.
  {
    EDSI *e = edsi(proj);
    vector<MetaDataRequest> rq;
    rq.push_back(req(NS + "state"));
    rq.push_back(req("application"));
    rq.push_back(req("DAV:getcontentlength"));
    vector<ResourceMetaDataResult> res;
    bool ok = e->listCollection(rq, res);
    delete e;
    check(ok && res.size() == 1, "listCollection hides sidecar",
          "entries: " + string(1, '0' + (char)res.size()));
    string st, app;
    if (res.size() == 1) {
      for (size_t i = 0; i < res[0].metaData.size(); i++) {
        if (res[0].metaData[i].name == NS + "state") st = res[0].metaData[i].value;
        if (res[0].metaData[i].name == "application") app = res[0].metaData[i].value;
      }
    }
    check(st == "created", "listCollection returns stored property", st);
    check(app == "NWChem", "listCollection stored overrides default", app);
  }

  // removeMetaData, including of a missing property.
  {
    EDSI *e = edsi(file);
    vector<MetaDataRequest> rq;
    rq.push_back(req(NS + "state"));
    check(e->removeMetaData(rq), "removeMetaData");
    vector<MetaDataRequest> missing(1, req(NS + "never-set"));
    check(e->removeMetaData(missing), "removeMetaData of missing property succeeds");
    delete e;
    bool found = true;
    getProp(file, NS + "state", &found);
    check(!found, "removed property gone");
    check(getProp(file, NS + "annotation") == tricky, "others survive removal");
  }

  // Copy and move carry the properties; remove deletes them.
  {
    string copyPath = proj + "/copy.nw";
    EDSI *e = edsi(file);
    EcceURL target("file://" + copyPath);
    check(e->copyResource(target), "copyResource file");
    delete e;
    check(getProp(copyPath, NS + "annotation") == tricky, "copy carries properties");
    check(getProp(file, NS + "annotation") == tricky, "copy keeps source properties");

    string movedPath = proj + "/moved.nw";
    e = edsi(copyPath);
    EcceURL mtarget("file://" + movedPath);
    check(e->moveResource(mtarget), "moveResource file");
    delete e;
    check(getProp(movedPath, NS + "annotation") == tricky, "move carries properties");
    string side = slurp(proj + "/.ecce-meta");
    check(side.find("copy.nw") == string::npos, "move leaves no record of old name");

    e = edsi(movedPath);
    check(e->removeResource(), "removeResource file");
    delete e;
    side = slurp(proj + "/.ecce-meta");
    check(side.find("moved.nw") == string::npos, "remove deletes record");

    // A directory takes its subtree's properties with it.
    string sub = proj + "/sub";
    e = edsi(proj);
    EcceURL *subUrl = e->makeCollection("sub");
    delete e;
    string inner = subUrl->getPath() + "/in.txt";
    put(inner, "x");
    e = edsi(inner);
    vector<MetaDataResult> in(1, prop(NS + "k", "v"));
    e->putMetaData(in);
    delete e;
    e = edsi(subUrl->getPath());
    e->putMetaData(vector<MetaDataResult>(1, prop(NS + "dirk", "dv")));
    EcceURL dcopy("file://" + proj + "/sub2");
    check(e->copyResource(dcopy), "copyResource directory");
    delete e;
    check(getProp(proj + "/sub2/in.txt", NS + "k") == "v", "directory copy carries child properties");
    check(getProp(proj + "/sub2", NS + "dirk") == "dv", "directory copy carries own properties");
    e = edsi(proj + "/sub2");
    EcceURL dmove("file://" + proj + "/sub3");
    check(e->moveResource(dmove), "moveResource directory");
    delete e;
    check(getProp(proj + "/sub3/in.txt", NS + "k") == "v", "directory move carries properties");
    e = edsi(proj + "/sub3");
    check(e->removeResource(), "removeResource directory");
    delete e;
    check(access((proj + "/sub3").c_str(), F_OK) != 0, "directory removed");
    delete subUrl;
  }

  // appendDataSet and ranged reads.
  {
    string data = proj + "/data.xml";
    check(put(data, "<a>\n</a>"), "putDataSet");
    EDSI *e = edsi(data);
    check(e->appendDataSet("<b/>\n</a>", 4), "appendDataSet overwrite 4");
    check(slurp(data) == "<a>\n<b/>\n</a>", "overwrite removes trailing bytes", slurp(data));
    check(e->appendDataSet("tail"), "appendDataSet no overwrite");
    check(slurp(data) == "<a>\n<b/>\n</a>tail", "append at end", slurp(data));
    istringstream more("++");
    check(e->appendDataSet(more, 2), "appendDataSet(istream) overwrite 2");
    check(slurp(data) == "<a>\n<b/>\n</a>ta++", "stream append", slurp(data));
    check(e->getDataSetSize() == slurp(data).size(), "getDataSetSize");

    istream *sub = e->getDataSubSet(4, 4);
    string s;
    if (sub) { ostringstream o; o << sub->rdbuf(); s = o.str(); delete sub; }
    check(s == "<b/>", "getDataSubSet(istream)", s);
    ostringstream os;
    check(e->getDataSubSet(os, 10, 100) && os.str() == slurp(data).substr(10), "getDataSubSet(ostream) clipped at EOF", os.str());
    ostringstream all;
    e->getDataSet(all);
    check(all.str() == slurp(data), "getDataSet is byte exact");
    check(e->isWritable(), "isWritable");
    delete e;

    // Append to a file that does not exist yet creates it.
    string fresh = proj + "/fresh.log";
    e = edsi(fresh);
    check(e->appendDataSet("hello"), "append creates file");
    check(slurp(fresh) == "hello", "created content");
    delete e;
  }

  cout << (failures ? "FAILED " : "ALL OK ") << failures << " failure(s)" << endl;
  return failures ? 1 : 0;
}
