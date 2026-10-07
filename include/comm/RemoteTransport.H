#ifndef REMOTETRANSPORT_HH
#define REMOTETRANSPORT_HH

#include <string>
#include <vector>

#include "comm/Transport.H"

// A long-running script's stream, owned by the transport that opened it.
struct RemoteStream {
  virtual ~RemoteStream() {}
};

// What RCommand needs from a transport to an ssh machine, so that libssh
// (SshTransport) and the OpenSSH client run as a subprocess (OpenSshTransport)
// are interchangeable.  One connection is used by one thread at a time.
class RemoteTransport : public Transport {
public:
  virtual const std::string& host() const = 0;
  virtual const std::string& user() const = 0;
  virtual const std::string& jumpHost() const = 0;
  // True when a front end is used by running ssh on it rather than by
  // forwarding a connection through it.
  virtual bool nested() const = 0;

  virtual bool connect(std::string& error) = 0;
  virtual void disconnect() = 0;
  virtual bool connected() const = 0;

  // The file mode (permission bits) travels with the content.
  virtual bool put(const std::string& localPath, const std::string& remotePath,
                   std::string& error) = 0;
  virtual bool get(const std::string& remotePath, const std::string& localPath,
                   std::string& error) = 0;
  // scp -r semantics: a directory source is copied with its contents, and
  // when the target is an existing directory the source lands inside it.
  virtual bool putTree(const std::string& localPath, const std::string& remotePath,
                       std::string& error) = 0;
  virtual bool getTree(const std::string& remotePath, const std::string& localPath,
                       std::string& error) = 0;
  // 1 directory, 0 other file, -1 does not exist.
  virtual int remoteKind(const std::string& remotePath) = 0;
  // The remote shell's expansion of a wildcard pattern (sorted; a pattern
  // matching nothing comes back as an error).
  virtual bool remoteGlob(const std::string& pattern,
                          std::vector<std::string>& out, std::string& error) = 0;

  // A long-running script, for a reader that needs a file descriptor.  fd is
  // one end of a socketpair: read the script's stdout and stderr from it,
  // write what the script should get on stdin.  The script runs in dir()
  // with the env set.  Reading EOF means the script or the connection ended.
  virtual RemoteStream* openStream(const std::string& script, int& fd,
                                   std::string& error) = 0;
  // The same, on the login this connection already made, so that nothing
  // asks for credentials again.  Until closeStream() the connection serves
  // only this stream.  A transport whose streams already share its login
  // (the OpenSSH client on a shared connection) needs nothing more.
  virtual RemoteStream* openStreamOnLogin(const std::string& script, int& fd,
                                          std::string& error)
  { return openStream(script, fd, error); }
  // Asks the script to stop, as ^C would.
  virtual void interruptStream(RemoteStream* s) = 0;
  // Sends EOF, gives the script graceMs to finish, then ends it and frees s.
  virtual void closeStream(RemoteStream* s, int graceMs = 2000) = 0;
};

#endif
