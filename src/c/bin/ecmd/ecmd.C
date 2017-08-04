#include <unistd.h>
#include <strings.h>

#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

#include "util/TempStorage.H"
#include "util/Preferences.H"

#include "tdat/RefMachine.H"
#include "tdat/AuthCache.H"

#include "dsm/MachineConfig.H"

#include "comm/RCommand.H"

int main(int argc, char** argv)
{
  string output;
  string err;

  string pipeName = argv[2];
  string remShell = argv[3];
  string user = argv[4];
  string machine = argv[5];
  string command = argv[6];

  bool status;

  // cache authentications for lookup below
  AuthCache& auth = AuthCache::getCache();
  auth.pipeIn(argv[2]);

  // create or append to a preferences file that contains a list of all ecmd
  // process ids.  When the gateway exits, depending on user preference,
  // it iterates through the list calling kill() on each process id.
  string pidFile = TempStorage::getTempRootPath();
  pidFile += "/ecmdProcesses";
  Preferences *pidPrefs = new Preferences(pidFile, true);
  vector<int> pidList;
  (void)pidPrefs->getIntList("pids", pidList);
  pidList.push_back((int)getpid());
  (void)pidPrefs->setIntList("pids", pidList);
  pidPrefs->saveFile();
  delete pidPrefs;
  
  RefMachine* refMachine = RefMachine::refLookup(machine);

  status = RCommand::command(command, output, err,
                             refMachine->fullname(), remShell,
                             refMachine->shell(), user, "",
                             refMachine->frontendMachine(),
                             refMachine->frontendBypass());

  if (!status)
    cerr << err << endl;
  else if (output != "")
    cout << output << endl;

  delete refMachine;

  return status? 0: -1;
}

