// RCommand to a machine that is only reachable through a front end (#204):
// the values and output recorded from the old pty path, which ssh'd from the
// front end, must keep coming back through either a forwarded connection or a
// nested ssh.  golden/rcommand_frontend_*.txt.  Run by sshd/frontend_test.sh,
// which provides the containers and ~/.ssh; exit 77 without them.
//   testRCommandFrontend <frontend> <expected mode: forward|nested>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "comm/RCommand.H"
#include "golden.H"

using namespace std;

static int failures = 0;

static void check(const string& name, bool ok, bool counts = true)
{
  cout << (ok ? "ok   " : counts ? "FAIL " : "note ") << name << endl;
  if (!ok && counts) failures++;
}

static string esc(const string& s)
{
  string r;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\r') r += "\\r";
    else if (s[i] == '\n') r += "\\n";
    else r += s[i];
  }
  return r;
}

static string rec(const string& name, bool ret, const string& out,
                  const string& err = "")
{
  return name + ": ret=" + (ret ? "1" : "0") + " out=[" + esc(out) + "]" +
         (err.empty() ? "" : " err=[" + esc(err) + "]");
}

static string readFile(const string& f)
{
  ifstream in(f.c_str());
  return string((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
}

static string trim(string o)
{
  while (!o.empty() && (o[o.size()-1] == '\n' || o[o.size()-1] == '\r'))
    o.erase(o.size()-1);
  return o;
}

static bool scenario(const string& user, const string& shell,
                     const string& front, const string& rdir,
                     const string& ldir, const string& wantMode,
                     vector<string>& log)
{
  RCommand rc("node", "ssh", shell, user, "", front);
  if (!rc.isOpen()) {
    cout << "no session (" << user << "): " << rc.commError() << endl;
    return false;
  }
  check("front end mode " + wantMode + " (" + user + ")",
        rc.frontEndMode() == wantMode);

  string o;
  bool r;
#define EXECOUT(name, cmd) \
  do { o = "untouched"; r = rc.execout(cmd, o); \
       log.push_back(rec(name, r, o, r ? "" : rc.commError())); } while (0)
#define STATUS_ONLY(name, cmd) \
  do { r = rc.execout(cmd, o); log.push_back(rec(name, r, "")); } while (0)

  // Which machine the commands ran on is the point of a front end.
  EXECOUT("hostname", "hostname");
  EXECOUT("echo", "echo hello");
  EXECOUT("multi-line", "echo a; echo b; echo c");
  EXECOUT("status 1", "(exit 1)");
  STATUS_ONLY("status 3", "echo before; (exit 3)");
  STATUS_ONLY("status 127", "no_such_command_ecce");
  EXECOUT("pipe", "echo abc | tr a-c A-C");
  r = rc.exec("echo quiet");
  log.push_back(rec("exec ok", r, ""));
  r = rc.exec("(exit 1)", "custom failure text");
  log.push_back(rec("exec fail", r, "", rc.commError()));

  r = rc.cd(rdir);
  log.push_back(rec("cd abs", r, "", r ? "" : rc.commError()));
  EXECOUT("pwd", "pwd");
  r = rc.cd("sub");
  log.push_back(rec("cd relative", r, "", r ? "" : rc.commError()));
  EXECOUT("ls in sub", "ls");
  r = rc.cd("no_such_dir");
  log.push_back(rec("cd missing", r, "", rc.commError()));
  r = rc.cd("..");
  EXECOUT("pwd after ..", "pwd");

  const char* names[] = { "plain", "script", "sub", "missing" };
  for (int i = 0; i < 4; i++) {
    string n = names[i];
    log.push_back(rec("exists " + n, rc.exists(n), ""));
    log.push_back(rec("directory " + n, rc.directory(n), ""));
    log.push_back(rec("executable " + n, rc.executable(n), ""));
  }
  string path;
  r = rc.which("sh", path);
  log.push_back(rec("which sh", r, path));
  r = rc.which("no_such_command_ecce", path);
  log.push_back(rec("which missing", r, path));

  o = "";
  r = rc.execbg("sleep 20", o);
  bool alive = r && atol(o.c_str()) > 1 && rc.exec("kill -0 " + o);
  log.push_back(rec("execbg", r, alive ? "live pid" : "no live pid [" + o + "]"));
  if (alive) rc.exec("kill -9 " + o);

  string dest = rdir + "/copies";
  rc.exec("mkdir -p " + dest);
  string fromPath = ldir + "/lplain", fromScript = ldir + "/lscript";
  const char* from[] = { fromPath.c_str(), fromScript.c_str(), 0 };
  r = rc.shellput(from, dest);
  log.push_back(rec("shellput", r, "", r ? "" : rc.commError()));
  EXECOUT("shellput content", "cat " + dest + "/lplain");
  check("shellput keeps the exec bit", rc.executable(dest + "/lscript"));

  string getdir = ldir + "/got";
  mkdir(getdir.c_str(), 0755);
  unlink((getdir + "/plain").c_str());
  vector<string> src;
  src.push_back(rdir + "/plain");
  r = rc.shellget(src, getdir);
  log.push_back(rec("shellget", r, "", r ? "" : rc.commError()));
  log.push_back(rec("shellget content", true, readFile(getdir + "/plain")));
  src.clear();
  src.push_back(rdir + "/missing");
  r = rc.shellget(src, getdir);
  log.push_back(rec("shellget missing", r, ""));

  // Big enough to cross many channel packets, and binary.
  rc.exec("head -c 300000 /dev/urandom > " + rdir + "/big && cksum < " + rdir +
          "/big > " + rdir + "/big.sum");
  src.clear();
  src.push_back(rdir + "/big");
  unlink((getdir + "/big").c_str());
  r = rc.shellget(src, getdir);
  string sum;
  {
    string c = "cksum < " + getdir + "/big";
    FILE* p = popen(c.c_str(), "r");
    char b[128];
    if (p && fgets(b, sizeof b, p)) sum = b;
    if (p) pclose(p);
  }
  string rsum;
  rc.execout("cat " + rdir + "/big.sum", rsum);
  check("shellget of a 300 kB binary file is intact",
        r && trim(sum) == trim(rsum) && !sum.empty());
  return true;
}

// hop() from the front end to the node: commands then run on the node.
static void hopChecks(const string& user, const string& shell,
                      const string& front)
{
  RCommand rc(front, "ssh", shell, user);
  string o;
  if (!rc.isOpen()) { check("hop: open " + front, false); return; }
  rc.execout("hostname", o);
  check("hop: starts on the front end (" + trim(o) + ")", trim(o) == front);
  bool r = rc.hop("node", shell, user);
  check(string("hop: hop() succeeds ") + (r ? "" : rc.commError()), r);
  rc.execout("hostname", o);
  check("hop: commands now run on node (" + trim(o) + ")", trim(o) == "node");
  check("hop: cd and pwd on node", rc.cd("/tmp") && rc.execout("pwd", o) &&
        trim(o) == "/tmp");
  check("hop: connection still open", rc.isOpen());
}

int main(int argc, char** argv)
{
  if (argc < 3) { cout << "usage: testRCommandFrontend <frontend> <mode>" << endl; return 2; }
  string front = argv[1], wantMode = argv[2];
  if (!getenv("ECCE_REALUSER")) {
    const char* u = getenv("USER");
    setenv("ECCE_REALUSER", u ? u : "root", 1);
  }
  setenv("ECCE_AUTHCACHE_NO_BROADCAST", "1", 1);
  const char* home = getenv("HOME");
  if (home && !getenv("ECCE_REALUSERHOME")) setenv("ECCE_REALUSERHOME", home, 1);
  if (!home || access((string(home) + "/.ssh/ecce_test_key").c_str(), R_OK) != 0) {
    cout << "SKIP: run through tests/transport/sshd/frontend_test.sh" << endl;
    return 77;
  }

  char tmpl[] = "/tmp/testRCommandFrontXXXXXX";
  string ldir = mkdtemp(tmpl);
  { ofstream f((ldir + "/lplain").c_str()); f << "local text\nline two\n"; }
  { ofstream f((ldir + "/lscript").c_str()); f << "#!/bin/sh\necho s\n"; }
  chmod((ldir + "/lscript").c_str(), 0755);

  struct Acct { const char* user; const char* shell; } accts[] = {
    { "cshuser", "csh" }, { "bashuser", "bash" } };
  int diffs = 0;

  if (argc > 3 && string(argv[3]) == "hop") {   // hop checks alone, for debugging
    hopChecks("cshuser", "csh", front);
    return failures ? 1 : 0;
  }

  for (int a = 0; a < 2; a++) {
    string user = accts[a].user, shell = accts[a].shell;
    cout << "== " << user << " (" << shell << ") via " << front << endl;
    string rdir = "/tmp/ecce_fe_" + user;
    {
      RCommand rc("node", "ssh", shell, user, "", front);
      string o;
      if (!rc.isOpen()) {
        cout << "FAIL: cannot open ssh session: " << rc.commError() << endl;
        return 1;
      }
      rc.execout("rm -rf " + rdir + " && mkdir -p " + rdir + "/sub && "
                 "printf 'plain text\\nsecond line\\n' > " + rdir + "/plain && "
                 "printf '#!/bin/sh\\necho hi\\n' > " + rdir + "/script && "
                 "chmod 755 " + rdir + "/script && echo x > " + rdir + "/sub/inner", o);
    }
    vector<string> log;
    golden::Subs subs;
    const string name = "rcommand_frontend_" + shell;
    if (!scenario(user, shell, front, rdir, ldir, wantMode, log)) {
      cout << "FAIL: no ssh session" << endl;
      return 1;
    }
    diffs += golden::compare(name, log, subs);
    hopChecks(user, shell, front);
    RCommand rc("node", "ssh", shell, user, "", front);
    string o;
    rc.execout("rm -rf " + rdir, o);
  }
  failures += diffs;
  string cmd = "rm -rf " + ldir;
  if (system(cmd.c_str()) != 0) {}
  cout << (failures ? "FAILED " : "PASSED ") << failures << " difference(s)" << endl;
  return failures ? 1 : 0;
}
