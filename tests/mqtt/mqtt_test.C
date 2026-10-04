// Messaging library test: starts a private mosquitto on a Unix socket and
// drives the real JMSPublisher/JMSSubscriber through it.  It checks that
// messages are DELIVERED (a granted SUBACK proves nothing), so every
// "not delivered" check is followed by a later message from the same
// publisher that must arrive.
//
//   mqtt_test <source dir>                      the test
//   mqtt_test child <spec>...                   second process (internal)
//        spec = topic:tag[:targetname]

#include <dirent.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/JMSSubscriber.H"

using namespace std;

static int g_fail = 0;

static void check(bool ok, const string& what)
{
  cout << (ok ? "PASS  " : "FAIL  ") << what << endl;
  if (!ok) g_fail++;
}

// ------------------------------------------------------------ child role

static int childMain(int argc, char** argv)
{
  JMSPublisher pub("child");
  for (int i = 2; i < argc; i++) {
    string spec = argv[i];
    size_t a = spec.find(':');
    size_t b = spec.find(':', a + 1);
    string topic = spec.substr(0, a);
    string tag = spec.substr(a + 1, b == string::npos ? b : b - a - 1);
    string target = b == string::npos ? "" : spec.substr(b + 1);

    JMSMessage* msg = target.empty() ? pub.newMessage()
                                     : pub.newMessage(Target(target, ""));
    msg->addProperty("tag", tag);
    if (!pub.publish(topic, *msg)) return 2;
    delete msg;
  }
  return 0;   // static destructors flush the publishes
}

// ----------------------------------------------------------- driver role

class Probe : public JMSSubscriber {
public:
  explicit Probe(const string& name) : JMSSubscriber(name) {}
  void on(JMSMessage& m) { got.push_back(m.getProperty("tag")); }
  bool has(const string& tag) const {
    for (size_t i = 0; i < got.size(); i++) if (got[i] == tag) return true;
    return false;
  }
  vector<string> got;
};

static vector<Probe*> g_probes;

static void pump(int ms)
{
  for (int waited = 0; waited < ms; waited += 20) {
    fd_set fds;
    FD_ZERO(&fds);
    int maxfd = -1;
    for (size_t i = 0; i < g_probes.size(); i++) {
      int fd = g_probes[i]->getSocketID();
      FD_SET(fd, &fds);
      if (fd > maxfd) maxfd = fd;
    }
    struct timeval tv = {0, 20000};
    if (select(maxfd + 1, &fds, 0, 0, &tv) > 0)
      for (size_t i = 0; i < g_probes.size(); i++)
        g_probes[i]->processMessage();
  }
}

static bool waitFor(Probe& p, const string& tag, int ms = 5000)
{
  for (int waited = 0; waited < ms && !p.has(tag); waited += 20) pump(20);
  return p.has(tag);
}

static string g_self;

