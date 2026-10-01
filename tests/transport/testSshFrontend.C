// SshTransport through a front end (#204): a forwarded connection, a nested
// ssh, and the fall-back between them, without RCommand, so it builds with
// just g++ and libssh.  Usage: testSshFrontend KEYFILE FRONT_PORT MODE, where
// MODE is forward or nested (what the front end is expected to end up doing).
// The front end is 127.0.0.1:FRONT_PORT, the target is called "node" and
// must be reachable from it only; host keys are accepted into a scratch file.

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

#include "comm/SshTransport.H"

static int failures = 0;

static void check(const std::string& name, bool ok, const std::string& why = "")
{
  std::cout << (ok ? "ok   " : "FAIL ") << name;
  if (!ok && !why.empty()) std::cout << "  (" << why << ")";
  std::cout << std::endl;
  if (!ok) failures++;
}

static std::string slurp(const std::string& f)
{
  std::ifstream in(f.c_str(), std::ios::binary);
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

int main(int argc, char** argv)
{
  if (argc < 4) { std::cerr << "usage: testSshFrontend KEYFILE PORT MODE\n"; return 2; }
  std::string key = argv[1], mode = argv[3];
  int port = atoi(argv[2]);
  char tmpl[] = "/tmp/testSshFrontXXXXXX";
  std::string dir = mkdtemp(tmpl);

  for (int u = 0; u < 2; u++) {
    std::string user = u ? "bashuser" : "cshuser";
    std::string err, kh = dir + "/kh_" + user;
    SshTransport t("node", 0, user);
    t.setUseConfig(false);
    t.setKnownHostsFile(kh);
    t.addIdentityFile(key);
    t.setJumpHost("127.0.0.1", port, user);
    std::string asked;
    t.setHostKeyCallback([&](const std::string& h, const std::string&, const std::string&) {
      asked += h + " "; return true; });
    bool ok = t.connect(err);
    check(user + ": connect through the front end", ok, err);
    if (!ok) continue;
    // Forwarded: the client sees both keys.  Nested: the front end checks
    // the node's against its own known_hosts.
    check(user + ": host keys asked about under their own names",
          asked.find("127.0.0.1") != std::string::npos &&
          (asked.find("node") != std::string::npos) == (mode == "forward"), asked);
    check(user + ": mode is " + mode, t.nested() == (mode == "nested"));

    TransportResult r = t.run("hostname; echo $USER-$HOME | sed 's,/home/,,'", 30);
    check(user + ": runs on the node", r.status == 0 &&
          r.out == "node\n" + user + "-" + user + "\n", r.out + r.err + r.error);
    r = t.run("echo err >&2; exit 7", 30);
    check(user + ": exit status and stderr", r.status == 7 && r.err == "err\n", r.err);
    t.setDir("/tmp");
    r = t.run("pwd", 30);
    check(user + ": working directory", r.out == "/tmp\n");
    t.setDir("");
    r = t.run("sleep 3; echo late", 1);
    check(user + ": idle timeout", r.timedOut);

    std::string lf = dir + "/local", back = dir + "/back";
    std::string data;
    for (int i = 0; i < 70000; i++) data += (char)(i * 7 + (i >> 8));
    { std::ofstream f(lf.c_str(), std::ios::binary); f << data; }
    chmod(lf.c_str(), 0750);
    std::string rf = "/tmp/fe_" + user + "_file";
    check(user + ": put", t.put(lf, rf, err), err);
    check(user + ": get", t.get(rf, back, err), err);
    struct stat sb;
    check(user + ": file round trip intact, mode kept",
          slurp(back) == data && stat(back.c_str(), &sb) == 0 &&
          (sb.st_mode & 07777) == 0750);
    check(user + ": remoteKind", t.remoteKind("/tmp") == 1 && t.remoteKind(rf) == 0 &&
          t.remoteKind("/tmp/fe_nothing") == -1);
    r = t.run("rm -f " + rf + "; echo x | cat > /tmp/fe_pid_" + user, 30);

    // The monitor stream opens its own connection through the front end.
    int fd = -1;
    RemoteStream* s = t.openStream("echo stream-$(hostname); cat", fd, err);
    check(user + ": stream opens", s != 0, err);
    if (s) {
      if (write(fd, "ping\n", 5) < 0) {}
      std::string got;
      for (int i = 0; i < 50 && got.find("ping") == std::string::npos; i++) {
        fd_set f; FD_ZERO(&f); FD_SET(fd, &f);
        struct timeval tv = { 0, 100000 };
        if (select(fd + 1, &f, 0, 0, &tv) > 0) {
          char b[256];
          ssize_t n = read(fd, b, sizeof b);
          if (n > 0) got.append(b, n);
        }
      }
      check(user + ": stream runs on the node and echoes", got == "stream-node\nping\n", got);
      t.closeStream(s, 1000);
    }
  }
  std::string cmd = "rm -rf " + dir;
  if (system(cmd.c_str()) != 0) {}
  std::cout << (failures ? "FAILED " : "PASSED ") << failures << std::endl;
  return failures ? 1 : 0;
}
