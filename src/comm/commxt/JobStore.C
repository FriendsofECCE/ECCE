//#define JSPERLTIMER
//#define JSDBTIMER
//#define BENCHMARK
#ifdef BENCHMARK
//#define DONT_STORE
#endif
///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: JobStore.C
//
//
// PURPOSE:
//  JobStore.C implements the client side of job monitoring.
//  It receives message blocks from eccejobmonitor running on the
//  compute server and processes them.  This includes parsing properties
//  into the database and sending messages to update other apps.
//
// DESCRIPTION:
//  (To be filled in later.)
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h> // exit
#include <stdio.h> // snprintf
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <algorithm>
#include <time.h>

#include <iostream>
  using std::cout;
  using std::endl;
  using std::ios;
#include <fstream>
  using std::ofstream;

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/Preferences.H"
#include "util/SFile.H"
#include "util/EcceURL.H"
#include "util/TDateTime.H"
#include "util/EcceException.H"
#include "util/TypedFile.H"
#include "util/StringConverter.H"
#include "util/TempStorage.H"
#include "util/ResourceUtils.H"
#include "util/StringTokenizer.H"
#include "util/DavException.H"

#include "util/JMSSubscriber.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"

#include "tdat/AuthCache.H"
#include "tdat/RefMachine.H"
#include "tdat/DefaultDavAuth.H"

#include "dsm/MachinePreferences.H"
#include "dsm/JCode.H"
#include "dsm/TaskJob.H"
#include "dsm/EDSIFactory.H"
#include "dsm/ActivityLog.H"
#include "dsm/DavDebug.H"
#include "dsm/VDoc.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/EDSI.H" // MetaDataResult -- forward-declared elsewhere, need
                       // the full definition here to fill one in below.

#include "comm/RCommand.H"
#include "comm/JobParser.H"
#include "comm/JobFailureReason.H"
#include "comm/JobLogDates.H"
#include "util/WaitingJobs.H"


//
// types
//
typedef struct _FileInfoStruct
{
  int                     callCount;
  bool                    dbStoreContents;
  bool                    dbStoreName;
  string                  fileName;
  string                  parseType;
  struct _FileInfoStruct* next;
} FileInfoStruct;

typedef struct
{
  int             count;
  FileInfoStruct* list;
} FileListStruct;

enum LogModeJobStore
{
  LOGMODE_YES,              //Do log.
  LOGMODE_REMOVE_IF_OK,     //Do log, but remove if no errors.
  LOGMODE_NO,               //Do not log. 
  LOGMODE_UNKNOWN
};

//
// function prototypes
//
void calcLoad(void);
JobParser* jobparserGet(void);
void calcPutOutFileName(const string& fileName, const string& parseType);
void calcStoreData(string parseType, int callCount,
                   const char* data, bool fromFile);
void calcUpdateState(ResourceDescriptor::RUNSTATE state);
void cleanup(int exitStatus);
string unsuccessfulReason(int status);
string gensubLogReason(const string& outputFilePath);
void recordStatusReason(const string& reason);
void configRead(const string& fileName);
void envRead(void);
void fail(const char* name, const string& msg);
void fileAdd(int callCount, bool dbStoreContents,
             bool dbStoreName, string fileName, string parseType);
void goodbye(int exitStatus);
void init(void);
void initCache(void);
void initConn(void);
void initMon(void);
void initDAV(void);
void convertToNewlines(char* data);
void interactProperty(char* propData, char seqCode);
void interactStatus(char* statusData);
static bool gKilledLocally = false;   // ecce_ejs_kill: leave the remote alone
static bool gStateReported = false;   // the monitor has sent a job status
void interactUp(char* upData);
void interactError(char* errorData);
void interactFile(char* fileData);
void interactNode(char* nodeData);
void interactGetOutput(void);
void interactGetFiles(void);
void getJobMonitorInput(int fid);
void restart(const char* name, const string& msg);
void restartSystem(const char* name, const string& msg);
void park(const char* name, const string& msg, int exitStatus);
void setTimeout(const unsigned long& seconds);
void handleSignal(int signalValue);
void handleTimeout(unsigned long milliseconds);
void dispatchSignals(void);
extern "C" void signalHandler(int signalValue);
void initSignals();

string logModeJobStoreToString(LogModeJobStore mode);
LogModeJobStore logModeJobStoreValue(string& mode);

void logActivityBundle(const string& activities);
void logInfo(const char* event, const string& msg);
void logMessage(const char* event, const string& msg);
void logErr(const char* event, const string& msg);
void logPrint(const ActivityLog::EventType type,
              const char* event, const string& msg);
char* firstToken(char* data);
char* nextToken(void);
const char* strErrno(void);
string localDirectory(void);

// Messaging functions
void initMessaging(void);
void exitMessaging(void);
void msgDispatch(void);
void ejsKillMCB(JMSMessage& msg);
void calcRenamedMCB(JMSMessage& msg);
void calcRemovedMCB(JMSMessage& msg);
void authMCB(JMSMessage& msg);
void ecceExitMCB(JMSMessage& msg);
void sendNotify(const char* op, const string& key1="", const string& name = "",
                const string& key2 = "", const string& value = "");
void interactProgress(char* text);
void progressStore(bool force);
void progressClear(void);
void contactStore(const string& value);
void contactLost(void);
void contactHeard(void);
void contactInit(void);
void reconnectAfterSilence(void);

// Messaging state
static JMSSubscriber *vsubscriber = 0;
static int vMsgFd = -1;     // subscriber's wake-up pipe, -1 when closed

// communication protocol constants
static const char* MSG_TYPE_E_INFO  = "info";
static const char* MSG_TYPE_E_WARN  = "warn";
static const char* MSG_TYPE_E_FATAL = "fatal";
static const char* MSG_TYPE_F_PROP  = "prop";
static const char* MSG_TYPE_F_JOF   = "output";
static const char* MSG_TYPE_F_SCHED = "sched";
static const char  MSG_BLOCK_1_OF_N = '1';
static const char  MSG_BLOCK_M_OF_N = '2';
static const char  MSG_BLOCK_N_OF_N = '3';
static const char  MSG_BLOCK_1_OF_1 = '0';
static const int   MAX_BLOCK_LENGTH = 2048;

// timeouts and retries
static const int   MAX_READ_TRIES = 100;
static const unsigned long PROCESS_TIMEOUT_MILLISECONDS = 1800000; // 30 minutes
// Silence from the monitor (#284): after gContactSeconds the Organizer is
// told the job is out of contact; after the read timeout this store hands
// back to eccejobmaster for a fresh connection.  Both set in contactInit().
static unsigned long readTimeoutMilliseconds = 180000;
static int gContactSeconds = 60;
static bool gContactLost = false;
static time_t gLastHeard = 0;                 // wall clock, for the Organizer
static struct timespec gLastHeardMono = {0, 0};

// file names
static const char* programName = "eccejobstore";

// global variables
static RCommand* remoteconn = 0;
static RCommand* localconn = 0;
static int fdesc;
static TaskJob* calculation = 0;
static JobParser* jobparser = 0;
static bool restartFlag = false;
static bool doneFlag = false;
static bool filesFlag = false;
static string nodeForRestart = "";
static string tmpStorage;
static string bookmarkFileName;
static FileListStruct filesToGet;
static bool logIsOpen = false;
static int logNumErrors = 0;
static string parseIn;
static LogModeJobStore logModeJobStore;
// Self-pipe: the handler writes the signal number, the event loop reads
// it, so handlers run outside signal context (signalfd is Linux-only).
static int sigPipe[2] = {-1, -1};
// One pending timeout; 0 = none.  Monotonic, so clock changes do not fire it.
static unsigned long timeoutMilliseconds = 0;
static struct timespec timeoutDeadline;
static ResourceDescriptor::RUNSTATE endState=ResourceDescriptor::STATE_ILLEGAL;

// The numeric gensub exit code (211/221/231/other) behind the most recent
// STATE_UNSUCCESSFUL, 0 if the job hasn't gone unsuccessful.  Recorded by
// interactStatus() the instant the code is seen -- before the calc's own
// output file has been fetched back from the compute server, which
// happens later in run() -- so cleanup() can look for something more
// specific once that file is actually on disk (see gensubLogReason()).
static int gUnsuccessfulStatus = 0;

// Name of the queue system's stderr file when the monitor sent one.
static string gSchedulerErrFile;

// From the monitor's status message ("305 state=TIMEOUT end=<ISO>Z"): the
// scheduler's end state when it ended the job, and its end time (UTC ISO)
// when it knows one.  The latter dates a job whose script never got to
// write "Completed Job".
static string gSchedState;
static long gSchedEndTime = -1;

// Same idea as gUnsuccessfulStatus, for the STATE_FAILED/STATE_SYSTEM_FAILURE
// paths (fail()/restart()/restartSystem()): they already have a specific,
// human-written reason in hand (their own msg argument) at the point they
// call calcUpdateState() -- no output file or code to consult, unlike the
// gensub-detected STATE_UNSUCCESSFUL case above.
static string gFailReason;
static string lastRestartFileName = "";
static int lastRestartCallCount = 0;

// The latest progress snapshot (#284) and the one last stored with the
// calculation.  Storing is a metadata write on the data server, so it
// happens at most every PROGRESS_STORE_SECONDS and once at the end;
// the broker gets every snapshot (the monitor sends at most one a second).
static string gProgress;
static string gProgressStored;
static time_t gProgressStoredAt = 0;
static const int PROGRESS_STORE_SECONDS = 10;

// Used for storing status information which
// will be saved as meta data for the calculation task 
static string eccejoblog = "";
static bool ecceJobLogAll = false;

// configuration parameters
static string cpConfigFileName;
static string cpCalcURL;
static string cpCodeName;
static string cpJobId;
static string cpRemoteDir;
static string cpServerRef;
static string cpServerName;
static string cpRemoteShell;
static string cpLocalShell;
static string cpUserName;
static string cpImportDir;
static string cpMdStoreTrj;
static string cpRxnMetadynamicsTask;

#ifdef JSPERLTIMER
ETimer perlTimer;
#endif
#ifdef JSDBTIMER
ETimer dbTimer;
#endif

void jobstore_main(RCommand* rconn, const string& jobId,
                   const string& configFileName, const bool& restart)
{
  remoteconn = rconn;
  cpJobId = jobId;
  cpConfigFileName = configFileName;
  restartFlag = restart;

  Ecce::initialize();

  // initialize everything but DB
  DavDebug::setDebugContext("/tmp/davjobstore");

  DefaultDavAuth authhandler;
  EDSIFactory::addAuthEventListener(&authhandler);

  init();

  // connect to server
  initConn();

  initDAV();

  initSignals();

  // invoke eccejobmonitor
  initMon();

  // establish local RCommand connection for property parsing scripts
  initCache();

  // initialize Messaging and DAV after remote connection to avoid RCommand
  // hanging on initial connection.  This appears to be some kind of
  // resource allocation problem (fd's?) where expect trashes over what
  // Messaging and DAV allocate whereas Messaging and DAV don't trash what
  // expect allocates.  
  // 5/3/2002 - Don't know if this comment is relevent to new messaging
  // sytstem or not.
  initMessaging();

  // A new run starts without the last run's progress; a restarted job
  // store keeps it until the monitor sends more.
  if (!restartFlag)
    progressClear();

  // A restart after a monitor error or a park sets the state back to
  // submitted; one after silence left the job running, which it stays
  // until the monitor reports otherwise (#284).
  if (restartFlag && calculation &&
      calculation->getState() != ResourceDescriptor::STATE_RUNNING)
    calcUpdateState(ResourceDescriptor::STATE_SUBMITTED);

  // cache store command args since storeProperty is called so often
  jobparser->cacheParseArgs(tmpStorage);

  // receive the output of the calculation
  logMessage("eccejobmonitor",
             "Receiving calculation output from eccejobmonitor"); 
  interactGetOutput();

  // issue a ctrl-C to break out of jobmonitor if necessary
  // this is commented out because it seems to work fine without it,
  // but just in case I need to revert, I'll leave it
  //(void)remoteconn->exec("\003"); 

  // mpp2 MD monitoring requires restart of ejm on the head compute node for
  // access to parse files
  if (nodeForRestart != "") {
    // hop to nodeForRestart
    initConn();

    // invoke eccejobmonitor, this time on head compute node
    initMon();

    // receive the output of the calculation
    string msg = "Receiving calculation output from eccejobmonitor "
                 "started on head compute node " + nodeForRestart;
    logMessage("eccejobmonitor", msg);

    interactGetOutput();

    // issue a ctrl-C to break out of jobmonitor if necessary
    // this is commented out because it seems to work fine without it,
    // but just in case I need to revert, I'll leave it
    //(void)remoteconn->exec("\003"); 
  }

  if (filesFlag)
    interactGetFiles();

#ifdef JSPERLTIMER
cout << "total JobParser::storeProperty perl parsing time (" << perlTimer.elapsedTime() << ")" << endl;
#endif
#ifdef JSDBTIMER
cout << "total JobParser::storeProperty DAV storage time (" << dbTimer.elapsedTime() << ")" << endl;
#endif

  // clean up everything
  cleanup(0);

  // success
  exit(0);
}


