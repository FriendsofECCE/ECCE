// Driver for tests/launch: create a calculation on the data server, launch
// it through the real Launch class, and report its state and properties.
// It mirrors WxLauncher::saveJob()/buildArgs()/launchCalc() without the GUI.
//
//   launchjob create <parentURL> <name> <resourceType> <deckFile> <deckName>
//                    <machine> <runDir> <user>            prints the calc URL
//                    [queue=Q] [nodes=N] [procs=P] [wall="D H:M"] [mem=MB]
//                                 batch settings, as the launcher's queue controls set them;
//                                 LAUNCHJOB_ACCOUNT in the environment sets the account at launch
//   launchjob launch <calcURL>                            exit 0 when submitted
//   launchjob kill   <calcURL>                            RunMgmt::terminate, as the Organizer's Kill
//   launchjob jobid  <calcURL>                            prints the job id Launch parsed
//   launchjob state  <calcURL>                            prints the run state
//   launchjob props  <calcURL>                            one property per line
//   launchjob restart <calcURL> <deckFile> <deckName>     "Reset for Restart", then
//                                                         store the edited deck
//
// Credentials arrive as an AuthCache pipe file named by -pipe <file>,
// exactly as ecmd and eccejobmaster receive them.

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "comm/Launch.H"
#include "comm/RunMgmt.H"
#include "dsm/CodeFactory.H"
#include "dsm/EDSIFactory.H"
#include "dsm/JCode.H"
#include "dsm/PropertyTask.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/TaskJob.H"
#include "tdat/AuthCache.H"
#include "tdat/DefaultDavAuth.H"
#include "util/Ecce.H"
#include "util/EcceMap.H"
#include "util/EcceURL.H"
#include "util/StringConverter.H"
#include "util/TempStorage.H"
#include "util/TypedFile.H"

using namespace std;

static const char* stateName(ResourceDescriptor::RUNSTATE s)
{
  switch (s) {
    case ResourceDescriptor::STATE_CREATED: return "created";
    case ResourceDescriptor::STATE_READY: return "ready";
    case ResourceDescriptor::STATE_SUBMITTED: return "submitted";
    case ResourceDescriptor::STATE_RUNNING: return "running";
    case ResourceDescriptor::STATE_COMPLETED: return "completed";
    case ResourceDescriptor::STATE_LOADED: return "loaded";
    case ResourceDescriptor::STATE_KILLED: return "killed";
    case ResourceDescriptor::STATE_UNSUCCESSFUL: return "unsuccessful";
    case ResourceDescriptor::STATE_FAILED: return "failed";
    case ResourceDescriptor::STATE_SYSTEM_FAILURE: return "system_failure";
    default: return "illegal";
  }
}

static TaskJob* getTask(const string& url)
{
  Resource* rsc = EDSIFactory::getResource(EcceURL(url));
  return rsc ? dynamic_cast<TaskJob*>(rsc) : 0;
}