// Runs a second process (its own broker connection) as the given account.
static bool runChild(const string& home, const string& display,
                     const vector<string>& specs)
{
  pid_t pid = fork();
  if (pid == 0) {
    setenv("ECCE_REALUSERHOME", home.c_str(), 1);
    setenv("DISPLAY", display.c_str(), 1);
    vector<char*> args;
    args.push_back((char*)g_self.c_str());
    args.push_back((char*)"child");
    for (size_t i = 0; i < specs.size(); i++) args.push_back((char*)specs[i].c_str());
    args.push_back(0);
    execv(g_self.c_str(), &args[0]);
    _exit(127);
  }
  int status = 0;
  waitpid(pid, &status, 0);
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static void writeBrokerFile(const string& home, const string& sock,
                            const string& user)
{
  mkdir(home.c_str(), 0700);
  string dir = home + "/.ECCE";
  mkdir(dir.c_str(), 0700);
  ofstream f((dir + "/broker").c_str());
  f << "# test\nsocket=" << sock << "\nuser=" << user << "\nfuture_key=1\n";
}

static void rmTree(const string& path)
{
  DIR* d = opendir(path.c_str());
  if (d) {
    while (struct dirent* e = readdir(d)) {
      string n = e->d_name;
      if (n == "." || n == "..") continue;
      string p = path + "/" + n;
      struct stat st;
      if (lstat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) rmTree(p);
      else unlink(p.c_str());
    }
    closedir(d);
  }
  rmdir(path.c_str());
}

int main(int argc, char** argv)
{
  g_self = "/proc/self/exe";
  char exe[4096];
  ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
  if (n > 0) { exe[n] = 0; g_self = exe; }

  if (argc >= 2 && string(argv[1]) == "child") return childMain(argc, argv);
  if (argc != 2) {
    cerr << "usage: mqtt_test <source dir>" << endl;
    return 1;
  }
  if (system("command -v mosquitto >/dev/null 2>&1") != 0) {
    cout << "SKIP  no mosquitto binary" << endl;
    return 77;
  }

  const char* tmpbase = getenv("TMPDIR");
  string tmpl = string(tmpbase ? tmpbase : "/tmp") + "/mqtt_testXXXXXX";
  vector<char> buf(tmpl.begin(), tmpl.end());
  buf.push_back(0);
  if (!mkdtemp(&buf[0])) { perror("mkdtemp"); return 1; }
  string tmp = &buf[0];

  string sock = tmp + "/mq.sock";
  string alice = tmp + "/alice", bob = tmp + "/bob";
  writeBrokerFile(alice, sock, "alice");
  writeBrokerFile(bob, sock, "bob");
  {
    ofstream c((tmp + "/mosquitto.conf").c_str());
    c << "listener 0 " << sock << "\nallow_anonymous true\n"
      << "persistence false\n";
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

  int rc = 1;
  if (up) {
    setenv("ECCE_HOME", argv[1], 1);
    setenv("ECCE_REALUSERHOME", alice.c_str(), 1);
    setenv("HOST", "testhost", 1);
    setenv("DISPLAY", ":7", 1);
    unsetenv("ECCE_NO_MESSAGING");

    Probe p1("P1"), p2("P2");
    g_probes.push_back(&p1);
    g_probes.push_back(&p2);
    jmsCBFunc cb1 = static_cast<jmsCBFunc>(&Probe::on);
    p1.subscribe("ecce_poll", cb1);                      // session
    p1.subscribe("ecce_preferences_misc", cb1);          // USER
    p1.subscribe("ecce_url_state", cb1, true);           // NONE, self filtered
    p1.subscribe("ecce_machreg_changed", cb1);           // read from all users
    p2.subscribe("ecce_poll", cb1);
    p2.subscribe("ecce_url_state", cb1, false);
    p2.subscribe("ecce_url_created", cb1, false);
    check(p1.startSubscriber() && p2.startSubscriber(), "subscribers started");
    usleep(300000);   // let the broker register the subscriptions

    // Another process, same account and session.
    vector<string> a;
    a.push_back("ecce_poll:s_poll");
    a.push_back("ecce_preferences_misc:u_prefs");
    a.push_back("ecce_url_state:n_state");
    a.push_back("ecce_poll:tgt:P2");
    a.push_back("ecce_poll:sentinel1");
    check(runChild(alice, ":7", a), "child published as alice on :7");
    bool s1 = waitFor(p1, "sentinel1"), s2 = waitFor(p2, "sentinel1");
    check(s1 && s2, "both subscribers received the last message of the batch");
    check(p1.has("s_poll") && p2.has("s_poll"),
          "session topic delivered to both subscribers");
    check(p1.has("u_prefs"), "USER topic delivered");
    check(p1.has("n_state") && p2.has("n_state"),
          "NONE topic from another process delivered (filterSelf on and off)");
    check(p2.has("tgt") && !p1.has("tgt"),
          "targeted message reached only its target");

    // Another session, another account.
    vector<string> o;
    o.push_back("ecce_poll:other_session");
    check(runChild(alice, ":8", o), "child published to session :8");
    vector<string> b;
    b.push_back("ecce_preferences_misc:bob_prefs");
    b.push_back("ecce_url_state:bob_state");
    b.push_back("ecce_machreg_changed:bob_machreg");
    check(runChild(bob, ":7", b), "child published as bob");
    vector<string> z;
    z.push_back("ecce_url_state:sentinel2");
    runChild(alice, ":7", z);
    waitFor(p1, "sentinel2");
    waitFor(p2, "sentinel2");
    check(!p1.has("other_session") && !p2.has("other_session"),
          "a message for another session is not delivered");
    check(!p1.has("bob_prefs") && !p1.has("bob_state") && !p2.has("bob_state"),
          "another account's USER and NONE topics are not delivered");
    check(p1.has("bob_machreg"),
          "ecce_machreg_changed from another account is delivered");

    // filterSelf, in this process.
    JMSPublisher pub("driver");
    JMSMessage* m;
    const char* own[][2] = {{"ecce_url_state", "self_state"},
                            {"ecce_url_created", "self_created"},
                            {"ecce_poll", "self_poll"},
                            {"ecce_preferences_misc", "self_prefs"}};
    for (int i = 0; i < 4; i++) {
      m = pub.newMessage();
      m->addProperty("tag", own[i][1]);
      check(pub.publish(own[i][0], *m), string("published ") + own[i][0]);
      delete m;
    }
    pump(500);
    vector<string> y;
    y.push_back("ecce_url_state:sentinel3");
    runChild(alice, ":7", y);
    waitFor(p1, "sentinel3");
    waitFor(p2, "sentinel3");
    check(p2.has("self_state") && p2.has("self_created"),
          "filterSelf off: own messages delivered");
    check(!p1.has("self_state"),
          "filterSelf on: own message not delivered (topic shared with a "
          "filterSelf-off subscriber)");
    check(!p1.has("self_poll") && !p2.has("self_poll") &&
          !p1.has("self_prefs"),
          "filterSelf on: own messages not delivered (No Local)");

    // hold
    p1.holdMessages();
    vector<string> h;
    h.push_back("ecce_poll:held");
    h.push_back("ecce_url_state:sentinel4");
    runChild(alice, ":7", h);
    waitFor(p2, "sentinel4");
    pump(200);
    check(p2.has("sentinel4") && !p1.has("held") && !p1.has("sentinel4"),
          "held subscriber dropped messages");
    p1.resumeMessaging();
    vector<string> r;
    r.push_back("ecce_poll:resumed");
    runChild(alice, ":7", r);
    check(waitFor(p1, "resumed") && !p1.has("held"),
          "resumed subscriber receives again, held ones stay dropped");

    p1.unsubscribe();
    p2.unsubscribe();
    rc = g_fail ? 1 : 0;
  }

  kill(broker, SIGTERM);
  waitpid(broker, 0, 0);
  rmTree(tmp);
  cout << (rc == 0 ? "ALL PASS" : "FAILED") << endl;
  return rc;
}
