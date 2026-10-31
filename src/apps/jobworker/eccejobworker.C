///////////////////////////////////////////////////////////////////////////////
// eccejobworker: headless submission, monitoring, and monitoring recovery.
//
// On Windows the Launcher creates a local named pipe and starts this process.
// The worker connects, reads a one-shot START/RESUME request, and sends framed
// lifecycle events back over the same pipe. Closing the GUI breaks only the
// event channel; the worker and its SSH connection keep running.
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "comm/JobWorker.H"
#include "comm/LaunchBatch.H"
#include "comm/RCommand.H"
#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/TaskJob.H"
#include "tdat/AuthCache.H"
#include "util/Ecce.H"
#include "util/EcceURL.H"

using std::cerr;
using std::cout;
using std::endl;
using std::string;

#ifdef _WIN32
static HANDLE gIpcPipe = INVALID_HANDLE_VALUE;

static bool writePipe(void *context, const string& frame)
{
  HANDLE pipe = (HANDLE)context;
  size_t sent = 0;
  while (sent < frame.size()) {
    DWORD n = 0;
    if (!WriteFile(pipe, frame.data() + sent,
                   (DWORD)(frame.size() - sent), &n, 0) || n == 0)
      return false;
    sent += n;
  }
  return true;
}

static bool writeLine(HANDLE pipe, const string& line)
{ return writePipe(pipe, line + "\n"); }

static string readPipeLine(HANDLE pipe)
{
  string line;
  char c;
  DWORD n = 0;
  while (ReadFile(pipe, &c, 1, &n, 0) && n == 1 && c != '\n')
    line += c;
  if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
  return line;
}

static HANDLE connectPipe(const string& name)
{
  for (int attempt = 0; attempt < 200; ++attempt) {
    HANDLE pipe = CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                              NULL, OPEN_EXISTING, 0, NULL);
    if (pipe != INVALID_HANDLE_VALUE) {
      DWORD mode = PIPE_READMODE_BYTE;
      SetNamedPipeHandleState(pipe, &mode, NULL, NULL);
      return pipe;
    }
    if (GetLastError() != ERROR_PIPE_BUSY && GetLastError() != ERROR_FILE_NOT_FOUND)
      break;
    WaitNamedPipeA(name.c_str(), 50);
  }
  return INVALID_HANDLE_VALUE;
}

static bool confirmWarning(const string& warning)
{
  if (gIpcPipe == INVALID_HANDLE_VALUE) return false;
  const JobWorker::Record record = JobWorker::activeRecord();
  JobWorker::transition(record.phase, JobWorker::WARNING, warning);
  return readPipeLine(gIpcPipe) == "CONTINUE";
}
#endif

static int finishError(JobWorker::Phase phase, const string& message)
{
  if (!JobWorker::transition(phase, JobWorker::FAILURE, message))
    JobWorker::emitEvent(JobWorker::FAILURE, phase, message);
  JobWorker::releaseRecordLock();
#ifdef _WIN32
  if (gIpcPipe != INVALID_HANDLE_VALUE) {
    FlushFileBuffers(gIpcPipe);
    CloseHandle(gIpcPipe);
    gIpcPipe = INVALID_HANDLE_VALUE;
  }
#endif
  return 1;
}

