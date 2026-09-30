#ifndef DIRECTTRANSPORT_HH
#define DIRECTTRANSPORT_HH

#include "comm/Transport.H"

// Runs scripts on this machine with fork/exec of /bin/sh.
class DirectTransport : public Transport {
public:
  virtual TransportResult run(const std::string& script, int timeoutSec = -1);
  virtual long spawnDetached(const std::string& script, std::string& error,
                             const std::string& logFile = "");
};

#endif
