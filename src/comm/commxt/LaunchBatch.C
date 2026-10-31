#include <algorithm>
#include <climits>
#include <cstdio>

#include "comm/Launch.H"
#include "comm/LaunchBatch.H"
#include "dsm/ActivityLog.H"
#include "dsm/EDSI.H"
#include "dsm/JCode.H"
#include "dsm/VDoc.H"
#include "util/TDateTime.H"
#include "dsm/MachinePreferences.H"
#include "dsm/MdTask.H"
#include "tdat/Queue.H"
#include "tdat/QueueMgr.H"
#include "tdat/RefMachine.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/TaskJob.H"
#include "dsm/AllocationProfiles.H"
#include "util/EcceMap.H"
#include "util/StringConverter.H"
#include "util/TempStorage.H"
#include "util/TypedFile.H"

using std::string;
using std::vector;

bool LaunchBatch::wallTimeRequired(MachinePreferences *prefs)
{
  if (!prefs->isOptionSupported("TL") || !prefs->isOptionSupported("Q"))
    return false;
  const QueueManager *qm = QueueManager::lookup(prefs->getItemKey());
  return qm && qm->queueMgrName() != "Shell";
}


bool LaunchBatch::noWallTime(const string& maxwall)
{
  int days = 0, hours = 0, mins = 0;
  sscanf(maxwall.c_str(), "%d %d:%d", &days, &hours, &mins);
  return days == 0 && hours == 0 && mins == 0;
}


// The value the Launcher would show (WxLauncher::refreshWallTime): the
// machine's last one, else the queue's default, else two hours, within the
// queue's limit; a calculation prepared without opening the Launcher has none.
string LaunchBatch::defaultWallTime(MachinePreferences *prefs,
                                    const string& queue)
{
  long value = prefs->getMaxWallTime(), dflt = 120, max = 0;
  RefMachine *rm = prefs->getRegisteredMachine();
  const QueueManager *qm = rm ? QueueManager::lookup(rm->refname()) : 0;
  const Queue *q = qm ? qm->queue(queue) : 0;
  if (q) {
    if (q->runLimit() > 0) max = q->runLimit();
    if (q->defRun() > 0) dflt = q->defRun();
  }
  if (value <= 0) value = dflt;
  if (max > 0 && value > max) value = max;
  char buf[32];
  snprintf(buf, sizeof buf, "%ld %ld:%ld:00", value/1440, value%1440/60, value%60);
  return buf;
}


static string wallMinutes(const string& w)
{
  int d = 0, h = 0, m = 0;
  sscanf(w.c_str(), "%d %d:%d", &d, &h, &m);
  return std::to_string(d*24 + h) + ":" + (m < 10 ? "0" : "") + std::to_string(m) + " h";
}

static string memGB(unsigned long mb)
{
  char b[32];
  snprintf(b, sizeof b, "%g", mb/1024.0);
  return b;
}

static string dash(const string& s) { return s.empty() ? "-" : s; }

vector<string> LaunchBatch::profileNames(const string& machine)
{
  MachinePreferences *prefs = MachinePreferences::lookup(machine);
  return prefs ? prefs->profileNames() : vector<string>();
}

