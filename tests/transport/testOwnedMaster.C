// ECCE's own ssh master (#487) against a real sshd; owned_master_test.sh.
//   testOwnedMaster streams N   N monitor streams held open at once
//   testOwnedMaster kill N      master killed under a stream, N openers
//   testOwnedMaster sessions N  N sessions' masters lost at once, two stores
//                               each reconnect: one new login wanted (#631)
//   testOwnedMaster prefill     a known password: no dialog at all
//   testOwnedMaster failing N   N openers lose the master and the login is
//                               not answered (ECCE_SSH_ASK_TIMEOUT) or
//                               cancelled: one dialog, all see a cancel
//   testOwnedMaster bothdead N  streams on the overflow master, both masters
//                               killed, N processes: one new login
// Needs ECCE_SSH_CONTROL_DIR, ECCE_TEST_SSH_CONFIG (host `own`), ECCE_ASKPASS
// and ASKLOG, one line of which is written per password dialog.
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "comm/OpenSshTransport.H"

using std::string;

static int failures = 0;
static void check(const string& name, bool ok, const string& why = "")
{
  std::cout << (ok ? "ok   " : "FAIL ") << name;
  if (!ok && !why.empty()) std::cout << "  (" << why << ")";
  std::cout << std::endl;
  if (!ok) failures++;
}

static int dialogs()
{
  std::ifstream in(getenv("ASKLOG"));
  string l;
  int n = 0;
  while (std::getline(in, l)) if (l.compare(0, 10, "passdialog") == 0) n++;
  return n;
}

static OpenSshTransport* make(const string& pw = "")
{
  OpenSshTransport* t = new OpenSshTransport("own", 0, "bashuser");
  t->setConfigFile(getenv("ECCE_TEST_SSH_CONFIG"));
  t->setAskpass(getenv("ECCE_ASKPASS"));
  t->setPassword(pw);
  return t;
}

// A stream that has printed "up": its session is open on a master.
static RemoteStream* openUp(OpenSshTransport* t, string& err)
{
  intptr_t fd = -1;
  RemoteStream* s = t->openStream("echo up; sleep 60", fd, err);
  if (!s) return 0;
  char buf[8];
  ssize_t n = read(fd, buf, sizeof buf);
  return n > 0 ? s : 0;
}

// The pid of the master behind a control socket, 0 if none.
static long masterPid(const string& socket)
{
  string cmd = "ssh -F '" + string(getenv("ECCE_TEST_SSH_CONFIG")) +
               "' -o ControlPath='" + socket + "' -O check own 2>&1";
  string out;
  FILE* p = popen(cmd.c_str(), "r");
  char b[256];
  while (p && fgets(b, sizeof b, p)) out += b;
  if (p) pclose(p);
  size_t at = out.find("pid=");
  return at == string::npos ? 0 : atol(out.c_str() + at + 4);
}

