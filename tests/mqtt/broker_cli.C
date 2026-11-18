// eccebroker: the built-in broker (#990, #1007) as a program.
//   eccebroker USER:PASSWORD [port]   listen on 127.0.0.1, print the port
//   eccebroker --session [grace]      the session broker (ECCE_SESSION_ID,
//                                     ECCE_REALUSERHOME): writes the broker file,
//                                     runs until stdin is closed, then lingers
//                                     as the Organizer's does (default 10 s)
//   eccebroker --serve [grace]        (Linux, macOS) the local session's broker,
//                                     started by ecce-gateway-start: goes into
//                                     the background once the broker file is
//                                     written and ends grace seconds (default
//                                     10) after its last client left
//   eccebroker --alive SOCKET         (Linux, macOS) exit 0 if something accepts
//                                     a connection on the Unix socket; the reap
//                                     sweep's test for a stale mqtt-*.sock
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#ifndef _WIN32
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#include "util/MqttBroker.H"

#ifndef _WIN32
static void onSignal(int) { MqttBroker::requestStop(); }

// A separate program, not the gateway: `ecce` waits for the gateway, and a
// job store that outlives the session must keep its broker, as with the
// per-user mosquitto.  The fork comes before any thread exists.
static int serve(int grace)
{
  int fds[2];
  if (pipe(fds) != 0) { perror("eccebroker: pipe"); return 1; }
  pid_t pid = fork();
  if (pid < 0) { perror("eccebroker: fork"); return 1; }
  if (pid > 0) {
    close(fds[1]);
    std::string said;
    char buf[512];
    ssize_t n;
    while ((n = read(fds[0], buf, sizeof buf)) > 0) said.append(buf, n);
    close(fds[0]);
    if (said.compare(0, 3, "ok ") == 0) { std::cout << said.substr(3) << std::endl; return 0; }
    std::cerr << "eccebroker: " << (said.empty() ? "the broker process ended" : said) << std::endl;
    return 1;
  }
  close(fds[0]);
  setsid();
  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  signal(SIGTERM, onSignal);
  signal(SIGINT, onSignal);
  int nul = open("/dev/null", O_RDWR);
  // Nothing of the caller's held open: ecce-gateway-start reads our parent's
  // output through a pipe, and would wait for us as well.
  if (nul >= 0) { dup2(nul, 0); dup2(nul, 1); dup2(nul, 2); if (nul > 2) close(nul); }
  std::string e = MqttBroker::startForSession();
  std::string msg = !e.empty() ? e
    : MqttBroker::hostsSession() ? "ok session broker started"
    : "ok session broker already running";
  if (write(fds[1], msg.data(), msg.size()) < 0) {}
  close(fds[1]);
  if (!e.empty()) return 1;
  MqttBroker::serveForSession(grace, 60);
  return 0;
}

static int alive(const char* path)
{
  sockaddr_un a;
  memset(&a, 0, sizeof a);
  if (strlen(path) >= sizeof a.sun_path) return 1;
  a.sun_family = AF_UNIX;
  strcpy(a.sun_path, path);
  int s = socket(AF_UNIX, SOCK_STREAM, 0);
  if (s < 0) return 1;
  int ok = connect(s, (sockaddr*)&a, sizeof a) == 0;
  close(s);
  return ok ? 0 : 1;
}
#endif

int main(int argc, char** argv)
{
  std::string a = argc > 1 ? argv[1] : "";
#ifndef _WIN32
  if (a == "--alive" && argc > 2) return alive(argv[2]);
  if (a == "--serve") return serve(argc > 2 ? atoi(argv[2]) : 10);
#endif
  MqttBroker b;
  bool session = a == "--session";
  if (session) {
    std::string e = MqttBroker::startForSession();
    if (!e.empty()) { std::cerr << "eccebroker: " << e << std::endl; return 1; }
    std::cout << "session broker started" << std::endl;
  } else {
    size_t c = a.find(':');
    if (c == std::string::npos) { std::cerr << "usage: eccebroker USER:PASSWORD [port] | --session [grace] | --serve [grace]" << std::endl; return 2; }
    b.addUser(a.substr(0, c), a.substr(c + 1));
    if (!b.start(argc > 2 ? atoi(argv[2]) : 0)) { std::cerr << "eccebroker: cannot listen" << std::endl; return 1; }
    std::cout << b.port() << std::endl;
  }
  while (getchar() != EOF) {}
  if (session) MqttBroker::lingerForSession(argc > 2 ? atoi(argv[2]) : 10);
  return 0;
}
