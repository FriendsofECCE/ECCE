///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: ReconnectRules.H
//
// DESCRIPTION:
//    Which calculations Run Management > Reconnect Selected (#601) can
//    reconnect, and why not for the others.  Plain values in, no services:
//    the Organizer's dialog only shows the answer, and the unit test
//    (tests/progress/reconnect_rules_test.C) covers every reason.
///////////////////////////////////////////////////////////////////////////////
#ifndef RECONNECTRULES_HH
#define RECONNECTRULES_HH

#include <string>

#include "dsm/ResourceDescriptor.H"

namespace ReconnectRules {

struct Input {
  ResourceDescriptor::RUNSTATE state;
  std::string machine;       // launch machine stored with the calculation
  bool machineRegistered;    // registered on this client
  std::string jobid;
  std::string jobpath;       // run directory on the machine
  Input() : state(ResourceDescriptor::STATE_ILLEGAL), machineRegistered(false) {}
};

inline std::string stateText(ResourceDescriptor::RUNSTATE s)
{
  switch (s) {
    case ResourceDescriptor::STATE_CREATED: return "Created";
    case ResourceDescriptor::STATE_READY: return "Ready";
    case ResourceDescriptor::STATE_SUBMITTED: return "Submitted";
    case ResourceDescriptor::STATE_RUNNING: return "Running";
    case ResourceDescriptor::STATE_WAITING: return "Waiting for Login";
    case ResourceDescriptor::STATE_COMPLETED: return "Completed";
    case ResourceDescriptor::STATE_LOADED: return "Loaded";
    case ResourceDescriptor::STATE_KILLED: return "Killed";
    case ResourceDescriptor::STATE_UNSUCCESSFUL: return "Unsuccessful";
    case ResourceDescriptor::STATE_FAILED: return "Failed";
    case ResourceDescriptor::STATE_SYSTEM_FAILURE: return "System failure";
    default: return "Unknown";
  }
}

// Empty when the calculation can be reconnected, else the reason.  Only a job
// that may still be on the machine, or whose monitoring was lost, qualifies.
inline std::string problem(const Input& in)
{
  switch (in.state) {
    case ResourceDescriptor::STATE_SUBMITTED:
    case ResourceDescriptor::STATE_RUNNING:
    case ResourceDescriptor::STATE_WAITING:
      break;
    case ResourceDescriptor::STATE_CREATED:
    case ResourceDescriptor::STATE_READY:
      return "not launched";
    default:
      return "already finished";
  }
  if (in.machine.empty()) return "no machine stored";
  if (!in.machineRegistered)
    return "machine \"" + in.machine + "\" is not registered here";
  if (in.jobid.empty()) return "no job id stored";
  if (in.jobpath.empty()) return "no run directory stored";
  return "";
}

}

#endif
