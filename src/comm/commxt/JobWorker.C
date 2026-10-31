#include "comm/JobWorker.H"

#include <iostream>

namespace JobWorker
{
  const char *phaseName(Phase phase)
  {
    switch (phase) {
      case CREATED: return "created";
      case PREPARING: return "preparing";
      case AUTHENTICATING: return "authenticating";
      case SUBMITTING: return "submitting";
      case SUBMITTED: return "submitted";
      case MONITORING: return "monitoring";
      case RECOVERING: return "recovering";
      case COMPLETE: return "complete";
      case FAILED: return "failed";
    }
    return "unknown";
  }

  const char *eventName(EventType type)
  {
    switch (type) {
      case PHASE_CHANGED: return "phase_changed";
      case AUTHENTICATION_REQUIRED: return "authentication_required";
      case PROGRESS: return "progress";
      case OUTPUT_AVAILABLE: return "output_available";
      case WARNING: return "warning";
      case FAILURE: return "failure";
      case FINISHED: return "finished";
    }
    return "unknown";
  }

  void emitEvent(EventType type, Phase phase, const std::string& message)
  {
    static unsigned long sequence = 0;
    std::string escaped;
    for (std::string::const_iterator it = message.begin(); it != message.end(); ++it) {
      if (*it == '"' || *it == '\\') escaped += '\\';
      if (*it == '\n') escaped += "\\n";
      else if (*it == '\r') escaped += "\\r";
      else escaped += *it;
    }
    std::cout << "{\"event\":\"" << eventName(type)
              << "\",\"phase\":\"" << phaseName(phase)
              << "\",\"sequence\":" << ++sequence
              << ",\"message\":\"" << escaped << "\"}" << std::endl;
  }
}
