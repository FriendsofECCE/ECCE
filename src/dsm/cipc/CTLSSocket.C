// CTLSSocket.C -- TLS client socket (OpenSSL); see CTLSSocket.H.

#include "dsm/CTLSSocket.H"
#include "util/RemoteServerDir.H"

#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/pem.h>

#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace ipc {

static std::string sslErrors(void)
{
  std::string s;
  unsigned long e;
  while ((e = ERR_get_error()) != 0) {
    char buf[256];
    ERR_error_string_n(e, buf, sizeof(buf));
    if (!s.empty()) s += "; ";
    s += buf;
  }
  return s;
}

// OpenSSL writes with write(2), so a server that closed would raise SIGPIPE
// and kill the GUI; block it for the call and discard one raised by it.
// macOS has no sigtimedwait; there the socket carries SO_NOSIGPIPE instead
// (csocket_open), so the guard has nothing to do.
#ifdef __APPLE__
class SigpipeGuard {};
#else
class SigpipeGuard {
public:
  SigpipeGuard(void)
  {
    sigemptyset(&set_);
    sigaddset(&set_, SIGPIPE);
    sigset_t pend;
    sigpending(&pend);
    pendedBefore_ = sigismember(&pend, SIGPIPE) == 1;
    pthread_sigmask(SIG_BLOCK, &set_, &old_);
  }
  ~SigpipeGuard(void)
  {
    sigset_t pend;
    sigpending(&pend);
    if (!pendedBefore_ && sigismember(&pend, SIGPIPE) == 1) {
      struct timespec zero = {0, 0};
      sigtimedwait(&set_, 0, &zero);
    }
    pthread_sigmask(SIG_SETMASK, &old_, 0);
  }
private:
  sigset_t set_, old_;
  bool     pendedBefore_;
};
#endif

CTLSClientSocket::string_type CTLSClientSocket::pinnedCertPath(void)
{
  const char * home = getenv("ECCE_HOME");
  const char * rdir = getenv("ECCE_REMOTE_DIR");
  if ((!home || !*home) && (!rdir || !*rdir)) return "";
  std::string p = remoteServerDir() + "/server.pem";
  return access(p.c_str(), F_OK) == 0 ? p : std::string();
}

CTLSClientSocket::CTLSClientSocket(const string_type& host, port_type port)
  : CClientSocket(host, port), ctx_(0), ssl_(0)
{
  X509 * pinned = 0;
  try {
    ctx_ = SSL_CTX_new(TLS_client_method());
    if (!ctx_)
      throw CTLSError("TLS: cannot create context", false);
    SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);
    SSL_CTX_set_verify(ctx_, SSL_VERIFY_PEER, 0);
    // Servers close without close_notify; HTTP framing detects truncation.
    SSL_CTX_set_options(ctx_, SSL_OP_IGNORE_UNEXPECTED_EOF);

    // A pin file that exists but cannot be used is an error, never a reason
    // to fall back to the system store.
    const std::string pinPath = pinnedCertPath();
    const bool        pin     = !pinPath.empty();
    if (pin) {
      FILE * f = fopen(pinPath.c_str(), "r");
      if (f) {
        pinned = PEM_read_X509(f, 0, 0, 0);
        fclose(f);
      }
      X509_STORE * store = SSL_CTX_get_cert_store(ctx_);
      if (!pinned || !store || !X509_STORE_add_cert(store, pinned))
        throw CTLSError("TLS: cannot use pinned certificate " + pinPath, true);
      // Lets a non-self-signed leaf be the anchor; the exact-match check
      // after the handshake keeps it to that one certificate.
      X509_STORE_set_flags(store, X509_V_FLAG_PARTIAL_CHAIN);
    } else if (SSL_CTX_set_default_verify_paths(ctx_) != 1) {
      throw CTLSError("TLS: cannot load system certificate store", false);
    }

    ssl_ = SSL_new(ctx_);
    if (!ssl_ || !SSL_set_fd(ssl_, fd()))
      throw CTLSError("TLS: cannot create session", false);

    unsigned char addr[sizeof(struct in6_addr)];
    const bool isIP = inet_pton(AF_INET, host.c_str(), addr) == 1 ||
                      inet_pton(AF_INET6, host.c_str(), addr) == 1;
    if (!isIP)
      SSL_set_tlsext_host_name(ssl_, host.c_str());
    if (!pin) {
      int ok = isIP ? X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(ssl_),
                                                    host.c_str())
                    : SSL_set1_host(ssl_, host.c_str());
      if (ok != 1)
        throw CTLSError("TLS: cannot set host name to verify", false);
    }

    int connected;
    {
      SigpipeGuard guard;
      connected = SSL_connect(ssl_);
    }
    if (connected != 1) {
      long vr = SSL_get_verify_result(ssl_);
      if (vr != X509_V_OK)
        throw CTLSError(std::string("Server certificate rejected: ") +
                        X509_verify_cert_error_string(vr), true);
      throw CTLSError("TLS handshake failed: " + sslErrors(), false);
    }

    if (pin) {
      X509 * peer = SSL_get1_peer_certificate(ssl_);
      bool same = peer && X509_cmp(peer, pinned) == 0;
      if (peer) X509_free(peer);
      if (!same)
        throw CTLSError("Server certificate rejected: it is not the "
                        "certificate pinned in " + pinPath, true);
    }
    X509_free(pinned);
  }
  catch (...) {
    if (pinned) X509_free(pinned);
    if (ssl_) SSL_free(ssl_);
    if (ctx_) SSL_CTX_free(ctx_);
    ssl_ = 0; ctx_ = 0;
    throw;
  }
}

