#ifndef JOBWORKER_HH
#define JOBWORKER_HH

#include <string>

/**
 * Host-independent lifecycle vocabulary for the ECCE job worker.
 *
 * The request and event types are deliberately free of wxWidgets and OS
 * handles. A Windows named-pipe adapter and a data-server API adapter can
 * therefore exchange the same lifecycle records. Credentials are never
 * members of these records; the worker receives authentication through the
 * existing AuthCache handoff and keeps SSH/TOTP secrets in memory only.
 */
namespace JobWorker
{
  enum Phase {
    CREATED, PREPARING, AUTHENTICATING, SUBMITTING, SUBMITTED,
    MONITORING, RECOVERING, COMPLETE, FAILED
  };

  enum EventType {
    PHASE_CHANGED, AUTHENTICATION_REQUIRED, PROGRESS, OUTPUT_AVAILABLE,
    WARNING, FAILURE, FINISHED
  };

  struct Request {
    std::string calculationUrl;
    // Reserved for the local/data-server choice; a worker never changes the
    // target after submission begins.
    std::string executionLocation;
  };

  struct Record {
    std::string calculationUrl;
    std::string schedulerJobId;
    std::string monitorBookmark;
    Phase phase;
    unsigned long sequence;
    Record() : phase(CREATED), sequence(0) {}
  };

  struct Event {
    EventType type;
    Phase phase;
    unsigned long sequence;
    std::string message;
    Event() : type(PHASE_CHANGED), phase(CREATED), sequence(0) {}
  };

  /** Stable wire names used by the newline-delimited worker event protocol. */
  const char *phaseName(Phase phase);
  const char *eventName(EventType type);
  // Writes one protocol event to stdout. The monotonically increasing
  // sequence number is process-local and shared by the launcher and monitor.
  void emitEvent(EventType type, Phase phase, const std::string& message);
}

#endif
