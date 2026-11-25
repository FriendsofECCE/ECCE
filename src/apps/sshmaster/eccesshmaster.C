// eccesshmaster: ECCE's connection master for the built-in ssh client
// (#1284, comm/SshMaster.H).  Started by the program that needs the shared
// login (Launch, Reconnect); logs in once with the usual dialogs, then
// serves channels to ECCE programs of the same user until unused.
//
//   eccesshmaster -host H -port P -user U -jump J -endpoint EP -status FILE
//                 [-pipe AUTHPIPE]

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "util/Ecce.H"
#include "tdat/AuthCache.H"
#include "comm/RCommand.H"
#include "comm/SshMaster.H"
#include "comm/SshTransport.H"

using std::string;

static void setEnv(const char* name, const char* value)
{
#ifdef _WIN32
  _putenv_s(name, value);
#else
  setenv(name, value, 1);
#endif
}

int main(int argc, char** argv)
{
  string host, user, jump, ep, status, pipe;
  int port = 0;
  for (int i = 1; i + 1 < argc; i += 2) {
    const string k = argv[i], v = argv[i + 1];
    if (k == "-host") host = v;
    else if (k == "-port") port = atoi(v.c_str());
    else if (k == "-user") user = v;
    else if (k == "-jump") jump = v;
    else if (k == "-endpoint") ep = v;
    else if (k == "-status") status = v;
    else if (k == "-pipe") pipe = v;
  }
  if (host.empty() || ep.empty()) {
    std::cerr << "usage: eccesshmaster -host H -port P -user U -jump J "
                 "-endpoint EP -status FILE [-pipe AUTHPIPE]" << std::endl;
    return 2;
  }
#ifndef _WIN32
  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
#endif
  SshMaster::writeStatus(status, "started");
  // The credentials first: the pipe's writer gives up after 10 s.
  if (!pipe.empty()) AuthCache::getCache().pipeIn(pipe);

  // Another master of this route is logging in or serving: use it.
  const auto until = std::chrono::steady_clock::now() + std::chrono::minutes(15);
  while (!SshMaster::lockRoute(ep)) {
    if (SshMaster::live(ep)) { SshMaster::writeStatus(status, "ok"); return 0; }
    if (std::chrono::steady_clock::now() >= until) {
      SshMaster::writeStatus(status, "failed\nanother eccesshmaster holds " + ep);
      return 1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  if (SshMaster::live(ep)) { SshMaster::writeStatus(status, "ok"); return 0; }

  // This process is the master: its own connection is the login.
  setEnv("ECCE_SSH_MASTER", "serve");
  setEnv("ECCE_SSH_BACKEND", "libssh");
  RCommand rc(host, "ssh", "bash", user, "", jump, "", "", "", "", port);
  SshTransport* t = rc.isOpen() ? rc.libsshTransport() : 0;
  if (!t) {
    const char* kind = rc.loginCancelled() ? "cancelled" :
                       rc.loginRefused() ? "refused" :
                       rc.loginUnreachable() ? "unreachable" : "failed";
    SshMaster::writeStatus(status, string(kind) + "\n" + rc.commError());
    return 1;
  }
  std::cerr << "eccesshmaster: logged in to " << host << ", serving " << ep
            << std::endl;
  return SshMaster::serve(*t, ep, SshMaster::idleSeconds(), status);
}
