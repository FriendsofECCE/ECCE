// RCommand::bgcommand -> ecmd -> RCommand::command end to end (#204), over
// ssh, against the test sshd from tests/transport/sshd/run.sh; bgcommand_test.sh sets up $ECCE_HOME, the
// machine list and ~/.ssh.  Exit 77 if the setup is absent.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <sys/time.h>
#include <unistd.h>

#include "comm/RCommand.H"
#include "tdat/AuthCache.H"

using namespace std;

static int failures = 0;

static void check(const string& name, bool ok)
{
  cout << (ok ? "ok   " : "FAIL ") << name << endl;
  if (!ok) failures++;
}

static double now()
{
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + tv.tv_usec / 1e6;
}

static string trim(string s)
{
  while (!s.empty() && (s[s.size()-1] == '\n' || s[s.size()-1] == '\r'))
    s.erase(s.size()-1);
  return s;
}

// What the background command did on the remote machine is read with the
// OpenSSH client, independently of ECCE.
static string remote(const string& user, const string& cmd)
{
  string q = "'";
  for (size_t i = 0; i < cmd.size(); i++) {
    if (cmd[i] == '\'') q += "'\\''";
    else q += cmd[i];
  }
  q += "'";
  string line = "ssh -o BatchMode=yes -o ConnectTimeout=15 -l " + user +
                " 127.0.0.1 " + q + " </dev/null 2>/dev/null";
  FILE* p = popen(line.c_str(), "r");
  if (!p) return "";
  string out;
  char buf[512];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, p)) > 0) out.append(buf, n);
  int st = pclose(p);
  return st == 0 ? trim(out) : "";
}

// Waits for the command's marker file to hold something.
static string waitFor(const string& user, const string& file, int secs)
{
  for (int i = 0; i < secs; i++) {
    string s = remote(user, "cat " + file);
    if (!s.empty()) return s;
    usleep(1000000);
  }
  return "";
}

static void scenario(const string& user, const string& machine,
                     const string& password)
{
  const string tag = user + "/" + machine;
  const string id = user + machine;
  // An earlier run's background command keeps its ssh child for a while,
  // and the child check below would count it.
  for (int i = 0; i < 30 && system("pgrep -x ssh >/dev/null") == 0; i++)
    usleep(1000000);
  if (password != "") AuthCache::getCache().addAuthentication("ssh://" + machine, user, password, "", false);

  remote(user, "rm -f /tmp/bg-" + id + " /tmp/xt-" + id);
  string err;
  double t0 = now();
  bool ok = RCommand::bgcommand("sh -c 'hostname > /tmp/bg-" + id +
                                "; sleep 8'", err, machine, "ssh", user,
                                password);
  double took = now() - t0;
  check(tag + ": bgcommand returns true", ok);
  check(tag + ": returns promptly", took < 1.5);
  string host = waitFor(user, "/tmp/bg-" + id, 20);
  string want = remote(user, "hostname");
  check(tag + ": marker [" + host + "] holds the remote hostname [" + want + "]",
        host != "" && host == want);

  // The marker is written first and the command then sleeps, so ecmd is
// still connected: an ssh child would mean the OpenSSH client, not libssh.
  bool sshChild = system("pgrep -x ssh >/dev/null") == 0;
  check(tag + ": ecmd has no ssh child (libssh)", !sshChild);

  // EcceShell's shape: a program with its own arguments, here a stub in
  // place of xterm, which records the DISPLAY it was started with.
  // The shebang is spelled in octal: a literal "!/" is history expansion in
  // an interactive login shell, and a script without one runs under csh.
  remote(user, "printf '\\043\\041/bin/sh\\necho \"$DISPLAY|$*\" > /tmp/xt-" + id +
               "\\n' > /tmp/xterm-stub-" + id + "; chmod +x /tmp/xterm-stub-" + id + "");
  check(tag + ": stub installed", remote(user, "ls /tmp/xterm-stub-" + id + "") != "");
  t0 = now();
  ok = RCommand::bgcommand("/tmp/xterm-stub-" + id + " -title T -e", "sh -c \"cd /tmp && $SHELL\"",
                           err, machine, "ssh", user, password);
  took = now() - t0;
  check(tag + ": shell-shaped bgcommand returns true", ok);
  check(tag + ": shell-shaped returns promptly", took < 1.5);
  string rec = waitFor(user, "/tmp/xt-" + id, 20);
  cout << "     stub saw: [" << rec << "]" << endl;
  check(tag + ": stub ran with its arguments", rec.find("|-title T -e") != string::npos);
  cout << "     (remote DISPLAY: [" << rec.substr(0, rec.find('|'))
       << "]; libssh does no X11 forwarding)" << endl;
  remote(user, "rm -f /tmp/bg-" + id + " /tmp/xt-" + id);
}

int main(int argc, char** argv)
{
  (void)argv;
  if (argc < 1 || !getenv("ECCE_HOME")) {
    cout << "usage: testBgcommand (via bgcommand_test.sh)" << endl;
    return 77;
  }
  const char* users[] = { "cshuser", "bashuser" };
  for (int i = 0; i < 2; i++)
    scenario(users[i], "sshbg", "");
  // pwhost has no key: the password travels through the AuthCache FIFO.
  scenario("bashuser", "pwhost", "ecce-test");
  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}
