// CTLSSocket.H -- TLS client socket (OpenSSL) for the https data server.
//
// Trust: if $ECCE_HOME/siteconfig/RemoteServer/server.pem exists, that
// certificate is the only trust anchor and the presented certificate must be
// identical to it (host name is not checked, so IP addresses and aliases
// work).  Otherwise the system store is used and the host name is verified.
// There is no mode in which verification is skipped.

#if !defined( __CTLSSOCKET_H )
#define __CTLSSOCKET_H

#include <string>
#include "dsm/CSocket.H"

typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_st     SSL;

namespace ipc {

class CTLSError : public CSocketError
{
public :
  // certificate = true when the server's certificate was rejected
  CTLSError(const std::string& msg, bool certificate)
    : CSocketError(CreateError), msg_(msg), cert_(certificate) {}
  virtual ~CTLSError(void) {}

  bool certificateRejected(void) const { return cert_; }
  virtual string_type message(void) const { return msg_; }

private :
  std::string msg_;
  bool        cert_;
};

class CTLSClientSocket : public CClientSocket
{
public :

  // Connect, handshake and verify.  Throws CSocketError (TCP) or CTLSError.
  CTLSClientSocket(const string_type& host, port_type port);

  // The SSL session belongs to exactly one socket.
  CTLSClientSocket(const CTLSClientSocket&)              = delete;
  CTLSClientSocket& operator = (const CTLSClientSocket&) = delete;

  virtual ~CTLSClientSocket(void);

  virtual bool poll(size_type seconds = 0, size_type microseconds = 0);
  virtual size_type receive(void * buff, size_type nbytes, int flags = 0);
  virtual size_type send(const string_type&);
  virtual size_type send(const void * buff, size_type nbytes);

  // Path of the pinned certificate, or "" when the system store applies.
  static string_type pinnedCertPath(void);

private :
  SSL_CTX * ctx_;
  SSL     * ssl_;
};

} // namespace ipc

#endif
