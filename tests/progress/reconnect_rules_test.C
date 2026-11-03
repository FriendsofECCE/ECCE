// Reconnect (#601, #689): which calculations qualify and the reason for the
// others; the one rule behind every Reconnect item. No wx, no services.
// Argument: ResourceDescriptor.xml, whose reconnectJob task must show the
// item in every state the rule accepts.
#include <fstream>
#include <iostream>
#include <sstream>
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

int main(int argc, char **argv)
{
  typedef ResourceDescriptor RD;
  //  Incomplete (Unsuccessful), Monitor Error and System Error can still have
  //  output in the run directory (#689).
  const RD::RUNSTATE ok[] = {RD::STATE_SUBMITTED, RD::STATE_RUNNING,
      RD::STATE_WAITING, RD::STATE_UNSUCCESSFUL, RD::STATE_FAILED,
      RD::STATE_SYSTEM_FAILURE};
  for (RD::RUNSTATE s : ok)
    check(ReconnectRules::problem(good(s)).empty(),
          "eligible: " + ReconnectRules::stateText(s));

  check(ReconnectRules::problem(good(RD::STATE_READY)) == "not launched", "ready");
  check(ReconnectRules::problem(good(RD::STATE_CREATED)) == "not launched", "created");
  check(ReconnectRules::problem(good(RD::STATE_COMPLETED)).find("already finished") == 0,
        "completed: re-parsed, not reconnected");
  check(ReconnectRules::problem(good(RD::STATE_KILLED)).find("already finished") == 0,
        "killed: a user's cancel is kept");
  check(ReconnectRules::problem(good(RD::STATE_LOADED)) == "imported", "imported");
  check(ReconnectRules::stateText(RD::STATE_UNSUCCESSFUL) == "Incomplete",
        "the dialog names states as the tree does");

  //  No job record: an incomplete calculation without one is refused too.
  for (RD::RUNSTATE s : ok) {
    ReconnectRules::Input in = good(s);
    in.jobid = "";
    check(ReconnectRules::problem(in) == "no job id stored",
          "no job record: " + ReconnectRules::stateText(s));
  }

  if (argc > 1) {
    std::ifstream xml(argv[1]);
    std::stringstream all;
    all << xml.rdbuf();
    const string text = all.str();
    check(!text.empty(), string("read ") + argv[1]);
    const string want[] = {"Submitted", "Running", "Waiting", "Unsuccessful",
                           "Failed", "System"};
    int blocks = 0;
    for (size_t at = text.find("name=\"reconnectJob\""); at != string::npos;
         at = text.find("name=\"reconnectJob\"", at + 1)) {
      size_t end = text.find("</RunManagementTask>", at);
      string block = text.substr(at, end - at);
      if (block.find("<State ") == string::npos) continue;  // the label
      blocks++;
      for (const string& w : want)
        if (block.find("<State name=\"" + w + "\"/>") == string::npos)
          check(false, "reconnectJob task " + std::to_string(blocks) +
                " shows Reconnect in " + w);
    }
    check(blocks > 0, "reconnectJob tasks found: " + std::to_string(blocks));
  }

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
  check(ReconnectRules::problem(in).find("already finished") == 0, "state first");

  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}
