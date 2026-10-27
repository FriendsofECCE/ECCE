// Reconnect Selected (#601): which calculations qualify and the reason for the
// others. No wx, no services.
#include <iostream>
#include <string>

#include "comm/ReconnectRules.H"

using namespace std;

static int failures = 0;

static void check(bool ok, const string& what)
{
  cout << (ok ? "ok    " : "FAIL  ") << what << endl;
  if (!ok) failures++;
}

static ReconnectRules::Input good(ResourceDescriptor::RUNSTATE s)
{
  ReconnectRules::Input in;
  in.state = s;
  in.machine = "m1";
  in.machineRegistered = true;
  in.jobid = "123";
  in.jobpath = "/scratch/run";
  return in;
}

int main()
{
  typedef ResourceDescriptor RD;
  const RD::RUNSTATE ok[] = {RD::STATE_SUBMITTED, RD::STATE_RUNNING, RD::STATE_WAITING};
  for (RD::RUNSTATE s : ok)
    check(ReconnectRules::problem(good(s)).empty(),
          "eligible: " + ReconnectRules::stateText(s));

  check(ReconnectRules::problem(good(RD::STATE_READY)) == "not launched", "ready");
  check(ReconnectRules::problem(good(RD::STATE_CREATED)) == "not launched", "created");
  const RD::RUNSTATE done[] = {RD::STATE_COMPLETED, RD::STATE_LOADED, RD::STATE_KILLED,
      RD::STATE_UNSUCCESSFUL, RD::STATE_FAILED, RD::STATE_SYSTEM_FAILURE};
  for (RD::RUNSTATE s : done)
    check(ReconnectRules::problem(good(s)) == "already finished",
          "finished: " + ReconnectRules::stateText(s));

  ReconnectRules::Input in = good(RD::STATE_WAITING);
  in.machine = "";
  check(ReconnectRules::problem(in) == "no machine stored", "no machine");
  in = good(RD::STATE_WAITING);
  in.machineRegistered = false;
  check(ReconnectRules::problem(in) == "machine \"m1\" is not registered here",
        "machine not registered");
  in = good(RD::STATE_RUNNING);
  in.jobid = "";
  check(ReconnectRules::problem(in) == "no job id stored", "no job id");
  in = good(RD::STATE_RUNNING);
  in.jobpath = "";
  check(ReconnectRules::problem(in) == "no run directory stored", "no run directory");
  //  The state decides before anything else is looked at.
  in = good(RD::STATE_COMPLETED);
  in.machine = "";
  check(ReconnectRules::problem(in) == "already finished", "state first");

  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}
