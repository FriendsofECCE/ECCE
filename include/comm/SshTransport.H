#ifndef SSHTRANSPORT_HH
#define SSHTRANSPORT_HH

#include <functional>
#include <string>
#include <vector>

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

private:
  SshTransport(const SshTransport&);
  SshTransport& operator=(const SshTransport&);

  bool checkHostKey(std::string& error);
  bool authenticate(std::string& error);
  bool authKeyboardInteractive(int& rc, bool& asked, std::string& error);
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