int main(int argc, char** argv)
{
  if (argc < 2 || !getenv("ASKLOG") || !getenv("ECCE_TEST_SSH_CONFIG") ||
      !getenv("ECCE_ASKPASS") || OpenSshTransport::ownedControlDir().empty()) {
    std::cerr << "needs ASKLOG, ECCE_TEST_SSH_CONFIG, ECCE_ASKPASS, a valid ECCE_SSH_CONTROL_DIR" << std::endl;
    return 2;
  }
  const string mode = argv[1];
  const int n = argc > 2 ? atoi(argv[2]) : 1;

  if (mode == "streams") {
    // A job store logs in (connect) before it opens its monitor stream.
    { OpenSshTransport* c = make(); string e; check("login", c->connect(e), e); delete c; }
    std::vector<OpenSshTransport*> ts(n);
    std::vector<RemoteStream*> ss(n, (RemoteStream*)0);
    std::vector<std::thread> th;
    for (int i = 0; i < n; i++) {
      ts[i] = make();
      th.push_back(std::thread([&, i]() { string e; ss[i] = openUp(ts[i], e); }));
    }
    for (size_t i = 0; i < th.size(); i++) th[i].join();
    int up = 0;
    for (int i = 0; i < n; i++) if (ss[i]) up++;
    check("all streams are up", up == n, std::to_string(up) + " of " + std::to_string(n));
    check("exactly two logins for " + std::to_string(n) + " streams (master + overflow)",
          dialogs() == 2, std::to_string(dialogs()) + " dialogs");
    for (int i = 0; i < n; i++) if (ss[i]) ts[i]->closeStream(ss[i], 200);
  } else if (mode == "kill") {
    OpenSshTransport* t = make();
    string e;
    check("login", t->connect(e), e);
    RemoteStream* s = openUp(t, e);
    check("first stream up", s != 0, e);
    check("one login so far", dialogs() == 1);
    string out;
    {
      string cmd = "ssh -F '" + string(getenv("ECCE_TEST_SSH_CONFIG")) +
                   "' -o ControlPath='" + t->controlSocketPath() + "' -O check own 2>&1";
      FILE* p = popen(cmd.c_str(), "r");
      char b[256];
      while (p && fgets(b, sizeof b, p)) out += b;
      if (p) pclose(p);
    }
    size_t at = out.find("pid=");
    long pid = at == string::npos ? 0 : atol(out.c_str() + at + 4);
    check("master pid found", pid > 1, out);
    if (pid > 1) kill((pid_t)pid, SIGKILL);
    usleep(500000);
    std::vector<std::thread> th;
    std::vector<int> st(n, -1);
    for (int i = 0; i < n; i++)
      th.push_back(std::thread([&, i]() {
        OpenSshTransport* u = make();
        st[i] = u->run("true").status;
        delete u;
      }));
    for (size_t i = 0; i < th.size(); i++) th[i].join();
    int ok = 0;
    for (int i = 0; i < n; i++) if (st[i] == 0) ok++;
    check("every opener got through", ok == n, std::to_string(ok) + " of " + std::to_string(n));
    check("the dead master cost exactly one new login with " + std::to_string(n) + " openers",
          dialogs() == 2, std::to_string(dialogs()) + " dialogs in all");
    if (s) t->closeStream(s, 200);
  } else if (mode == "failing") {
    // RCommand falls back to libssh (a second dialog) unless the login was
    // cancelled or refused: every failed opener must say so.
    std::vector<std::thread> th;
    std::vector<int> st(n, -1), said(n, 0), asked(n, 0);
    for (int i = 0; i < n; i++)
      th.push_back(std::thread([&, i]() {
        OpenSshTransport* u = make();
        string e;
        st[i] = u->connect(e) ? 0 : 1;
        said[i] = u->loginCancelled() || u->loginRefused();
        asked[i] = u->loginPrompted();
        delete u;
      }));
    for (size_t i = 0; i < th.size(); i++) th[i].join();
    int failed = 0, cancelled = 0, prompted = 0;
    for (int i = 0; i < n; i++) { failed += st[i] == 1; cancelled += said[i]; prompted += asked[i]; }
    check("every opener failed", failed == n, std::to_string(failed) + " of " + std::to_string(n));
    check("every opener reports a login that must not fall back to libssh",
          cancelled == n, std::to_string(cancelled) + " of " + std::to_string(n));
    check("one dialog in all, put up by one process",
          dialogs() == 1 && prompted == 1,
          std::to_string(dialogs()) + " dialogs, " + std::to_string(prompted) + " prompted");
  } else if (mode == "bothdead") {
    { OpenSshTransport* c = make(); string e; check("login", c->connect(e), e); delete c; }
    std::vector<OpenSshTransport*> ts(5);
    std::vector<RemoteStream*> ss(5, (RemoteStream*)0);
    std::vector<std::thread> th;
    for (int i = 0; i < 5; i++) {
      ts[i] = make();
      th.push_back(std::thread([&, i]() { string e; ss[i] = openUp(ts[i], e); }));
    }
    for (size_t i = 0; i < th.size(); i++) th[i].join();
    check("two logins so far (master + overflow)", dialogs() == 2, std::to_string(dialogs()));
    OpenSshTransport* t = make();
    const string second = t->controlSocketPath();
    check("the index moved to the overflow master", second.size() > 2 &&
          second.compare(second.size() - 2, 2, "-1") == 0, second);
    long p1 = masterPid(second), p0 = masterPid(second.substr(0, second.size() - 2));
    check("both master pids found", p0 > 1 && p1 > 1, std::to_string(p0) + " " + std::to_string(p1));
    if (p0 > 1) kill((pid_t)p0, SIGKILL);
    if (p1 > 1) kill((pid_t)p1, SIGKILL);
    usleep(500000);
    std::vector<pid_t> kids;
    for (int i = 0; i < n; i++) {
      pid_t k = fork();
      if (k == 0) {
        OpenSshTransport* u = make();
        _exit(u->run("true").status == 0 ? 0 : 1);
      }
      kids.push_back(k);
    }
    int ok = 0;
    for (size_t i = 0; i < kids.size(); i++) {
      int w = 0;
      waitpid(kids[i], &w, 0);
      if (WIFEXITED(w) && WEXITSTATUS(w) == 0) ok++;
    }
    check("every process got through", ok == n, std::to_string(ok) + " of " + std::to_string(n));
    check("both masters dead cost exactly one new login",
          dialogs() == 3, std::to_string(dialogs()) + " dialogs in all");
  } else if (mode == "sessions") {
    // #631: job stores keep the control directory of the session that
    // started them.  N sessions each log in, every master dies at once, and
    // two stores per session reconnect: the user should be asked once.
    // ECCE_SESSION_DIRS: the directories ecce_ssh_control_setup gave each
    // session, colon-separated; else one directory per session of our own.
    const string base = OpenSshTransport::ownedControlDir();
    std::vector<string> dirs;
    std::vector<string> given;
    if (const char* sd = getenv("ECCE_SESSION_DIRS")) {
      string rest = sd;
      while (!rest.empty()) {
        size_t c = rest.find(':');
        given.push_back(rest.substr(0, c));
        rest = c == string::npos ? "" : rest.substr(c + 1);
      }
    }
    std::vector<long> pids;
    for (int i = 0; i < n; i++) {
      const string d = given.empty() ? base + "-s" + std::to_string(i)
                                     : given[i % given.size()];
      mkdir(d.c_str(), 0700);
      dirs.push_back(d);
      setenv("ECCE_SSH_CONTROL_DIR", d.c_str(), 1);
      OpenSshTransport* c = make();
      string e;
      check("session " + std::to_string(i) + " logs in", c->connect(e), e);
      pids.push_back(masterPid(c->controlSocketPath()));
      delete c;
    }
    const int want = given.empty() ? n : 1;
    check("logins so far", dialogs() == want, std::to_string(dialogs()));
    for (size_t i = 0; i < pids.size(); i++) if (pids[i] > 1) kill((pid_t)pids[i], SIGKILL);
    usleep(500000);
    const int before = dialogs();
    std::vector<pid_t> kids;
    for (int i = 0; i < 2 * n; i++) {
      pid_t k = fork();
      if (k == 0) {
        setenv("ECCE_SSH_CONTROL_DIR", dirs[i % n].c_str(), 1);
        OpenSshTransport* u = make();
        _exit(u->run("true").status == 0 ? 0 : 1);
      }
      kids.push_back(k);
    }
    int ok = 0;
    for (size_t i = 0; i < kids.size(); i++) {
      int w = 0;
      waitpid(kids[i], &w, 0);
      if (WIFEXITED(w) && WEXITSTATUS(w) == 0) ok++;
    }
    check("every store got through", ok == 2 * n, std::to_string(ok) + " of " + std::to_string(2 * n));
    check("the lost masters of " + std::to_string(n) + " sessions cost one new login",
          dialogs() - before == 1, std::to_string(dialogs() - before) + " new dialogs");
    setenv("ECCE_SSH_CONTROL_DIR", base.c_str(), 1);
  } else if (mode == "prefill") {
    OpenSshTransport* t = make("ecce-test");
    TransportResult r = t->run("echo hi");
    check("login with the known password works", r.status == 0 && r.out == "hi\n", r.err + r.error);
    check("no dialog at all", dialogs() == 0, std::to_string(dialogs()));
  } else {
    return 2;
  }
  std::cout << (failures ? "FAILED " : "PASSED ") << failures << std::endl;
  return failures ? 1 : 0;
}