// Another machine: queue, cores, memory and wall time are kept where the
// machine's queue allows them; account and directories are the profile's.
static void carryOver(TaskJob *task, Launchdata& l,
                      const LaunchBatch::Target& target, string& changes,
                      string& problem)
{
  MachinePreferences *prefs = MachinePreferences::lookup(target.machine);
  RefMachine *rm = prefs ? prefs->getRegisteredMachine() : 0;
  if (!rm) {
    problem = "machine \"" + target.machine + "\" is not registered here";
    return;
  }
  const JCode *app = task->application();
  string code = app ? app->getCodeName() : "";
  if (!code.empty() && !rm->offersCode(code)) {
    problem = "code " + code + " is not registered on " + target.machine;
    return;
  }
  const Launchdata old = l;
  //  The limits that were applied come first: the Organizer column is narrow.
  vector<string> ch, where;
  if (old.machine != target.machine)
    where.push_back("machine " + dash(old.machine) + " \xE2\x86\x92 " + target.machine);

  vector<string> names = prefs->profileNames();
  string pname = target.profile;
  if (std::find(names.begin(), names.end(), pname) == names.end())
    pname = AllocationProfiles::defaultName();
  AllocationProfiles::Profile prof = prefs->getProfile(pname);
  l.machine = target.machine;
  l.profile = AllocationProfiles::isDefault(pname) ? "" : pname;
  if (l.profile != old.profile)
    where.push_back("profile " + dash(old.profile.empty() ? "Default" : old.profile) +
                 " \xE2\x86\x92 " + pname);
  l.account = prefs->isOptionSupported("AA") ? prof.account : "";
  if (l.account != old.account)
    where.push_back("account " + dash(old.account) + " \xE2\x86\x92 " + dash(l.account));
  l.rundir = prof.rundir;
  l.scratchdir = prof.scratchdir;
  if (l.rundir != old.rundir)
    where.push_back("run directory " + dash(old.rundir) + " \xE2\x86\x92 " + dash(l.rundir));
  l.remoteShell = prefs->getRemoteShell();
  l.user = prefs->isOptionSupported("UN") ? prefs->getUsername() : "";

  const QueueManager *qm = QueueManager::lookup(rm->refname());
  const Queue *q = 0;
  if (prefs->isOptionSupported("Q")) {
    const ESQHDict *queues = qm ? qm->queues() : 0;
    string dflt = prof.queue.empty() ? prefs->getQueueName() : prof.queue;
    if (queues && !queues->empty()) {
      if (dflt.empty() || !queues->count(dflt)) dflt = queues->begin()->first;
      l.queue = queues->count(old.queue) ? old.queue : dflt;
      q = qm->queue(l.queue);
    } else {
      l.queue = old.queue.empty() ? dflt : old.queue;
    }
    if (l.queue != old.queue)
      ch.push_back("queue " + dash(old.queue) + " \xE2\x86\x92 " + dash(l.queue));
  } else {
    l.queue = "";
    l.priority = StringConverter::toString(prefs->getPriority());
  }

  unsigned long maxP = ULONG_MAX, minP = 1;
  if (rm->proccount() > 0) maxP = rm->proccount();
  if (q) {
    if (q->maxProcessors() > 0) maxP = std::min<unsigned long>(maxP, q->maxProcessors());
    minP = std::max<unsigned long>(minP, q->minProcessors());
  }
  if (old.totalprocs > 0) {
    l.totalprocs = std::max(std::min(old.totalprocs, maxP), std::min(minP, maxP));
    if (l.totalprocs != old.totalprocs)
      ch.push_back(std::to_string(old.totalprocs) + " \xE2\x86\x92 " +
                   std::to_string(l.totalprocs) + " cores");
  }
  if (prefs->isOptionSupported("Q")) {
    l.nodes = old.nodes;
    if (rm->nodes() > 0 && l.nodes > rm->nodes()) l.nodes = rm->nodes();
    if (l.totalprocs > 0 && l.nodes > l.totalprocs) l.nodes = l.totalprocs;
    if (l.nodes != old.nodes)
      ch.push_back("nodes " + std::to_string(old.nodes) + " \xE2\x86\x92 " +
                   std::to_string(l.nodes));
  } else {
    l.nodes = 1;
  }

  l.maxmemory = prefs->isOptionSupported("MM") ? old.maxmemory : 0;
  if (q && l.maxmemory > 0 && q->memLimit() > 0 && q->memLimit() < (unsigned)INT_MAX &&
      l.maxmemory > q->memLimit())
    l.maxmemory = q->memLimit();
  if (l.maxmemory != old.maxmemory)
    ch.push_back("memory " + memGB(old.maxmemory) + " \xE2\x86\x92 " +
                 memGB(l.maxmemory) + " GB");
  l.minscratch = prefs->isOptionSupported("SS") ? old.minscratch : 0;

  if (!prefs->isOptionSupported("TL")) {
    l.maxwall = "0 0:0";
  } else if (!LaunchBatch::noWallTime(old.maxwall)) {
    int d = 0, h = 0, m = 0;
    sscanf(old.maxwall.c_str(), "%d %d:%d", &d, &h, &m);
    long mins = d*1440L + h*60L + m;
    if (q && q->runLimit() > 0 && mins > (long)q->runLimit()) {
      mins = q->runLimit();
      char b[32];
      snprintf(b, sizeof b, "%ld %ld:%ld", mins/1440, mins%1440/60, mins%60);
      l.maxwall = b;
    }
  }
  if (l.maxwall != old.maxwall && !(LaunchBatch::noWallTime(l.maxwall) &&
                                    LaunchBatch::noWallTime(old.maxwall)))
    ch.push_back("wall time " + wallMinutes(old.maxwall) + " \xE2\x86\x92 " +
                 wallMinutes(l.maxwall));

  ch.insert(ch.end(), where.begin(), where.end());
  for (size_t i = 0; i < ch.size(); i++) changes += (i ? "; " : "") + ch[i];
}

