#ifndef TRANSPORT_HH
#define TRANSPORT_HH

#include <map>
#include <string>

// Outcome of one Transport::run().  status is the exit code, 128+signal if
// the script was killed, or -1 if it could not be started at all.
struct TransportResult {
  TransportResult() : status(-1), timedOut(false) {}
  int status;
  std::string out, err;
  std::string error;   // why it could not start or timed out; empty otherwise
  bool timedOut;
};

// Runs POSIX sh scripts somewhere: here (DirectTransport), or on a remote
// host.  The script travels as data on a private pipe, never through a login
// shell's line editor or quoting, so the account's shell does not matter.
class Transport {
public:
  virtual ~Transport();

  // /bin/sh running the script, in dir() with the env overrides.  The script
  // is read from a private descriptor; the script's own stdin is /dev/null.
  // timeoutSec is seconds without output (any data on stdout or stderr
  // restarts it); <= 0 waits indefinitely.
  virtual TransportResult run(const std::string& script, int timeoutSec = -1) = 0;

  // Starts the script in its own session with no controlling terminal;
  // stdin from /dev/null, stdout/stderr to /dev/null or appended to logFile.
  // Returns the pid of the process running it, or -1 with error set.
  virtual long spawnDetached(const std::string& script, std::string& error,
                             const std::string& logFile = "") = 0;

  void setDir(const std::string& dir) { p_dir = dir; }
  const std::string& dir() const { return p_dir; }
  void setEnv(const std::string& name, const std::string& value);
  void unsetEnv(const std::string& name);

protected:
  // The script with `cd` prepended when a directory is set.  Done in sh, not
  // chdir(), so a missing directory is reported (status 97, sh's message).
  std::string withDir(const std::string& script) const;

  std::string p_dir;
  std::map<std::string, std::string> p_env;
  std::map<std::string, bool> p_unset;
};

#endif