string logModeJobStoreToString(LogModeJobStore mode)
{
  string ret;

  if (mode == ::LOGMODE_YES )
    ret = "yes";
  else if (mode == ::LOGMODE_NO)
    ret = "no";
  else if (mode == ::LOGMODE_REMOVE_IF_OK)
    ret = "rmifok";

  return ret;
}


LogModeJobStore logModeJobStoreValue(string& mode)
{
  LogModeJobStore ret;

  if (mode.find("yes")!=string::npos || mode.find("true")!=string::npos)
    ret = ::LOGMODE_YES;
  else if (mode.find("no")!=string::npos || mode.find("false")!=string::npos)
    ret = ::LOGMODE_NO;
  else if (mode.find("rmifok") != string::npos)
    ret = ::LOGMODE_REMOVE_IF_OK;
  else 
    ret = ::LOGMODE_UNKNOWN;
  return ret;
}


// ------------------------------------------------------------------------- //
// Retrieve the calculation from the database.
// ------------------------------------------------------------------------- //
void calcLoad(void)
{
  EcceURL loadTmp(cpCalcURL);

  // GDB 9/6/02
  // This fixes a nasty bug where eccejobstore will fail to authenticate
  // if you have never started another app like builder (the first thing
  // you do when you run Ecce for a new install is import a calc is the
  // biggee) and you run jobstore.  It fails down the line (actually just
  // goes out to lunch forever) when accessing the calculation due to
  // authentication failing.  This fix causes it to use any password for
  // the server that at least matches the user name.
  AuthCache& auth = AuthCache::getCache();
  auth.setURLPolicy(AuthCache::LAST_URL);

  calculation = dynamic_cast<TaskJob*>(EDSIFactory::getResource(loadTmp));

  if (!calculation) {
    string errMsg = "EDSIFactory::getResource(" + cpCalcURL + ") failed";
    fail("DAV Server", errMsg);
  }

  if (logModeJobStore!=::LOGMODE_NO) {
    // don't start a new job log when it is a restart
    if (restartFlag) {
      logIsOpen = true;
      logMessage("Appending to job log after monitoring restart of calculation",
                 calculation->getURL().toString());
    } else {
      if (calculation->startJobLog("eccejobstore")) {
        logIsOpen = true;
        logMessage("Job log started for calculation",
                   calculation->getURL().toString());
        logMessage("eccejobstore running on client",
                   RCommand::whereami());
      } else
        logErr("DAV Server", "Could not open job log");
    }
  }
}

// ------------------------------------------------------------------------- //
// Retrieve the jobparser.
// ------------------------------------------------------------------------- //

JobParser* jobparserGet(void)
{
  EcceURL loadTmp(cpCalcURL);
  JobParser* ret = new JobParser(loadTmp);
  if (!ret) {
    string errMsg = "Failed to create JobParser for " + cpCalcURL;
    fail("Property Parser", errMsg);
  }
  return ret;
}


/**
 * Record an output file name in the database.  If there is a parse type, 
 * publish an "ecce_url_property_data" message (file contains property data)
 */
void calcPutOutFileName(const string& fileName, const string& parseType)
{ 
  string outputFile = tmpStorage + "/" + fileName; 
  string key, value;
  ifstream ifs(outputFile.c_str(), ios::in | ios::binary);
  VDoc *vdoc = EDSIFactory::getVdoc(calculation->getURL());
  if (vdoc != 0) {

    // Get suffix mappings and see if there is a match
    // If a match is found then rename fileName
    // so that when stored on dav server it will be correctly
    // mime-typed.
    string newFileName = fileName;
    const JCode *codeCap = calculation->application();
    map<string, string> suffixMapping = codeCap->getSuffixMapping();
    map<string, string>::iterator mapIt = suffixMapping.begin();
    while (mapIt != suffixMapping.end()) {

      string fromSuffix = mapIt->first;
      string toSuffix = mapIt->second;
      string fileNameSuffix = "";
      string fileNameBase = "";

      // get suffix and base of fileName
      if ((fileName.rfind(".") != (fileName.size() - 1)) &&
          (fileName.rfind(".") != string::npos))  {
        fileNameSuffix = fileName.substr(fileName.rfind(".")+1, fileName.size() -1);
        if (fileName.rfind(".") != 0) {
          fileNameBase = fileName.substr(0, fileName.rfind(".")+1);
        }
      }
      // Found a match - rename file
      if (fromSuffix == fileNameSuffix) {
        newFileName = fileNameBase + toSuffix;
      }
      mapIt++;
    }


    try {
       if (calculation->putOutputFile(newFileName, &ifs)) {
          // GDB 2/26/13  Sending out this property message can result in non-
          // XML data based being run through the XML parser resulting in a
          // crash.  This came up with parsing the meta.dat file data from
          // a condensed phase metadynamics calculation, but no idea why
          // this hasn't been a problem before.  Just comment out the code
          // instead of getting rid of it in case it is discovered that
          // there really is a purpose in sending out these messages and a
          // different solution needs to be devised.
          //if (!parseType.empty()) {
          //   key = parseType;
          //   value = vdoc->getOutput(newFileName).toString();
          //   sendNotify("ecce_url_property_data", "name", key, "value", value);
          //}
       }
       else {
          logErr("DAV Server", "Attempt to save output file:: "
                "BEGIN EDSI Message :: " +
                calculation->messages() +
                "END EDSI Message " + 
                newFileName + 
                " failed");
       }
    } catch (DavException& ex) {
          logErr("DAV Server", "Attempt to save output file:: "
                "BEGIN EDSI Message :: " +
                calculation->messages() +
                "END EDSI Message " + 
                newFileName + 
                " failed. " + ex.what());
    }
    delete vdoc;
  }
  else {
    logErr("DAV Server", "VDoc retrieval failed:: "
                         "BEGIN EDSI Message :: " +
                         calculation->messages() +
                         "END EDSI Message " + 
                         calculation->getURL().toString() +
                         " (" + fileName + ")" +
                         " failed");
  }
  ifs.close();

}


// ------------------------------------------------------------------------- //
// Store the data for the calculation in the database.
// ------------------------------------------------------------------------- //
void calcStoreData(string parseType, int callCount,
                   const char* data, bool fromFile)
{
  string storedProps;
  propertyMapPairToValue properties;
  pair < string, int > mapPair;
  jobparser->storeProperty(localconn, cpCodeName.c_str(), parseType.c_str(),
                           properties, callCount, data, eccejoblog, fromFile);
  if (!eccejoblog.empty()) {
    logActivityBundle(eccejoblog);
    eccejoblog.clear();
  }
  propertyMapPairToValue::iterator p;
  string key;
  string value;
  for (p = properties.begin(); p != properties.end(); p++) {
    mapPair = (*p).first;
    key = mapPair.first;
    value = (*p).second; 
    if (!key.empty() && !value.empty()) 
      sendNotify("ecce_url_property_data", "name", key, "value", value);
  }
}


// ------------------------------------------------------------------------- //
// Update the state of the calculation in the database,
// and notify other client applications.
// ------------------------------------------------------------------------- //
void calcUpdateState(ResourceDescriptor::RUNSTATE state)
{
  // calculation stays NULL until calcLoad() successfully assigns it --
  // fail()/restart()/restartSystem() call calcUpdateState()
  // unconditionally, though, including on calcLoad()'s own
  // EDSIFactory::getResource() failure path, where calculation is still
  // NULL. Confirmed live via a real core dump. Nothing meaningful to
  // update if the calc resource itself never loaded.
  if (!calculation) return;

  if (state < ResourceDescriptor::STATE_COMPLETED) {
    // Only the first "running" is the start: a restarted monitor reports
    // running again, which must not move the start to the restart (#463).
    // A new run starts with no date, as setState(<= READY) clears it.
    if (state == ResourceDescriptor::STATE_RUNNING &&
        calculation->startDate().empty()) {
      TDateTime sdate;
      calculation->startDate(sdate.toString());
    }

    calculation->setState(state);
    string stateStr = ResourceUtils::stateToString(state);
    logMessage("Calculation State Change", stateStr);
  } else {
    TDateTime edate;
    calculation->completionDate(edate.toString());

    endState = state;
  }
}


// ------------------------------------------------------------------------- //
// Look for gensub's own "...setting status to..." line inside the calc's
// primary output file, in the "-----ECCE Log Information-----" block that
// gensub's submit script appends to it after the job exits (see
// scripts/gensub's determineStatus()/getSubmitScript()). This names the
// actual file/marker that tripped (e.g. "Output file orca.orcaout does
// not exist - setting status to failed"), which is strictly more useful
// than the generic numeric-code decode in unsuccessfulReason() above, so
// callers should prefer it when it's present. Returns "" if the file
// can't be read or has no such line (e.g. a code's own plain nonzero
// exit, which gensub doesn't annotate beyond logging the exit status).
// ------------------------------------------------------------------------- //
string gensubLogReason(const string& outputFilePath)
{
  ifstream ifs(outputFilePath.c_str());
  if (!ifs)
    return "";

  string line, lastReason;
  bool inLogBlock = false;
  while (getline(ifs, line)) {
    if (line.find("-----ECCE Log Information-----") != string::npos)
      inLogBlock = true;
    else if (inLogBlock && line.find("setting status") != string::npos)
      lastReason = line;
  }
  return lastReason;
}


// ------------------------------------------------------------------------- //
// Record (or, given "", clear) why the calc ended up unsuccessful as
// metadata on the calc itself -- same namespaced-key convention
// TaskJob::setState() already uses for "state" -- and tell other running
// client apps (Organizer, Launcher) about it the same way state changes
// themselves are announced, so a lamp-only "incomplete"/"failed" icon
// isn't the only place the reason can be seen.
// ------------------------------------------------------------------------- //
void recordStatusReason(const string& reason)
{
  //  NEVER DOWNGRADE.  A job store can run more than once for one job (a
  //  restart), and a later pass that could not see the cause recorded
  //  gensub's generic "... setting status to failed" line over the
  //  specific one -- so the Launcher showed both.  A generic reason
  //  (gensub's echo, or a bare exit-code decode) does not replace a
  //  specific one already recorded; an empty reason (success) still
  //  clears it.
  if (!reason.empty() &&
      (reason.find("setting status") != string::npos ||
       reason.find("(exit code") != string::npos)) {
    const string existing =
        calculation->getProp(VDoc::getEcceNamespace() + ":runStatusReason");
    if (!existing.empty() &&
        existing.find("setting status") == string::npos &&
        existing.find("(exit code") == string::npos) {
      return;
    }
  }
  vector<MetaDataResult> results(1);
  results[0].name = VDoc::getEcceNamespace() + ":runStatusReason";
  results[0].value = reason;
  calculation->addProps(results);
  calculation->notifyProperty("runStatusReason", reason);
}