LaunchBatch::Plan LaunchBatch::plan(TaskJob *task, const Target& target)
{
  Plan p;
  if (!task) { p.problem = "not a calculation"; return p; }
  p.name = task->getName();
  p.url = task->getURL().toString();
  Launchdata l = task->launchdata();
  p.machine = l.machine;
  p.queue = l.queue;
  p.wall = l.maxwall;
  p.procs = l.totalprocs;
  p.data = l;

  ResourceDescriptor::RUNSTATE state = task->getState();
  if (state != ResourceDescriptor::STATE_READY &&
      state != ResourceDescriptor::STATE_CREATED) {
    p.problem = "not ready (already submitted or finished)";
    return p;
  }
  if (target.any()) {
    carryOver(task, l, target, p.changes, p.problem);
    if (!p.problem.empty()) return p;
    p.retarget = true;
    p.data = l;
    p.machine = l.machine;
    p.queue = l.queue;
    p.wall = l.maxwall;
    p.procs = l.totalprocs;
  }
  if (l.machine.empty()) { p.problem = "no machine stored"; return p; }
  MachinePreferences *prefs = MachinePreferences::lookup(l.machine);
  if (!prefs || !prefs->getRegisteredMachine()) {
    p.problem = "machine \"" + l.machine + "\" is not registered here";
    return p;
  }
  if (l.rundir.empty()) { p.problem = "no run directory stored"; return p; }
  if (l.totalprocs == 0) { p.problem = "no core count stored"; return p; }
  if (prefs->isOptionSupported("Q") && l.queue.empty()) {
    p.problem = "no queue stored";
    return p;
  }
  if (wallTimeRequired(prefs) && noWallTime(l.maxwall))
    p.wall = defaultWallTime(prefs, l.queue);
  p.ready = true;
  return p;
}

bool LaunchBatch::storeTarget(TaskJob *task, const Plan& plan)
{
  return !plan.retarget || task->launchdata(plan.data);
}

bool LaunchBatch::storeRunDirectory(TaskJob *task)
{
  Launchdata l = task->launchdata();
  Jobdata j;
  j.jobpath = l.rundir + TempStorage::getJobRunDirectoryPath(task->getURL());
  return task->jobdata(j);
}

static void setReason(TaskJob *task, const string& reason)
{
  vector<MetaDataResult> r(1);
  r[0].name = VDoc::getEcceNamespace() + ":runStatusReason";
  r[0].value = reason;
  task->addProps(r);
  task->notifyProperty("runStatusReason", reason);
}

void LaunchBatch::recordFailure(TaskJob *task, const string& reason)
{
  if (!task || reason.empty()) return;
  //  Recording must never turn a failed submit into a crash, and the reason
  //  must not depend on the run log being writable.
  try { setReason(task, reason); } catch (...) {}
  try {
    //  The job store opens the run log only once a job runs.
    TDateTime date;
    string entry = ActivityLog::entry(ActivityLog::ERROR, "Job submission failed",
                                      date, reason.c_str());
    task->appendJobLogCreating("eccejobstore", entry.c_str());
  } catch (...) {}
}

void LaunchBatch::clearFailure(TaskJob *task)
{
  if (!task || task->getProp(VDoc::getEcceNamespace() + ":runStatusReason").empty())
    return;
  try { setReason(task, ""); } catch (...) {}
}

