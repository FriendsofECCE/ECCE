#include <ctime>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>
#include <vector>

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

std::vector<AllocationQuery::Target> AllocationQuery::targets()
{
  std::vector<Target> out;
  std::set<string> seen;
  for (int i = 0; i < MachinePreferences::countItems(); i++) {
    MachinePreferences *prefs = MachinePreferences::getItemAt(i);
    RefMachine *rgstn = prefs ? prefs->getRegisteredMachine() : NULL;
    if (!rgstn || rgstn->noRemoteAccess())
      continue;
    const QueueManager *qm = QueueManager::lookup(prefs->getItemKey());
    const RefQueueManager *rq = qm ?
      RefQueueManager::lookup(qm->queueMgrName()) : NULL;
    if (RefQueueManager::allocationCommand(rq, rgstn->allocationCommand()).empty())
      continue;
    const std::vector<string> names = prefs->profileNames();
    for (size_t k = 0; k < names.size(); k++) {
      const string a = prefs->getProfile(names[k]).account;
      if (!AllocationLeft::validAccount(a) ||
          !seen.insert(prefs->getItemKey() + "\n" + a).second)
        continue;
      Target t;
      t.machine = prefs->getItemKey();
      t.account = a;
      out.push_back(t);
    }
  }
  return out;
}

//  A stale answer is shown until the refresh lands; a good answer is kept 15
//  minutes, a failure is retried after RETRY_SECONDS.
AllocationQuery::Result AllocationQuery::check(const Target& t, bool force,
                                               std::function<void()> onDone)
{
  Result r;
  MachinePreferences *prefs = MachinePreferences::lookup(t.machine);
  RefMachine *rgstn = prefs ? prefs->getRegisteredMachine() : NULL;
  if (!rgstn || rgstn->noRemoteAccess() ||
      !AllocationLeft::validAccount(t.account))
    return r;
  r.host = rgstn->fullname();
  const QueueManager *queueMngr = QueueManager::lookup(t.machine);
  const RefQueueManager *refQMngr = queueMngr ?
    RefQueueManager::lookup(queueMngr->queueMgrName()) : NULL;
  string command = RefQueueManager::allocationCommand(refQMngr, rgstn->allocationCommand());
  if (command.empty())
    return r;
  //  Slurm's own default lists a user's fair share; usage per account needs
  //  the account-keyed form. The Machine Browser keeps the default.
  if (rgstn->allocationCommand().empty() && command == "sshare -l -U -u $USER")
    command = AllocationLeft::slurmCommand();
  command = AllocationLeft::substitute(command, t.account);

  //  Per host, not per ECCE machine: entries for one cluster (one per node
  //  type, say) share the answer, so the command runs once per cluster.
  const string host = r.host;
  const string key = AllocationLeft::Cache::key(host, t.account);
  const string account = t.account;
  const long now = (long)time(NULL);
  AllocationLeft::Cache::Entry e;
  bool have, fresh, failed;
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
    fresh = !force && gAllocCache.fresh(key, now);
    std::map<string, long>::const_iterator f = gAllocFailed.find(key);
    failed = f != gAllocFailed.end();
    if (failed && !force && now - f->second < RETRY_SECONDS)
      fresh = true;                       // failed a moment ago
    if (!fresh && gAllocBusy.count(key))
      fresh = true;                       // being read
    else if (!fresh)
      gAllocBusy.insert(key);
    if (gAllocBusy.count(key))
      r.state = Result::Checking;
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
        text = AllocationLeft::encode(out, account);
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
    r.state = Result::Checking;
  }
  if (have) {
    AllocationLeft::decode(e.text, r.used, r.raw);
    r.when = e.when;
    if (r.state != Result::Checking)
      r.state = Result::Value;
  } else if (r.state != Result::Checking) {
    r.state = failed ? Result::NotLoggedIn : Result::Checking;
  }
  return r;
}