// ------------------------------------------------------------------------- //
// Clean up everything in preparation for termination.
// ------------------------------------------------------------------------- //
void cleanup(int exitStatus)
{
  // Before the final state: the Organizer reads the snapshot when the
  // state change arrives.
  progressStore(true);

  // Set the final run state (if set) just before exitting to guarantee
  // output files have been uplodaed to data server
  if (endState != ResourceDescriptor::STATE_ILLEGAL) {
    // Dates from the job script's own log block, else the scheduler's end
    // time; calcUpdateState() left "now", which is wrong on a Reconnect of
    // a finished job (#650).
    {
      TypedFile dFile;
      calculation->getDataFile(JCode::PRIMARY_OUTPUT, dFile);
      long endT = -1, startT = -1;
      if (!dFile.name().empty()) {
        const string dPath = tmpStorage + "/" + dFile.name();
        endT = JobLogDates::logBlockTime(dPath, "Completed Job");
        startT = JobLogDates::logBlockTime(dPath, "Starting Job");
      }
      if (endT < 0)
        endT = gSchedEndTime;
      if (endT >= 0) {
        TDateTime edate;
        edate.setFromTimeT((time_t)endT);
        calculation->completionDate(edate.toString());
      }
      if (startT >= 0 && calculation->startDate().empty()) {
        TDateTime sdate;
        sdate.setFromTimeT((time_t)startT);
        calculation->startDate(sdate.toString());
      }
    }

    calculation->setState(endState);
    string stateStr = ResourceUtils::stateToString(endState);
    logMessage("Calculation State Change", stateStr);

    // special STTR reaction rate study logic to reset the state of all
    // follow-on tasks to force input file regeneration because it's likely
    // the chemical system has changed (GEOMTRACE property)
    Resource* parent =
              EDSIFactory::getResource(calculation->getURL().getParent());
    if (endState>=ResourceDescriptor::STATE_COMPLETED &&
        parent->getApplicationType()==ResourceDescriptor::AT_REACTION_STUDY) {
      calculation->resetReactionTasks();
    }

    if (endState == ResourceDescriptor::STATE_UNSUCCESSFUL) {
      // By now interactGetFiles() (called from run(), before cleanup())
      // has already fetched the calc's output files into tmpStorage, so
      // the log-block line -- unlike at the point interactStatus() first
      // saw the status code -- is actually readable here.
      string reason = unsuccessfulReason(gUnsuccessfulStatus);

      TypedFile outFile;
      calculation->getDataFile(JCode::PRIMARY_OUTPUT, outFile);
      if (gUnsuccessfulStatus == 305) {
        // The scheduler's verdict names the cause, its own slurm.err line
        // follows.  The code's output is not scanned: it only stopped early.
        reason = JobLogDates::schedulerEndReason(gSchedState);
        if (reason.empty())
          reason = "The scheduler ended the job (" + gSchedState + ")";
        const string errName =
            gSchedulerErrFile.empty() ? string("slurm.err") : gSchedulerErrFile;
        reason += JobFailureReason::schedulerErrNote(
                      errName, tmpStorage + "/" + errName, true);
      } else if (!outFile.name().empty()) {
        string outPath = tmpStorage + "/" + outFile.name();

        string logReason = gensubLogReason(outPath);
        if (!logReason.empty())
          reason = logReason;

        // gensub's line names the symptom it detected (missing marker,
        // missing file, unexpected core) not the underlying cause --
        // look for something more specific in the code's own output
        // (and slurm.err alongside it, if that happened to be fetched
        // too) and prefer it when found.  Andy, 2026-09-27: the
        // "no ORCA TERMINATED NORMALLY" message "doesn't tell us WHY".
        vector<string> scanFiles;
        scanFiles.push_back(outPath);
        if (!gSchedulerErrFile.empty())
          scanFiles.push_back(tmpStorage + "/" + gSchedulerErrFile);
        scanFiles.push_back(tmpStorage + "/slurm.err");
        scanFiles.push_back(tmpStorage + "/slurm.out");
        string specificReason = JobFailureReason::extractReason(scanFiles);
        if (!specificReason.empty())
          reason = specificReason;

        if (reason != unsuccessfulReason(gUnsuccessfulStatus)) {
          // Only worth a second run-log line when it adds real detail
          // beyond the coarse decode interactStatus() already logged.
          logMessage("Calculation Incomplete", reason);
        }
      }

      if (gUnsuccessfulStatus != 305 && !gSchedulerErrFile.empty())
        reason += JobFailureReason::schedulerErrNote(
                      gSchedulerErrFile, tmpStorage + "/" + gSchedulerErrFile);

      recordStatusReason(reason);
    } else if ((endState == ResourceDescriptor::STATE_FAILED ||
                endState == ResourceDescriptor::STATE_SYSTEM_FAILURE) &&
               !gFailReason.empty()) {
      recordStatusReason(gFailReason);
    } else {
      recordStatusReason("");
    }
  }

  // Monitoring ended for good (done, or failed beyond retrying).
  if ((exitStatus == 0 || exitStatus == 3) && !gKilledLocally)
    WaitingJobs::remove(cpCalcURL);

  if (exitStatus == 0) {
    if (gKilledLocally) {
      // A newer store owns the run directory and the login; this one only
      // lets go of its own stream.
      if (remoteconn != (RCommand*)0)
        remoteconn->stopStream();
    } else if (remoteconn != (RCommand*)0 && remoteconn->isOpen()) {
      // The monitor stream may still hold the login (a kill message).
      remoteconn->stopStream();
      (void)remoteconn->exec("/bin/rm -f eccejobmonitor eccejobmonitor.conf "
                             "eccejobmonitor.propbuf *.desc");
      delete remoteconn;
    }

    exitMessaging();

    if (localconn != (RCommand*)0)
      delete localconn;
  }

  delete jobparser;
  goodbye(exitStatus);

  // don't add the "close document" tags for a restart. calculation can
  // still be NULL here (calcLoad()'s own getResource() failure reaching
  // cleanup() via fail()) -- confirmed live via a real core dump, same
  // shape as the other calculation-NULL fixes in this file.
  if (exitStatus != 2 && calculation)
    calculation->endJobLog("eccejobstore");
}


// ------------------------------------------------------------------------- //
// Read the configuration file.
//
// The configuration file should contain the following parameters:
//
// codeName
//     Name of the computational code.
// host
//     Name of server.
// calcURL
//     URL of calculation.
// remoteDir
//     Full pathname of directory on compute server in which the
//     the calculation will be run.
// remoteShell
//     Remote communications shell (ssh, rsh) to be used.
// userName
//     Login name of user running the computation.
// importDir
//     For an import, the full pathname of directory where the input file
//     was selected, empty otherwise.
// ------------------------------------------------------------------------- //

void configRead(const string& fileName)
{
  // see if the configuration file exists
  string message;
  SFile configFile(fileName.c_str());
  if (!configFile.exists()) {
    message = "Configuration file '" + fileName + "' does not exist";
    fail("Configuration Parameter", message);
  }

  // read the configuration file
  EcceMap *param = EcceMap::load(fileName);

  // parse the configuration file and check for missing parameters
  message = "";

  if (!param->findValue("calcURL", cpCalcURL)) {
    message += "\nMissing 'calcURL' parameter";
  }
  if (!param->findValue("codeName", cpCodeName)) {
    message += "\nMissing 'codeName' parameter";
  }
  if (!param->findValue("remoteDir", cpRemoteDir)) {
    message += "\nMissing 'remoteDir' parameter";
  }
  if (!param->findValue("host", cpServerRef)) {
    message += "\nMissing 'host' parameter";
  }
  if (!param->findValue("remoteShell", cpRemoteShell)) {
    message += "\nMissing 'remoteShell' parameter";
  }
  if (!param->findValue("userName", cpUserName)) {
    message += "\nMissing 'userName' parameter";
  }
  if (!param->findValue("importDir", cpImportDir)) {
    message += "\nMissing 'importDir' parameter";
  }
  string mdStoreTrjStr;
  if (!param->findValue("mdStoreTrj", mdStoreTrjStr)) {
    message += "\nMissing 'mdStoreTrj' parameter";
  } else {
    cpMdStoreTrj = mdStoreTrjStr;
    if (mdStoreTrjStr=="server" && !Ecce::ecceStoreTrajectories())
      cpMdStoreTrj = "none";
  }
  if (!param->findValue("rxnMetadynamicsTask", cpRxnMetadynamicsTask)) {
    message += "\nMissing 'rxnMetadynamicsTask' parameter";
  }
  delete param;

  if (message != "") {
    string errMsg = "Errors in configuration file '" +
                    fileName + "':" + message;
    fail("Configuration File", errMsg);
  }
}


// ------------------------------------------------------------------------- //
// Read environment variables of interest.
// ------------------------------------------------------------------------- //
void envRead(void)
{
  char* value;

  // set default values
  logModeJobStore = ::LOGMODE_REMOVE_IF_OK;

  // ECCE_JOB_LOGMODE has one of the following forms:
  //
  // jobstoreMode
  // ,jobmonitorMode
  // jobstoreMode,jobmonitorMode
  //
  // jobstoreMode and jobmonitorMode each have one of the following values:
  //
  // value    meaning
  // -----    -------
  // yes      do log
  // rmifok   do log but remove if no errors
  // no       do not log
  //
  // Note that jobmonitorMode can be ignored by eccejobmonitor if
  // eccejobmonitor is configured to override that eccejobstore parameter.
  
  if ((value = getenv("ECCE_JOB_LOGMODE")) != NULL) {
    // parse value
    string modeString;
    string modes = value;
    int it;
    if ((it = modes.find(',')) == string::npos)
      modeString = modes;
    else
      modeString = modes.substr(0, it);

    // set and verify value
    if ((logModeJobStore = logModeJobStoreValue(modeString))
         == ::LOGMODE_UNKNOWN)
      logModeJobStore = ::LOGMODE_REMOVE_IF_OK;
  }

  if ((value = getenv("ECCE_JOB_LOGALL")) != NULL)
    ecceJobLogAll = true; 
}


// ------------------------------------------------------------------------- //
// Log the given message, clean up, and exit--4 variations:
// fail => immediate exit with email for things to investigate
// restart => invoke eccejobstore again with email
// restartSystem => the job was lost: park it, invoke ejs again
// park => the job was lost: "waiting for login", exit 4 (retry) or 5
// ------------------------------------------------------------------------- //
void fail(const char* name, const string& msg)
{
  logPrint(ActivityLog::FATAL, name, msg);
  logNumErrors++;

  // initialize messaging if needed
  initMessaging();

  gFailReason = msg;
  calcUpdateState(ResourceDescriptor::STATE_FAILED);
  cleanup(3);
  exit(3);
}

void restart(const char* name, const string& msg)
{
  logPrint(ActivityLog::FATAL, name, msg);
  logNumErrors++;

  logInfo("eccejobstore", "returning to eccejobmaster for restart");

  // initialize messaging if needed
  initMessaging();

  gFailReason = msg;
  calcUpdateState(ResourceDescriptor::STATE_FAILED);
  cleanup(2);
  exit(2);
}

// The job store lost the job (connection, monitor or login), which says
// nothing about the job itself: park it rather than fail it (#208).
void restartSystem(const char* name, const string& msg)
{
  logInfo("eccejobstore", "returning to eccejobmaster for restart");
  park(name, msg, 4);
}

// Stop monitoring with the calculation "waiting for login": recorded in
// WaitingJobs, so that the next session start or Reconnect catches it up
// from the job's own files.  Exit 4 lets eccejobmaster try again, 5 not.
// Set when another process of the session asked for the login that this one
// waited for: it told the user, this one only parks.
static bool parkQuiet = false;

void park(const char* name, const string& msg, int exitStatus)
{
  if (parkQuiet)
    logMessage(name, msg);
  else {
    logPrint(ActivityLog::WARNING, name, msg);
    logNumErrors++;
  }

  initMessaging();

  WaitingJobs::add(cpCalcURL);
  // An end state the monitor reported is not final until the outputs are
  // stored, which has not happened.
  endState = ResourceDescriptor::STATE_ILLEGAL;
  calcUpdateState(ResourceDescriptor::STATE_WAITING);
  if (calculation)
    recordStatusReason("Monitoring stopped (" + msg + "); the job may still "
                       "be running or finished.  ECCE checks it at the next "
                       "session start, or on Run Management > Reconnect.");
  cleanup(exitStatus);
  exit(exitStatus);
}

void handleSignal(int signalValue)
{
  switch (signalValue)
  {
    // custom signals
    case SIGCONT  :
      logMessage("Signal", "SIGCONT: Connect to new messaging session");
      initMessaging();
      break;

    // fatal signals
    case SIGINT   :
      restart("Signal", "SIGINT: Interrupt received"); break;
    case SIGSEGV  :
      restart("Signal", "SIGSEGV: Segmentation fault received"); break;
    case SIGBUS   :
      restart("Signal", "SIGBUS: Bus error"); break;
    // the client is shutting down or logging out; the job goes on
    case SIGTERM  :
      park("Signal", "SIGTERM: the client is going away", 5); break;

    // warning signals
    case SIGPIPE  :
      logMessage("Signal", "SIGPIPE: Write on a pipe with no one to read it");
      break;
    case SIGFPE   :
      logMessage("Signal", "SIGFPE: Floating point exception"); break;
    case SIGSYS   :
      logMessage("Signal", "SIGSYS: Bad argument to system call"); break;
    case SIGTSTP  :
      logMessage("Signal", "SIGTSTP: User stop requested from tty"); break;
    case SIGXFSZ  :
      logMessage("Signal", "SIGXFSZ: Exceeded file size limit"); break; 
  }
}