static int doCreate(const vector<string>& a)
{
  if (a.size() < 8) { cerr << "create: wrong argument count" << endl; return 2; }
  const string& parentUrl = a[0], &name = a[1], &code = a[2], &deckFile = a[3],
                &deckName = a[4], &machine = a[5], &runDir = a[6], &user = a[7];

  Resource* parent = EDSIFactory::getResource(EcceURL(parentUrl));
  if (!parent) { cerr << "no such parent: " << parentUrl << endl; return 1; }

  ResourceDescriptor& rd = ResourceDescriptor::getResourceDescriptor();
  ResourceType* projType = 0;
  ResourceType* calcType = 0;
  vector<ResourceType*> types = rd.getResourceTypes();
  for (size_t i = 0; i < types.size(); i++) {
    if (types[i]->getName() == "project") projType = types[i];
    if (types[i]->getName() == code) calcType = types[i];
  }
  if (!projType || !calcType) { cerr << "resource types not found" << endl; return 1; }

  Resource* project = parent->createChild(name + "-project", projType);
  if (!project) { cerr << "could not create project" << endl; return 1; }
  Resource* rsc = project->createChild(name, calcType);
  TaskJob* task = rsc ? dynamic_cast<TaskJob*>(rsc) : 0;
  if (!task) { cerr << "could not create calculation" << endl; return 1; }

  ifstream deck(deckFile.c_str());
  if (!deck || !task->putInputFile(deckName, &deck)) {
    cerr << "could not store input deck" << endl;
    return 1;
  }

  Launchdata ldat;
  ldat.machine = machine;
  ldat.nodes = 1;
  ldat.totalprocs = 1;
  ldat.rundir = runDir;
  ldat.user = user;
  ldat.remoteShell = "ssh";
  ldat.maxwall = "0 0:0";
  ldat.forceCsh = "true";
  for (size_t i = 8; i < a.size(); i++) {
    string::size_type eq = a[i].find('=');
    string k = a[i].substr(0, eq), v = eq == string::npos ? "" : a[i].substr(eq + 1);
    if (k == "queue") ldat.queue = v;
    else if (k == "nodes") ldat.nodes = strtoul(v.c_str(), 0, 10);
    else if (k == "procs") ldat.totalprocs = strtoul(v.c_str(), 0, 10);
    else if (k == "wall") ldat.maxwall = v;
    else if (k == "mem") ldat.maxmemory = strtoul(v.c_str(), 0, 10);
    else { cerr << "create: unknown setting " << a[i] << endl; return 2; }
  }
  Jobdata jdata;
  jdata.jobpath = runDir + TempStorage::getJobRunDirectoryPath(task->getURL());
  if (!task->launchdata(ldat) || !task->jobdata(jdata)) {
    cerr << "could not save launch settings" << endl;
    return 1;
  }
  task->setState(ResourceDescriptor::STATE_READY);
  cout << task->getURL().toString() << endl;
  return 0;
}

static void buildArgs(TaskJob* task, EcceMap& kv)
{
  TypedFile f;
  task->getDataFile(JCode::PRIMARY_INPUT, f);      kv["##input##"] = f.name();
  task->getDataFile(JCode::PARSE_OUTPUT, f);       kv["##parse##"] = f.name();
  task->getDataFile(JCode::PRIMARY_OUTPUT, f);     kv["##output##"] = f.name();
  task->getDataFile(JCode::PROPERTY_OUTPUT, f);    kv["##property##"] = f.name();
  task->getDataFile(JCode::AUXILIARY_OUTPUT, f);   kv["##auxiliary##"] = f.name();
  kv["##title##"] = task->getName();
  kv["##forcecsh##"] = "true";
  Launchdata ldat = task->launchdata();
  kv["##numProcs##"] = StringConverter::toString((int)ldat.totalprocs);
  kv["##numNodes##"] = StringConverter::toString((int)ldat.nodes);
  kv["##runDir##"] = task->jobdata().jobpath;
  kv["##priority##"] = "";
  //  WxLauncher sets these only for a machine whose options include Q/AA/MM/TL.
  if (!ldat.queue.empty()) {
    kv["##queue##"] = ldat.queue;
    if (getenv("LAUNCHJOB_ACCOUNT")) kv["##account_no##"] = getenv("LAUNCHJOB_ACCOUNT");
    kv["##maxmemory##"] = StringConverter::toString((size_t)ldat.maxmemory);
    int days = 0, hours = 0, mins = 0;
    char buf[32];
    sscanf(ldat.maxwall.c_str(), "%d %d:%d:00", &days, &hours, &mins);
    hours += days*24;
    sprintf(buf, "%d:%d:00", hours, mins);
    kv["##wall_clock_time##"] = buf;
    sprintf(buf, "%d", days*86400 + hours*3600 + mins*60);
    kv["##wall_clock_seconds##"] = buf;
    sprintf(buf, "%d:%d", hours, mins);
    kv["##wall_clock_hrmin##"] = buf;
  }
}

