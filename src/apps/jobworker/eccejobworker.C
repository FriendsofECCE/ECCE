///////////////////////////////////////////////////////////////////////////////
// eccejobworker: headless submission and monitoring worker.
//
// This first worker adapter is intentionally small: it takes a calculation
// URL plus the existing AuthCache pipe handoff, runs LaunchBatch without wx,
// and enables Launch's in-process monitor path. Events are JSON lines on
// stdout so a future Windows named-pipe client or data-server adapter can
// relay the same contract without coupling the job lifecycle to wxWidgets.
// JobStore's existing diagnostic output can follow the protocol events while
// monitoring, so a dedicated named-pipe event channel is still required for
// the eventual GUI adapter.
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>

#include "comm/JobWorker.H"
#include "comm/LaunchBatch.H"
#include "dsm/EDSI.H"
#include "dsm/TaskJob.H"
#include "tdat/AuthCache.H"
#include "util/Ecce.H"
#include "util/EcceURL.H"

using std::cerr;
using std::cout;
using std::endl;
using std::string;

int main(int argc, char** argv)
{
  string authPipe, calculationUrl;
  for (int i = 1; i < argc; ++i) {
    if (string(argv[i]) == "-pipe" && i + 1 < argc) authPipe = argv[++i];
    else if (string(argv[i]) == "-calcURL" && i + 1 < argc)
      calculationUrl = argv[++i];
    else if (string(argv[i]) == "-v") {
      cout << "eccejobworker lifecycle protocol 1" << endl;
      return 0;
    } else {
      cerr << "Unknown or incomplete argument: " << argv[i] << endl;
      return 2;
    }
  }
  if (authPipe.empty() || calculationUrl.empty()) {
    cerr << "Usage: eccejobworker -pipe auth-cache-pipe -calcURL calculation-url"
         << endl;
    return 2;
  }

  // Import cached ECCE credentials before loading the task. Compute-server
  // passwords and TOTP answers continue through the established RCommand
  // authentication path and are never written into worker records/events.
  AuthCache::getCache().pipeIn(authPipe);
  Ecce::initialize();

  TaskJob *task = dynamic_cast<TaskJob*>(
      EDSIFactory::getResource(EcceURL(calculationUrl)));
  if (!task) {
    JobWorker::emitEvent(JobWorker::FAILURE, JobWorker::FAILED,
         "Unable to load calculation " + calculationUrl);
    return 1;
  }

  JobWorker::Record record;
  record.calculationUrl = calculationUrl;
  record.phase = JobWorker::PREPARING;
  JobWorker::emitEvent(JobWorker::PHASE_CHANGED, record.phase,
       "Preparing stored launch settings");

  LaunchBatch::Job job(task, true);
  string error;
  if (!job.begin(error)) {
    record.phase = JobWorker::FAILED;
    JobWorker::emitEvent(JobWorker::FAILURE, record.phase, error);
    return 1;
  }

  // A headless worker cannot safely accept an interactive launch warning.
  // Surface it to the caller and stop before any submission operation.
  string warning = job.takeWarning();
  if (!warning.empty()) {
    record.phase = JobWorker::FAILED;
    JobWorker::emitEvent(JobWorker::WARNING, record.phase, warning);
    JobWorker::emitEvent(JobWorker::FAILURE, record.phase,
         "Launch needs user review before submission");
    return 1;
  }

  record.phase = JobWorker::SUBMITTING;
  JobWorker::emitEvent(JobWorker::PHASE_CHANGED, record.phase,
       "Submitting calculation");
  while (!job.done()) {
    if (!job.step()) {
      record.phase = JobWorker::FAILED;
      JobWorker::emitEvent(JobWorker::FAILURE, record.phase, job.message());
      return 1;
    }
    warning = job.takeWarning();
    if (!warning.empty()) {
      // Warnings discovered mid-launch are reported. LaunchBatch has no
      // pause/resume confirmation boundary yet, so don't silently continue.
      record.phase = JobWorker::FAILED;
      JobWorker::emitEvent(JobWorker::WARNING, record.phase, warning);
      JobWorker::emitEvent(JobWorker::FAILURE, record.phase,
           "Launch needs user review before submission");
      return 1;
    }
    JobWorker::emitEvent(JobWorker::PROGRESS, record.phase,
         job.description());
  }

  // The in-process JobStore takes over at the launch's monitoring operation
  // and normally exits this worker only after results have been stored.
  record.phase = JobWorker::MONITORING;
  JobWorker::emitEvent(JobWorker::FINISHED, JobWorker::COMPLETE,
       "Calculation and monitoring complete");
  return 0;
}
