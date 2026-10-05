// Messaging library test: starts a private mosquitto on a Unix socket and
// drives the real JMSPublisher/JMSSubscriber through it.  It checks that
// messages are DELIVERED (a granted SUBACK proves nothing), so every
// "not delivered" check is followed by a later message from the same
// publisher that must arrive.
//
//   mqtt_test <source dir>                      the test (Unix socket)
//   mqtt_test auth <source dir>                 two accounts on a TCP broker
//                                               with the shipped access rules
//   mqtt_test child <spec>...                   second process (internal)
//        spec = topic:tag[:targetname]

#include <dirent.h>
#include <fcntl.h>
#include <mosquitto.h>
#include <mqtt_protocol.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
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
#include "util/MqttLink.H"

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
  // The account of a TCP broker, as AuthCache would supply it.
  const char* cu = getenv("MQTT_TEST_USER");
  const char* cp = getenv("MQTT_TEST_PASS");
  if (cu) {
    string user = cu, pass = cp ? cp : "";
    MqttLink::setCredentialProvider(
      [user, pass](const string&, const string&, string& u, string& p) {
        u = user; p = pass; return true; });
  } else if (getenv("MQTT_TEST_NOLOGIN")) {
    MqttLink::setCredentialProvider(
      [](const string&, const string&, string&, string&) { return false; });
  }
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
static int runChildStatus(const string& home, const string& display,
                          const vector<string>& specs,
                          const vector<string>& env = vector<string>(),
                          const string& errFile = "")
{
  pid_t pid = fork();
  if (pid == 0) {
    for (size_t i = 0; i + 1 < env.size(); i += 2)
      setenv(env[i].c_str(), env[i + 1].c_str(), 1);
    if (!errFile.empty()) {
      int fd = open(errFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
      if (fd >= 0) dup2(fd, 2);
    }
    setenv("ECCE_REALUSERHOME", home.c_str(), 1);
    setenv("HOST", "testhost", 1);
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
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static bool runChild(const string& home, const string& display,
                     const vector<string>& specs)
{
  return runChildStatus(home, display, specs) == 0;
}

static void writeBrokerFile(const string& home, const string& sock,
                            const string& user, int port = 0)
{
  mkdir(home.c_str(), 0700);
  string dir = home + "/.ECCE";
  mkdir(dir.c_str(), 0700);
  // One file per session, named like ecce-gateway-start names it; HOST is
  // "testhost" for every process below.
  const char* displays[] = {":7", ":8"};
  for (int i = 0; i < 2; i++) {
    string key = string("testhost_") + displays[i];
    for (size_t j = 0; j < key.size(); j++)
      if (key[j] == ':') key[j] = '_';
    ofstream f((dir + "/broker_" + key).c_str());
    if (port)   // a TCP broker: the account comes from the credential
      f << "# test\nhost=127.0.0.1\nport=" << port << "\n";
    else
      f << "# test\nsocket=" << sock << "\nuser=" << user << "\nfuture_key=1\n";
  }
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


// ---------------------------------------------------- authenticated broker

// A client that is not a JMSPublisher: it can publish into any topic its
// account is let into and subscribe to any filter, which is what a user
// with a libmosquitto of their own can do.
class RawClient {
public:
  RawClient(const string& user, const string& pass, int port)
    : connack(-1), puback(-1), m_port(port)
  {
    m_mosq = mosquitto_new(0, true, this);
    mosquitto_int_option(m_mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
    if (!user.empty()) mosquitto_username_pw_set(m_mosq, user.c_str(), pass.c_str());
    mosquitto_connect_v5_callback_set(m_mosq, onConnect);
    mosquitto_publish_v5_callback_set(m_mosq, onPublish);
    mosquitto_message_callback_set(m_mosq, onMessage);
    mosquitto_reconnect_delay_set(m_mosq, 1, 1, false);
    mosquitto_connect_async(m_mosq, "127.0.0.1", port, 30);
    mosquitto_loop_start(m_mosq);
    for (int i = 0; i < 100 && connack < 0; i++) usleep(30000);
  }
  ~RawClient()
  {
    mosquitto_disconnect(m_mosq);
    mosquitto_loop_stop(m_mosq, false);
    mosquitto_destroy(m_mosq);
  }
  void subscribe(const string& filter)
  {
    mosquitto_subscribe(m_mosq, 0, filter.c_str(), 1);
    usleep(300000);
  }
  // The PUBACK reason code: 0 accepted, 135 not authorized.
  int publish(const string& topic, const string& body)
  {
    puback = -1;
    mosquitto_publish(m_mosq, 0, topic.c_str(), body.size(), body.c_str(), 1, false);
    for (int i = 0; i < 100 && puback < 0; i++) usleep(30000);
    return puback;
  }
  bool got(const string& topicPart)
  {
    for (int i = 0; i < 100; i++) {
      { for (size_t j = 0; j < topics.size(); j++)
          if (topics[j].find(topicPart) != string::npos) return true; }
      usleep(30000);
    }
    return false;
  }
  vector<string> topics;
  volatile int connack, puback;
private:
  static void onConnect(mosquitto*, void* o, int rc, int, const mosquitto_property*)
  { static_cast<RawClient*>(o)->connack = rc; }
  static void onPublish(mosquitto*, void* o, int, int rc, const mosquitto_property*)
  { static_cast<RawClient*>(o)->puback = rc; }
  static void onMessage(mosquitto*, void* o, const mosquitto_message* m)
  { static_cast<RawClient*>(o)->topics.push_back(m->topic); }
  mosquitto* m_mosq;
  int m_port;
};

static string slurp(const string& path)
{
  ifstream f(path.c_str());
  return string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
}

static int freePort()
{
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in a;
  memset(&a, 0, sizeof a);
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  int port = 0;
  if (::bind(fd, (struct sockaddr*)&a, sizeof a) == 0) {
    socklen_t len = sizeof a;
    getsockname(fd, (struct sockaddr*)&a, &len);
    port = ntohs(a.sin_port);
  }
  close(fd);
  return port;
}

// htpasswd with the password on stdin, as ecce-dataserver-adduser does.
// flags: "-B" bcrypt, "" the default (apr1 MD5), "-c" to create the file.
static bool htpasswd(const string& file, const string& flags, const string& user,
                     const string& pw)
{
  string cmd = "htpasswd -i " + flags + " " + file + " " + user + " >/dev/null 2>&1";
  FILE* p = popen(cmd.c_str(), "w");
  if (!p) return false;
  fprintf(p, "%s\n", pw.c_str());
  return pclose(p) == 0;
}

// With a plugin path, the broker checks logins against an htpasswd file
// through ecce_users_auth (a central server's, ecce-gateway-start);
// without, against a mosquitto password file (the shared broker's).
static int authMain(const string& src, const string& plugin)
{
  if (system("command -v mosquitto >/dev/null 2>&1 && "
             "command -v mosquitto_passwd >/dev/null 2>&1 && "
             "command -v htpasswd >/dev/null 2>&1") != 0) {
    cout << "SKIP  no mosquitto or htpasswd binary" << endl;
    return 77;
  }
  const char* tmpbase = getenv("TMPDIR");
  string tmpl = string(tmpbase ? tmpbase : "/tmp") + "/mqtt_authXXXXXX";
  vector<char> buf(tmpl.begin(), tmpl.end());
  buf.push_back(0);
  if (!mkdtemp(&buf[0])) { perror("mkdtemp"); return 1; }
  string tmp = &buf[0];
  int port = freePort();

  // The account list is what ecce-dataserver-adduser writes: hashed by
  // mosquitto_passwd -U from a private file.
  if (plugin.empty()) {
    int fd = open((tmp + "/passwd").c_str(), O_WRONLY | O_CREAT, 0600);
    string plain = "alice:alicepw\nbob:bobpw\n";
    if (write(fd, plain.c_str(), plain.size()) < 0) perror("write");
    close(fd);
    if (system(("mosquitto_passwd -U " + tmp + "/passwd >/dev/null 2>&1").c_str()) != 0) {
      cout << "FAIL  mosquitto_passwd" << endl;
      return 1;
    }
  } else if (!htpasswd(tmp + "/passwd", "-B -c", "alice", "alicepw") ||
             !htpasswd(tmp + "/passwd", "", "bob", "bobpw")) {
    cout << "FAIL  htpasswd" << endl;
    return 1;
  }
  string acl = src + "/packaging/gateway/ecce-mosquitto.acl";
  {
    ofstream c((tmp + "/mosquitto.conf").c_str());
    c << "per_listener_settings true\npersistence false\n"
      << "log_dest file " << tmp << "/mosquitto.log\n"
      << "listener " << port << " 127.0.0.1\nallow_anonymous false\n"
      << (plugin.empty() ? "password_file " + tmp + "/passwd\n"
                         : "plugin " + plugin + "\nplugin_opt_users_file " + tmp + "/passwd\n")
      << "acl_file " << acl << "\n";
  }
  pid_t broker = fork();
  if (broker == 0) {
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
    execlp("mosquitto", "mosquitto", "-c", (tmp + "/mosquitto.conf").c_str(), (char*)0);
    _exit(127);
  }
  bool up = false;
  for (int i = 0; i < 100 && !up; i++) {
    RawClient probe("alice", "alicepw", port);
    up = probe.connack == 0;
    if (!up) usleep(50000);
  }
  check(up, "mosquitto listening on TCP with a password file and the shipped ACL");

  int rc = 1;
  if (up) {
    string alice = tmp + "/alice", bob = tmp + "/bob";
    writeBrokerFile(alice, "", "", port);
    writeBrokerFile(bob, "", "", port);
    setenv("ECCE_HOME", src.c_str(), 1);
    setenv("ECCE_REALUSERHOME", alice.c_str(), 1);
    setenv("HOST", "testhost", 1);
    setenv("DISPLAY", ":7", 1);
    unsetenv("ECCE_NO_MESSAGING");
    MqttLink::setCredentialProvider(
      [](const string&, const string&, string& u, string& p) {
        u = "alice"; p = "alicepw"; return true; });

    Probe pa("A");
    g_probes.push_back(&pa);
    jmsCBFunc cb = static_cast<jmsCBFunc>(&Probe::on);
    pa.subscribe("ecce_url_created", cb, false);
    pa.subscribe("ecce_ejs_kill", cb, false);
    pa.subscribe("ecce_machreg_changed", cb, false);
    pa.subscribe("ecce_poll", cb, false);
    check(pa.startSubscriber(), "alice's subscriber started with her login");
    usleep(300000);

    // What bob can see: his own subtree and the site-wide registrations.
    RawClient bobSub("bob", "bobpw", port);
    check(bobSub.connack == 0, "bob connects with his own password");
    bobSub.subscribe("ecce/alice/#");
    bobSub.subscribe("ecce/+/ecce_url_created");
    bobSub.subscribe("ecce/alice/ecce_ejs_kill");
    bobSub.subscribe("ecce/+/ecce_machreg_changed");

    // Alice's own processes, then the site-wide message last: bob getting
    // that one shows the earlier ones were withheld, not slow.
    vector<string> a;
    a.push_back("ecce_url_created:a_url");
    a.push_back("ecce_ejs_kill:a_kill");
    a.push_back("ecce_poll:a_poll");
    a.push_back("ecce_machreg_changed:a_machreg");
    vector<string> aenv;
    aenv.push_back("MQTT_TEST_USER"); aenv.push_back("alice");
    aenv.push_back("MQTT_TEST_PASS"); aenv.push_back("alicepw");
    check(runChildStatus(alice, ":7", a, aenv) == 0, "another process of alice published");
    check(waitFor(pa, "a_machreg") && pa.has("a_url") && pa.has("a_kill") &&
          pa.has("a_poll"),
          "alice received her own ecce_url_created, ecce_ejs_kill and session message");
    check(bobSub.got("ecce_machreg_changed"),
          "bob receives alice's ecce_machreg_changed");
    check(!bobSub.got("ecce_url_created") && !bobSub.got("ecce_ejs_kill") &&
          !bobSub.got("/session/"),
          "bob receives none of alice's ecce_url_*, ecce_ejs_kill or session messages");

    // Bob writing into alice's topics: refused, and never delivered.
    RawClient bobPub("bob", "bobpw", port);
    int r1 = bobPub.publish("ecce/alice/ecce_ejs_kill", "x");
    int r2 = bobPub.publish("ecce/alice/ecce_url_created", "x");
    int r3 = bobPub.publish("ecce/alice/session/testhost__7/ecce_quit", "x");
    int r4 = bobPub.publish("ecce/bob/ecce_machreg_changed", "x");
    check(r1 == 135 && r2 == 135 && r3 == 135,
          "bob's publish into ecce/alice/ is refused (not authorized)");
    check(r4 == 0, "bob's publish into his own ecce/bob/ is accepted");
    // The same through the library, as bob.
    vector<string> b;
    b.push_back("ecce_url_created:b_url");
    b.push_back("ecce_ejs_kill:b_kill");
    b.push_back("ecce_machreg_changed:b_machreg");
    vector<string> benv;
    benv.push_back("MQTT_TEST_USER"); benv.push_back("bob");
    benv.push_back("MQTT_TEST_PASS"); benv.push_back("bobpw");
    check(runChildStatus(bob, ":7", b, benv) == 0, "a process of bob published as bob");
    check(waitFor(pa, "b_machreg"), "alice receives bob's ecce_machreg_changed");
    check(!pa.has("b_url") && !pa.has("b_kill") && !pa.has("x"),
          "alice receives nothing of bob's but ecce_machreg_changed, and nothing "
          "bob tried to write into her topics");

    // Authentication.
    RawClient wrong("alice", "wrong", port), anon("", "", port);
    check(wrong.connack == 134 || wrong.connack == 135,
          "a wrong password is refused at connect");
    check(anon.connack == 5 || anon.connack == 134 || anon.connack == 135,
          "an anonymous client is refused on TCP");
    string err = tmp + "/wrong.err";
    vector<string> wenv;
    wenv.push_back("MQTT_TEST_USER"); wenv.push_back("alice");
    wenv.push_back("MQTT_TEST_PASS"); wenv.push_back("wrong");
    vector<string> w;
    w.push_back("ecce_poll:w");
    int st = runChildStatus(alice, ":7", w, wenv, err);
    string said = slurp(err);
    check(st != 0 && said.find("refused the connection") != string::npos &&
          said.find("'alice'") != string::npos,
          "the library says why: the broker refused the login of 'alice'");
    cout << "      " << said.substr(0, said.find('\n')) << endl;
    string err2 = tmp + "/nologin.err";
    vector<string> nenv;
    nenv.push_back("MQTT_TEST_NOLOGIN"); nenv.push_back("1");
    st = runChildStatus(alice, ":7", w, nenv, err2);
    check(st != 0 && slurp(err2).find("no data server login") != string::npos,
          "with no login yet the library says so and does not connect");

    if (!plugin.empty()) {
      // The file changes under a running broker: an account made the 8.x
      // way (default htpasswd hash, nothing else written), then a password
      // change.
      check(htpasswd(tmp + "/passwd", "", "carol", "carolpw"),
            "carol added to the users file with the default htpasswd hash");
      RawClient carol("carol", "carolpw", port);
      check(carol.connack == 0, "carol logs in without a broker restart");
      RawClient carolBad("carol", "nope", port);
      check(carolBad.connack == 134 || carolBad.connack == 135,
            "carol with a wrong password is refused");
      RawClient carolSees("carol", "carolpw", port);
      carolSees.subscribe("ecce/alice/#");
      RawClient carolPub("carol", "carolpw", port);
      check(carolPub.publish("ecce/alice/ecce_ejs_kill", "x") == 135,
            "the ACL still stops carol writing into alice's topics");
      check(!carolSees.got("ecce/alice"), "carol receives nothing of alice's");
      check(htpasswd(tmp + "/passwd", "-B", "alice", "newpw"),
            "alice's password changed in the users file");
      RawClient aliceNew("alice", "newpw", port), aliceOld("alice", "alicepw", port);
      check(aliceNew.connack == 0, "alice logs in with the new password");
      check(aliceOld.connack == 134 || aliceOld.connack == 135,
            "the old password is refused");
      RawClient nobody("nobody", "x", port);
      check(nobody.connack == 134 || nobody.connack == 135,
            "an account that is not in the file is refused");
    }
    pa.unsubscribe();
    rc = g_fail ? 1 : 0;
  }
  kill(broker, SIGTERM);
  waitpid(broker, 0, 0);
  rmTree(tmp);
  cout << (rc == 0 ? "ALL PASS" : "FAILED") << endl;
  return rc;
}

int main(int argc, char** argv)
{
  g_self = "/proc/self/exe";
  char exe[4096];
  ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
  if (n > 0) { exe[n] = 0; g_self = exe; }

  if (argc >= 2 && string(argv[1]) == "child") return childMain(argc, argv);
  if (argc >= 3 && string(argv[1]) == "auth")
    return authMain(argv[2], argc > 3 ? argv[3] : "");
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
