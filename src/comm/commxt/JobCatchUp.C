#include <string>
#include <vector>

#include "util/EcceMap.H"
#include "util/EcceURL.H"
#include "util/TypedFile.H"
#include "util/WaitingJobs.H"

#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/JCode.H"
#include "dsm/MdTask.H"
#include "dsm/NWChemMDModel.H"
#include "dsm/PropertyTask.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/TaskJob.H"
#include "dsm/VDoc.H"

#include "comm/JobCatchUp.H"
#include "comm/OpenSshTransport.H"
#include "comm/Launch.H"
#include <fstream>
#include "util/TempStorage.H"
#include "util/SDirectory.H"
#include "comm/ReconnectRules.H"
#include "dsm/MachinePreferences.H"
#include "tdat/RefMachine.H"

using std::string;
using std::vector;


static void leaveWaiting(TaskJob* calc, const string& why)
{
  WaitingJobs::add(calc->getURL().toString());
  calc->setState(ResourceDescriptor::STATE_WAITING);
  vector<MetaDataResult> results(1);
  results[0].name = VDoc::getEcceNamespace() + ":runStatusReason";
  results[0].value = "Could not reconnect to the job (" + why + "); ECCE "
                     "tries again at the next session start, or on Run "
                     "Management > Reconnect.";
  calc->addProps(results);
  calc->notifyProperty("runStatusReason", results[0].value);
}


bool JobCatchUp::reconnect(TaskJob* calc, string& message)
{
  Jobdata job = calc->jobdata();
  if (job.jobid.empty()) {
    message = "No job id available to reconnect";
    return false;
  }
  ResourceDescriptor::RUNSTATE state = calc->getState();

  // The new monitor parses the output from the start.
  PropertyTask* ptask = dynamic_cast<PropertyTask*>(calc);
  if (ptask && !ptask->deleteProperties()) {
    message = "Unable to delete existing output properties";
    return false;
  }

  // Run logs of monitor errors are kept for debugging.
  if (state != ResourceDescriptor::STATE_FAILED)
    calc->removeJobLog();
  calc->removeOutputFiles();

  WaitingJobs::remove(calc->getURL().toString());
  calc->setState(ResourceDescriptor::STATE_SUBMITTED);

  EcceMap kvargs;
  if (!calc->application()) {
    message = "No code registration data found for the calculation";
    leaveWaiting(calc, message);
    return false;
  }
  TypedFile file;
  calc->getDataFile(JCode::PRIMARY_OUTPUT, file);
  kvargs["##output##"] = file.name();
  calc->getDataFile(JCode::PARSE_OUTPUT, file);
  kvargs["##parse##"] = file.name();
  calc->getDataFile(JCode::PROPERTY_OUTPUT, file);
  kvargs["##property##"] = file.name();
  calc->getDataFile(JCode::AUXILIARY_OUTPUT, file);
  kvargs["##auxiliary##"] = file.name();

  MdTask* mdTask = dynamic_cast<MdTask*>(calc);
  if (mdTask != 0) {
    NWChemMDModel taskModel;
    try {
      mdTask->getTaskModel(taskModel);
      kvargs["##output_frag##"] = mdTask->getOutputFragmentName();
      kvargs["##restart##"] = mdTask->getRestartName();
      kvargs["##md_output##"] = mdTask->getMdOutputName();
      kvargs["##topology##"] = mdTask->getTopologyName();
    } catch (...) {
    }
  }

  Launch launch(calc, kvargs);
  //  A Reconnect asks for the login again, even right after a cancel.
  OpenSshTransport::forgetMasterAttempts();
  bool ok = launch.validateLocalDir() &&
            launch.validateRemoteLogin() &&
            launch.generateJobMonitoringFiles() &&
            launch.moveJobMonitoringFiles() &&
            launch.startJobStore("");
  if (ok) {
    message = "Monitoring reconnected to job " + job.jobid + " on " +
              calc->launchdata().machine;
  } else {
    message = launch.message();
    leaveWaiting(calc, message);
  }
  return ok;
}


