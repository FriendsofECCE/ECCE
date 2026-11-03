// A copy from a host that cannot be asked about its files (dead login,
// timeout) must fail, not report success with nothing copied (#732): the job
// store then stored an empty output for a finished job.  A host that
// answers "no such file" still only skips that file, as scp did.

#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

#define private public
#include "comm/RCommand.H"
#undef private
#include "comm/RemoteTransport.H"

using namespace std;

static int failures = 0;
static void check(const string& name, bool ok)
{
  cout << (ok ? "ok   " : "FAIL ") << name << endl;
  if (!ok) failures++;
}

// Files the "host" has, and whether it answers at all.
class FakeTransport : public RemoteTransport {
public:
  map<string, string> files;
  bool answers = true;
  string h = "host", u = "user", j;

  TransportResult run(const string&, int) override
  { TransportResult r; r.error = "timed out"; r.timedOut = true; return r; }
  long spawnDetached(const string&, string& e, const string&) override
  { e = "no"; return -1; }
  const string& host() const override { return h; }
  const string& user() const override { return u; }
  const string& jumpHost() const override { return j; }
  bool nested() const override { return false; }
  bool connect(string&) override { return true; }
  void disconnect() override {}
  bool connected() const override { return true; }
  bool put(const string&, const string&, string&) override { return false; }
  bool get(const string& r, const string& l, string& e) override
  {
    if (!answers || !files.count(r)) { e = "cannot read " + r; return false; }
    FILE* f = fopen(l.c_str(), "w");
    if (!f) return false;
    fputs(files[r].c_str(), f);
    fclose(f);
    return true;
  }
  bool putTree(const string&, const string&, string&) override { return false; }
  bool getTree(const string& r, const string& l, string& e) override
  {
    struct stat sb;
    string t = l;
    if (stat(l.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))
      t = l + "/" + r.substr(r.rfind('/') + 1);
    return get(r, t, e);
  }
  int remoteKind(const string& p) override
  {
    if (!answers) return -2;
    return files.count(p) ? 0 : -1;
  }
  bool remoteGlob(const string& pat, vector<string>& out, string& e) override
  {
    out.clear();
    if (pat.find('*') == string::npos) { out.push_back(pat); return true; }
    if (!answers) { e = "cannot expand " + pat; return false; }
    e = pat + ": No such file or directory";
    return false;
  }
  RemoteStream* openStream(const string&, int&, string& e) override
  { e = "no"; return 0; }
  void interruptStream(RemoteStream*) override {}
  void closeStream(RemoteStream*, int) override {}
};

static long long sizeOf(const string& p)
{
  struct stat sb;
  return stat(p.c_str(), &sb) == 0 ? (long long)sb.st_size : -1;
}

int main()
{
  char tmpl[] = "/tmp/ecce-copy-unreachable.XXXXXX";
  string dir = mkdtemp(tmpl);

  FakeTransport t;
  t.files["run/orca.orcaout"] = "ORCA TERMINATED NORMALLY\n";
  vector<string> want = {"run/orca.orcain.gbw", "run/orca.orcaout",
                         "run/*.cub"};
  string err;

  bool ok = RCommand::transportCopy(&t, false, want, dir, err);
  check("a host that answers: copy succeeds", ok);
  check("a host that answers: the output arrives",
        sizeOf(dir + "/orca.orcaout") > 0);
  check("a host that answers: a missing file is only skipped",
        sizeOf(dir + "/orca.orcain.gbw") < 0);
  unlink((dir + "/orca.orcaout").c_str());

  t.answers = false;
  err.clear();
  ok = RCommand::transportCopy(&t, false, want, dir, err);
  check("a host that cannot be asked: copy fails", !ok);
  check("a host that cannot be asked: the error says so",
        err.find("cannot reach the host") != string::npos);
  check("a host that cannot be asked: nothing arrives",
        sizeOf(dir + "/orca.orcaout") < 0);

  rmdir(dir.c_str());
  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}
