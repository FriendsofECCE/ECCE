// A host that accepts TCP on its ssh port but never sends the ssh banner
// (a hung sshd) must fail within the connect timeout, OpenSSH and libssh
// alike, with no libssh retry after OpenSSH found the host silent.
// Everything runs in a private HOME with its own ssh config (a wrapper ssh
// on PATH passes -F), so the real ~/.ssh is never read or written.
// ECCE_SSH_CONNECT_TIMEOUT shortens the wait; exit 77 without /usr/bin/ssh.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "comm/RCommand.H"
#include "comm/OpenSshTransport.H"
#ifdef ECCE_HAVE_LIBSSH
#include "comm/SshTransport.H"
#endif

using namespace std;

static int failures = 0;
static void check(const string& name, bool ok)
{
  cout << (ok ? "ok   " : "FAIL ") << name << endl;
  if (!ok) failures++;
}

static double since(chrono::steady_clock::time_point t0)
{
  return chrono::duration<double>(chrono::steady_clock::now() - t0).count();
}

// Accepts and holds every connection, never writing a byte.
static int silentListener(int& port)
{
  int s = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in a;
  memset(&a, 0, sizeof a);
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(s, (sockaddr*)&a, sizeof a) != 0 || listen(s, 16) != 0) return -1;
  socklen_t l = sizeof a;
  getsockname(s, (sockaddr*)&a, &l);
  port = ntohs(a.sin_port);
  thread([s]() {
    vector<int> held;
    for (;;) { int c = accept(s, 0, 0); if (c < 0) break; held.push_back(c); }
  }).detach();
  return s;
}

int main()
{
  // Only a host that did not answer skips the libssh retry; a master that
  // failed for another reason still gets it.
  check("banner timeout: no answer", RCommand::hostDidNotAnswer(
        "ssh: Connection timed out during banner exchange"));
  check("refused: no answer", RCommand::hostDidNotAnswer(
        "ssh: connect to host x port 22: Connection refused"));
  check("unknown name: no answer", RCommand::hostDidNotAnswer(
        "ssh: Could not resolve hostname x: Name or service not known"));
  check("forwarding refused: answered", !RCommand::hostDidNotAnswer(
        "channel 0: open failed: administratively prohibited"));
  check("master failure: answered", !RCommand::hostDidNotAnswer(
        "ControlSocket /x/y already exists, disabling multiplexing"));

  if (access("/usr/bin/ssh", X_OK) != 0) { cout << "no /usr/bin/ssh" << endl; return 77; }
  int port = 0;
  if (silentListener(port) < 0) { cout << "cannot listen" << endl; return 1; }

  char tmpl[] = "/tmp/ecce-silent-XXXXXX";
  const string dir = mkdtemp(tmpl);
  const string ssh = dir + "/.ssh", bin = dir + "/bin", cm = dir + "/cm";
  mkdir(ssh.c_str(), 0700); mkdir(bin.c_str(), 0700); mkdir(cm.c_str(), 0700);
  const string cfg = ssh + "/config";
  ofstream(cfg) << "Host silenthost\n  HostName 127.0.0.1\n  Port " << port
                << "\n  UserKnownHostsFile " << ssh << "/known_hosts\n"
                << "  StrictHostKeyChecking yes\n  BatchMode yes\n";
  ofstream(bin + "/ssh") << "#!/bin/sh\nexec /usr/bin/ssh -F " << cfg << " \"$@\"\n";
  chmod((bin + "/ssh").c_str(), 0755);
  setenv("HOME", dir.c_str(), 1);
  setenv("ECCE_REALUSERHOME", dir.c_str(), 1);
  if (!getenv("ECCE_REALUSER")) setenv("ECCE_REALUSER", getenv("USER") ? getenv("USER") : "ecce", 1);
  setenv("PATH", (bin + ":" + getenv("PATH")).c_str(), 1);
  setenv("ECCE_HOME", dir.c_str(), 1);   // no scripts/ecce-askpass: no dialog
  unsetenv("DISPLAY");
  unsetenv("WAYLAND_DISPLAY");
  const char* te = getenv("ECCE_SSH_CONNECT_TIMEOUT");
  const double limit = (te ? atof(te) : 60) + 10;

  // RCommand as the Organizer uses it, with and without ECCE's own master.
  // libssh reads the passwd home's config, not this one, so a libssh retry
  // would show as a "resolve" error in place of the timeout.
  setenv("ECCE_SSH_BACKEND", "openssh", 1);
  for (int owned = 0; owned < 2; owned++) {
    if (owned) setenv("ECCE_SSH_CONTROL_DIR", cm.c_str(), 1);
    else unsetenv("ECCE_SSH_CONTROL_DIR");
    auto t0 = chrono::steady_clock::now();
    // A password skips the machine registry lookup (no ECCE install here).
    RCommand rc("silenthost", "ssh", "bash", "", "unused");
    const double dt = since(t0);
    const string tag = owned ? "owned master" : "plain";
    cout << tag << ": " << dt << " s: " << rc.commError() << endl;
    check(tag + ": connect fails", !rc.isOpen());
    check(tag + ": within " + to_string((int)limit) + " s", dt < limit);
    check(tag + ": message names the timeout",
          rc.commError().find("timed out") != string::npos);
  }

#ifdef ECCE_HAVE_LIBSSH
  {
    unsetenv("ECCE_SSH_CONTROL_DIR");
    unsetenv("ECCE_SSH_BACKEND");
    SshTransport t("127.0.0.1", port, "");
    t.setConnectTimeout(te ? atoi(te) : 60);
    t.setPasswordAttempts(3);
    string err;
    auto t0 = chrono::steady_clock::now();
    bool ok = t.connect(err);
    const double dt = since(t0);
    cout << "libssh: " << dt << " s: " << err << endl;
    check("libssh: connect fails", !ok);
    check("libssh: within " + to_string((int)limit) + " s", dt < limit);
  }
#endif
  if (system(("rm -rf " + dir).c_str()) != 0) {}
  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}