static int doLaunch(const string& url)
{
  TaskJob* task = getTask(url);
  if (!task) { cerr << "not a calculation: " << url << endl; return 1; }
  EcceMap kv;
  buildArgs(task, kv);
  cout << "files: input=" << kv["##input##"] << " output=" << kv["##output##"]
       << " parse=" << kv["##parse##"] << endl;

  Launch launch(task, kv, true);
  bool valid = true;
  while (valid && !launch.done()) {
    cout << "launch: " << launch.description() << endl;
    valid = launch.nextOperation();
    if (!valid) cerr << "launch failed: " << launch.message() << endl;
    else if (!launch.message().empty()) cout << "  " << launch.message() << endl;
    else if (!launch.info().empty()) cout << "  " << launch.info() << endl;
  }
  if (!launch.done()) return 1;
  cout << "state after launch: " << stateName(task->getState()) << endl;
  cout << "run directory: " << task->jobdata().jobpath << endl;
  return 0;
}

// What CalcMgr::resetForRestart() does, then the user's edited input deck.
static int doRestart(const vector<string>& a)
{
  if (a.size() != 3) { cerr << "restart: <calcURL> <deckFile> <deckName>" << endl; return 2; }
  TaskJob* task = getTask(a[0]);
  if (!task) { cerr << "not a calculation: " << a[0] << endl; return 1; }
  if (!task->resetForRestart()) { cerr << "resetForRestart failed" << endl; return 1; }
  ifstream deck(a[1].c_str());
  if (!deck || !task->putInputFile(a[2], &deck)) {
    cerr << "could not store input deck" << endl;
    return 1;
  }
  return 0;
}

int main(int argc, char** argv)
{
  Ecce::initialize();
  DefaultDavAuth authhandler;
  EDSIFactory::addAuthEventListener(&authhandler);

  vector<string> a;
  string pipe;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-pipe") && i + 1 < argc) pipe = argv[++i];
    else a.push_back(argv[i]);
  }
  if (a.empty()) { cerr << "usage: launchjob [-pipe file] create|launch|state|props ..." << endl; return 2; }
  if (!pipe.empty()) AuthCache::getCache().pipeIn(pipe);

  const string mode = a[0];
  a.erase(a.begin());
  if (mode == "create") return doCreate(a);
  if (mode == "restart") return doRestart(a);
  if (a.size() != 1) { cerr << mode << ": needs a calculation URL" << endl; return 2; }
  if (mode == "launch") return doLaunch(a[0]);
  if (mode == "kill") {
    TaskJob* t = getTask(a[0]);
    if (!t) { cerr << "not a calculation: " << a[0] << endl; return 1; }
    cout << RunMgmt::terminate(t) << endl;
    return 0;
  }
  if (mode == "names") {
    TaskJob* t = getTask(a[0]);
    if (!t) { cerr << "not a calculation: " << a[0] << endl; return 1; }
    EcceMap kv;
    buildArgs(t, kv);
    cout << "files: input=" << kv["##input##"] << " output=" << kv["##output##"]
         << " parse=" << kv["##parse##"] << endl;
    return 0;
  }
  if (mode == "jobid") {
    TaskJob* t = getTask(a[0]);
    if (!t) { cerr << "not a calculation: " << a[0] << endl; return 1; }
    cout << t->jobdata().jobid << endl;
    return 0;
  }

  TaskJob* task = getTask(a[0]);
  if (!task) { cerr << "not a calculation: " << a[0] << endl; return 1; }
  if (mode == "state") { cout << stateName(task->getState()) << endl; return 0; }
  if (mode == "props") {
    PropertyTask* pt = dynamic_cast<PropertyTask*>(task);
    if (!pt) { cerr << "not a property task" << endl; return 1; }
    vector<string> names = pt->propertyNames();
    for (size_t i = 0; i < names.size(); i++) cout << names[i] << endl;
    return 0;
  }
  cerr << "unknown mode " << mode << endl;
  return 2;
}
