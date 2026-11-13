// eccebroker: the built-in broker (#990) as a program, for tests and by-hand
// checks on any platform.
//   eccebroker USER:PASSWORD [port]   listen on 127.0.0.1, print the port
//   eccebroker --session              the session broker (ECCE_SESSION_ID,
//                                     ECCE_REALUSERHOME): writes the broker file
// Runs until its standard input is closed; the session broker then lingers as
// the Organizer's does (--session [grace seconds], default 10).
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "util/MqttBroker.H"

int main(int argc, char** argv)
{
  std::string a = argc > 1 ? argv[1] : "";
  MqttBroker b;
  bool session = a == "--session";
  if (session) {
    std::string e = MqttBroker::startForSession();
    if (!e.empty()) { std::cerr << "eccebroker: " << e << std::endl; return 1; }
    std::cout << "session broker started" << std::endl;
  } else {
    size_t c = a.find(':');
    if (c == std::string::npos) { std::cerr << "usage: eccebroker USER:PASSWORD [port] | --session" << std::endl; return 2; }
    b.addUser(a.substr(0, c), a.substr(c + 1));
    if (!b.start(argc > 2 ? atoi(argv[2]) : 0)) { std::cerr << "eccebroker: cannot listen" << std::endl; return 1; }
    std::cout << b.port() << std::endl;
  }
  while (getchar() != EOF) {}
  if (session) MqttBroker::lingerForSession(argc > 2 ? atoi(argv[2]) : 10);
  return 0;
}
