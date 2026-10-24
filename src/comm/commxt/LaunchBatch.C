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
#include "tdat/QueueMgr.H"
#include "tdat/RefMachine.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/TaskJob.H"
#include "util/EcceMap.H"
#include "util/StringConverter.H"
#include "util/TempStorage.H"
#include "util/TypedFile.H"

using std::string;

bool LaunchBatch::wallTimeRequired(MachinePreferences *prefs)
{
  if (!prefs->isOptionSupported("TL") || !prefs->isOptionSupported("Q"))
    return false;
  const QueueManager *qm = QueueManager::lookup(prefs->getItemKey());
  return qm && qm->queueMgrName() != "Shell";
}


LaunchBatch::Plan LaunchBatch::plan(TaskJob *task)
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

  ResourceDescriptor::RUNSTATE state = task->getState();
  if (state != ResourceDescriptor::STATE_READY &&
      state != ResourceDescriptor::STATE_CREATED) {
    p.problem = "not ready (already submitted or finished)";
    return p;
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
  if (wallTimeRequired(prefs) &&
      (l.maxwall.empty() || l.maxwall == "0 0:0" || l.maxwall == "0 0:0:00")) {
    p.problem = "no wall time stored";
    return p;
  }
  p.ready = true;
  return p;
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
  try {
    //  The job store opens the run log only once a job runs.
    TDateTime date;
    string entry = ActivityLog::entry(ActivityLog::ERROR, "Job submission failed",
                                      date, reason.c_str());
    task->appendJobLogCreating("eccejobstore", entry.c_str());
    setReason(task, reason);
  } catch (...) {
    //  Recording must never turn a failed submit into a crash.
  }
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
    if (prefs->isOptionSupported("AA"))
      kv["##account_no##"] = prefs->getAllocationAccount();
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

LaunchBatch::Job::Job(TaskJob *task) : p_task(task), p_launch(0), p_args(0) {}

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