JobCatchUp::Plan JobCatchUp::plan(TaskJob* calc)
{
  Plan p;
  if (!calc) {
    p.problem = "not a calculation";
    return p;
  }
  p.url = calc->getURL().toString();
  p.name = calc->getName();
  Launchdata l = calc->launchdata();
  p.machine = l.machine;
  ReconnectRules::Input in;
  in.state = calc->getState();
  in.machine = l.machine;
  if (!l.machine.empty()) {
    MachinePreferences* prefs = MachinePreferences::lookup(l.machine);
    in.machineRegistered = prefs && prefs->getRegisteredMachine();
  }
  Jobdata j = calc->jobdata();
  in.jobid = j.jobid;
  in.jobpath = j.jobpath;
  p.state = ReconnectRules::stateText(in.state);
  p.problem = ReconnectRules::problem(in);
  return p;
}


vector<JobCatchUp::Result> JobCatchUp::run(
    std::function<void(size_t, size_t)> progress)
{
  vector<Result> results;
  vector<string> urls = WaitingJobs::list();
  for (size_t i = 0; i < urls.size(); i++) {
    if (progress)
      progress(i, urls.size());
    Result r;
    r.url = urls[i];
    r.ok = false;
    if (WaitingJobs::watched(urls[i]))
      continue;

    TaskJob* calc =
        dynamic_cast<TaskJob*>(EDSIFactory::getResource(EcceURL(urls[i])));
    if (!calc) {
      // Removed or renamed meanwhile; nothing left to catch up.
      WaitingJobs::remove(urls[i]);
      continue;
    }
    ResourceDescriptor::RUNSTATE state = calc->getState();
    if (state != ResourceDescriptor::STATE_WAITING &&
        state != ResourceDescriptor::STATE_SUBMITTED &&
        state != ResourceDescriptor::STATE_RUNNING) {
      // Ended, reset or relaunched from elsewhere.
      WaitingJobs::remove(urls[i]);
      continue;
    }
    r.ok = reconnect(calc, r.message);
    r.message = calc->getName() + ": " + r.message;
    results.push_back(r);
  }
  return results;
}


//  The stored outputs are copied to a scratch directory and read by the job
//  store as an import is: that is the one path that parses without a login.
bool JobCatchUp::reparse(TaskJob* calc, std::string& message)
{
  if (!calc || !calc->application()) {
    message = "No code registration data found for the calculation";
    return false;
  }
  const ResourceDescriptor::RUNSTATE state = calc->getState();
  if (state == ResourceDescriptor::STATE_SUBMITTED ||
      state == ResourceDescriptor::STATE_RUNNING) {
    message = "the job is still being monitored; Reconnect or wait";
    return false;
  }

  SDirectory* dir = TempStorage::getTempJobDirectory(
                              calc->getURL().getFilePathTail().c_str());
  const std::string stage = dir->path();

  EcceMap kvargs;
  const JCode::CodeFileType kinds[2] = { JCode::PARSE_OUTPUT,
                                         JCode::PRIMARY_OUTPUT };
  const char* keys[2] = { "##parse##", "##output##" };
  int copied = 0;
  for (int i = 0; i < 2; i++) {
    TypedFile file;
    calc->getDataFile(kinds[i], file);
    std::istream* is = calc->getDataFile(kinds[i]);
    if (!is || file.name().empty()) { delete is; continue; }
    std::ofstream out((stage + "/" + file.name()).c_str(), std::ios::binary);
    out << is->rdbuf();
    delete is;
    if (!out) continue;
    kvargs[keys[i]] = file.name();
    copied++;
  }
  if (!copied) {
    message = "the calculation has no stored output to parse";
    return false;
  }
  if (kvargs["##parse##"].empty()) kvargs["##parse##"] = kvargs["##output##"];
  if (kvargs["##output##"].empty()) kvargs["##output##"] = kvargs["##parse##"];

  Launch launch(calc, kvargs, false, stage + "/", calc->getName(), true);
  bool ok = launch.validateLocalDir() &&
            launch.validateRemoteLogin() &&
            launch.generateJobMonitoringFiles() &&
            launch.moveJobMonitoringFiles() &&
            launch.startJobStore(stage + "/");
  message = ok ? "re-parsing the stored outputs" : launch.message();
  return ok;
}
