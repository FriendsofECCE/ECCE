// Connects the real MqttLink to the broker named by this session's broker
// file and reports how that went. Driven by server_tls.py.
//
//   tls_broker_test <account> <password>
//     exit 0  connected
//     exit 3  the link refused to trust the broker's certificate ("TLS:")
//     exit 1  neither within 10 s

#include <unistd.h>

#include <atomic>
#include <iostream>
#include <string>

#include "util/MqttLink.H"

int main(int argc, char** argv)
{
  if (argc != 3) return 2;
  std::string user = argv[1], pass = argv[2];
  std::atomic<bool> tlsRefused(false);
  MqttLink::setCredentialProvider(
    [user, pass](const std::string&, std::string& u, std::string& p) {
      u = user; p = pass; return true; });
  MqttLink::setRefusalHandler(
    [&](const std::string&, const std::string&, int, const std::string& why) {
      if (why.compare(0, 4, "TLS:") == 0) tlsRefused = true;
    });
  MqttLink& link = MqttLink::instance();
  if (!link.ensureConnected()) { std::cout << "no link" << std::endl; return 1; }
  for (int i = 0; i < 100; i++) {
    if (link.connected()) { std::cout << "connected" << std::endl; return 0; }
    if (tlsRefused) { std::cout << "TLS refused" << std::endl; return 3; }
    usleep(100000);
  }
  std::cout << "timeout" << std::endl;
  return 1;
}
