// A shared ssh connection (ControlMaster) is the only way in, and it goes
// away mid-session (#204).  Nothing may hang: the monitor stream ends, the
// next command fails at once with a message that says to log in once, a new
// RCommand says the same, and once the user logs in again everything works.
// Run by tests/transport/sshd/controlmaster_test.sh, which sets up the client
// (host "cm", ControlMaster auto, the key moved away after the master is up).
//   testControlMasterLoss USER
// with KEY and KEYAWAY naming the private key's two places.

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

#include <sys/select.h>
#include <unistd.h>

#include "comm/RCommand.H"

using namespace std;

static int failures = 0;

static void check(const string& name, bool ok, const string& why = "")
{
  cout << (ok ? "ok   " : "FAIL ") << name;
  if (!ok && !why.empty()) cout << "  (" << why << ")";
  cout << endl;
  if (!ok) failures++;
}

static int sh(const string& c) { return system(c.c_str()); }

int main(int argc, char** argv)
{
  if (argc == 3 && string(argv[1]) == "--libssh-refused") {
    // ECCE_SSH_BACKEND=libssh with only the shared connection to get in by:
    // libssh cannot use it, which is why the OpenSSH backend exists.
    setenv("ECCE_REALUSERHOME", getenv("HOME"), 1);
    RCommand rc("cm", "ssh", "bash", argv[2]);
    check("libssh cannot reuse the shared connection: " + rc.commError(),
          !rc.isOpen() && rc.sshBackend() == "" && rc.commError() != "");
    cout << (failures ? "FAILED " : "PASSED ") << failures << endl;
    return failures ? 1 : 0;
  }
  if (argc < 2 || !getenv("KEY") || !getenv("KEYAWAY")) {
    cout << "usage: KEY=... KEYAWAY=... testControlMasterLoss USER" << endl;
    return 2;
  }
  const string user = argv[1], host = "cm", key = getenv("KEY"),
               away = getenv("KEYAWAY");
  if (!getenv("ECCE_REALUSER")) setenv("ECCE_REALUSER", getenv("USER") ? getenv("USER") : "root", 1);
  setenv("ECCE_AUTHCACHE_NO_BROADCAST", "1", 1);
  if (getenv("HOME") && !getenv("ECCE_REALUSERHOME"))
    setenv("ECCE_REALUSERHOME", getenv("HOME"), 1);
  setenv("ECCE_TRANSPORT", "ssh", 1);

  const string interactive = "ssh to " + host + " needs an interactive login; "
    "log in once with 'ssh " + host + "' (your connection sharing / two-factor), "
    "then try again.";

  string o;
  RCommand rc(host, "ssh", "bash", user);
  check("opens over the shared connection", rc.isOpen(), rc.commError());
  check("served by the OpenSSH client", rc.sshBackend() == "openssh", rc.sshBackend());
  check("runs a command", rc.execout("echo before", o) && o == "before\r\n", o);
  check("starts the monitor stream", rc.startStream("cat"));
  int fd = rc.expfid();
  check("stream echoes", rc.expwrite("ping") && [&] {
    fd_set f; FD_ZERO(&f); FD_SET(fd, &f);
    struct timeval tv = { 5, 0 };
    char b[64];
    return select(fd + 1, &f, 0, 0, &tv) > 0 && read(fd, b, sizeof b) > 0; }());

  // The master dies.
  check("master told to exit", sh("ssh -O exit -o BatchMode=yes -l " + user + " " + host + " 2>/dev/null") == 0);

  time_t t0 = time(0);
  bool eof = false;
  for (int i = 0; i < 150 && !eof; i++) {
    fd_set f; FD_ZERO(&f); FD_SET(fd, &f);
    struct timeval tv = { 0, 100000 };
    if (select(fd + 1, &f, 0, 0, &tv) > 0) {
      char b[64];
      eof = read(fd, b, sizeof b) <= 0;
    }
  }
  check("the stream ends instead of hanging", eof && time(0) - t0 < 15,
        to_string(time(0) - t0) + " s");
  rc.stopStream(500);

  t0 = time(0);
  bool ok = rc.execout("echo after", o);
  check("a command on the open connection fails at once, with the message",
        !ok && time(0) - t0 < 20 && rc.commError().find(interactive) != string::npos,
        rc.commError());
  check("the connection is marked closed", !rc.isOpen());

  t0 = time(0);
  RCommand rc2(host, "ssh", "bash", user);
  check("a new connection is refused with the message",
        !rc2.isOpen() && rc2.commError() == interactive && time(0) - t0 < 20,
        rc2.commError());

  RCommand rc3(host, "ssh", "bash", user);
  check("and again (nothing is cached as good)", !rc3.isOpen());

  // The user logs in once more, by hand, with the key; then it goes again.
  sh("mv " + away + " " + key);
  check("user logs in again", sh("ssh -fN -o BatchMode=yes -l " + user + " " + host) == 0);
  sh("mv " + key + " " + away);
  RCommand rc4(host, "ssh", "bash", user);
  check("works again over the new master", rc4.isOpen() && rc4.execout("echo recovered", o) &&
        o == "recovered\r\n", rc4.commError());
  check("monitor stream works again", rc4.startStream("cat") && rc4.expwrite("pong"));
  rc4.stopStream(500);

  cout << (failures ? "FAILED " : "PASSED ") << failures << endl;
  return failures ? 1 : 0;
}
