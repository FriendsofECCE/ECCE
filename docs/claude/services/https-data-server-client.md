---
type: rule
title: "The data-server client speaks TLS only when the URL says https, and never falls back"
area: services
section: "Pitfalls"
paths: ["src/dsm/cipc/CTLSSocket.C", "src/dsm/cipc/CHTTPConnection.C", "src/dsm/dav/EcceDAVClient.C", "tests/tls/run_tests.py"]
issues: [236]
---
**`https://` selects `ipc::CTLSClientSocket` (OpenSSL); `http://` is the
unchanged plain socket. There is no fallback from one to the other.**
- Trust: if `$ECCE_HOME/siteconfig/RemoteServer/server.pem` exists it is the
  only trust anchor and the presented certificate must be identical to it
  (host name unchecked). A pin file that exists but cannot be read is an
  error, not a reason to use the system store. Without the file the system
  store and host-name verification apply (CA mode).
- A rejected certificate gives `EcceDAVStatus::CERTIFICATE_REJECTED` (EDSI
  message `CERTIFICATE_REJECTED`); other TLS failures are `UNABLE_TO_CONNECT`.
- `poll()` must check `SSL_pending()` first, and a readable fd is confirmed
  by a non-blocking `SSL_peek` (TLS 1.3 tickets make the fd readable with no
  application data).
- The TLS socket cannot be copied; a copied `CHTTPConnection` that held a TLS
  session starts disconnected and re-verifies on `connect()`.
- https defaults to port 443, and the `Host` header omits the port only for
  the scheme's default.
- `ctest -R tls_client` covers pinned, CA, mismatch and no-fallback cases.