extern "C" void signalHandler(int signalValue)
{
  int savedErrno = errno;

  // re-establish the signal handler--only applicable to warning signals
  signal(signalValue, signalHandler);

  // queue the signal for the event loop; a full pipe drops it
  unsigned char byte = (unsigned char)signalValue;
  ssize_t ignored = write(sigPipe[1], &byte, 1);
  (void)ignored;

  errno = savedErrno;
}

// Handle every signal queued since the last call, in arrival order.
void dispatchSignals(void)
{
  unsigned char buf[64];
  ssize_t n;
  while ((n = read(sigPipe[0], buf, sizeof(buf))) > 0)
    for (ssize_t i = 0; i < n; i++)
      handleSignal(buf[i]);
}

void handleTimeout(unsigned long milliseconds)
{
  if (milliseconds == PROCESS_TIMEOUT_MILLISECONDS)
    restart("eccejobstore",
            "Processing timeout--eccejobstore hung while processing message");
  else
    reconnectAfterSilence();
}

// The monitor fell silent (no heartbeat, no output) for the read timeout.
// That says nothing about the job, so its state stays as it is (running,
// shown "running?") and eccejobmaster starts a fresh store and monitor,
// which read on from the bookmark (#284, #208).  Only the last try parks
// the job waiting for login, since nothing will watch it after that.
void reconnectAfterSilence(void)
{
  const string msg =
      "Read timeout--job monitor failed to send heartbeat message";
  const char* left = getenv("ECCE_JOB_TRIES_LEFT");
  if (left && atoi(left) <= 0) {
    restartSystem("eccejobstore", msg);
    return;
  }
  logPrint(ActivityLog::WARNING, "eccejobstore", msg);
  logMessage("eccejobstore", "reconnecting; the run state is left as it is");
  logNumErrors++;
  initMessaging();
  if (!gContactLost)
    contactLost();
  WaitingJobs::add(cpCalcURL);
  endState = ResourceDescriptor::STATE_ILLEGAL;
  cleanup(4);
  exit(4);
}

void ecceExitMCB(JMSMessage& msg)
{
  logMessage("Messaging",
             "ECCE quit message--messaging closed");
  exitMessaging();
}

void ejsKillMCB(JMSMessage& msg)
{
  if (calculation) {
    string urlstr = msg.getProperty("url");

    string calcURL = calculation->getURL().toString();
    if (calcURL == urlstr) {
      logMessage("Messaging",
                 "Job monitoring kill (reconnect or terminate import) message");

      // The new store has been (or is being) started in the same run
      // directory and owns the state: this one, possibly from an earlier
      // session with its own login, must not touch the remote host.  The
      // old monitor ends on SIGPIPE at its next write.
      gKilledLocally = true;
      endState = ResourceDescriptor::STATE_ILLEGAL;
      const char* delay = getenv("ECCE_TEST_EJS_KILL_DELAY_MS");
      if (delay && atoi(delay) > 0)
        usleep(atoi(delay) * 1000);
      cleanup(0);
      exit(0);
    }
  }
}

void calcRenamedMCB(JMSMessage& msg)
{
  string oldURL = msg.getProperty("oldurl");
  string newURL = msg.getProperty("newurl");

  if (calculation && calculation->getURL().isChildOrMe(oldURL)) {
    EDSIFactory::renamePoolResource(oldURL, newURL);
    string calcurl = calculation->getURL().toString();

    // just to stay consistent -- not currently used at this point
    cpCalcURL = calcurl;
    jobparser->setURL(calcurl);
    
    string message = "Calculation renamed to " + calcurl;
    logMessage("Messaging", message);

    logMessage("Cache",
               "Successfully updated property parsing cache to new URL");
  }
}

void calcRemovedMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");

  if (calculation && calculation->getURL().isChildOrMe(urlstr)) {
    // it's already over--just die as quickly and quietly as possible
    exit(0);
  }
}

void authMCB(JMSMessage& msg)
{
  // fake the callerID argument because jobstore never prompts for passwords
  // anyway so we will never send an auth_changed message
  AuthCache::getCache().msgIn(msg, "");
}


void initSignals()
{
  // Non-blocking so a flood cannot block the handler or the drain; close-
  // on-exec so the ssh/bash children RCommand starts do not inherit it.
  if (pipe(sigPipe) != 0)
    fail("System", string("Could not create signal pipe: ") + strErrno());
  for (int i = 0; i < 2; i++) {
    fcntl(sigPipe[i], F_SETFL, fcntl(sigPipe[i], F_GETFL) | O_NONBLOCK);
    fcntl(sigPipe[i], F_SETFD, FD_CLOEXEC);
  }

  // These should be failures/restarts
  signal(SIGINT, signalHandler); 
  signal(SIGSEGV, signalHandler);
  signal(SIGBUS, signalHandler);
  signal(SIGTERM, signalHandler);

  // These should just be logged but should not cause death
  signal(SIGPIPE, signalHandler);
  signal(SIGFPE, signalHandler);
  signal(SIGSYS, signalHandler);
  signal(SIGTSTP, signalHandler);
  signal(SIGXFSZ, signalHandler);

  // Override system signal for reconnecting messaging
  // Originally SIGUSR1 was used but this killed vim sessions
  signal(SIGCONT, signalHandler);
}


// ------------------------------------------------------------------------- //
// Store the given file info.
// ------------------------------------------------------------------------- //
void fileAdd(int callCount, bool dbStoreContents, bool dbStoreName,
             string fileName, string parseType)
{
  // store the highest restart file sequence number between invocations
  static int maxSeq = -1;

  // create new object
  FileInfoStruct* fileInfo = new FileInfoStruct;
  if (fileInfo == (FileInfoStruct *)0)
    fail("System", "Could not get new FileInfoStruct object");

  // hardwired file name matching for MD restart files because of the
  // fancy logic to keep track of which one is last in the sequence
  if (fileName.find(".rst")==string::npos) {
    // copy info into new object
    fileInfo->callCount       = callCount;
    fileInfo->dbStoreContents = dbStoreContents;
    fileInfo->dbStoreName     = dbStoreName;
    fileInfo->fileName        = fileName;
    fileInfo->parseType       = parseType;

    // prepend this object to list
    fileInfo->next = filesToGet.list;
    filesToGet.list = fileInfo;

    // count this file
    filesToGet.count++;

  } else if (cpMdStoreTrj != "server") {
    // smarts to hold onto the highest sequence number restart file
    int dot = fileName.find('.');
    if (dot != string::npos) {
      string base = fileName.substr(0, dot);
      int idx;
      // set idx to the first digit in the restart file sequence
      for (idx = base.length(); idx>0 && isdigit(base[idx-1]); idx--);

      int seqIndex = atoi((char*)&(base.c_str()[idx]));

      if (seqIndex > maxSeq) {
        maxSeq = seqIndex;
        lastRestartFileName = fileName;
        lastRestartCallCount = callCount;
      }
    }
  }
}


