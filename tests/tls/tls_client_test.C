// One HEAD request through the real data-server client (EcceDAVClient ->
// CHTTPConnection -> plain or TLS socket).  Driven by run_tests.py.
//
//   tls_client_test <url> ok|cert|fail
//     ok    expects HTTP 200
//     cert  expects EcceDAVStatus::CERTIFICATE_REJECTED
//     fail  expects UNABLE_TO_CONNECT (so not CERTIFICATE_REJECTED, not 200)
//     trunc GET of a body cut short by the server: anything but 200, and
//           it must return rather than wait for bytes that never come

#include <iostream>
#include <sstream>
#include <string>

#include "dsm/EcceDAVClient.H"
#include "dsm/EcceDAVStatus.H"
#include "util/EcceURL.H"

int main(int argc, char ** argv)
{
  if (argc != 3) return 2;
  EcceDAVClient client((EcceURL(argv[1])));
  std::string want = argv[2];
  std::ostringstream body;
  int status = want == "trunc" ? client.get_document(body) : client.head();
  int expect = want == "ok"   ? 200
             : want == "cert" ? (int) EcceDAVStatus::CERTIFICATE_REJECTED
                              : (int) EcceDAVStatus::UNABLE_TO_CONNECT;
  std::cout << argv[1] << " -> " << status << " "
            << EcceDAVStatus::text(status) << std::endl;
  if (want == "trunc")
    return status != 200 ? 0 : 1;
  return status == expect ? 0 : 1;
}