int main(int argc, char** argv)
{
  string authPipe, ipcName, calculationUrl;
  bool resume = false;
  for (int i = 1; i < argc; ++i) {
    if ((string(argv[i]) == "-authPipe" || string(argv[i]) == "-pipe") && i + 1 < argc)
      authPipe = argv[++i];
    else if (string(argv[i]) == "-ipc" && i + 1 < argc) ipcName = argv[++i];
    else if (string(argv[i]) == "-calcURL" && i + 1 < argc)
      calculationUrl = argv[++i];
    else if (string(argv[i]) == "-resume") resume = true;
    else if (string(argv[i]) == "-v") {
      cout << "eccejobworker lifecycle protocol 2" << endl;
      return 0;
    } else {
      cerr << "Unknown or incomplete argument: " << argv[i] << endl;
      return 2;
    }
  }
  if (authPipe.empty() || (ipcName.empty() && calculationUrl.empty())) {
    cerr << "Usage: eccejobworker -authPipe path [-ipc pipe-name] "
            "[-calcURL url [-resume]]" << endl;
    return 2;
  }

  // Load the session's existing credentials before touching DAV or SSH. The
  // TOTP seed and one-time answers are not copied into the worker state file.
  AuthCache::getCache().pipeIn(authPipe);
  Ecce::initialize();

#ifdef _WIN32
  string action = resume ? "RESUME" : "START";
  if (!ipcName.empty()) {
    gIpcPipe = connectPipe(ipcName);
    if (gIpcPipe == INVALID_HANDLE_VALUE) {
      cerr << "Cannot connect to Launcher worker pipe " << ipcName << endl;
      return 1;
    }
    JobWorker::setEventWriter(writePipe, gIpcPipe);
    if (!writeLine(gIpcPipe, "READY")) return finishError(JobWorker::FAILED,
                                               "Cannot handshake with Launcher");
    action = readPipeLine(gIpcPipe);
    calculationUrl = readPipeLine(gIpcPipe);
    if (action != "START" && action != "RESUME")
      return finishError(JobWorker::FAILED, "Invalid worker request");
    resume = action == "RESUME";
  }
#endif

  JobWorker::Record record;
  const string statePath = JobWorker::recordPathFor(calculationUrl);
  if (statePath.empty())
    return finishError(JobWorker::FAILED,
                       "Cannot locate the per-user worker state directory");
  const bool haveRecord = JobWorker::loadRecord(calculationUrl, record);
  string lockError;
  if (!JobWorker::acquireRecordLock(statePath, lockError)) {
    return finishError(JobWorker::RECOVERING, lockError);
  }

  if (resume) {
    if (!haveRecord || record.schedulerJobId.empty() || record.configFile.empty())
      return finishError(JobWorker::NEEDS_RECONCILIATION,
          "No saved scheduler job and monitor configuration; refusing to resubmit");
    if (record.phase == JobWorker::COMPLETE || record.phase == JobWorker::FAILED)
      return finishError(record.phase, "This worker record is already terminal");
    JobWorker::setActiveRecord(statePath, record);
    JobWorker::transition(JobWorker::RECOVERING, JobWorker::PHASE_CHANGED,
        "Resuming saved job " + record.schedulerJobId + " from its monitor bookmark");
    extern void jobstore_main(RCommand* remoteconn, const string& jobId,
                const string& configFileName, const bool& restartFlag);
    jobstore_main((RCommand*)0, record.schedulerJobId, record.configFile, true);
    // JobStore exits this process after recording completion or failure.
    return 0;
  }

  if (haveRecord && (record.phase == JobWorker::SUBMITTING ||
                     record.phase == JobWorker::NEEDS_RECONCILIATION) &&
      record.schedulerJobId.empty()) {
    JobWorker::setActiveRecord(statePath, record);
    return finishError(JobWorker::NEEDS_RECONCILIATION,
        "Submission may have reached the scheduler before the worker stopped. "
        "Reconcile the scheduler job before trying again.");
  }
  if (haveRecord && !record.schedulerJobId.empty() &&
      record.phase != JobWorker::COMPLETE && record.phase != JobWorker::FAILED) {
    JobWorker::setActiveRecord(statePath, record);
    return finishError(JobWorker::RECOVERING,
        "A submitted job record exists; resume monitoring instead of resubmitting");
  }

  record = JobWorker::Record();
  record.calculationUrl = calculationUrl;
  record.phase = JobWorker::CREATED;
  string storeError;
  if (!JobWorker::initializeRecord(statePath, record, storeError))
    return finishError(JobWorker::FAILED, storeError);
  JobWorker::transition(JobWorker::PREPARING, JobWorker::PHASE_CHANGED,
                        "Preparing stored launch settings");

  TaskJob *task = dynamic_cast<TaskJob*>(
      EDSIFactory::getResource(EcceURL(calculationUrl)));
  if (!task)
    return finishError(JobWorker::FAILED,
                       "Unable to load calculation " + calculationUrl);

  LaunchBatch::Job job(task, true);
  string error;
  if (!job.begin(error)) return finishError(JobWorker::FAILED, error);

  // A headless worker cannot accept an interactive launch warning. The
  // Launcher handles confirmation before it starts the worker; any warning
  // here indicates stored settings changed after that confirmation.
  string warning = job.takeWarning();
  if (!warning.empty()) {
#ifdef _WIN32
    if (!confirmWarning(warning))
#endif
      return finishError(JobWorker::FAILED,
                         "Launch warning was not approved: " + warning);
  }

  while (!job.done()) {
    const string stepDescription = job.description();
    const JobWorker::Phase phase = stepDescription.find("Running submit script") != string::npos
        ? JobWorker::SUBMITTING : JobWorker::PREPARING;
    // Persist before the operation starts. If submission loses its reply,
    // startup will require scheduler reconciliation rather than resubmitting.
    JobWorker::transition(phase, JobWorker::PROGRESS, stepDescription);
    if (!job.step()) {
      record = JobWorker::activeRecord();
      if (record.phase == JobWorker::SUBMITTING && record.schedulerJobId.empty())
        return finishError(JobWorker::NEEDS_RECONCILIATION,
            "Submission outcome is unclear; reconcile the scheduler before retrying. " +
            job.message());
      return finishError(JobWorker::FAILED, job.message());
    }
    warning = job.takeWarning();
    if (!warning.empty()) {
#ifdef _WIN32
      if (!confirmWarning(warning))
#endif
        return finishError(JobWorker::FAILED,
                           "Launch warning was not approved: " + warning);
    }
  }

  JobWorker::transition(JobWorker::COMPLETE, JobWorker::FINISHED,
                        "Launch completed without starting a monitor");
  JobWorker::releaseRecordLock();
#ifdef _WIN32
  if (gIpcPipe != INVALID_HANDLE_VALUE) CloseHandle(gIpcPipe);
#endif
  return 0;
}