// ------------------------------------------------------------------------- //
// Copy the files remembered via fileAdd() into the current directory.
// ------------------------------------------------------------------------- //
void interactGetFiles(void)
{
  filesFlag = false;

  FileInfoStruct* fi;
  string fileNames;

  // add the highest sequence number restart file
  if (lastRestartFileName!="") {
    // copy info into new object

    // create new object
    FileInfoStruct* fileInfo = new FileInfoStruct;
    if (fileInfo == (FileInfoStruct *)0)
      fail("System", "Could not get new FileInfoStruct object");

    fileInfo->callCount       = lastRestartCallCount;
    fileInfo->dbStoreContents = false;
    fileInfo->dbStoreName     = true;
    fileInfo->fileName        = lastRestartFileName;
    fileInfo->parseType       = "";

    // prepend this object to list
    fileInfo->next = filesToGet.list;
    filesToGet.list = fileInfo;

    // count this file
    filesToGet.count++;
  }

  if (filesToGet.count == 0)
    // nothing to do
    return;

  // retrieve files locally or remotely
  string calcdir = tmpStorage;

  // create extra space in the array for .cub, .cube, and possibly
  // MD Traject.trj, .rst, and .pmf dependent upon whether trajectories will be stored
  // then add the +1 for the null entry at the end of the array
  // allocate extra space as needed for files added below
  int extraCount = 4;
  if (cpMdStoreTrj != "none")
    extraCount += 3;
  if (cpRxnMetadynamicsTask == "yes")
    extraCount += 5;

  char **fileStrs = (char**)malloc((filesToGet.count+extraCount)*sizeof(char*));
  int it=0;

  // construct list of full pathnames.  There can be duplicates (parse and
  // output file are the same) but we don't want to transfer a file more
  // than once so add special logic.
  string fullFile;
  for (fi = filesToGet.list; fi; fi = fi->next) {
    bool addit=true;
    for (int idx=0; idx<it; idx++) 
      if (strstr(fileStrs[idx], fi->fileName.c_str()) != 0) {
        addit=false;
        break;
      }

    if (addit) {
      fullFile = cpRemoteDir + "/" + fi->fileName;
      fileStrs[it++] = strdup((char*)fullFile.c_str());
    }
  }

  // add the extra files
  fullFile = cpRemoteDir + "/*.cub";
  fileStrs[it++] = strdup((char*)fullFile.c_str());

  fullFile = cpRemoteDir + "/*.cube";
  fileStrs[it++] = strdup((char*)fullFile.c_str());

  // DirDyVTST specific file
  fullFile = cpRemoteDir + "/*.file30";
  fileStrs[it++] = strdup((char*)fullFile.c_str());

  if (cpMdStoreTrj != "none") {
    fullFile = cpRemoteDir + "/*.trj";
    fileStrs[it++] = strdup((char*)fullFile.c_str());

    fullFile = cpRemoteDir + "/*.rst";
    fileStrs[it++] = strdup((char*)fullFile.c_str());

    fullFile = cpRemoteDir + "/*.pmf";
    fileStrs[it++] = strdup((char*)fullFile.c_str());
  }

  if (cpRxnMetadynamicsTask == "yes") {
    fullFile = cpRemoteDir + "/*.db";
    fileStrs[it++] = strdup((char*)fullFile.c_str());

    fullFile = cpRemoteDir + "/*.movecs";
    fileStrs[it++] = strdup((char*)fullFile.c_str());

    fullFile = cpRemoteDir + "/*.vmovecs";
    fileStrs[it++] = strdup((char*)fullFile.c_str());

    fullFile = cpRemoteDir + "/*.vpp";
    fileStrs[it++] = strdup((char*)fullFile.c_str());

    fullFile = cpRemoteDir + "/*.psp";
    fileStrs[it++] = strdup((char*)fullFile.c_str());
  } 

  // last char* must be NULL
  fileStrs[it] = NULL;

  string copyFileName = calcdir + "/.eccejobstore.copy";
  ofstream copyfile;
  copyfile.open(copyFileName.c_str(), (ios::out | ios::trunc));
  if (!copyfile) {
    string errMsg = "Could not open copy log file '" + copyFileName +
                    "' for writing";
    logErr("File System", errMsg);
  }

  string copyErrMessage;
  bool status;

  RefMachine* refMachine = RefMachine::refLookup(cpServerRef);
  EE_RT_ASSERT(refMachine,EE_FATAL,"No RefMachine object");

  if (cpRemoteShell.find("shellcp")!=string::npos ||
      refMachine->singleConnect() ||
      (refMachine->frontendMachine()!="" &&
       (refMachine->frontendBypass()=="" ||
        !RCommand::isSameDomain(refMachine->frontendBypass())))) {
    if (remoteconn != (RCommand*)0 && remoteconn->isOpen()) {
      logMessage("Benchmark", "Before shell based file transfer of output files");
      status = remoteconn->shellget((const char**)fileStrs, calcdir);
      logMessage("Benchmark", "After shell based file transfer of output files");
    } else {
      logMessage("Benchmark", "Could not attempt shell based file transfer of "
                 "output files--remote shell connection not available");
    }
  } else {
    // copy files from server
    logMessage("Benchmark", "Before file copy of output files");
    // Over the monitor's own login when there is one: a new login would ask
    // a two-factor site for another code.
    status = false;
    if (remoteconn != (RCommand*)0 && remoteconn->isOpen() &&
        remoteconn->sshBackend() != "") {
      vector<string> fs;
      for (int n = 0; fileStrs[n] != NULL; n++) fs.push_back(fileStrs[n]);
      status = remoteconn->copyOnLogin(false, fs, calcdir, copyErrMessage);
      if (!status)
        logMessage("File System", "Copy over the monitor's login failed (" +
                   copyErrMessage + "); logging in again to copy");
    }
    if (!status)
      status = RCommand::get(copyErrMessage, cpServerName, cpRemoteShell,
                             cpUserName, "", (const char**)fileStrs, calcdir);
    logMessage("Benchmark", "After file copy of output files");
  }

  if (status) {
    string msg = "Successfully copied files from server--\nRCommand::get(" +
                 cpServerName + ", " + cpRemoteShell + ", " + cpUserName +
                 ", <elided>, {";
    for (it=0; fileStrs[it]!=NULL;  it++) {
      msg += fileStrs[it];
      msg += ",";
    }
    msg.resize(msg.length() - 1);
    msg += "}, " + calcdir + ") succeeded";

    if (copyfile) {
      copyfile << msg << endl;
      copyfile.close();
    }

    logMessage("File Copy", msg);
  }
  else {
    string errMsg = "Could not get files from server--\nRCommand::get(" +
                    cpServerName + ", " + cpRemoteShell + ", " + cpUserName +
                    ", <elided>, {";
    for (it=0; fileStrs[it]!=NULL;  it++) {
      errMsg += fileStrs[it];
      errMsg += ",";
      // free memory now because we return before it is free'd otherwise
      free(fileStrs[it]);
    }
    free((char*)fileStrs);

    errMsg.resize(errMsg.length() - 1);
    errMsg += "}, " + calcdir + ") failed: " + copyErrMessage;

    if (copyfile) {
      copyfile << errMsg << endl;
      copyfile.close();
    }
    logErr("File Copy", errMsg);

    return;
  }

  for (it=0; fileStrs[it]!=NULL; it++)
    free(fileStrs[it]);

  free((char*)fileStrs);

  // store files in database
  for (fi = filesToGet.list; fi; fi = fi->next) {
    if (fi->dbStoreContents) {

      // store data in database
      calcStoreData(fi->parseType, fi->callCount,
                    fi->fileName.c_str(), true);
    }

    if (fi->dbStoreName) {
      // store file name in database
      calcPutOutFileName(fi->fileName, fi->parseType);
    }
  }

  // .cub, .cube, .trj, .rst, and .pmf files must be handled specially
  // because they were copied over from the compute server with a wildcard.
  // Need to figure out what got copied over with a local ls command
  // and then upload each of those to the data server
  string lsout, fileName;
  if (localconn->execout("/bin/ls *.cub *.cube", lsout)) {
    StringTokenizer cubeFiles(lsout, " \t\n\r");
    while (!(fileName=cubeFiles.next()).empty()) {
      calcPutOutFileName(fileName, "");
    }
  }

  if (localconn->execout("/bin/ls *.file30", lsout)) {
    StringTokenizer file30Files(lsout, " \t\n\r");
#if 0000
    char buf[256];
#endif
    while (!(fileName=file30Files.next()).empty()) {

#if 0000
      // Disable HACK--TITLE directive in NWChem DirDyVTST input fixes problem
      // HACK Alert
      // NWChem DirDyVTST spits out a bogus first line in the file30
      // output file that has a bunch of null characters.
      // This line needs to be stripped out or ECCE won't be able to
      // successfully write out the file for the POLYRATE job
      ifstream ifs(fileName.c_str());
      if (!ifs.getline(buf, 255)) {
        ifs.close();
        string cmd = "sed -i '1,1d' " + RCommand::quotePath(fileName);
        localconn->exec(cmd);
      } else
        ifs.close();
#endif

      calcPutOutFileName(fileName, "");
    }
  }

  //  The auxiliary output a code asks to keep (ORCA's .gbw).  Only a
  //  file that really arrived is stored; a failed job may leave none.
  {
    const JCode *jc = calculation->application();
    string retain;
    string only;
    if (jc != 0 && jc->get_string("RetainOutputFile", only) && !only.empty()) {
      if (localconn->exists(tmpStorage + "/" + only))
        calcPutOutFileName(only, "");
    } else if (jc != 0 && jc->get_string("RetainAuxiliaryOutput", retain) &&
        retain == "yes") {
      vector<TypedFile> aux = jc->getCodeFiles(JCode::AUXILIARY_OUTPUT);
      for (size_t a = 0; a < aux.size(); a++) {
        if (!aux[a].name().empty() &&
            localconn->exists(tmpStorage + "/" + aux[a].name()))
          calcPutOutFileName(aux[a].name(), "");
      }
    }
  }

  if (cpRxnMetadynamicsTask == "yes") {
    if (localconn->execout("/bin/ls *.db *.movecs *.vmovecs *.vpp *.psp", lsout)) {
      StringTokenizer metaFiles(lsout, " \t\n\r");
      while (!(fileName=metaFiles.next()).empty()) {
        calcPutOutFileName(fileName, "");
      }
    }
  }

  if (cpMdStoreTrj != "none") {
    bool uploadTrj = false;
    bool uploadRst = false;
    bool uploadPmf = false;
    if (localconn->execout("/bin/ls *.trj", lsout)) {
      StringTokenizer trjFiles(lsout, " \t\n\r");
      while (!(fileName=trjFiles.next()).empty()) {
        if (cpMdStoreTrj == "server")
          calcPutOutFileName(fileName, "");
        else
          uploadTrj = true;
      }
    }

    if (localconn->execout("/bin/ls *.rst", lsout)) {
      StringTokenizer rstFiles(lsout, " \t\n\r");
      while (!(fileName=rstFiles.next()).empty()) {
        if (cpMdStoreTrj == "server")
          calcPutOutFileName(fileName, "");
        else
          uploadRst = true;
      }
    }

    if (localconn->execout("/bin/ls *.pmf", lsout)) {
      StringTokenizer pmfFiles(lsout, " \t\n\r");
      while (!(fileName=pmfFiles.next()).empty()) {
        if (cpMdStoreTrj == "server")
          calcPutOutFileName(fileName, "");
        else
          uploadPmf = true;
      }
    }

    int uploadCount = 0;
    if (uploadTrj)
      uploadCount++;
    if (uploadRst)
      uploadCount++;
    if (uploadPmf)
      uploadCount++;

    if (uploadCount > 0) {
      fileStrs = (char**)malloc((uploadCount+1)*sizeof(char*));
      uploadCount = 0;
      string fileStr;
      if (uploadTrj) {
        fileStr = calcdir + "/*.trj";
        fileStrs[uploadCount++] = strdup(fileStr.c_str());
      }
      if (uploadRst) {
        fileStr = calcdir + "/*.rst";
        fileStrs[uploadCount++] = strdup(fileStr.c_str());
      }
      if (uploadPmf) {
        fileStr = calcdir + "/*.pmf";
        fileStrs[uploadCount++] = strdup(fileStr.c_str());
      }
      fileStrs[uploadCount] = NULL;

      // upload files to NWFS archive
      logMessage("Benchmark", "Before upload to NWFS archive");

      // JACK:  These "cp..." variables are the ones that need to be
      // derived from the user's nwfs configuration rather than being
      // hardwired like they are now.  At least these three should not
      // be hardwired:  cpArchiveUserName, cpArhiveRootDir, <password>
      // The <password> I didn't put in a variable name for slightly
      // better security

      string msg;
      MachinePreferences *prefs = MachinePreferences::lookup("nwfs");
      if (prefs != NULL) {
        const string cpArchiveName = "n2.emsl.pnl.gov";
        const string cpArchiveShell = "sftp";
        string cpArchiveRootDir = prefs->getRemoteDirectory();
        string cpArchiveUserName = prefs->getUsername();
      
        cpArchiveRootDir += TempStorage::getJobRunDirectoryPath(
                                                  calculation->getURL());
        status = RCommand::put(copyErrMessage, cpArchiveName, cpArchiveShell,
                               cpArchiveUserName, "",
                               (const char**)fileStrs, cpArchiveRootDir);
        logMessage("Benchmark", "After upload to NWFS archive");
 
        if (status) {
          msg = "Successfully copied files to NWFS archive--\nRCommand::put(" +
                       cpArchiveName + ", " + cpArchiveShell + ", " +
                       cpArchiveUserName + ", <elided>, {";
          msg.resize(msg.length() - 1);
          msg += "}, " + calcdir + ") succeeded";
        } else {
          msg = "Could not copy files to NWFS archive--\nRCommand::put(" +
                       cpArchiveName + ", " + cpArchiveShell + ", " +
                       cpArchiveUserName + ", <elided>, {";
          msg.resize(msg.length() - 1);
          msg += "}, " + calcdir + ") failed: " + copyErrMessage;
        }
      } else {
        msg ="Could not copy files to NWFS archive: NWFS access not configured";
      }
 
      string nwfsFileName = calcdir + "/.eccejobstore.nwfs";
      ofstream nwfsfile;
      nwfsfile.open(copyFileName.c_str(), (ios::out | ios::trunc));
      if (!nwfsfile) {
        string errMsg = "Could not open archive log file '" + nwfsFileName +
                        "' for writing";
        logErr("File System", errMsg);
      }
 
      if (nwfsfile) {
        nwfsfile << msg << endl;
        nwfsfile.close();
      }
 
      logMessage("File Archive", msg);
 
      for (it=0; fileStrs[it]!=NULL; it++)
        free(fileStrs[it]);
 
      free((char*)fileStrs);
   
    }
  }
}

// ------------------------------------------------------------------------- //
// Log a goodbye message.
// ------------------------------------------------------------------------- //
void goodbye(int exitStatus)
{
  // construct the message
  string msg = programName;
  if (exitStatus == 0) 
    msg += " terminating normally, exit status ";
  else
    msg += " terminating abnormally, exit status ";

  char buf[8];
  (void)sprintf(buf, "%d", exitStatus);
  msg += buf;

  logMessage("eccejobstore exit", msg);
}


// ------------------------------------------------------------------------- //
// Initialize everything but RCommand connection, Messaging, and DB.
// ------------------------------------------------------------------------- //
void init(void)
{
  // read environment variables
  envRead();

  //Setup EDSI communications to begin working with server.

  // read parameters from the configuration file
  configRead(cpConfigFileName);

  // initialize other global variables
  filesToGet.count = 0;
  filesToGet.list = (FileInfoStruct *)0;
}

void msgDispatch(void)
{
  vsubscriber->processMessage();
}

// ------------------------------------------------------------------------- //
// Initialize Messaging.
// ------------------------------------------------------------------------- //
void initMessaging(void)
{
  if (vsubscriber == (JMSSubscriber*)0) {

    vsubscriber = new JMSSubscriber("EcceJobStore");

    bool status = true;
    status &= vsubscriber->subscribe("ecce_url_renamed",calcRenamedMCB);
    status &= vsubscriber->subscribe("ecce_url_removed",calcRemovedMCB);
    status &= vsubscriber->subscribe("ecce_quit",ecceExitMCB);
    status &= vsubscriber->subscribe("ecce_ejs_kill",ejsKillMCB);
    // Subscribes at the broker (must happen AFTER all subscribe calls).
    // The socket below is the subscriber's wake-up pipe: libmosquitto's
    // thread never calls into this loop, which polls the pipe instead.
    status &= vsubscriber->startSubscriber();

    if (!status)
      logErr("Messaging", "Unable to subscribe to one or more message types");

    int msgsocket = vsubscriber->getSocketID();

    if (msgsocket == -1)
      // Should this result in calling fail()?
      logErr("Messaging", "Subscriber socket is -1");

    vMsgFd = msgsocket;
  }
}

void exitMessaging(void)
{
  if (vsubscriber != (JMSSubscriber*)0) {
    // Have to remove from event loop
    vMsgFd = -1;
    delete vsubscriber;
    vsubscriber = (JMSSubscriber*)0;
  }
}

