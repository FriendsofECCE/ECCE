// One HEAD request through the real data-server client (EcceDAVClient ->
// CHTTPConnection -> plain or TLS socket).  Driven by run_tests.py.
//
//   tls_client_test <url> ok|cert|fail
//     ok    expects HTTP 200
//     cert  expects EcceDAVStatus::CERTIFICATE_REJECTED
//     fail  expects UNABLE_TO_CONNECT (so not CERTIFICATE_REJECTED, not 200)

#include <iostream>
#include <string>

#include "dsm/EcceDAVClient.H"
#include "dsm/EcceDAVStatus.H"
#include "util/EcceURL.H"

int main(int argc, char ** argv)
{
  if (argc != 3) return 2;
  EcceDAVClient client((EcceURL(argv[1])));
  int status = client.head();
  std::string want = argv[2];
  int expect = want == "ok"   ? 200
             : want == "cert" ? (int) EcceDAVStatus::CERTIFICATE_REJECTED
                              : (int) EcceDAVStatus::UNABLE_TO_CONNECT;
  std::cout << argv[1] << " -> " << status << " "
            << EcceDAVStatus::text(status) << std::endl;
  return status == expect ? 0 : 1;
}
