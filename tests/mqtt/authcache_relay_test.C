// AuthCache across processes (#487): a compute machine's password that one
// process learned (the Launcher, after its ssh login) must reach the other
// processes of the session (calced, organizer, gateway), which hand it to
// the next Launcher on its -pipe. Runs a private mosquitto on a Unix
// socket, as a local session does.
//
//   authcache_relay_test <source dir>
//
// SKIPs (77) without the mosquitto binary.

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/JMSSubscriber.H"
#include "tdat/AuthCache.H"

using namespace std;

static const char* SESSION = "a1b2c3d4e5f60718";
static const char* MACHINE_URL = "ssh://cluster.example.org";
static const char* DATA_URL = "http://localhost:1/Ecce/users/alice/";
static const char* DUMMY = "dummy-not-a-password";

static int g_fail = 0;
static void check(bool ok, const string& what)
{
  cout << (ok ? "PASS  " : "FAIL  ") << what << endl;
  if (!ok) g_fail++;
}

// The process that logged in: what RCommand::setPassCache does, then a
// data server credential as the last message, so the receiver knows the
// first one has been through.
static int childMain()
{
  AuthCache& c = AuthCache::getCache();
  c.addAuthentication(MACHINE_URL, "alice", DUMMY, "", true);
  c.addAuthentication(DATA_URL, "alice", "dataserver-dummy", "", true);
  return 0;   // static destructors flush the publishes
}

class Receiver : public JMSSubscriber {
public:
  Receiver() : JMSSubscriber("Receiver") {}
  void on(JMSMessage& m) {
    AuthCache::getCache().msgIn(m, getMyID());
    if (string(m.getProperty("url")).find("http") == 0) sawLast = true;
  }
  bool sawLast = false;
};

int main(int argc, char** argv)
{
  if (argc >= 2 && string(argv[1]) == "child") return childMain();
  if (argc != 2) {
    cerr << "usage: authcache_relay_test <source dir>" << endl;
    return 1;
  }
  if (system("command -v mosquitto >/dev/null 2>&1") != 0) {
    cout << "SKIP  no mosquitto binary" << endl;
    return 77;
  }
  char exe[4096];
  ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
  if (n <= 0) return 1;
  exe[n] = 0;

  const char* tmpbase = getenv("TMPDIR");
  string tmpl = string(tmpbase ? tmpbase : "/tmp") + "/authcache_testXXXXXX";
  char buf[4096];
  strncpy(buf, tmpl.c_str(), sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = 0;
  if (!mkdtemp(buf)) { perror("mkdtemp"); return 1; }
  string tmp = buf, sock = tmp + "/mq.sock", home = tmp + "/home";
  mkdir(home.c_str(), 0700);
  mkdir((home + "/.ECCE").c_str(), 0700);
  {
    ofstream f((home + "/.ECCE/broker_testhost_" + SESSION).c_str());
    f << "socket=" << sock << "\nuser=alice\n";
    ofstream c((tmp + "/mosquitto.conf").c_str());
    c << "listener 0 " << sock << "\nallow_anonymous true\npersistence false\n";
  }
  pid_t broker = fork();
  if (broker == 0) {
    string log = tmp + "/mosquitto.log";
    freopen(log.c_str(), "w", stdout);
    freopen(log.c_str(), "w", stderr);
    execlp("mosquitto", "mosquitto", "-c", (tmp + "/mosquitto.conf").c_str(),
           (char*)0);
    _exit(127);
  }
  bool up = false;
  for (int i = 0; i < 100 && !up; i++) {
    struct stat st;
    up = stat(sock.c_str(), &st) == 0 && S_ISSOCK(st.st_mode);
    if (!up) usleep(50000);
  }
  check(up, "mosquitto listening on the Unix socket");

  if (up) {
    setenv("ECCE_HOME", argv[1], 1);
    setenv("ECCE_REALUSERHOME", home.c_str(), 1);
    setenv("HOST", "testhost", 1);
    unsetenv("ECCE_HOST");
    setenv("DISPLAY", ":7", 1);
    setenv("ECCE_SESSION_ID", SESSION, 1);
    unsetenv("ECCE_NO_MESSAGING");
    unsetenv("ECCE_AUTHCACHE_NO_BROADCAST");

    Receiver r;
    r.subscribe("ecce_auth_changed", static_cast<jmsCBFunc>(&Receiver::on));
    check(r.startSubscriber(), "receiver subscribed");
    usleep(300000);

    pid_t pid = fork();
    if (pid == 0) { execl(exe, exe, "child", (char*)0); _exit(127); }
    int status = 0;
    waitpid(pid, &status, 0);
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "sender ran");

    for (int waited = 0; waited < 5000 && !r.sawLast; waited += 20) {
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(r.getSocketID(), &fds);
      struct timeval tv = {0, 20000};
      if (select(r.getSocketID() + 1, &fds, 0, 0, &tv) > 0) r.processMessage();
    }
    check(r.sawLast, "both ecce_auth_changed messages arrived");

    BasicAuth* ba = AuthCache::getCache().getAuthentication(DATA_URL, "alice");
    check(ba && ba->m_pass == "dataserver-dummy",
          "data server password relayed (session store)");
    delete ba;
    ba = AuthCache::getCache().getAuthentication(MACHINE_URL, "alice");
    check(ba && ba->m_pass == DUMMY,
          "compute machine password relayed to the other process");
    delete ba;

    // Compute machine passwords stay in memory: never in the session store.
    ifstream s((home + "/.ECCE/authcache_testhost_" + SESSION).c_str());
    stringstream all;
    all << s.rdbuf();
    check(all.str().find(DUMMY) == string::npos,
          "compute machine password not written to the session store");
  }

  if (broker > 0) { kill(broker, SIGTERM); waitpid(broker, 0, 0); }
  system(("rm -rf '" + tmp + "'").c_str());
  return g_fail ? 1 : 0;
}