// ------------------------------------------------------------------
//
void initCache(void)
{
  // localconn may be corrupted so just don't even mess with looking at
  // the old one--leave it as a memory leak
  // Only exec/execout/cd go through localconn, so direct mode can serve it.
  localconn = new RCommand("system", "", "bash", "", "", "", "", "", "", "");
  if (localconn->isOpen())
    logMessage("System", "Started local RCommand connection");
  else
    restart("System", "Unable to establish local RCommand connection");

  if (!localconn->cd(tmpStorage)) {
    string message =  "Unable to cd to local calculation cache directory " +
                      tmpStorage;
    fail("Cache not found ", message);
  }
}  

// ------------------------------------------------------------------------- //
// Initialize DAV
// ------------------------------------------------------------------------- //
void initDAV(void)
{
  calcLoad();

  jobparser = jobparserGet();
  logMessage("Cache", "Successfully started property parsing cache");
}

// ------------------------------------------------------------------------- //
// Establish RCommand connection with the compute server and start the
// job monitor.  Also start local RCommand for invoking parsing scripts.
// ------------------------------------------------------------------------- //
void initConn(void)
{
  RefMachine* refMachine = RefMachine::refLookup(cpServerRef);
  EE_RT_ASSERT(refMachine,EE_FATAL,"No RefMachine object");

  cpServerName = refMachine->fullname();

  string message;
  if (nodeForRestart == "")
    message = "Connecting to compute server " + cpServerName + " as " +
              cpUserName + " via " + cpRemoteShell;
  else
    message = "Connecting to head compute node " + nodeForRestart +
              " of compute server " + cpServerName + " as " + cpUserName +
              " via " + cpRemoteShell;

  logMessage("Compute Server", message);

  cpLocalShell = refMachine->shell();

  string shellPath = refMachine->shellPath();
  string libPath = refMachine->libPath();
  string sourceFile = refMachine->sourceFile();
  string frontendMachine = refMachine->frontendMachine();
  string frontendBypass = refMachine->frontendBypass();

  const char* comms = getenv("ECCE_JOB_COMMS");
  if (comms && (string(comms) == "socket" || string(comms) == "socketlocal"))
    logMessage("Compute Server", string("ECCE_JOB_COMMS=") + comms +
               " is no longer supported; monitoring over stdio");

  if (nodeForRestart == "") {
    if (remoteconn == (RCommand*)0)
      remoteconn = new RCommand(cpServerName, cpRemoteShell, cpLocalShell,
                                cpUserName, "", frontendMachine, frontendBypass,
                                shellPath, libPath, sourceFile);
  } else
    (void)remoteconn->hop(nodeForRestart, cpLocalShell, cpUserName,
                          "", shellPath, libPath, sourceFile);

  if (remoteconn->commError() != "") {
    message = "Failure to connect to compute server: " +
              remoteconn->commError();
    logMessage("Compute Server", message);
  }
}


void initMon(void)
{
  string message;

  if (remoteconn->isOpen()) {
    // specify command to invoke eccejobmonitor
    // -progress: this job store relays jmPROG; an older one would treat it
    // as a protocol error, so the monitor sends it only when asked.
    string cmd = "perl eccejobmonitor -configFile eccejobmonitor.conf"
                 " -progress 1";

    if (cpJobId != "")
      cmd += " -jobId " + cpJobId;

    string bookmarkStr = "0";

    tmpStorage = localDirectory();
    logMessage("Local cache directory", tmpStorage);
    bookmarkFileName = tmpStorage + "/bookmark";
    parseIn = tmpStorage + "/parseIn";

    if (restartFlag) {
      ifstream bookmarkFile(bookmarkFileName.c_str());
      if (bookmarkFile) {
        char buf[64];
        bookmarkFile.getline(buf, 64);
        bookmarkStr = buf;
        bookmarkFile.close();

        if (bookmarkStr == "EOF")
          fail("eccejobstore",
               "Cannot re-parse frequency 'last' type properties");
      }
    } else
      // make sure nothing is left over from previous invocations
      (int)unlink(bookmarkFileName.c_str());

    cmd += " -bookmark " + bookmarkStr;

    // The value echoed below serves as the indicator in output that something
    // went wrong--unexpected exit from eccejobmonitor.  It is more reliable
    // than looking for a "+go+" prompt, which a bumped connection could send
    // while the monitor is still running fine
    cmd += "; echo eccejobmonitor_went_bye_bye";

    // cd to the calculation directory so all paths can be relative
    if (!remoteconn->cd(cpRemoteDir)) {
      message = "Unable to cd on the compute server to calculation directory "+
                cpRemoteDir + " " + remoteconn->commError();
      restartSystem("System", message);
    }

    {
      // infinite timeout
      remoteconn->streamTimeout(-1);

      // The monitor's stdout/stderr come back on a pipe and the framed
      // protocol is read from it as usual.
      // On the login initConn() made: a second login would cost a
      // two-factor site another code for every monitored job.
      if (!remoteconn->startStream(cmd, true))
        restartSystem("System", remoteconn->commError());
      logMessage("Job Monitor",
                 "Started job monitor (stdio comms) with command: " + cmd);

      // set file descriptor for stdio monitoring
      fdesc = remoteconn->streamFd();
    }
  } else {
    string why = remoteconn->commError();
    if (remoteconn->loginCancelled()) {
      why = "login to " + cpServerName + " cancelled or not answered; jobs on " +
            cpServerName + " wait for a new login";
      parkQuiet = !remoteconn->loginPrompted();
    }
    restartSystem("System", why);
  }
}


// Replace the pending timeout; 0 clears it.
void setTimeout(const unsigned long& milliseconds)
{
  timeoutMilliseconds = milliseconds;
  if (milliseconds != 0) {
    clock_gettime(CLOCK_MONOTONIC, &timeoutDeadline);
    timeoutDeadline.tv_sec += milliseconds / 1000;
    timeoutDeadline.tv_nsec += (long)(milliseconds % 1000) * 1000000L;
    if (timeoutDeadline.tv_nsec >= 1000000000L) {
      timeoutDeadline.tv_sec++;
      timeoutDeadline.tv_nsec -= 1000000000L;
    }
  }
}

// Milliseconds until the pending timeout (0 if due), -1 if none.
static int timeoutRemaining(void)
{
  if (timeoutMilliseconds == 0)
    return -1;
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  long long ns = (long long)(timeoutDeadline.tv_sec - now.tv_sec) * 1000000000LL +
                 (timeoutDeadline.tv_nsec - now.tv_nsec);
  if (ns <= 0)
    return 0;
  return (int)((ns + 999999) / 1000000);
}

// Milliseconds until the monitor's silence counts as out of contact (0 if
// it does now), -1 when already reported.
static int contactRemaining(void)
{
  if (gContactLost || gLastHeard == 0)
    return -1;
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  long long ms = (long long)(gLastHeardMono.tv_sec + gContactSeconds -
                             now.tv_sec) * 1000LL +
                 (gLastHeardMono.tv_nsec - now.tv_nsec) / 1000000LL;
  return ms <= 0 ? 0 : (int)ms;
}

// ------------------------------------------------------------------------- //
// Get the calculation output from eccejobmonitor.
// Return if successful, exit otherwise.
// ------------------------------------------------------------------------- //
// Declare static data buffer for performance
static char databuf[MAX_BLOCK_LENGTH];

enum ReadState {idState, countState, dataState};

void getJobMonitorInput(int fid)
{
  static enum ReadState readState = idState;
  static int needRead = 1;
  static int readTries = 0;
  static char* dataptr = databuf;
  static string nonMonStr = "";
  static char seqCode;
  static bool ignoreFlag = false;

  int nbytes = read(fid, dataptr, needRead);
  if (nbytes > 0)
    contactHeard();

  if (nbytes == needRead) {
    readTries = 0;

    if (readState == idState) {
      if (*databuf == '\001') {
        ignoreFlag = false;
        readState = countState;
        needRead = 5;

        if (nonMonStr != "") {
          logErr("Skipped output not from eccejobmonitor", nonMonStr);
          nonMonStr = "";
        }
      } else if (!ignoreFlag) {
        // append a single character
        nonMonStr += *databuf;

        if (nonMonStr.find("eccejobmonitor_went_bye_bye") != string::npos) {
          logErr("Unexpected eccejobmonitor exit", nonMonStr);
          restartSystem("eccejobmonitor", "Job monitor has died");
        }
      }
    } else if (readState == countState) {
      readState = dataState;
      dataptr = databuf;
      seqCode = *databuf;
      databuf[5] = '\0';
      needRead = atoi(&databuf[1]);

      if (needRead <= 0) {
        string msg = "eccejobmonitor message has invalid "
                     "continuation/length field: ";
        msg += databuf;
        restart("eccejobmonitor", msg);
      }
    } else {
      // null terminate the data even for partial reads
      dataptr[needRead] = '\0';

      readState = idState;
      dataptr = databuf;
      needRead = 1;

      // process the message
      setTimeout(PROCESS_TIMEOUT_MILLISECONDS);

      if (seqCode==MSG_BLOCK_M_OF_N || seqCode==MSG_BLOCK_N_OF_N)
        interactProperty(databuf, seqCode);
      else if (strcmp(databuf, "jmPROP") == 0)
        interactProperty(&databuf[7], seqCode);
      else if (strcmp(databuf, "jmPROG") == 0)
        interactProgress(&databuf[7]);
      else if (strcmp(databuf, "jmUP") == 0)
        interactUp(&databuf[5]);
      else if (strcmp(databuf, "jmSTATUS") == 0)
        interactStatus(&databuf[9]);
      else if (strcmp(databuf, "jmFILE") == 0)
        interactFile(&databuf[7]);
      else if (strcmp(databuf, "jmEOFS") == 0)
        filesFlag = true;
      else if (strcmp(databuf, "jmERROR") == 0)
        interactError(&databuf[8]);
      else if (strcmp(databuf, "jmNODE") == 0) {
        interactNode(&databuf[7]);
        doneFlag = true;
      } else if (strcmp(databuf, "jmDONE") == 0) {
        doneFlag = true;
      } else {
        if (*databuf != '\0') {
          string msg = "Unrecognized eccejobmonitor message type: '";
          msg += databuf;
          msg += "'";
          restart("eccejobmonitor", msg);
        } else
          restartSystem("eccejobmonitor",
                        "Possible eccejobmonitor exception (killed?)");
      }
    }

  } else if (nbytes > 0) {
    readTries = 0;

    // partial message read, update counts and keep trying for remainder
    needRead -= nbytes;
    dataptr += nbytes;
  } else {
    readTries++;

    if (nbytes == 0)
      logErr("eccejobmonitor", "Read failed (read of 0 bytes)--retrying");
    else if (errno == EINTR)
      logErr("eccejobmonitor", "Read failed (caused by signal)--retrying");
    else
      logErr("eccejobmonitor", "Read failed (not a signal)--retrying");

    if (readTries >= MAX_READ_TRIES)
      restartSystem("eccejobmonitor",
                    "Job monitor has died (exceeded maximum read retries)");
  }

  setTimeout(readTimeoutMilliseconds);
}


#ifdef _WIN32
// poll() for CRT pipe descriptors, which WSAPoll cannot watch: a pipe is ready
// when it holds data or its writer has gone; anything else is a socket.
static int winPoll(struct pollfd* fds, int n, int timeoutMs)
{
  ULONGLONG start = GetTickCount64();
  for (;;) {
    int ready = 0;
    for (int i = 0; i < n; i++) {
      fds[i].revents = 0;
      if (fds[i].fd < 0) continue;
      HANDLE h = (HANDLE)_get_osfhandle(fds[i].fd);
      DWORD avail = 0;
      if (h != INVALID_HANDLE_VALUE && GetFileType(h) == FILE_TYPE_PIPE) {
        if (PeekNamedPipe(h, 0, 0, 0, &avail, 0))
          fds[i].revents = avail > 0 ? POLLIN : 0;
        else
          fds[i].revents = POLLHUP;
      } else {
        struct pollfd one = fds[i];
        if (poll(&one, 1, 0) > 0) fds[i].revents = one.revents;
      }
      if (fds[i].revents) ready++;
    }
    if (ready > 0 || timeoutMs == 0) return ready;
    if (timeoutMs > 0 && GetTickCount64() - start >= (ULONGLONG)timeoutMs)
      return 0;
    Sleep(10);
  }
}
#define poll winPoll
#endif

