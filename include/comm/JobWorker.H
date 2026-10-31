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
    MONITORING, RECOVERING, NEEDS_RECONCILIATION, COMPLETE, FAILED
  };

  enum EventType {
    PHASE_CHANGED, AUTHENTICATION_REQUIRED, PROGRESS, OUTPUT_AVAILABLE,
    WARNING, FAILURE, FINISHED
  };

  struct Request {
    std::string calculationUrl;
    // Reserved for the local/data-server choice; once submission begins the
    // selected execution location must remain fixed for this job.
    std::string executionLocation;
    bool resume;
    Request() : resume(false) {}
  };

  struct Record {
    std::string calculationUrl;
    std::string schedulerJobId;
    std::string monitorBookmark;
    std::string configFile;
    std::string remoteDirectory;
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
  /** Persistent per-user state location, stable for a calculation URL. */
  std::string recordPathFor(const std::string& calculationUrl);
  std::string recordDirectoryFor(const std::string& calculationUrl);
  bool loadRecord(const std::string& calculationUrl, Record& record);
  bool initializeRecord(const std::string& path, const Record& record,
                        std::string& error);
  bool acquireRecordLock(const std::string& path, std::string& error);
  void releaseRecordLock();
  void setActiveRecord(const std::string& path, const Record& record);
  Record activeRecord();
  bool persistSubmission(const std::string& jobId,
                         const std::string& configFile,
                         const std::string& remoteDirectory,
                         std::string& error);
  void updateBookmark(const std::string& bookmark);
  bool transition(Phase phase, EventType type, const std::string& message);

  typedef bool (*EventWriter)(void *context, const std::string& frame);
  void setEventWriter(EventWriter writer, void *context);
  // Writes one protocol event to the configured IPC stream, or stdout when
  // the worker was started without an IPC adapter.
  void emitEvent(EventType type, Phase phase, const std::string& message);
}

#endif
