#ifndef DIRECTTRANSPORT_HH
#define DIRECTTRANSPORT_HH

#include <vector>

#include "comm/Transport.H"

// Runs scripts on this machine with fork/exec of /bin/sh.
class DirectTransport : public Transport {
public:
  virtual TransportResult run(const std::string& script, int timeoutSec = -1);
  virtual long spawnDetached(const std::string& script, std::string& error,
                             const std::string& logFile = "");

  // A long-running script with pipes instead of a pty: wfd is its stdin,
  // rfd its stdout and stderr merged.  The script leads its own process
  // group, so interruptStream() reaches everything it started.
  struct Stream {
    Stream() : pid(-1), wfd(-1), rfd(-1) {}
    long pid;
    int wfd, rfd;
  };

  // A plain subprocess with no shell (the OpenSSH backend runs ssh this way):
  // argv[0] is looked up on PATH.  stdin is `input` followed by what is read
  // from inFd when that is >= 0.  stdout is captured, or written to outFd
  // when that is >= 0.  The child leads its own process group; timeoutSec as
  // in run().
  static TransportResult runProcess(const std::vector<std::string>& argv,
                                    const std::string& input = "",
                                    int inFd = -1, int outFd = -1,
                                    int timeoutSec = -1);
  // Starts argv in its own process group with the given descriptors as
  // stdin, stdout and stderr (every other descriptor is closed).  Returns
  // the pid, or -1 with error set; the caller reaps it.
  static long spawnProcess(const std::vector<std::string>& argv, int in,
                           int out, int err, std::string& error);

  bool openStream(const std::string& script, Stream& s, std::string& error);
  bool writeStream(Stream& s, const std::string& data);
  void interruptStream(Stream& s);
  // Closes both pipes and reaps the script, giving it graceMs to exit after
  // EOF on stdin before SIGTERM, then SIGKILL.  Returns its exit status.
  int closeStream(Stream& s, int graceMs = 2000);
};

#endif
