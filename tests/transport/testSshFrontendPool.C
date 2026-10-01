// Front-end logins (#204): inner sessions that go through the same front end
// share one authenticated session to it, so a front end that asks for a
// second factor asks once per process.  Without libssh's pool every inner
// session, and every monitor stream, logged in again.
//   testSshFrontendPool KEYFILE PORT [unpooled]
// The front end is 127.0.0.1:PORT, the target "node" is reachable from it
// only (as in testSshFrontend).  Prints "FRONTEND_LOGINS n", which the
// caller compares with the "Accepted" lines in the front end's sshd log.
// With "unpooled" (ECCE_SSH_FRONTEND_POOL=0) it expects one login per session.

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <sys/select.h>
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

static std::string key, dir;
static int port;

static SshTransport* make(const std::string& user)
{
  SshTransport* t = new SshTransport("node", 0, user);
  t->setUseConfig(false);
  t->setKnownHostsFile(dir + "/kh_" + user);
  t->addIdentityFile(key);
  t->setJumpHost("127.0.0.1", port, user);
  t->setFrontEndMode(SshTransport::FE_FORWARD);
  t->setHostKeyCallback([](const std::string&, const std::string&, const std::string&) {
    return true; });
  return t;
}

static std::string readUntil(int fd, const std::string& want, int ms)
{
  std::string got;
  for (int w = 0; w < ms && got.find(want) == std::string::npos;) {
    fd_set f; FD_ZERO(&f); FD_SET(fd, &f);
    struct timeval tv = { 0, 100000 };
    if (select(fd + 1, &f, 0, 0, &tv) > 0) {
      char b[4096];
      ssize_t n = read(fd, b, sizeof b);
      if (n <= 0) break;
      got.append(b, n);
    } else w += 100;
  }
  return got;
}

int main(int argc, char** argv)
{
  if (argc < 3) { std::cerr << "usage: testSshFrontendPool KEYFILE PORT [unpooled]\n"; return 2; }
  key = argv[1];
  port = atoi(argv[2]);
  const bool unpooled = argc > 3 && std::string(argv[3]) == "unpooled";
  char tmpl[] = "/tmp/testSshPoolXXXXXX";
  dir = mkdtemp(tmpl);
  const int N = 4;
  std::string err;

  // N inner sessions at once, each running commands from its own thread,
  // and a monitor stream on two of them.
  std::vector<std::unique_ptr<SshTransport> > ts;
  for (int i = 0; i < N; i++) ts.emplace_back(make("bashuser"));
  std::vector<std::thread> th;
  std::vector<std::string> res(N), fail(N);
  for (int i = 0; i < N; i++)
    th.emplace_back([&, i] {
      std::string e;
      if (!ts[i]->connect(e)) { fail[i] = "connect: " + e; return; }
      for (int k = 0; k < 5; k++) {
        TransportResult r = ts[i]->run("echo n" + std::to_string(i) + "-" +
                                       std::to_string(k) + "; hostname", 30);
        res[i] += r.out;
        if (r.status != 0) fail[i] += "run: " + r.error + r.err;
      }
    });
  for (size_t i = 0; i < th.size(); i++) th[i].join();
  for (int i = 0; i < N; i++) {
    std::string want;
    for (int k = 0; k < 5; k++)
      want += "n" + std::to_string(i) + "-" + std::to_string(k) + "\nnode\n";
    check("session " + std::to_string(i) + " ran its commands on the node",
          fail[i].empty() && res[i] == want, fail[i] + res[i]);
  }
  int fds[2];
  RemoteStream* st[2];
  for (int i = 0; i < 2; i++) {
    st[i] = ts[i]->openStream("echo stream-$(hostname); cat", fds[i], err);
    check("monitor stream " + std::to_string(i) + " opens", st[i] != 0, err);
  }
  for (int i = 0; i < 2; i++) {
    if (!st[i]) continue;
    if (write(fds[i], "ping\n", 5) < 0) {}
    check("stream " + std::to_string(i) + " echoes",
          readUntil(fds[i], "ping", 5000) == "stream-node\nping\n");
  }
  const int sessions = N + 2;
  int logins = SshTransport::frontEndLogins();
  std::cout << "front-end logins for " << sessions << " sessions: " << logins << std::endl;
  check(unpooled ? "unpooled: one login per session" : "one login for all of them",
        logins == (unpooled ? sessions : 1), std::to_string(logins));

  for (int i = 0; i < 2; i++) if (st[i]) ts[i]->closeStream(st[i], 1000);
  // A closed channel does not end the others.
  ts[0].reset();
  TransportResult r = ts[1]->run("echo still-up", 30);
  check("closing one session leaves the others working", r.out == "still-up\n", r.error);
  ts.clear();

  // Within the idle time the login is kept: a new session reuses it.
  if (!unpooled) {
    std::unique_ptr<SshTransport> t(make("bashuser"));
    bool ok = t->connect(err);
    r = ok ? t->run("echo again", 30) : TransportResult();
    check("a later session reuses the login", ok && r.out == "again\n" &&
          SshTransport::frontEndLogins() == 1, err + std::to_string(SshTransport::frontEndLogins()));
  }

  // Another account is another login.
  {
    std::unique_ptr<SshTransport> c(make("cshuser"));
    bool ok = c->connect(err);
    r = ok ? c->run("echo $USER", 30) : TransportResult();
    check("another account logs in separately", ok && r.out == "cshuser\n", err + r.out);
    int want = unpooled ? sessions + 1 : 2;
    check("two accounts, two logins", SshTransport::frontEndLogins() == want,
          std::to_string(SshTransport::frontEndLogins()));
  }

  // Logged out on request, then in again on demand.
  if (!unpooled) {
    SshTransport::shutdownFrontEnds();
    std::unique_ptr<SshTransport> t(make("bashuser"));
    bool ok = t->connect(err);
    r = ok ? t->run("echo relogin", 30) : TransportResult();
    check("after shutdown the next session logs in again", ok && r.out == "relogin\n" &&
          SshTransport::frontEndLogins() == 3, err + std::to_string(SshTransport::frontEndLogins()));

    // The front end drops the shared session behind our back.
    std::string kill = "ssh -o BatchMode=yes -o StrictHostKeyChecking=no "
      "-o UserKnownHostsFile=" + dir + "/kh_kill -o IdentitiesOnly=yes -i " + key +
      " -p " + std::to_string(port) + " bashuser@127.0.0.1 "
      "'pkill -KILL -u bashuser sshd; true' >/dev/null 2>&1";
    if (system(kill.c_str()) != 0) {}
    sleep(1);
    r = t->run("echo dead", 10);
    check("the inner session notices the loss", r.status != 0 || !r.error.empty());
    t.reset();
    std::unique_ptr<SshTransport> u(make("bashuser"));
    ok = u->connect(err);
    r = ok ? u->run("echo recovered", 30) : TransportResult();
    check("a new session after the loss logs in again and works",
          ok && r.out == "recovered\n" && SshTransport::frontEndLogins() == 4,
          err + r.error + std::to_string(SshTransport::frontEndLogins()));
  }

  std::cout << "FRONTEND_LOGINS " << SshTransport::frontEndLogins() << std::endl;
  SshTransport::shutdownFrontEnds();
  std::string cmd = "rm -rf " + dir;
  if (system(cmd.c_str()) != 0) {}
  std::cout << (failures ? "FAILED " : "PASSED ") << failures << std::endl;
  return failures ? 1 : 0;
}