// Wait for and handle one round of events: queued signals first, then an
// expired timeout, then input from the monitor and the broker.
static void processEvents(int monFd)
{
  const short ready = POLLIN | POLLHUP | POLLERR | POLLNVAL;
  for (;;) {
    struct pollfd fds[3] = {
      {sigPipe[0], POLLIN, 0}, {monFd, POLLIN, 0}, {vMsgFd, POLLIN, 0}};

    int wait = timeoutRemaining();
    const int contact = contactRemaining();
    if (contact >= 0 && (wait < 0 || contact < wait))
      wait = contact;
    if (poll(fds, 3, wait) < 0) {
      if (errno == EINTR)
        continue;
      restartSystem("eccejobstore", string("poll failed: ") + strErrno());
    }

    if (fds[0].revents & POLLIN) {
      dispatchSignals();
      return;
    }

    if (timeoutRemaining() == 0) {
      unsigned long ms = timeoutMilliseconds;
      timeoutMilliseconds = 0;
      handleTimeout(ms);
      return;
    }

    // Only while nothing is waiting from the monitor: a backlog read after
    // a long property store is contact, not silence.
    if (!(fds[1].revents & ready) && contactRemaining() == 0)
      contactLost();

    bool handled = false;
    if (fds[1].revents & ready) {
      getJobMonitorInput(monFd);
      handled = true;
    }
    // vMsgFd may have been closed by a callback above.
    if (!doneFlag && (fds[2].revents & ready) && vMsgFd == fds[2].fd) {
      msgDispatch();
      handled = true;
    }
    if (handled)
      return;
  }
}

void interactGetOutput(void)
{
  doneFlag = false;
  contactInit();
  setTimeout(readTimeoutMilliseconds);

#ifdef BENCHMARK
  ETimer time;
  time.start();
#endif

  while (!doneFlag)
    processEvents(fdesc);

#ifdef BENCHMARK
  time.stop();
  message =  "Parsing output file took ";
  message += time.elapsedTime();
  logInfo("Benchmark", message);
#endif

  // clear timeout
  setTimeout(0);

  if (remoteconn != (RCommand*)0)
    remoteconn->stopStream();
}


// ------------------------------------------------------------------------- //
// Print the given communications message to the log file.
// ------------------------------------------------------------------------- //
void logMessage(const char* event, const string& msg)
{
  logPrint(ActivityLog::MESSAGE, event, msg);
}

// ------------------------------------------------------------------------- //
// Print the given information message to the log.
// ------------------------------------------------------------------------- //
void logInfo(const char* event, const string& msg)
{
  if (ecceJobLogAll)
    logPrint(ActivityLog::INFO, event, msg);
}

// ------------------------------------------------------------------------- //
// Print the given message to the log file,
// prefixed by an error indication and followed by a newline.
// ------------------------------------------------------------------------- //
void logErr(const char* event, const string& msg)
{
  logPrint(ActivityLog::ERROR, event, msg);
  logNumErrors++;
}

// ------------------------------------------------------------------------- //
// Print the given prefix and message to the log file,
// followed by a newline.
// ------------------------------------------------------------------------- //

void logPrint(const ActivityLog::EventType type,
              const char* event, const string& msg)
{
  if (logModeJobStore!=::LOGMODE_NO && event && !msg.empty()) {
    TDateTime date;
    string logEntry = ActivityLog::entry(type, event, date, msg.c_str());
    // Always echo to stderr too, not just as a fallback when the DAV job
    // log write itself fails below. fail()/restart()/restartSystem() can
    // all be reached before logIsOpen is ever set true (any early
    // failure in init()/initConn()/initDAV(), before the calc's own DAV
    // resource has loaded) -- previously logIsOpen==false meant the
    // *entire* failure reason was silently dropped, with nothing written
    // anywhere: eccejobstore.log (stdout+stderr, captured by
    // eccejobmaster's own redirect) came back completely empty even for
    // a real, specific failure, making this class of bug nearly
    // impossible to diagnose after the fact from the logs alone.
    cerr << programName << ": " << logEntry << endl;
    if (logIsOpen) {
      if (!calculation->appendJobLog(logEntry.c_str())) {
        // don't set logIsOpen to false--make it try to append to the log
        // each time because on calc renames it can fail intermittently
        // logIsOpen = false;
      }
    }
  }
}


// ------------------------------------------------------------------------- //
// Print bundle of messages received from the cache when the parser returns.
// We will trust that the entries have already been put in an Activity Log
// format.
// ------------------------------------------------------------------------- //
void logActivityBundle(const string& activities)
{
  if (logModeJobStore!=::LOGMODE_NO && !activities.empty()) {
    if (logIsOpen) {
      if (!calculation->appendJobLog(activities.c_str())) {
        // don't set logIsOpen to false--make it try to append to the log
        // each time because on calc renames it can fail intermittently
        // logIsOpen = false;
        cerr << programName << ": " << activities << endl;
      }
    }
  }
}


void convertToNewlines(char* data)
{
  char* ptr;
  for (ptr=data; *ptr != '\0'; ptr++)
    if (*ptr == '\002')
      *ptr = '\n';
}


void interactProperty(char* propData, char seqCode)
{
  static int callCount;
  static string parseType;
  FILE* parseInFile = NULL;
  char *props;

  if (seqCode==MSG_BLOCK_1_OF_1 || seqCode==MSG_BLOCK_1_OF_N) {
    // get bookmark file position and save it to a file for restarting
    string bookmarkStr = firstToken(propData);
    if (bookmarkStr == "")
      restart("eccejobmonitor",
              "Unexpected end of calculation output (wanted bookmark)");
    else {
      ofstream bookmarkFile(bookmarkFileName.c_str(), (ios::out | ios::trunc));
      if (bookmarkFile) {
        bookmarkFile << bookmarkStr << endl;
        bookmarkFile.close();
      }
    }

    parseType = nextToken();
    if (parseType == "")
      restart("eccejobmonitor",
              "Unexpected end of calculation output (wanted parse type)");

    // get call count
    char* countStr = nextToken();
    if (countStr==NULL || *countStr=='\0')
      restart("eccejobmonitor",
              "Unexpected end of calculation output (wanted count)");

    callCount = atoi(countStr);

    props = nextToken();

    if ((parseInFile = fopen(parseIn.c_str(), "w")) == NULL) {
      string errMsg = "Could not open property parse input file '" + parseIn +
                      "' for write";
      restart("System", errMsg);
    }
  } else {
    props = firstToken(propData);

    if ((parseInFile = fopen(parseIn.c_str(), "a+")) == NULL) {
      string errMsg = "Could not open property parse input file '" + parseIn +
                      "' for append";
      restart("System", errMsg);
    }
  }

  if (props==NULL || *props=='\0')
    restart("eccejobmonitor",
            "Unexpected end of calculation output (wanted property data)");

  convertToNewlines(props);

  int proplen = strlen(props);
  if (fwrite(props, 1, proplen, parseInFile) != proplen) {
    string errMsg = "Could not write data to property parse input file '" +
                    parseIn + "'";
    restart("System", errMsg);
  }

  fclose(parseInFile);
  parseInFile = NULL;

  if (seqCode==MSG_BLOCK_1_OF_1 || seqCode==MSG_BLOCK_N_OF_N) {
#if (!defined(INSTALL) && defined(DEBUG))
    ETimer time;
    time.start();
#endif

#ifdef BENCHMARK
    string message = "storing property (count): " + parseType + " (";
    message += countStr;
    message +=  ")";
    logInfo("Benchmark", message); 
#endif
#ifndef DONT_STORE
    // store data in database
    calcStoreData(parseType, callCount, NULL, false);
#endif

#if (!defined(INSTALL) && defined(DEBUG))
    time.stop();
    cout << "Storing data took " << time.elapsedTime() << endl;
#endif
  }
}


void interactUp(char* upData)
{
  string msg = firstToken(upData);

  msg += ": job monitor running on ";
  msg += nextToken();

  logMessage("eccejobmonitor", msg);

  // A queued job sends no progress, so the heartbeat is what tells the
  // Organizer the queue state is fresh ("updated n s ago", #606).  Older
  // Organizers read any value but "lost <epoch>" as in contact.
  static time_t sent = 0;
  const time_t now = time(0);
  if (calculation && !gContactLost && now - sent >= 50 &&
      calculation->getState() == ResourceDescriptor::STATE_SUBMITTED) {
    sent = now;
    const string value = "ok " + std::to_string((long)now);
    sendNotify("ecce_url_progress", "contact", value);
    contactStore(value);
  }
}


// scripts/gensub's determineStatus()/defineStatusSymbols() write one of
// these numeric exit codes into the job's status file when the code did
// not finish cleanly; the codes themselves never reach any UI (#--- job
// shows only "incomplete" in Organizer, see gensub's own comments beside
// each "setting status to failed" echo).  This is the same set of codes
// gensub defines -- keep it in sync with scripts/gensub's
// defineStatusSymbols() if a new one is ever added there.
string unsuccessfulReason(int status)
{
  switch (status) {
    case 211:
      return "the job's input/output file did not exist when it "
             "finished (exit code 211)";
    case 221:
      return "an unexpected core file was found after the job exited "
             "(exit code 221)";
    case 231:
      return "the code exited 0 but printed no normal-termination line "
             "(exit code 231)";
    case 241:
      return "the code stopped without converging (exit code 241)";
    default: {
      char buf[64];
      snprintf(buf, sizeof(buf), "job exited with status %d", status);
      return string(buf);
    }
  }
}

void interactStatus(char* statusData)
{
  // cleanup temp properties before updating state

  static int lastStatus = -1;
  gStateReported = true;

  int status = 0;
  bool emptyStatus = (statusData==NULL || *statusData=='\0');

  if (emptyStatus)
    status = 302;
  else
    status = atoi(statusData);

  if (!emptyStatus) {
    const string sd = statusData;
    size_t p = sd.find(" state=");
    if (p != string::npos)
      gSchedState = sd.substr(p + 7, sd.find(' ', p + 7) - (p + 7));
    p = sd.find(" end=");
    if (p != string::npos)
      gSchedEndTime = JobLogDates::parse(sd.substr(p + 5));
  }

  if (lastStatus != status) {
    lastStatus = status;
    // update state in database
    if (status == 305) {
      // The scheduler ended the job (out of memory, time limit, ...): the
      // output and properties are kept, as for a run that did not
      // converge; the reason is made in cleanup() (#649).
      gUnsuccessfulStatus = status;
      calcUpdateState(ResourceDescriptor::STATE_UNSUCCESSFUL);
      logMessage("Calculation Incomplete",
                 JobLogDates::schedulerEndReason(gSchedState));
    } else if (status == 0) {
      gUnsuccessfulStatus = 0;
      calcUpdateState(ResourceDescriptor::STATE_COMPLETED);
    } else if (status == 301) {
      gUnsuccessfulStatus = 0;
      calcUpdateState(ResourceDescriptor::STATE_RUNNING);

      // special STTR reaction rate study logic to reset the state of all
      // follow-on tasks to force input file regeneration because it's likely
      // the chemical system has changed (GEOMTRACE property)
      Resource* parent =
                EDSIFactory::getResource(calculation->getURL().getParent());
      if (parent->getApplicationType()==ResourceDescriptor::AT_REACTION_STUDY) {
        calculation->resetReactionTasks();
      }
    } else if (status == 302) {
      gUnsuccessfulStatus = 0;
      // 302 only means the job is gone.  It is a cancel only if the user
      // asked for one (RunMgmt::terminate); read it fresh, the request was
      // made by another process.
      if (calculation && calculation->killRequested()) {
        calcUpdateState(ResourceDescriptor::STATE_KILLED);
      } else {
        gFailReason = emptyStatus
          ? "The job monitor sent an empty status (possible file system "
            "problem on the compute server, disk full?) and the job was "
            "not cancelled by the user"
          : "The job ended with neither an output file nor a status file, "
            "and was not cancelled by the user";
        logErr("eccejobmonitor", gFailReason);
        calcUpdateState(ResourceDescriptor::STATE_SYSTEM_FAILURE);
      }
    } else if (status == 303) {
      gUnsuccessfulStatus = 0;
      calcUpdateState(ResourceDescriptor::STATE_LOADED);
    } else if (status == 304) {
      // The scheduler says the job was cancelled (scancel on the machine):
      // a user's cancel, made outside ECCE (#636).
      gUnsuccessfulStatus = 0;
      recordStatusReason("cancelled on " + cpServerName +
                         " (the queue system reports it cancelled)");
      calcUpdateState(ResourceDescriptor::STATE_KILLED);
    } else {
      gUnsuccessfulStatus = status;
      calcUpdateState(ResourceDescriptor::STATE_UNSUCCESSFUL);

      // Say why, in the calc's run log (already viewable from Organizer
      // via "View Run Log"/CalcMgr::viewRunLog) -- otherwise the only
      // trace of the reason is gensub's own log text buried inside the
      // output file's "-----ECCE Log Information-----" block, which
      // nothing else reads. This is only the coarse, code-decoded reason;
      // cleanup() below refines it once the output file itself is back,
      // and that's also what ends up in the calc's runStatusReason
      // metadata / JMS notification that Organizer and Launcher read.
      logMessage("Calculation Incomplete", unsuccessfulReason(status));
    }
  }

}

