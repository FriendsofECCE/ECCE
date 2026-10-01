#ifndef OPENSSHTRANSPORT_HH
#define OPENSSHTRANSPORT_HH

#include <string>
#include <vector>

#include "comm/RemoteTransport.H"

// The RemoteTransport that runs the OpenSSH client as a plain subprocess
// (no pty, BatchMode, the real exit status) through DirectTransport's process
// machinery.  libssh cannot share a connection that `ssh` already holds open
// with ControlMaster, so a user who logs in once with a second factor and
// lets every later ssh reuse that connection needs this one.
//
// Never prompts: when a connection cannot be made without a question, the
// error says to log in once with ssh.  A command is the script sent on stdin,
// as in SshTransport; files are streamed through `cat`.
class OpenSshTransport : public RemoteTransport {
public:
  OpenSshTransport(const std::string& host, int port = 0,
                   const std::string& user = "");
  virtual ~OpenSshTransport();

  // Options: set before connect().
  void setConnectTimeout(int sec) { p_connectTimeout = sec; }
  void setConfigFile(const std::string& path) { p_configFile = path; }
  void setSshProgram(const std::string& path) { p_program = path; }
  // The front end (ssh -J); the target is reached through it by OpenSSH.
  void setJumpHost(const std::string& host, int port = 0,
                   const std::string& user = "");

  virtual const std::string& host() const { return p_host; }
  virtual const std::string& user() const { return p_user; }
  virtual const std::string& jumpHost() const { return p_jumpHost; }
  virtual bool nested() const { return false; }

  // Makes one connection (an `exit 0`) to find out now whether it works.
  virtual bool connect(std::string& error);
  virtual void disconnect() { p_connected = false; }
  virtual bool connected() const { return p_connected; }

  virtual TransportResult run(const std::string& script, int timeoutSec = -1);
  virtual long spawnDetached(const std::string& script, std::string& error,
                             const std::string& logFile = "");

  virtual bool put(const std::string& localPath, const std::string& remotePath,
                   std::string& error);
  virtual bool get(const std::string& remotePath, const std::string& localPath,
                   std::string& error);
  virtual bool putTree(const std::string& localPath, const std::string& remotePath,
                       std::string& error);
  virtual bool getTree(const std::string& remotePath, const std::string& localPath,
                       std::string& error);
  virtual int remoteKind(const std::string& remotePath);
  virtual bool remoteGlob(const std::string& pattern,
                          std::vector<std::string>& out, std::string& error);

  virtual RemoteStream* openStream(const std::string& script, int& fd,
                                   std::string& error);
  virtual void interruptStream(RemoteStream* s);
  virtual void closeStream(RemoteStream* s, int graceMs = 2000);

  // ---- choosing the backend ----

  enum Backend { BACKEND_AUTO, BACKEND_LIBSSH, BACKEND_OPENSSH };
  // ECCE_SSH_BACKEND=auto|libssh|openssh; auto when unset or unrecognised.
  static Backend requestedBackend();
  // True when the user's configuration for this host shares connections: the
  // text of `ssh -G` (parsed here) has a controlmaster other than no/false,
  // or a controlpath.
  static bool sharesConnection(const std::string& sshGOutput);
  // The same for a host, from `ssh -G`; the answer is cached per process.
  // false when ssh cannot be run.
  static bool configSharesConnection(const std::string& host,
                                     const std::string& user = "",
                                     const std::string& configFile = "");
  // Which backend serves this host: the requested one, or in auto mode
  // OpenSSH exactly when its configuration shares connections.
  static Backend backendFor(const std::string& host,
                            const std::string& user = "",
                            const std::string& configFile = "");

  // What the user is told when ssh printed `err` and exited 255: a message
  // for a failure that needs a login by hand, or for a connection that could
  // not be made, or "" when err does not look like an ssh failure.
  static std::string explainFailure(const std::string& host,
                                    const std::string& err);

private:
  OpenSshTransport(const OpenSshTransport&);
  OpenSshTransport& operator=(const OpenSshTransport&);

  std::vector<std::string> sshArgs(const std::string& remoteCmd) const;
  std::string envPrefix(std::string& error) const;
  TransportResult runImpl(const std::string& script, int timeoutSec, bool useDir);
  // The script as the first line of stdin, the rest of stdin left for it.
  TransportResult runWithData(const std::string& script, int inFd, int outFd,
                              int timeoutSec);
  void explain(TransportResult& r) const;
  bool putInto(const std::string& local, const std::string& remote,
               std::string& error);

  std::string p_host, p_user, p_jumpHost, p_jumpUser, p_configFile, p_program;
  int p_port, p_jumpPort, p_connectTimeout;
  bool p_connected;
};

#endif
