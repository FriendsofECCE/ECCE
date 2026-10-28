#include <ctime>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>

#include "comm/AllocationQuery.H"
#include "comm/RCommand.H"
#include "dsm/AllocationLeft.H"
#include "dsm/MachinePreferences.H"
#include "tdat/QueueMgr.H"
#include "tdat/RefQueueMgr.H"
#include "util/Ecce.H"

using std::string;

namespace {
std::mutex gAllocMu;
AllocationLeft::Cache gAllocCache;
bool gAllocLoaded = false;
std::set<string> gAllocBusy;
//  When a query last failed (no live shared login, or the command failed), per
//  key. A failure is tried again after this, not after the 15 minutes of a
//  good answer: a monitor may have logged in meanwhile.
std::map<string, long> gAllocFailed;
const long RETRY_SECONDS = 30;

string allocCachePath()
{
  return string(Ecce::userRegistrationDir()) + "AllocationCache";
}
}

string AllocationQuery::account(const Launchdata& l)
{
  if (!l.account.empty() || l.machine.empty())
    return l.account;
  MachinePreferences *prefs = MachinePreferences::lookup(l.machine);
  return prefs ? prefs->getProfile(l.profile).account : string();
}

//  A stale answer is shown until the refresh lands; a good answer is kept 15
//  minutes, a failure is retried after RETRY_SECONDS.
string AllocationQuery::left(const Launchdata& l, std::function<void()> onDone)
{
  const string account = AllocationQuery::account(l);
  if (l.machine.empty() || !AllocationLeft::validAccount(account))
    return "";
  MachinePreferences *prefs = MachinePreferences::lookup(l.machine);
  RefMachine *rgstn = prefs ? prefs->getRegisteredMachine() : NULL;
  if (!rgstn || rgstn->noRemoteAccess())
    return "";
  const QueueManager *queueMngr = QueueManager::lookup(l.machine);
  const RefQueueManager *refQMngr = queueMngr ?
    RefQueueManager::lookup(queueMngr->queueMgrName()) : NULL;
  string command = RefQueueManager::allocationCommand(refQMngr, rgstn->allocationCommand());
  if (command.empty())
    return "";
  command = AllocationLeft::substitute(command, account);

  //  Per host, not per ECCE machine: entries for one cluster (one per node
  //  type, say) share the answer, so the command runs once per cluster.
  const string host = rgstn->fullname();
  const string key = AllocationLeft::Cache::key(host, account);
  const long now = (long)time(NULL);
  AllocationLeft::Cache::Entry e;
  bool have, fresh;
  {
    std::lock_guard<std::mutex> lock(gAllocMu);
    if (!gAllocLoaded) {
      gAllocLoaded = true;
      std::ifstream in(allocCachePath().c_str());
      std::stringstream all;
      all << in.rdbuf();
      gAllocCache.parse(all.str());
    }
    have = gAllocCache.find(key, e);
    fresh = gAllocCache.fresh(key, now);
    std::map<string, long>::const_iterator f = gAllocFailed.find(key);
    if (!fresh && f != gAllocFailed.end() && now - f->second < RETRY_SECONDS)
      fresh = true;                       // failed a moment ago
    if (!fresh && gAllocBusy.count(key))
      fresh = true;                       // being read
    else if (!fresh)
      gAllocBusy.insert(key);
  }

  if (!fresh) {
    //  The shared login is keyed on the host, not on ECCE's machine name.
    const string machine = host, user = prefs->getUsername(),
                 front = rgstn->frontendMachine(), bypass = rgstn->frontendBypass(),
                 shellPath = rgstn->shellPath();
    std::thread([=]() {
      string out, text;
      const bool ran = RCommand::execOnLiveLogin(machine, user, front, bypass,
                                                 shellPath, command, out);
      if (ran)
        text = AllocationLeft::summarize(out, account);
      {
        std::lock_guard<std::mutex> lock(gAllocMu);
        gAllocBusy.erase(key);
        if (ran) {
          gAllocFailed.erase(key);
          gAllocCache.put(key, (long)time(NULL), text);
          std::ofstream o(allocCachePath().c_str());
          o << gAllocCache.serialise();
        } else {
          //  In memory only: an older answer stays on file.
          gAllocFailed[key] = (long)time(NULL);
        }
      }
      if (onDone) onDone();
    }).detach();
  }
  if (!have || e.text.empty())
    return "";
  return e.text + " (checked " + AllocationLeft::checkedAt(e.when) + ")";
}
