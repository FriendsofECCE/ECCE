// ECCE's own ssh master (#487) against a real sshd; owned_master_test.sh.
//   testOwnedMaster streams N   N monitor streams held open at once
//   testOwnedMaster kill N      master killed under a stream, N openers
//   testOwnedMaster prefill     a known password: no dialog at all
// Needs ECCE_SSH_CONTROL_DIR, ECCE_TEST_SSH_CONFIG (host `own`), ECCE_ASKPASS
// and ASKLOG, one line of which is written per password dialog.
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <signal.h>
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
  int fd = -1;
  RemoteStream* s = t->openStream("echo up; sleep 60", fd, err);
  if (!s) return 0;
  char buf[8];
  ssize_t n = read(fd, buf, sizeof buf);
  return n > 0 ? s : 0;
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
