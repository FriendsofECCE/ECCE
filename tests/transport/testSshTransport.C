// SshTransport against the disposable sshd in tests/transport/sshd, for
// both the tcsh and the bash account.  Usage: testSshTransport KEYFILE
// [PORT].  Uses its own known_hosts.  Exit 0 only if every check passes.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "comm/SshTransport.H"

static int failures = 0;
static std::string tag;

static void check(const std::string& name, bool ok, const std::string& why = "")
{
  std::cout << (ok ? "ok   " : "FAIL ") << tag << name;
  if (!ok && !why.empty()) std::cout << "  (" << why << ")";
  std::cout << std::endl;
  if (!ok) failures++;
}

static double now()
{
  return std::chrono::duration<double>(
    std::chrono::steady_clock::now().time_since_epoch()).count();
}

static std::string slurp(const std::string& f)
{
  std::ifstream in(f.c_str(), std::ios::binary);
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

static std::string keyFile, dir;
static int port = 2222;

static SshTransport* make(const std::string& user, const std::string& kh)
{
  SshTransport* t = new SshTransport("127.0.0.1", port, user);
  t->setUseConfig(false);
  t->setKnownHostsFile(kh);
  return t;
}

static void runUser(const std::string& user)
{
  tag = user + ": ";
  std::string kh = dir + "/known_hosts_" + user;
  unlink(kh.c_str());
  std::string err;

  // 1. host key
  {
    int calls = 0;
    std::string fp;
    SshTransport* t = make(user, kh);
    t->addIdentityFile(keyFile);
    t->setHostKeyCallback([&](const std::string&, const std::string& f) {
      calls++; fp = f; return false; });
    bool ok = t->connect(err);
    check("unknown host key refused", !ok && calls == 1 &&
          fp.compare(0, 7, "SHA256:") == 0 && access(kh.c_str(), F_OK) != 0,
          err + " fp=" + fp);
    delete t;

    calls = 0;
    t = make(user, kh);
    t->addIdentityFile(keyFile);
    t->setHostKeyCallback([&](const std::string&, const std::string&) {
      calls++; return true; });
    ok = t->connect(err);
    check("unknown host key accepted", ok && calls == 1 &&
          slurp(kh).size() > 20, err);
    delete t;

    calls = 0;
    t = make(user, kh);
    t->addIdentityFile(keyFile);
    t->setHostKeyCallback([&](const std::string&, const std::string&) {
      calls++; return true; });
    ok = t->connect(err);
    check("known host key: no callback", ok && calls == 0, err);
    delete t;
  }

  // 2. authentication
  {
    int prompts = 0;
    SshTransport* t = make(user, kh);
    t->addIdentityFile(keyFile);
    t->setPromptCallback([&](const std::string&, bool, std::string&) {
      prompts++; return false; });
    bool ok = t->connect(err);
    check("key auth", ok && prompts == 0, err);
    delete t;

    t = make(user, kh);
    std::string prompt;
    bool echoSeen = true;
    t->setPromptCallback([&](const std::string& p, bool echo, std::string& a) {
      prompt = p; echoSeen = echo; a = "ecce-test"; return true; });
    ok = t->connect(err);
    TransportResult r;
    if (ok) r = t->run("echo hi");
    check("password auth", ok && r.out == "hi\n" && !echoSeen, err + " " + r.error);
    delete t;

    t = make(user, kh);
    int n = 0;
    t->setPromptCallback([&](const std::string&, bool, std::string& a) {
      n++; a = "wrong-secret"; return true; });
    ok = t->connect(err);
    check("wrong password fails cleanly", !ok && !t->connected() && n >= 1 &&
          err.find("wrong-secret") == std::string::npos, err);
    delete t;
  }

  SshTransport t("127.0.0.1", port, user);
  t.setUseConfig(false);
  t.setKnownHostsFile(kh);
  t.addIdentityFile(keyFile);
  if (!t.connect(err)) { check("connect", false, err); return; }

  // 3. run
  {
    TransportResult a = t.run("exit 0"), b = t.run("exit 7");
    check("exit codes", a.status == 0 && b.status == 7,
          std::to_string(a.status) + "," + std::to_string(b.status) + a.error);
  }
  {
    TransportResult r = t.run("echo out; echo err >&2");
    check("stdout/stderr separate", r.out == "out\n" && r.err == "err\n",
          r.out + "|" + r.err);
  }
  {
    TransportResult r = t.run(
      "i=0; while [ $i -lt 2048 ]; do "
      "head -c 1023 /dev/zero | tr '\\0' o; echo; "
      "head -c 1023 /dev/zero | tr '\\0' e >&2; echo >&2; "
      "i=$((i+1)); done", 60);
    check("2 MB on each stream", r.status == 0 && r.out.size() == 2048 * 1024 &&
          r.err.size() == 2048 * 1024,
          std::to_string(r.out.size()) + "," + std::to_string(r.err.size()) +
          " st=" + std::to_string(r.status) + r.error);
  }
  {
    TransportResult r = t.run(
      "echo 'a!b'; echo `echo tick`; echo $((6*7)); cat <<'EOT'\n"
      "here $HOME !! `x`\nEOT\n");
    check("csh-hostile text verbatim",
          r.out == "a!b\ntick\n42\nhere $HOME !! `x`\n", r.out + "|" + r.err);
  }
  {
    TransportResult r = t.run("cat; read x; echo \"x=[$x]\"; echo after");
    check("stdin is empty, script continues",
          r.status == 0 && r.out == "x=[]\nafter\n", r.out + "|" + r.err);
  }
  {
    t.setDir("/nonexistent-ecce-dir");
    TransportResult r = t.run("echo should-not-run");
    check("missing dir -> 97", r.status == 97 && r.out.empty(),
          std::to_string(r.status));
    t.setDir("/tmp");
    r = t.run("pwd");
    check("setDir", r.out == "/tmp\n", r.out);
    t.setDir("");
  }
  {
    std::string v = "it's a \"test\" $HOME `id` \\n\nsecond line  ";
    t.setEnv("ECCE_T1", v);
    TransportResult r = t.run("printf %s \"$ECCE_T1\"");
    check("setEnv round trip", r.out == v, r.out + "|" + r.err);
    t.unsetEnv("HOME");
    r = t.run("echo \"[${HOME-unset}]\"");
    check("unsetEnv", r.out == "[unset]\n", r.out);
    t.setEnv("HOME", "/tmp");
    t.setEnv("ECCE_T1", "");
  }

  // 4. idle timeout
  {
    double t0 = now();
    TransportResult r = t.run("sleep 30", 1);
    double dt = now() - t0;
    check("idle timeout", r.timedOut && dt < 3.0, std::to_string(dt));
    TransportResult ok = t.run("echo still-works");
    check("usable after timeout", ok.out == "still-works\n", ok.error);
  }
  {
    TransportResult r = t.run(
      "i=0; while [ $i -lt 8 ]; do echo tick; sleep 0.3; i=$((i+1)); done", 1);
    check("steady output outlives the timeout",
          !r.timedOut && r.status == 0 && r.out.size() == 40,
          std::to_string(r.out.size()) + r.error);
  }

  // 5. spawnDetached
  {
    double t0 = now();
    std::string e;
    long pid = t.spawnDetached("sleep 20", e);
    double dt = now() - t0;
    TransportResult k = t.run("kill -0 " + std::to_string(pid));
    check("spawnDetached", pid > 0 && dt < 3.0 && k.status == 0,
          e + " pid=" + std::to_string(pid) + " dt=" + std::to_string(dt));
    t.run("kill " + std::to_string(pid));
    long p2 = t.spawnDetached("echo logged", e, "/tmp/ecce-ssh-spawn.log");
    usleep(500000);
    TransportResult l = t.run("cat /tmp/ecce-ssh-spawn.log; rm -f /tmp/ecce-ssh-spawn.log");
    check("spawnDetached log file", p2 > 0 && l.out == "logged\n", l.out + e);
  }

  // 6. SFTP
  {
    std::string local = dir + "/blob", back = dir + "/blob.back";
    std::string data(1 << 20, '\0');
    unsigned x = 12345;
    for (size_t i = 0; i < data.size(); i++) { x = x * 1103515245 + 12345; data[i] = (char)(x >> 16); }
    { std::ofstream o(local.c_str(), std::ios::binary); o << data; }
    chmod(local.c_str(), 0755);
    std::string remote = "/tmp/ecce-ssh-blob-" + user;
    bool p = t.put(local, remote, err);
    TransportResult st = t.run("test -x " + remote + " && echo x");
    bool g = p && t.get(remote, back, err);
    struct stat sb;
    stat(back.c_str(), &sb);
    check("sftp put/get round trip", p && g && slurp(back) == data &&
          st.out == "x\n" && (sb.st_mode & 0777) == 0755, err);
    std::string e2;
    check("sftp get missing file fails", !t.get("/nonexistent-ecce", back, e2) && !e2.empty());
    t.run("rm -f " + remote);
  }

  // 7. changed host key
  {
    t.disconnect();
    std::string kline = slurp(kh);
    // Replace the key blob with a different valid ed25519 key.
    std::ofstream o(kh.c_str());
    o << kline.substr(0, kline.find(' ')) << " ssh-ed25519 "
      "AAAAC3NzaC1lZDI1NTE5AAAAIOMqqnkVzrm0SdG6UOoYmNlmUE5kvjgkqhn9xuyyRsdk\n";
    o.close();
    int calls = 0;
    SshTransport* c = make(user, kh);
    c->addIdentityFile(keyFile);
    c->setHostKeyCallback([&](const std::string&, const std::string&) {
      calls++; return true; });
    bool ok = c->connect(err);
    check("changed host key refused", !ok && calls == 0 &&
          err.find("CHANGED") != std::string::npos, err);
    delete c;
  }
}

int main(int argc, char** argv)
{
  if (argc < 2) { std::cerr << "usage: testSshTransport KEYFILE [PORT]\n"; return 2; }
  if (argc > 2) port = atoi(argv[2]);
  char tmpl[] = "/tmp/testSshXXXXXX";
  dir = mkdtemp(tmpl);
  keyFile = dir + "/key";
  { std::ofstream o(keyFile.c_str(), std::ios::binary); o << slurp(argv[1]); }
  chmod(keyFile.c_str(), 0600);

  runUser("cshuser");
  runUser("bashuser");

  std::string cmd = "rm -rf " + dir;
  if (system(cmd.c_str())) {}
  std::cout << (failures ? "FAILED: " : "all passed: ") << failures
            << " failure(s)" << std::endl;
  return failures ? 1 : 0;
}
