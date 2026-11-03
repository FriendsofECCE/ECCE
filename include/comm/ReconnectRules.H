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
    case ResourceDescriptor::STATE_LOADED: return "Imported";
    case ResourceDescriptor::STATE_KILLED: return "Killed";
    case ResourceDescriptor::STATE_UNSUCCESSFUL: return "Incomplete";
    case ResourceDescriptor::STATE_FAILED: return "Monitor Error";
    case ResourceDescriptor::STATE_SYSTEM_FAILURE: return "System Error";
    default: return "Unknown";
  }
}

// Empty when the calculation can be reconnected, else the reason.  The one
// rule for Run Mgmt > Reconnect, Reconnect Selected and both context menus.
// A job that ended badly (incomplete, monitor or system error) can still have
// its output in the run directory.  A reconnect deletes the stored outputs
// first, so a completed job is re-parsed instead, and a killed one (only a
// user's cancel kills) is left as the user left it.
inline std::string problem(const Input& in)
{
  switch (in.state) {
    case ResourceDescriptor::STATE_SUBMITTED:
    case ResourceDescriptor::STATE_RUNNING:
    case ResourceDescriptor::STATE_WAITING:
    case ResourceDescriptor::STATE_UNSUCCESSFUL:
    case ResourceDescriptor::STATE_FAILED:
    case ResourceDescriptor::STATE_SYSTEM_FAILURE:
      break;
    case ResourceDescriptor::STATE_CREATED:
    case ResourceDescriptor::STATE_READY:
      return "not launched";
    case ResourceDescriptor::STATE_COMPLETED:
      return "already finished (use Re-Parse Outputs)";
    case ResourceDescriptor::STATE_KILLED:
      return "already finished (cancelled)";
    case ResourceDescriptor::STATE_LOADED:
      return "imported";
    default:
      return "unknown state";
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
