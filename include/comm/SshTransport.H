#ifndef SSHTRANSPORT_HH
#define SSHTRANSPORT_HH

#include <functional>
#include <string>
#include <vector>

struct SshStream;

#include "comm/Transport.H"

struct ssh_session_struct;
struct sftp_session_struct;

// Runs scripts on a remote host over libssh: one exec channel per run(),
// the script delivered as data on the channel's stdin, SFTP for files.
// Needs libssh >= 0.10.  One connection is used by one thread at a time.
class SshTransport : public Transport {
public:
  // Asks the user something.  echo=false for a password.  Return false to
  // abort authentication.
  typedef std::function<bool(const std::string& prompt, bool echo,
                             std::string& answer)> PromptFn;
  // Asked for a host key not in known_hosts; true accepts and records it.
  typedef std::function<bool(const std::string& host,
                             const std::string& fingerprintSHA256)> HostKeyFn;

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

  // Connects, checks the host key, authenticates (agent and keys, then
  // keyboard-interactive, then password).
  bool connect(std::string& error);
  void disconnect();
  bool connected() const { return p_session != 0; }

  virtual TransportResult run(const std::string& script, int timeoutSec = -1);
  virtual long spawnDetached(const std::string& script, std::string& error,
                             const std::string& logFile = "");

  // SFTP; the file mode (permission bits) travels with the content.
  bool put(const std::string& localPath, const std::string& remotePath,
           std::string& error);
  bool get(const std::string& remotePath, const std::string& localPath,
           std::string& error);

  // scp -r semantics: a directory source is copied with its contents, and
  // when the target is an existing directory the source lands inside it.
  bool putTree(const std::string& localPath, const std::string& remotePath,
               std::string& error);
  bool getTree(const std::string& remotePath, const std::string& localPath,
               std::string& error);
  // 1 directory, 0 other file, -1 does not exist.
  int remoteKind(const std::string& remotePath);
  // The remote shell's expansion of a wildcard pattern (sorted; a pattern
  // matching nothing comes back as an error).
  bool remoteGlob(const std::string& pattern, std::vector<std::string>& out,
                  std::string& error);

  // A long-running script on its own ssh session, for a reader that needs
  // a file descriptor.  fd is one end of a socketpair: read the script's
  // stdout and stderr from it, write what the script should get on stdin.
  // A private thread owns the session and shuttles between the channel and
  // the socket; when the channel or the connection ends, it closes its end
  // so the reader sees EOF.  The script runs in dir() with the env set.
  SshStream* openStream(const std::string& script, int& fd, std::string& error);
  // SIGINT to the script's shell (the session has no tty for ^C).
  void interruptStream(SshStream* s);
  // Sends EOF, gives the script graceMs to finish, closes everything and
  // joins the thread.  Frees s.
  void closeStream(SshStream* s, int graceMs = 2000);

private:
  SshTransport(const SshTransport&);
  SshTransport& operator=(const SshTransport&);

  ssh_session_struct* newSession(std::string& error);
  bool checkHostKey(ssh_session_struct* s, std::string& error);
  bool authenticate(ssh_session_struct* s, std::string& error);
  bool authKeyboardInteractive(ssh_session_struct* s, int& rc, bool& asked,
                               std::string& error);
  bool sftp(std::string& error);
  std::string envPrefix(std::string& error) const;

  std::string p_host, p_user;
  int p_port;
  std::vector<std::string> p_identities;
  std::string p_knownHosts, p_configFile;
  bool p_useConfig;
  int p_connectTimeout, p_passwordAttempts;
  PromptFn p_prompt;
  HostKeyFn p_hostKey;
  ssh_session_struct* p_session;
  sftp_session_struct* p_sftp;
};

#endif
