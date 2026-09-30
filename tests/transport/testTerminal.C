// EcceShell's entry points over ECCE_TRANSPORT=ssh (#204) against the test
// sshd: the terminal is a stub that runs its -e command on a pty with typed
// input, so a real OpenSSH client logs in and a real login shell (csh or
// bash) answers.  Run by terminal_test.sh, which sets up the machines.

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "comm/EcceShell.H"

using namespace std;

static int failures = 0;
static int caseNo = 0;

static void check(const string& name, bool ok)
{
  cout << (ok ? "ok   " : "FAIL ") << name << endl;
  if (!ok) failures++;
}

static vector<string> lines(const string& file)
{
  vector<string> v;
  ifstream in(file.c_str());
  string l;
  while (getline(in, l)) {
    while (!l.empty() && l[l.size()-1] == '\r') l.erase(l.size()-1);
    v.push_back(l);
  }
  return v;
}

static string grab(const vector<string>& v, const string& prefix)
{
  for (size_t i = 0; i < v.size(); i++)
    if (v[i].compare(0, prefix.size(), prefix) == 0) return v[i].substr(prefix.size());
  return "";
}

static bool has(const vector<string>& v, const string& s)
{
  for (size_t i = 0; i < v.size(); i++) if (v[i] == s) return true;
  return false;
}

// The stub records <out>.argv, then <out>.log and <out>.done.
struct Run {
  string out, msg;
  int status;
  vector<string> argv, log;
  bool started;
};

static Run go(const string& tag, const string& user, const string& kind,
              const string& machine, const string& dir, const string& base,
              const string& cmd = "", const string& file = "")
{
  Run r;
  string flat = tag;
  for (size_t i = 0; i < flat.size(); i++) if (flat[i] == '/') flat[i] = '_';
  r.out = "/tmp/ecce-term-" + flat + "-" + char('a' + caseNo++);
  string rm = "rm -f " + r.out + ".*";
  if (system(rm.c_str())) {}
  setenv("STUB_OUT", r.out.c_str(), 1);
  EcceShell sh;
  if (kind == "dir")
    r.msg = sh.dirshell("T", machine, "ssh", user, "", base, dir);
  else if (kind == "cmd")
    r.msg = sh.cmdshell("T", machine, "ssh", user, "", cmd, file);
  else
    r.msg = sh.topshell(machine, "ssh", user, "");
  r.status = sh.lastStatus();
  for (int i = 0; r.status == 0 && i < 300 && access((r.out + ".done").c_str(), F_OK) != 0; i++)
    usleep(100000);
  r.started = access((r.out + ".argv").c_str(), F_OK) == 0;
  r.argv = lines(r.out + ".argv");
  r.log = lines(r.out + ".log");
  return r;
}

static void shape(const string& tag, const Run& r, const string& user)
{
  // -e ssh -t -l user host <one remote command>, no xset, no -X/-Y.
  bool ok = r.argv.size() >= 7 && r.argv[0] == "-e" && r.argv[1] == "ssh" &&
            r.argv[2] == "-t" && r.argv[3] == "-l" && r.argv[4] == user &&
            r.argv[5] == "127.0.0.1" && r.argv.size() == 7 &&
            r.argv[6].compare(0, 5, "exec ") == 0;
  check(tag + ": argv is -e ssh -t -l user host cmd", ok);
  if (!ok) for (size_t i = 0; i < r.argv.size(); i++) cout << "     [" << r.argv[i] << "]" << endl;
  bool xset = false;
  for (size_t i = 0; i < r.argv.size(); i++)
    if (r.argv[i].find("xset") != string::npos || r.argv[i] == "-X" ||
        r.argv[i] == "-Y") xset = true;
  check(tag + ": no xset, no X forwarding", !xset);
}

static void openShell(const string& user, const string& machine,
                      const string& dir, const string& base,
                      const string& wantPwd, const string& wantMsg,
                      const string& wantTV = "")
{
  const string tag = user + "/" + machine + "/" +
                     (base == "" ? "top" : dir.substr(dir.rfind('/') + 1));
  Run r = (base == "" && dir == "") ?
          go(tag, user, "top", machine, "", "") :
          go(tag, user, "dir", machine, dir, base);
  check(tag + ": terminal started", r.started);
  shape(tag, r, user);
  string pwd = grab(r.log, "PWD=");
  check(tag + ": shell is in " + wantPwd + " (was [" + pwd + "])", pwd == wantPwd);
  check(tag + ": message [" + r.msg + "]",
        wantMsg == "" ? r.msg == "" : r.msg.find(wantMsg) != string::npos);
  if (wantTV != "") {
    string tv = grab(r.log, "TV=");
    check(tag + ": sourceFile variable is [" + tv + "]", tv == wantTV);
  }
}

int main(int argc, char** argv)
{
  if (!getenv("ECCE_HOME") || !getenv("STUB_DIRS")) {
    cout << "usage: testTerminal (via terminal_test.sh)" << endl;
    return 77;
  }
  const char* users[] = { "bashuser", "cshuser" };
  for (int u = 0; u < 2; u++) {
    string user = users[u];
    string home = "/home/" + user;
    // A bash-syntax and a csh-syntax machine for each account, so both
    // login shells meet both dialects.
    const char* machines[] = { "tbash", "tcsh" };
    for (int m = 0; m < 2; m++) {
      string mc = machines[m];
      openShell(user, mc, "/tmp/ecce-tdir", "/tmp", "/tmp/ecce-tdir", "");
    }
    openShell(user, "tbash", "/tmp/ecce-nosuch", "/tmp/ecce-tdir",
              "/tmp/ecce-tdir", "does not exist--starting shell in base");
    openShell(user, "tcsh", "/tmp/ecce-nosuch", "/tmp/ecce-nosuch2",
              home, "do not exist--starting shell in home");
    openShell(user, "tcsh", "", "", home, "");
    openShell(user, "tbashsrc", "/tmp/ecce-tdir", "/tmp", "/tmp/ecce-tdir", "",
              "fromsh");
    openShell(user, "tcshsrc", "/tmp/ecce-tdir", "/tmp", "/tmp/ecce-tdir", "",
              "fromcsh");

    // Tail: the file's content reaches the terminal, in both dialects.
    for (int m = 0; m < 2; m++) {
      string mc = machines[m];
      string tag = user + "/" + mc + "/tail";
      Run r = go(tag, user, "cmd", mc, "", "",
                 "tail -n 3 /tmp/ecce-ttail.txt; sleep 3", "/tmp/ecce-ttail.txt");
      check(tag + ": terminal started", r.started && r.msg == "");
      shape(tag, r, user);
      check(tag + ": file content shown", has(r.log, "TAILLINE"));
    }
    Run r = go(user + "/missing", user, "cmd", "tbash", "", "", "tail -f /tmp/nosuch",
               "/tmp/nosuch");
    check(user + ": missing file is refused [" + r.msg + "]",
          !r.started && r.status == -1 && r.msg.find("does not exist") != string::npos);
  }
  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}