void LaunchBatch::buildArgs(TaskJob *task, EcceMap& kv)
{
  Launchdata l = task->launchdata();
  MachinePreferences *prefs = MachinePreferences::lookup(l.machine);
  char buf[32];

  TypedFile f;
  task->getDataFile(JCode::PRIMARY_INPUT, f);     kv["##input##"] = f.name();
  task->getDataFile(JCode::PARSE_OUTPUT, f);      kv["##parse##"] = f.name();
  task->getDataFile(JCode::PRIMARY_OUTPUT, f);    kv["##output##"] = f.name();
  task->getDataFile(JCode::PROPERTY_OUTPUT, f);   kv["##property##"] = f.name();
  task->getDataFile(JCode::AUXILIARY_OUTPUT, f);  kv["##auxiliary##"] = f.name();
  if (MdTask *md = dynamic_cast<MdTask*>(task)) {
    try {
      kv["##output_frag##"] = md->getOutputFragmentName();
      kv["##restart##"] = md->getRestartName();
      kv["##md_output##"] = md->getMdOutputName();
      kv["##topology##"] = md->getTopologyName();
    } catch (...) {}
  }
  kv["##title##"] = task->getName();

  if (prefs && prefs->isOptionSupported("Q")) {
    kv["##queue##"] = l.queue;
    //  The queue's own request lines, newlines as a literal \n: the param
    //  file is one value per line.
    if (const QueueManager *qm = QueueManager::lookup(l.machine))
      if (const Queue *q = qm->queue(l.queue)) {
        string r = q->requestLines(), enc;
        for (size_t i = 0; i < r.size(); i++)
          if (r[i] == '\n') enc += "\\n"; else enc += r[i];
        kv["##requestlines##"] = enc;
      }
    if (prefs->isOptionSupported("AA"))
      //  The account the calculation was launched with; the machine's
      //  current one only for calculations stored before it was recorded.
      kv["##account_no##"] = l.account.empty() ? prefs->getAllocationAccount()
                                               : l.account;
    if (prefs->isOptionSupported("MM"))
      kv["##maxmemory##"] = StringConverter::toString((size_t)l.maxmemory);
    if (prefs->isOptionSupported("SS"))
      kv["##minscratch##"] = StringConverter::toString((size_t)l.minscratch);
    if (prefs->isOptionSupported("TL")) {
      int days = 0, hours = 0, mins = 0;
      sscanf(l.maxwall.c_str(), "%d %d:%d:00", &days, &hours, &mins);
      hours += days*24;
      snprintf(buf, sizeof buf, "%d:%d:00", hours, mins);
      kv["##wall_clock_time##"] = buf;
      snprintf(buf, sizeof buf, "%d", days*86400 + hours*3600 + mins*60);
      kv["##wall_clock_seconds##"] = buf;
      snprintf(buf, sizeof buf, "%d:%d", hours, mins);
      kv["##wall_clock_hrmin##"] = buf;
    }
  }
  snprintf(buf, sizeof buf, "%lu", l.totalprocs);  kv["##numProcs##"] = buf;
  snprintf(buf, sizeof buf, "%lu", l.nodes);       kv["##numNodes##"] = buf;
  if (prefs && prefs->isOptionSupported("SD")) kv["##scratch##"] = l.scratchdir;
  kv["##runDir##"] = task->jobdata().jobpath;

  string nice;
  if (l.priority != "" && l.priority != "0") {
    nice = "nice";
    if (l.priority != "10") nice += " -" + l.priority;
  }
  kv["##priority##"] = nice;
}

LaunchBatch::Job::Job(TaskJob *task, bool workerMonitoring)
  : p_task(task), p_launch(0), p_args(0),
    p_workerMonitoring(workerMonitoring) {}

LaunchBatch::Job::~Job() { delete p_launch; delete p_args; }

bool LaunchBatch::Job::begin(string& error)
{
  if (!storeRunDirectory(p_task)) {
    error = "Unable to save launch settings for " + p_task->getName();
    return false;
  }
  Launchdata l = p_task->launchdata();
  MachinePreferences *prefs = MachinePreferences::lookup(l.machine);
  RefMachine *rm = prefs ? prefs->getRegisteredMachine() : 0;
  if (!rm) { error = "machine " + l.machine + " is not registered"; return false; }
  if (wallTimeRequired(prefs) && noWallTime(l.maxwall)) {
    l.maxwall = defaultWallTime(prefs, l.queue);
    if (!p_task->launchdata(l)) {
      error = "Unable to save launch settings for " + p_task->getName();
      return false;
    }
  }
  //  As the Launcher: a userSubmit or noRemoteAccess machine is only staged,
  //  and staging needs a shell the user drives, so it is not a batch job.
  if (rm->userSubmit() || rm->noRemoteAccess()) {
    error = "machine " + l.machine + " is set to staged launches; "
            "use the Launcher";
    return false;
  }
  p_args = new EcceMap();
  buildArgs(p_task, *p_args);
  p_launch = new Launch(p_task, *p_args, true);
  p_launch->setWorkerMonitoring(p_workerMonitoring);
  return true;
}

bool LaunchBatch::Job::done() const { return !p_launch || p_launch->done(); }
string LaunchBatch::Job::description() const
{ return p_launch ? p_launch->description() : string(); }
bool LaunchBatch::Job::step()
{
  try { return p_launch->nextOperation(); } catch (...) { return false; }
}
string LaunchBatch::Job::message() const
{ return p_launch ? p_launch->message() : string(); }
string LaunchBatch::Job::takeWarning()
{
  string w = p_launch->launchWarning();
  p_launch->clearLaunchWarning();
  return w;
}
