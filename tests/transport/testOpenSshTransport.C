// OpenSshTransport without a network: a stand-in `ssh` script applies the
// client's argument rules and runs the remote command here, so the process
// handling, file copies, streams and the error messages are checked against
// a real /bin/sh.  The backend choice is checked against the real `ssh -G`
// with private config files.

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <fcntl.h>
#include <sys/select.h>
#include <sys/stat.h>
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

static double now()
{
  return std::chrono::duration<double>(
    std::chrono::steady_clock::now().time_since_epoch()).count();
}

static string slurp(const string& f)
{
  std::ifstream in(f.c_str(), std::ios::binary);
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

static void spit(const string& f, const string& data, mode_t mode = 0644)
{
  std::ofstream(f.c_str(), std::ios::binary) << data;
  chmod(f.c_str(), mode);
}

static int sh(const string& cmd) { return system(cmd.c_str()); }

static string readFor(int fd, const string& want, int ms)
{
  string got;
  for (int waited = 0; waited < ms && got.find(want) == string::npos;) {
    fd_set f; FD_ZERO(&f); FD_SET(fd, &f);
    struct timeval tv = { 0, 100000 };
    if (select(fd + 1, &f, 0, 0, &tv) > 0) {
      char b[65536];
      ssize_t n = read(fd, b, sizeof b);
      if (n <= 0) break;
      got.append(b, n);
    } else waited += 100;
  }
  return got;
}

int main()
{
  char tmpl[] = "/tmp/testOpenSshXXXXXX";
  const string tmp = mkdtemp(tmpl);
  const string stub = tmp + "/ssh", log = tmp + "/ssh.log";
  spit(stub,
    "#!/bin/sh\n"
    "echo \"$@\" >> \"$STUB_LOG\"\n"
    "op=; master=\n"
    "while [ $# -gt 0 ]; do case \"$1\" in\n"
    "  -T|-x) shift;; -f|-N) master=1; shift;; -O) op=$2; shift 2;;\n"
    "  -o|-F|-p|-l|-J) shift 2;; *) break;; esac; done\n"
    "host=$1; shift\n"
    "case \"$host\" in\n"
    // A host with a shared connection that has to be opened by a login: -O
    // check and the command see the master file, -f -N makes it after
    // askpass has answered "secret".
    "  needmaster)\n"
    "    deny() { echo \"u@needmaster: Permission denied (publickey,password).\" >&2; exit 255; }\n"
    "    if [ -n \"$op\" ]; then [ -e \"$STUB_MASTER\" ] && exit 0\n"
    "      echo \"Control socket connect($STUB_MASTER): No such file or directory\" >&2; exit 255; fi\n"
    "    if [ -n \"$master\" ]; then\n"
    "      echo \"env: $SSH_ASKPASS_REQUIRE $ECCE_ASKPASS_HOST $ECCE_ASKPASS_USER\" >> \"$STUB_LOG\"\n"
    "      ans=$(\"$SSH_ASKPASS\" \"u@needmaster's password: \") || deny\n"
    "      [ \"$ans\" = secret ] || deny\n"
    "      : > \"$STUB_MASTER\"; exit 0; fi\n"
    "    [ -e \"$STUB_MASTER\" ] || deny;;\n"
    "  noauth) echo \"u@noauth: Permission denied (publickey,password).\" >&2; exit 255;;\n"
    "  badkey) echo \"Host key verification failed.\" >&2; exit 255;;\n"
    "  down) echo \"ssh: connect to host down port 22: Connection refused\" >&2; exit 255;;\n"
    "  mux) echo \"mux_client_request_session: read from master failed: Broken pipe\" >&2; exit 255;;\n"
    "esac\n"
    "exec sh -c \"$1\"\n", 0755);
  setenv("STUB_LOG", log.c_str(), 1);
  unsetenv("ECCE_SSH_KEEPALIVE");

  auto make = [&](const string& host, const string& user = "") {
    OpenSshTransport* t = new OpenSshTransport(host, 0, user);
    t->setSshProgram(stub);
    return t;
  };

  // ---- the command line ----
  {
    sh("rm -f " + log);
    OpenSshTransport* t = make("node1", "alice");
    t->setJumpHost("front", 2200, "bob");
    t->setConnectTimeout(7);
    string err;
    bool ok = t->connect(err);
    string args = slurp(log);
    check("connect through the stand-in", ok && t->connected(), err);
    check("batch mode, no tty, no X",
          args.find("-T -x ") != string::npos &&
          args.find("-o BatchMode=yes") != string::npos &&
          args.find("-o ConnectTimeout=7") != string::npos, args);
    check("user and front end map to -l and -J",
          args.find("-l alice") != string::npos &&
          args.find("-J bob@front:2200") != string::npos &&
          args.find(" node1 sh -c ") != string::npos, args);
    check("front end reported as forwarded", t->jumpHost() == "front" && !t->nested());
    delete t;
  }

  OpenSshTransport* t = make("localhost");
  string err;
  check("connect", t->connect(err), err);

  // ---- running scripts, as in testTransport ----
  {
    TransportResult r0 = t->run("exit 0"), r3 = t->run("exit 3"),
                    rk = t->run("kill -TERM $$");
    check("exit status", r0.status == 0 && r3.status == 3 && rk.status == 143,
          std::to_string(r0.status) + "," + std::to_string(r3.status) + "," +
          std::to_string(rk.status));
  }
  {
    TransportResult r = t->run("echo out; echo err >&2");
    check("stdout and stderr stay separate", r.out == "out\n" && r.err == "err\n");
  }
  {
    TransportResult r = t->run(
      "head -c 3000000 /dev/zero & head -c 3000000 /dev/zero >&2; wait", 60);
    check("3 MB on both streams", r.status == 0 && r.out.size() == 3000000 &&
          r.err.size() == 3000000 && !r.timedOut);
  }
  {
    string s = "exit 7\n";
    while (s.size() < 1048576) s += "# padding padding padding padding padding\n";
    TransportResult r = t->run(s, 60);
    check("1 MB script", r.status == 7 && !r.timedOut, std::to_string(r.status));
  }
  {
    string s =
      "echo 'a!b'; echo \"`echo hi`\"; echo x >&2 2>&1\n"
      "cat <<'EOT'\nline ! `not run` $nothing\nEOT\n";
    TransportResult r = t->run(s);
    check("csh-hostile script verbatim", r.status == 0 &&
          r.out == "a!b\nhi\nline ! `not run` $nothing\n", r.out);
  }
  {
    TransportResult r = t->run("cat\necho after\nread x; echo \"got:$x\"\n", 5);
    check("script cannot read its own stdin",
          r.status == 0 && !r.timedOut && r.out == "after\ngot:\n", r.out);
  }
  {
    OpenSshTransport* d = make("localhost");
    string v = "a b 'c' \"d\" $HOME ! \\n `x`\nsecond line";
    d->setEnv("TT_VAL", v);
    setenv("TT_INHERITED", "yes", 1);
    d->unsetEnv("TT_INHERITED");
    d->setDir("/tmp");
    TransportResult r = d->run("printf %s \"$TT_VAL\"; echo; pwd; echo \"[${TT_INHERITED+set}]\"");
    check("setEnv, unsetEnv and setDir", r.out == v + "\n/tmp\n[]\n", r.out + r.err);
    d->setDir("/nonexistent/it's here");
    TransportResult m = d->run("echo unreachable");
    check("missing directory gives 97", m.status == 97 && m.out.empty(),
          std::to_string(m.status));
    delete d;
  }
  {
    double t0 = now();
    TransportResult r = t->run("sleep 30", 1);
    check("idle timeout", r.timedOut && now() - t0 < 4 && !r.error.empty(),
          std::to_string(now() - t0));
    double t1 = now();
    TransportResult s = t->run(
      "i=0; while [ $i -lt 8 ]; do echo line$i; i=$((i+1)); sleep 0.3; done", 1);
    check("steady output outlives the timeout",
          !s.timedOut && s.status == 0 && now() - t1 > 2.0, s.out + s.error);
  }
  {
    string marker = tmp + "/marker", e;
    double t0 = now();
    long pid = t->spawnDetached("sleep 1; echo done > '" + marker + "'", e);
    check("spawnDetached returns fast", pid > 0 && now() - t0 < 3, e);
    sleep(2);
    check("spawnDetached ran the script", slurp(marker) == "done\n");
    string lg = tmp + "/dlog";
    long p2 = t->spawnDetached("echo hello; echo oops >&2", e, lg);
    sleep(1);
    check("spawnDetached log file", p2 > 0 && slurp(lg) == "hello\noops\n", slurp(lg));
  }

  // ---- the messages users see ----
  {
    const string interactive = "ssh to noauth needs an interactive login; "
      "log in once with 'ssh noauth' (your connection sharing / two-factor), "
      "then try again.";
    // ssh -G must see a config of ours, not the tester's own.
    spit(tmp + "/empty.cfg", "");
    spit(tmp + "/shared.cfg", "Host noauth\n  ControlMaster auto\n  ControlPath " +
         tmp + "/cm-%C\n");
    OpenSshTransport* n = make("noauth");
    n->setConfigFile(tmp + "/empty.cfg");
    string e;
    bool ok = n->connect(e);
    check("refused login: the interactive-login message", !ok && e == interactive, e);
    TransportResult r = n->run("echo hi");
    check("refused login on a command: status -1, same message, mentions the connection",
          r.status == -1 && r.error == interactive &&
          r.error.find("connection") != string::npos, r.error);
    delete n;

    OpenSshTransport* sh1 = make("noauth", "u");
    sh1->setConfigFile(tmp + "/shared.cfg");
    ok = sh1->connect(e);
    const string noMaster = "No shared ssh connection to noauth is open. Run "
      "\"ssh noauth\" once in a terminal (that opens it), then try again.";
    check("shared host, no master: says no shared connection is open",
          !ok && e == noMaster && e.find("password") == string::npos, e);
    TransportResult sr = sh1->run("echo hi");
    check("the same on a command", sr.status == -1 && sr.error == noMaster,
          sr.error);
    delete sh1;

    OpenSshTransport* k = make("badkey");
    ok = k->connect(e);
    check("unknown host key: tells the user to ssh once",
          !ok && e.find("host key of badkey") != string::npos &&
          e.find("\"ssh badkey\"") != string::npos, e);
    delete k;

    OpenSshTransport* d = make("down");
    ok = d->connect(e);
    check("refused connection", !ok && e.find("ssh connection to down failed: "
          "ssh: connect to host down port 22: Connection refused") == 0, e);
    delete d;

    OpenSshTransport* m = make("mux");
    TransportResult mr = m->run("true");
    check("shared connection killed mid-way", mr.status == -1 &&
          mr.error.find("ssh connection to mux failed") == 0 &&
          mr.error.find("connection") != string::npos, mr.error);
    delete m;

    OpenSshTransport* x = new OpenSshTransport("localhost");
    x->setSshProgram("/nonexistent/ssh");
    TransportResult xr = x->run("true");
    check("no ssh client", xr.status == -1 && !xr.error.empty(), xr.error);
    delete x;

    check("explainFailure ignores ordinary text",
          OpenSshTransport::explainFailure("h", "ls: cannot access x\n").empty());
  }

  // ---- opening the shared connection through askpass ----
  {
    const string master = tmp + "/master", asklog = tmp + "/ask.log";
    setenv("STUB_MASTER", master.c_str(), 1);
    setenv("STUB_ASKLOG", asklog.c_str(), 1);
    const string ok = tmp + "/askpass-ok", cancel = tmp + "/askpass-cancel",
                 slow = tmp + "/askpass-slow";
    spit(ok, "#!/bin/sh\necho \"$1\" >> \"$STUB_ASKLOG\"\necho secret\n", 0755);
    spit(cancel, "#!/bin/sh\necho \"$1\" >> \"$STUB_ASKLOG\"\nexit 1\n", 0755);
    spit(slow, "#!/bin/sh\necho \"$1\" >> \"$STUB_ASKLOG\"\nsleep 30\necho secret\n", 0755);
    spit(tmp + "/shared.cfg", "Host noauth needmaster\n  ControlMaster auto\n  ControlPath " +
         tmp + "/cm-%C\n");
    const string noMaster = "No shared ssh connection to needmaster is open. Run "
      "\"ssh needmaster\" once in a terminal (that opens it), then try again.";
    auto asked = [&]() {
      string s = slurp(asklog);
      int n = 0;
      for (size_t i = 0; i < s.size(); i++) n += s[i] == '\n';
      return n;
    };
    auto mk = [&](const string& askpass) {
      OpenSshTransport* x = make("needmaster", "u");
      x->setConfigFile(tmp + "/shared.cfg");
      if (!askpass.empty()) x->setAskpass(askpass);
      return x;
    };
    string e;

    OpenSshTransport::forgetMasterAttempts();
    sh("rm -f " + master + " " + asklog);
    sh("rm -f " + log);
    OpenSshTransport* a = mk(ok);
    bool c = a->connect(e);
    check("no master: askpass answers and the connection is made", c, e);
    check("the master exists and askpass was asked once, with the prompt",
          slurp(master) == "" && access(master.c_str(), F_OK) == 0 && asked() == 1 &&
          slurp(asklog) == "u@needmaster's password: \n", slurp(asklog));
    string args = slurp(log);
    check("opened with -f -N, not in batch mode, askpass forced for that host and user",
          args.find("-f -N ") != string::npos && args.find("BatchMode=no") != string::npos &&
          args.find("env: force needmaster u") != string::npos, args);
    TransportResult r = a->run("echo hi");
    check("the command runs, and nothing more is asked",
          r.status == 0 && r.out == "hi\n" && asked() == 1, r.out + r.error);
    delete a;

    sh("rm -f " + master + " " + asklog);
    OpenSshTransport::forgetMasterAttempts();
    OpenSshTransport* b = mk(cancel);
    c = b->connect(e);
    check("cancelled: the no-shared-connection message", !c && e == noMaster, e);
    check("cancelled: asked once", asked() == 1);
    c = b->connect(e);
    check("not asked again straight away", !c && e == noMaster && asked() == 1,
          std::to_string(asked()));
    OpenSshTransport* b2 = mk(ok);
    c = b2->connect(e);
    check("nor by another connection to the same host", !c && e == noMaster && asked() == 1);
    // The user opened it by hand in the meantime.
    spit(master, "");
    c = b2->connect(e);
    check("a master opened by hand is used without asking", c && asked() == 1, e);
    delete b; delete b2;

    sh("rm -f " + master + " " + asklog);
    OpenSshTransport::forgetMasterAttempts();
    OpenSshTransport* n = mk("");
    sh("rm -f " + log);
    c = n->connect(e);
    check("no askpass: the message, and no attempt to open one",
          !c && e == noMaster && slurp(log).find("-f -N") == string::npos, e);
    delete n;

    OpenSshTransport::forgetMasterAttempts();
    OpenSshTransport* s = mk(slow);
    s->setAskTimeout(2);
    double t0 = now();
    c = s->connect(e);
    check("an unanswered prompt gives up", !c && e == noMaster && now() - t0 < 8 &&
          access(master.c_str(), F_OK) != 0, e + " after " + std::to_string(now() - t0));
    delete s;

    OpenSshTransport::forgetMasterAttempts();
    sh("rm -f " + asklog);
    OpenSshTransport* u = make("noauth", "u");
    u->setConfigFile(tmp + "/empty.cfg");
    u->setAskpass(ok);
    c = u->connect(e);
    check("a host that does not share connections is never asked about",
          !c && e.find("interactive login") != string::npos && asked() == 0, e);
    delete u;
  }

  // ---- files ----
  {
    string src = tmp + "/src.bin", dst = tmp + "/dst.bin", back = tmp + "/back.bin";
    string data;
    for (int i = 0; i < 3 * 1024 * 1024 + 17; i++) data += (char)(i * 31 + i / 251);
    data[100] = 0; data[200] = '\n';
    spit(src, data, 0750);
    string e;
    bool p = t->put(src, dst, e);
    struct stat sb;
    stat(dst.c_str(), &sb);
    check("put: bytes and mode", p && slurp(dst) == data && (sb.st_mode & 07777) == 0750, e);
    bool g = t->get(dst, back, e);
    stat(back.c_str(), &sb);
    check("get: bytes and mode", g && slurp(back) == data && (sb.st_mode & 07777) == 0750, e);
    check("get of a missing file fails with a message",
          !t->get(tmp + "/nonexistent", tmp + "/x", e) && !e.empty(), e);
    check("put into a missing directory fails with a message",
          !t->put(src, tmp + "/nodir/f", e) && !e.empty(), e);
    string weird = tmp + "/it's a \"file\" $x";
    check("a name that needs quoting", t->put(src, weird, e) && slurp(weird) == data, e);
    spit(tmp + "/home_marker", "m");
    setenv("HOME", tmp.c_str(), 1);
    check("~ expands on the remote side", t->put(src, "~/tilde.bin", e) &&
          slurp(tmp + "/tilde.bin") == data, e);
    check("remoteKind", t->remoteKind(tmp) == 1 && t->remoteKind(src) == 0 &&
          t->remoteKind(tmp + "/nope") == -1);
    std::vector<string> g1;
    check("remoteGlob", t->remoteGlob(tmp + "/*.bin", g1, e) && g1.size() >= 3 &&
          g1[0] == tmp + "/back.bin", e);
    check("remoteGlob without a match is an error",
          !t->remoteGlob(tmp + "/nomatch*", g1, e) && !e.empty());
  }
  {
    string e;
    sh("mkdir -p " + tmp + "/tree/sub && echo a > " + tmp + "/tree/a && "
       "echo b > " + tmp + "/tree/sub/b && chmod 755 " + tmp + "/tree/a && "
       "mkdir " + tmp + "/into");
    check("putTree into an existing directory lands inside",
          t->putTree(tmp + "/tree", tmp + "/into", e) &&
          slurp(tmp + "/into/tree/sub/b") == "b\n", e);
    struct stat sb;
    stat((tmp + "/into/tree/a").c_str(), &sb);
    check("putTree keeps modes", (sb.st_mode & 0777) == 0755);
    check("putTree to a new name", t->putTree(tmp + "/tree", tmp + "/renamed", e) &&
          slurp(tmp + "/renamed/a") == "a\n", e);
    sh("mkdir " + tmp + "/gdst");
    check("getTree into an existing directory lands inside",
          t->getTree(tmp + "/tree", tmp + "/gdst", e) &&
          slurp(tmp + "/gdst/tree/sub/b") == "b\n", e);
    check("getTree to a new name", t->getTree(tmp + "/tree", tmp + "/gnew", e) &&
          slurp(tmp + "/gnew/a") == "a\n", e);
    check("getTree of a file", t->getTree(tmp + "/tree/a", tmp + "/gfile", e) &&
          slurp(tmp + "/gfile") == "a\n", e);
    check("getTree of nothing fails", !t->getTree(tmp + "/nothing", tmp + "/gx", e));
  }

  // ---- streams ----
  {
    string e;
    t->setDir(tmp);
    int fd = -1;
    RemoteStream* s = t->openStream("pwd; echo err 1>&2; cat; echo after-eof", fd, e);
    check("stream opens", s && fd > 2, e);
    string first = readFor(fd, "err\n", 5000);
    check("stream runs in the directory with stderr merged",
          first.find(tmp + "\n") != string::npos && first.find("err\n") != string::npos);
    ssize_t w = write(fd, "ping 1\n", 7);
    check("write reaches the script", w == 7 &&
          readFor(fd, "ping 1\n", 5000).find("ping 1\n") != string::npos);
    string big(200000, 'x');
    big += "\n";
    size_t off = 0;
    string got;
    while (off < big.size() || got.size() < big.size()) {
      if (off < big.size()) {
        ssize_t n = write(fd, big.data() + off, big.size() - off);
        if (n > 0) off += n;
      }
      fd_set f; FD_ZERO(&f); FD_SET(fd, &f);
      struct timeval tv = { 0, 20000 };
      if (select(fd + 1, &f, 0, 0, &tv) > 0) {
        char b[65536];
        ssize_t n = read(fd, b, sizeof b);
        if (n <= 0) break;
        got.append(b, n);
      }
    }
    check("a large reply survives in both directions", got == big, std::to_string(got.size()));
    double t0 = now();
    t->closeStream(s, 3000);
    check("closeStream is prompt", now() - t0 < 4, std::to_string(now() - t0));
    // Nothing of the script is left running.
    check("closeStream ended the cat", sh("pgrep -f 'cat' -P 1 >/dev/null 2>&1; true") == 0);
  }
  {
    string e;
    int fd = -1;
    RemoteStream* s = t->openStream("echo up; sleep 60", fd, e);
    readFor(fd, "up\n", 5000);
    usleep(300000);   // let the script reach its sleep: a signal during fork() orphans it
    double t0 = now();
    t->interruptStream(s);
    string rest = readFor(fd, "NEVER", 3000);   // returns at EOF
    char c;
    bool eof = read(fd, &c, 1) == 0;
    check("interruptStream ends the stream", eof && now() - t0 < 3.5, std::to_string(now() - t0));
    t->closeStream(s, 500);
    int fd2 = -1;
    RemoteStream* n = make("noauth")->openStream("cat", fd2, e);
    // The stand-in exits at once, so the script cannot be delivered.
    char b[16];
    ssize_t r = n ? read(fd2, b, sizeof b) : 0;
    check("a stream that cannot connect shows EOF, not junk", !n || r == 0 || r < 0);
    if (n) make("noauth")->closeStream(n, 100);
  }
  delete t;

  // ---- choosing the backend ----
  {
    check("default config: no sharing", !OpenSshTransport::sharesConnection(
      "host x\ncontrolmaster false\ncontrolpersist no\n"));
    check("ControlMaster auto: sharing", OpenSshTransport::sharesConnection(
      "controlmaster auto\ncontrolpath /home/u/.ssh/cm-1\ncontrolpersist 600\n"));
    check("ControlMaster true: sharing", OpenSshTransport::sharesConnection("controlmaster true\n"));
    check("ControlMaster autoask: sharing", OpenSshTransport::sharesConnection("controlmaster autoask\n"));
    check("ControlPath alone: sharing (a master started by hand)",
          OpenSshTransport::sharesConnection("controlmaster false\ncontrolpath /tmp/x-u\n"));
    check("ControlPath none: no sharing", !OpenSshTransport::sharesConnection(
      "controlmaster no\ncontrolpath none\n"));
    check("empty output: no sharing", !OpenSshTransport::sharesConnection(""));
    check("case does not matter", OpenSshTransport::sharesConnection("ControlMaster Auto\n"));

    // The real ssh -G on private config files.
    string c1 = tmp + "/cfg_cm", c2 = tmp + "/cfg_plain", c3 = tmp + "/cfg_path",
           c4 = tmp + "/cfg_none";
    spit(c1, "Host shared\n  ControlMaster auto\n  ControlPath ~/.ssh/cm-%C\n  ControlPersist 10m\n");
    spit(c2, "Host plain\n  User nobody\n");
    spit(c3, "Host pathonly\n  ControlPath /tmp/cm-%r@%h\n");
    spit(c4, "Host off\n  ControlMaster no\n  ControlPath none\n");
    check("ssh -G: ControlMaster auto + ControlPath",
          OpenSshTransport::configSharesConnection("shared", "", c1));
    check("ssh -G: host without sharing",
          !OpenSshTransport::configSharesConnection("plain", "", c2));
    check("ssh -G: ControlPath only",
          OpenSshTransport::configSharesConnection("pathonly", "", c3));
    check("ssh -G: explicitly off",
          !OpenSshTransport::configSharesConnection("off", "", c4));
    check("ssh -G: another host of the same file is unaffected",
          !OpenSshTransport::configSharesConnection("elsewhere", "", c1));

    check("proxyjump: proxied", OpenSshTransport::proxiesConnection(
      "proxyusefdpass no\nproxyjump u@jump\n"));
    check("proxycommand: proxied", OpenSshTransport::proxiesConnection(
      "proxycommand ssh -W %h:%p jump\n"));
    check("proxyusefdpass alone: not proxied", !OpenSshTransport::proxiesConnection(
      "proxyusefdpass no\ncontrolmaster false\n"));
    check("proxyjump none: not proxied", !OpenSshTransport::proxiesConnection("proxyjump none\n"));
    string c5 = tmp + "/cfg_pj";
    spit(c5, "Host viajump\n  ProxyJump u@somewhere\nHost viacmd\n  ProxyCommand ssh -W %h:%p x\n"
             "Host pjshared\n  ProxyJump somewhere\n  ControlMaster auto\n  ControlPath ~/.ssh/cm-%C\n");
    check("ssh -G: ProxyJump", OpenSshTransport::configProxiesConnection("viajump", "", c5));
    check("ssh -G: ProxyCommand", OpenSshTransport::configProxiesConnection("viacmd", "", c5));
    check("ssh -G: no proxy", !OpenSshTransport::configProxiesConnection("direct", "", c5));

    typedef OpenSshTransport O;
    unsetenv("ECCE_SSH_BACKEND");
    check("auto: ProxyJump -> openssh", O::backendFor("viajump", "", c5) == O::BACKEND_OPENSSH);
    check("auto: ProxyCommand -> openssh", O::backendFor("viacmd", "", c5) == O::BACKEND_OPENSSH);
    check("auto: no proxy -> libssh", O::backendFor("direct", "", c5) == O::BACKEND_LIBSSH);
    check("a proxied host that shares is left alone",
          O::configSharesConnection("pjshared", "", c5) &&
          O::configProxiesConnection("pjshared", "", c5));
    check("auto: shared -> openssh", O::backendFor("shared", "", c1) == O::BACKEND_OPENSSH);
    check("auto: plain -> libssh", O::backendFor("plain", "", c2) == O::BACKEND_LIBSSH);
    setenv("ECCE_SSH_BACKEND", "libssh", 1);
    check("ECCE_SSH_BACKEND=libssh wins", O::backendFor("shared", "", c1) == O::BACKEND_LIBSSH);
    check("ECCE_SSH_BACKEND=libssh wins over a proxy", O::backendFor("viajump", "", c5) == O::BACKEND_LIBSSH);
    setenv("ECCE_SSH_BACKEND", "openssh", 1);
    check("ECCE_SSH_BACKEND=openssh wins", O::backendFor("plain", "", c2) == O::BACKEND_OPENSSH);
    setenv("ECCE_SSH_BACKEND", "auto", 1);
    check("ECCE_SSH_BACKEND=auto", O::backendFor("shared", "", c1) == O::BACKEND_OPENSSH);
    setenv("ECCE_SSH_BACKEND", "bogus", 1);
    check("an unknown value means auto", O::requestedBackend() == O::BACKEND_AUTO);
    unsetenv("ECCE_SSH_BACKEND");
  }

  sh("rm -rf '" + tmp + "'");
  std::cout << (failures ? "FAILED" : "all passed") << std::endl;
  return failures ? 1 : 0;
}
