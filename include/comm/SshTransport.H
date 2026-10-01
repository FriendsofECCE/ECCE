#ifndef SSHTRANSPORT_HH
#define SSHTRANSPORT_HH

#include <functional>
#include <string>
#include <vector>

struct SshJump;

#include "comm/RemoteTransport.H"

struct ssh_session_struct;
struct sftp_session_struct;

// Runs scripts on a remote host over libssh: one exec channel per run(),
// the script delivered as data on the channel's stdin, SFTP for files.
// Needs libssh >= 0.10.  One connection is used by one thread at a time.
class SshTransport : public RemoteTransport {
public:
  // Asks the user something.  echo=false for a password.  Return false to
  // abort authentication.
  typedef std::function<bool(const std::string& prompt, bool echo,
                             std::string& answer)> PromptFn;
  // Asked for a host key not in known_hosts; true accepts and records it.
  typedef std::function<bool(const std::string& host,
                             const std::string& fingerprintSHA256,
                             const std::string& keyType)> HostKeyFn;

  SshTransport(const std::string& host, int port = 22,
               const std::string& user = "");
  virtual ~SshTransport();

  // Options: set before connect().
  void addIdentityFile(const std::string& path) { p_identities.push_back(path); }
  void setKnownHostsFile(const std::string& path) { p_knownHosts = path; }
  void setUseConfig(bool on) { p_useConfig = on; }
  void setConfigFile(const std::string& path) { p_configFile = path; }
  void setPromptCallback(const PromptFn& f) { p_prompt = f; }
  void setHostKeyCallback(const HostKeyFn& f) { p_hostKey = f; }
  void setConnectTimeout(int sec) { p_connectTimeout = sec; }
  void setPasswordAttempts(int n) { p_passwordAttempts = n; }

  // A front end (login node) the target is reached through.  FE_FORWARD
  // opens a direct-tcpip channel to the target's sshd over the front end's
  // connection (libssh before 0.11 cannot do ProxyJump itself, and 0.11's
  // shells out to ssh(1)); FE_NESTED runs `ssh target sh -c ...` in an exec
  // channel to the front end, which needs only the front end's own keys and
  // works where TCP forwarding is off; FE_AUTO tries forwarding first and
  // falls back to nesting when the front end refuses the channel.
  // ECCE_SSH_FRONTEND=forward|nested|auto overrides the mode.
  enum FrontEnd { FE_AUTO, FE_FORWARD, FE_NESTED };
  void setJumpHost(const std::string& host, int port = 0,
                   const std::string& user = "");
  void setJumpPromptCallback(const PromptFn& f) { p_jumpPrompt = f; }
  void setFrontEndMode(FrontEnd m) { p_feMode = m; }
  virtual const std::string& host() const { return p_host; }
  virtual const std::string& user() const { return p_user; }
  virtual const std::string& jumpHost() const { return p_jumpHost; }
  // True once connected through a front end by nested exec.
  virtual bool nested() const { return p_nested; }

  // Connects, checks the host key, authenticates (agent and keys, then
  // keyboard-interactive, then password).
  virtual bool connect(std::string& error);
  virtual void disconnect();
  virtual bool connected() const { return p_session != 0; }

  virtual TransportResult run(const std::string& script, int timeoutSec = -1);
  virtual long spawnDetached(const std::string& script, std::string& error,
                             const std::string& logFile = "");

  // SFTP; the file mode (permission bits) travels with the content.
  virtual bool put(const std::string& localPath, const std::string& remotePath,
           std::string& error);
  virtual bool get(const std::string& remotePath, const std::string& localPath,
           std::string& error);

  // scp -r semantics: a directory source is copied with its contents, and
  // when the target is an existing directory the source lands inside it.
  virtual bool putTree(const std::string& localPath, const std::string& remotePath,
               std::string& error);
  virtual bool getTree(const std::string& remotePath, const std::string& localPath,
               std::string& error);
  // 1 directory, 0 other file, -1 does not exist.
  virtual int remoteKind(const std::string& remotePath);
  // The remote shell's expansion of a wildcard pattern (sorted; a pattern
  // matching nothing comes back as an error).
  virtual bool remoteGlob(const std::string& pattern, std::vector<std::string>& out,
                  std::string& error);

  // A long-running script on its own ssh session, for a reader that needs
  // a file descriptor.  fd is one end of a socketpair: read the script's
  // stdout and stderr from it, write what the script should get on stdin.
  // A private thread owns the session and shuttles between the channel and
  // the socket; when the channel or the connection ends, it closes its end
  // so the reader sees EOF.  The script runs in dir() with the env set.
  virtual RemoteStream* openStream(const std::string& script, int& fd,
                                   std::string& error);
  // SIGINT to the script's shell (the session has no tty for ^C).
  virtual void interruptStream(RemoteStream* s);
  // Sends EOF, gives the script graceMs to finish, closes everything and
  // joins the thread.  Frees s.
  virtual void closeStream(RemoteStream* s, int graceMs = 2000);

private:
  SshTransport(const SshTransport&);
  SshTransport& operator=(const SshTransport&);

  // link is set when the session runs over a forwarded channel; whoever
  // frees the session then frees the link too (closeJump()).
  ssh_session_struct* newSession(std::string& error, SshJump** link = 0);
  ssh_session_struct* rawSession(const std::string& host, int port,
                                 const std::string& user, const PromptFn& prompt,
                                 int fd, std::string& error);
  bool checkHostKey(ssh_session_struct* s, const std::string& host,
                    std::string& error);
  bool authenticate(ssh_session_struct* s, const std::string& host,
                    const std::string& user, const PromptFn& prompt,
                    std::string& error);
  bool authKeyboardInteractive(ssh_session_struct* s, const PromptFn& prompt,
                               int& rc, bool& asked, std::string& error);
  // The exec request for a script command: run on this host, or on the
  // target through the front end when nested.
  std::string execLine(const std::string& remoteCmd) const;
  TransportResult runImpl(const std::string& script, int timeoutSec, bool useDir);
  // Files and trees when there is no SFTP to the target (nested).
  bool nestedPut(const std::string& local, const std::string& remote,
                 std::string& error);
  bool nestedGet(const std::string& remote, const std::string& local,
                 std::string& error);
  bool nestedPutInto(const std::string& local, const std::string& remote,
                     std::string& error);
  bool sftp(std::string& error);
  std::string envPrefix(std::string& error) const;

  std::string p_host, p_user;
  int p_port;
  std::vector<std::string> p_identities;
  std::string p_knownHosts, p_configFile;
  bool p_useConfig;
  int p_connectTimeout, p_passwordAttempts;
  PromptFn p_prompt, p_jumpPrompt;
  HostKeyFn p_hostKey;
  std::string p_jumpHost, p_jumpUser;
  int p_jumpPort;
  FrontEnd p_feMode;
  bool p_nested, p_forwardWorked, p_authFailed;
  SshJump* p_link;
  ssh_session_struct* p_session;
  sftp_session_struct* p_sftp;
};

#endif