CTLSClientSocket::~CTLSClientSocket(void)
{
  if (ssl_) {
    SigpipeGuard guard;
    SSL_shutdown(ssl_);
    SSL_free(ssl_);
  }
  if (ctx_) SSL_CTX_free(ctx_);
}

// Readable on the wire is not readable application data (TLS 1.3 session
// tickets), so confirm with a non-blocking peek before reporting true.
bool CTLSClientSocket::poll(size_type seconds, size_type microseconds)
{
  if (SSL_pending(ssl_) > 0)
    return true;

  struct timeval start, now;
  gettimeofday(&start, 0);
  const long total = (long) seconds * 1000000L + (long) microseconds;

  for (;;) {
    gettimeofday(&now, 0);
    long used = (now.tv_sec - start.tv_sec) * 1000000L +
                (now.tv_usec - start.tv_usec);
    long left = total - used;
    if (left < 0) left = 0;

    if (!CSocket::poll(left / 1000000L, left % 1000000L))
      return false;

    const int fl = fcntl(fd(), F_GETFL, 0);
    fcntl(fd(), F_SETFL, fl | O_NONBLOCK);
    char c;
    int n = SSL_peek(ssl_, &c, 1);
    int e = n > 0 ? SSL_ERROR_NONE : SSL_get_error(ssl_, n);
    ERR_clear_error();
    fcntl(fd(), F_SETFL, fl);

    if (n > 0 || (e != SSL_ERROR_WANT_READ && e != SSL_ERROR_WANT_WRITE))
      return true;       // data, or EOF/error that receive() will report
    if (left == 0)
      return false;
  }
}

CTLSClientSocket::size_type CTLSClientSocket::receive(
  void * buff, size_type nbytes, int flags)
{
  int n = (flags & MSG_PEEK) ? SSL_peek(ssl_, buff, (int) nbytes)
                             : SSL_read(ssl_, buff, (int) nbytes);
  if (n > 0)
    return (size_type) n;
  int e = SSL_get_error(ssl_, n);
  ERR_clear_error();
  if (e == SSL_ERROR_ZERO_RETURN)
    return 0;
  throw CSocketError(CSocketError::ReceiveError);
}

CTLSClientSocket::size_type CTLSClientSocket::send(const string_type& s)
{
  return send(s.c_str(), s.length());
}

CTLSClientSocket::size_type CTLSClientSocket::send(
  const void * buff, size_type nbytes)
{
  SigpipeGuard guard;
  size_type offset = 0;
  while (offset < nbytes) {
    int n = SSL_write(ssl_, (const char *) buff + offset,
                      (int) (nbytes - offset));
    if (n <= 0) {
      ERR_clear_error();
      throw CSocketError(CSocketError::SendError);
    }
    offset += n;
  }
  return offset;
}

} // namespace ipc