// ------------------------------------------------------------------------- //
// Process an error message by echoing it to the log file,
// and terminating if dieViaProgram is true or the message is fatal.
// ------------------------------------------------------------------------- //
void interactError(char* errorData)
{
  bool dieViaMsg = false;
  string message;

  // decode severity of error
  string severity = firstToken(errorData);

  if (severity == MSG_TYPE_E_FATAL) {
    message = "ECCE JOB MONITOR ERROR: ";
    dieViaMsg = true;
  }
  else if (severity == MSG_TYPE_E_WARN)
    message = "ECCE JOB MONITOR WARNING: ";
  else if (severity == MSG_TYPE_E_INFO)
    message = "ECCE JOB MONITOR INFO: ";
  else
    message = "ECCE JOB MONITOR MESSAGE: ";

  message += nextToken();
  logMessage("eccejobmonitor", message);

  // Before any job state the monitor has not got as far as looking at the
  // job (a missing or half-copied script or configuration): park, never
  // fail the job.
  if (dieViaMsg && !gStateReported)
    restartSystem("eccejobmonitor", "FATAL error message from eccejobmonitor "
                  "before the job was examined");
  if (dieViaMsg)
    restart("eccejobmonitor", "FATAL error message from eccejobmonitor");
}


// ------------------------------------------------------------------------- //
// Process a file message by echoing it to the log file, adding the file to
// the list of those to be copied, and storing the data in the database.
// ------------------------------------------------------------------------- //
void interactFile(char* fileData)
{
  filesFlag = false;

  string fileType = firstToken(fileData);

  string parseType = nextToken();

  // get call count
  char* countStr = nextToken();
  if (countStr==NULL || *countStr=='\0')
    restart("eccejobmonitor", "Unexpected end of message (wanted count)");

  int callCount = atoi(countStr);

  // get file name
  string fileName = nextToken();
  if (fileName == "")
    restart("eccejobmonitor", "Unexpected end of message (wanted file name)");

  bool dbStoreContents;
  bool dbStoreName;

  // decode type of message
  if (fileType == MSG_TYPE_F_PROP) {
    dbStoreContents = true;
    //  "ecce-parse-*" files exist only to be parsed (a GROMACS trajectory
    //  becomes GEOMTRACE); keeping them as outputs would store the data twice.
    dbStoreName = fileName.compare(0, 11, "ecce-parse-") != 0;

    // for imports, create a symbolic link immediately to the specified file
    if (cpImportDir != "") {
      // if the operation fails (file doesn't exist) just ignore
      string cmd = "ln -s " + RCommand::quotePath(cpImportDir + fileName) + " " + RCommand::quotePath(fileName);
      (void)localconn->exec(cmd);
    }
  }
  else if (fileType == MSG_TYPE_F_JOF || fileType == MSG_TYPE_F_SCHED) {
    dbStoreContents = false;
    dbStoreName = true;
    if (fileType == MSG_TYPE_F_SCHED)
      gSchedulerErrFile = fileName;
  }
  else {
    dbStoreContents = false;
    dbStoreName = false;
  }
  string message = "File marked to copy from compute host: " + fileName;
  logMessage("eccejobmonitor", message); 

  // store info for later retrieval
  fileAdd(callCount, dbStoreContents, dbStoreName, fileName, parseType);
}

void interactNode(char* nodeData)
{
  string node = firstToken(nodeData);
  if (node == "")
    restart("eccejobmonitor", "Unexpected end of message (wanted node)");
  else
    nodeForRestart = node;
}


// ------------------------------------------------------------------------- //
// Tokenizing methods.  Tokens are delimited by NULL characters.
// ------------------------------------------------------------------------- //
static char* tokenStr = NULL;

char* firstToken(char* data)
{
  tokenStr = data;

  return tokenStr;
}

char* nextToken(void)
{
  tokenStr += strlen(tokenStr) + 1;

  return tokenStr;
}


// ------------------------------------------------------------------------- //
// ------------------------------------------------------------------------- //
void sendNotify(const char* topic, const string& key1, const string& name, 
                const string& key2, const string& value)
{
  string message;
  JMSPublisher pub("eccejobstore");
  JMSMessage* msg = pub.newMessage();

  // add message contents as properties:
  msg->addProperty("url", calculation->getURL().toString());
  if (!key1.empty())
    msg->addProperty(key1, name);
  if (!key2.empty())
    msg->addProperty(key2, value);

  if (!pub.publish(topic, *msg)) {
    message = "Publish failed.";
    logMessage("Messaging",  message);
  }
  delete msg;

  if (ecceJobLogAll) {
    message = "Sent ";
    message += topic;
    message += " message for ";
    message += name;
    if (!value.empty()) {
      message += ", value ";
      message += value;
    }
    logInfo("Messaging",  message);
  }
}


// ------------------------------------------------------------------------- //
// A progress snapshot from eccejobmonitor (#284): relay it on the broker
// as ecce_url_progress (the owner's own topic, FILTER BY: NONE) and keep
// it for storing.  Not a property: nothing of it goes to Props/.
// ------------------------------------------------------------------------- //
void interactProgress(char* text)
{
  convertToNewlines(text);
  string snap = text;
  // When this job store relayed it, for "updated n s ago" after a restart
  // of the Organizer (the monitor's own clock may be another machine's).
  const string::size_type eol = snap.find('\n');
  if (snap.compare(0, 2, "# ") == 0 && eol != string::npos) {
    char at[32];
    snprintf(at, sizeof at, " at=%ld", (long)time(0));
    snap.insert(eol, at);
  }
  gProgress = snap;
  if (calculation)
    sendNotify("ecce_url_progress", "progress", gProgress);
  progressStore(false);
}


void progressStore(bool force)
{
  if (!calculation || gProgress.empty() || gProgress == gProgressStored)
    return;
  const time_t now = time(0);
  if (!force && now - gProgressStoredAt < PROGRESS_STORE_SECONDS)
    return;
  vector<MetaDataResult> results(1);
  results[0].name = VDoc::getEcceNamespace() + ":progress";
  results[0].value = gProgress;
  try {
    calculation->addProps(results);
    gProgressStored = gProgress;
    gProgressStoredAt = now;
  } catch (DavException& ex) {
    logErr("DAV Server", string("storing the progress snapshot: ") +
           ex.what());
  }
}


void progressClear(void)
{
  if (!calculation)
    return;
  // A new run is in contact until its own monitor falls silent.
  if (!calculation->getProp(VDoc::getEcceNamespace() + ":contact").empty()) {
    contactStore("");
    sendNotify("ecce_url_progress", "contact", "ok");
  }
  const string key = VDoc::getEcceNamespace() + ":progress";
  if (calculation->getProp(key).empty())
    return;
  vector<MetaDataResult> results(1);
  results[0].name = key;
  results[0].value = "";
  try {
    calculation->addProps(results);
  } catch (DavException& ex) {
    logErr("DAV Server", string("clearing the progress snapshot: ") +
           ex.what());
  }
  sendNotify("ecce_url_progress", "progress", "");
}


// ------------------------------------------------------------------------- //
// Out of contact (#284).  The monitor sends a heartbeat (jmUP) every sixth
// job check, one check per timePauseReadLine (10 s, 2 s for a local job),
// besides its output.  Three heartbeats missed (at least a minute, for
// slow property scripts) and the Organizer shows the job "running?"; the
// run state itself is not touched.  ECCE_JOB_CONTACT_SECONDS and
// ECCE_JOB_RECONNECT_SECONDS override both times (tests, slow schedulers).
// ------------------------------------------------------------------------- //
void contactInit(void)
{
  if (gLastHeard != 0)
    return;
  int pause = 10;
  ifstream conf((localDirectory() + "/eccejobmonitor.conf").c_str());
  string key;
  while (conf >> key) {
    string value;
    getline(conf, value);
    if (key == "timePauseReadLine" && atoi(value.c_str()) > 0)
      pause = atoi(value.c_str());
  }
  gContactSeconds = std::max(60, 3 * 6 * pause);
  int reconnect = std::max(180, gContactSeconds + 120);
  const char* env = getenv("ECCE_JOB_CONTACT_SECONDS");
  if (env && atoi(env) > 0)
    gContactSeconds = atoi(env);
  env = getenv("ECCE_JOB_RECONNECT_SECONDS");
  if (env && atoi(env) > 0)
    reconnect = atoi(env);
  readTimeoutMilliseconds = (unsigned long)reconnect * 1000UL;

  gLastHeard = time(0);
  clock_gettime(CLOCK_MONOTONIC, &gLastHeardMono);
  // A store restarted after silence carries on from the last one: the
  // Organizer already shows the job out of contact since then.
  if (restartFlag && calculation) {
    const string stored =
        calculation->getProp(VDoc::getEcceNamespace() + ":contact");
    if (stored.compare(0, 5, "lost ") == 0) {
      gContactLost = true;
      gLastHeard = (time_t)atol(stored.c_str() + 5);
    }
    if (gProgress.empty()) {
      gProgress = calculation->getProp(VDoc::getEcceNamespace() +
                                       ":progress");
      gProgressStored = gProgress;
    }
  }
  char buf[128];
  snprintf(buf, sizeof buf, "out of contact after %d s of silence, "
           "reconnect after %d s", gContactSeconds, reconnect);
  logMessage("eccejobstore", buf);
}


// The monitor's state as the Organizer sees it: "lost <epoch last heard>"
// or "" (in contact).  Stored so that a restarted Organizer shows it too.
void contactStore(const string& value)
{
  if (!calculation)
    return;
  const string key = VDoc::getEcceNamespace() + ":contact";
  if (value.empty() && calculation->getProp(key).empty())
    return;
  vector<MetaDataResult> results(1);
  results[0].name = key;
  results[0].value = value;
  try {
    calculation->addProps(results);
  } catch (DavException& ex) {
    logErr("DAV Server", string("storing the contact state: ") + ex.what());
  }
}


void contactLost(void)
{
  gContactLost = true;
  char lost[64];
  snprintf(lost, sizeof lost, "lost %ld", (long)gLastHeard);
  char msg[160];
  snprintf(msg, sizeof msg, "No message from the job monitor for %ld s; "
           "the job may still be running", (long)(time(0) - gLastHeard));
  logPrint(ActivityLog::WARNING, "eccejobmonitor", msg);
  if (calculation)
    sendNotify("ecce_url_progress", "contact", lost, "progress", gProgress);
  contactStore(lost);
}


void contactHeard(void)
{
  if (gLastHeard == 0)
    return;
  gLastHeard = time(0);
  clock_gettime(CLOCK_MONOTONIC, &gLastHeardMono);
  if (!gContactLost)
    return;
  gContactLost = false;
  logMessage("eccejobmonitor", "Job monitor in contact again");
  if (calculation)
    sendNotify("ecce_url_progress", "contact", "ok", "progress", gProgress);
  contactStore("");
}


// ------------------------------------------------------------------------- //
// Return a string describing the current errno.
// The returned value points to a static buffer that is overwritten
// by each call to this function.
// ------------------------------------------------------------------------- //
const char* strErrno(void)
{
  static char buf[256];
  (void)sprintf(buf, "errno=%d (%s)", errno, strerror(errno));
  return buf;
}


string localDirectory(void)
{
  string ret = cpConfigFileName;

  string::size_type slash = ret.rfind('/');
  if (slash != string::npos)
    ret.resize(slash); 

  return ret; 
}

