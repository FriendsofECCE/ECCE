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
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <iostream>
  using std::cout;
  using std::endl;
  using std::ios;
#include <fstream>
  using std::ofstream;
 
#include <X11/Intrinsic.h>

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

#include "comm/RCommand.H"
#include "comm/JobParser.H"

#include "comm/expect.h"

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
void interactStatusRequest(void);
void interactUp(char* upData);
void interactError(char* errorData);
void interactFile(char* fileData);
void interactPort(char* portData);
void interactNode(char* nodeData);
void interactGetOutput(void);
void interactGetFiles(void);
void xtGetJobMonitorInput(XtPointer client_data, int* fid, XtInputId* id);
void restart(const char* name, const string& msg);
void restartSystem(const char* name, const string& msg);
void setTimeout(const unsigned long& seconds);
void xtSignalHandler(XtPointer client_data, XtSignalId* id);
extern "C" void signalHandler(const int signalValue);
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
void socketClose(void);
bool socketConnect(int port);
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

// Messaging state
static JMSSubscriber *vsubscriber = 0;
static XtInputId vXtInputId = 0;

// communication protocol constants
static const char* MSG_TYPE_E_INFO  = "info";
static const char* MSG_TYPE_E_WARN  = "warn";
static const char* MSG_TYPE_E_FATAL = "fatal";
static const char* MSG_TYPE_F_PROP  = "prop";
static const char* MSG_TYPE_F_JOF   = "output";
static const char  MSG_BLOCK_1_OF_N = '1';
static const char  MSG_BLOCK_M_OF_N = '2';
static const char  MSG_BLOCK_N_OF_N = '3';
static const char  MSG_BLOCK_1_OF_1 = '0';
static const int   MAX_BLOCK_LENGTH = 2048;

// timeouts and retries
static const int   MAX_READ_TRIES = 100;
static const unsigned long READ_TIMEOUT_MILLISECONDS = 180000; // 3 minutes
static const unsigned long PROCESS_TIMEOUT_MILLISECONDS = 1800000; // 30 minutes

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
static bool socketComms = false;
static bool socketFail = false;
static string parseIn;
static LogModeJobStore logModeJobStore;
static XtAppContext appContext;
static XtSignalId signalId;
static int currentSignal = 0;
static ResourceDescriptor::RUNSTATE endState=ResourceDescriptor::STATE_ILLEGAL;
static string lastRestartFileName = "";
static int lastRestartCallCount = 0;

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

  // init X
  XtToolkitInitialize();
  appContext = XtCreateApplicationContext();

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

  if (restartFlag)
    // before restart we set state to monitor error since we aren't
    // guaranteed jobstore will be coming back.  When we do realize
    // that we're back again and that it is a restart we set the
    // state back to submitted.
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
  ifstream ifs(outputFile.c_str());
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
  if (state < ResourceDescriptor::STATE_COMPLETED) {
    if (state == ResourceDescriptor::STATE_RUNNING) {
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
// Clean up everything in preparation for termination.
// ------------------------------------------------------------------------- //
void cleanup(int exitStatus)
{

  // Set the final run state (if set) just before exitting to guarantee
  // output files have been uplodaed to data server
  if (endState != ResourceDescriptor::STATE_ILLEGAL) {
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
  }

  if (exitStatus == 0) {
    if (socketComms)
      socketClose();

    if (remoteconn->isOpen()) {
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

  // don't add the "close document" tags for a restart
  if (exitStatus != 2)
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
//     Remote communications shell (ssh, rsh, telnet) to be used.
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
// Log the given message, clean up, and exit--3 variations:
// fail => immediate exit with email for things to investigate
// restart => invoke eccejobstore again with email
// restartSystem => invoke ejs again with no email for "system" failure
// ------------------------------------------------------------------------- //
void fail(const char* name, const string& msg)
{
  logPrint(ActivityLog::FATAL, name, msg);
  logNumErrors++;

  // initialize messaging if needed
  initMessaging();

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

  calcUpdateState(ResourceDescriptor::STATE_FAILED);
  cleanup(2);
  exit(2);
}

void restartSystem(const char* name, const string& msg)
{
  logPrint(ActivityLog::FATAL, name, msg);
  logNumErrors++;

  logInfo("eccejobstore", "returning to eccejobmaster for restart");

  // initialize messaging if needed
  initMessaging();

  calcUpdateState(ResourceDescriptor::STATE_SYSTEM_FAILURE);
  cleanup(4);
  exit(4);
}

void xtSignalHandler(XtPointer client_data, XtSignalId* id)
{
  int* signalValue = (int*)(client_data);

  switch (*signalValue)
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
    case SIGTERM  :
      restart("Signal", "SIGTERM: Software termination signal from kill");break;

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

extern "C" void signalHandler(const int signalValue)
{
  currentSignal = signalValue;

  // re-establish the signal handler--only applicable to warning signals
  signal(signalValue, signalHandler);

  // put signal into Xt event processing queue
  XtNoticeSignal(signalId);
}

void xtTimeoutHandler(XtPointer client_data, XtIntervalId*)
{
  unsigned long milliseconds = (unsigned long)client_data;

  if (milliseconds == READ_TIMEOUT_MILLISECONDS)
    restartSystem("eccejobstore",
                  "Read timeout--job monitor failed to send heartbeat message");
  else
    restart("eccejobstore",
            "Processing timeout--eccejobstore hung while processing message");
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

      // get the attention of the jobmonitor script by issuing a Ctrl-C
      // interrupt.  Otherwise, it tends to just keep on trying to talk
      // to eccejobstore
      if (remoteconn != (RCommand*)0)
        (void)remoteconn->exec("\003"); 

      calcUpdateState(ResourceDescriptor::STATE_SUBMITTED);
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
  signalId = XtAppAddSignal(appContext, xtSignalHandler, &currentSignal);

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
    if (remoteconn->isOpen()) {
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
        string cmd = "sed -i '1,1d' " + fileName;
        localconn->exec(cmd);
      } else
        ifs.close();
#endif

      calcPutOutFileName(fileName, "");
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

  // Email failures to ecce-test if ECCE_JOB_ALLFAILMAIL tells us to
  if (exitStatus != 0) {
    string url = calculation->getURL().toString();
    string failmail = getenv("ECCE_JOB_ALLFAILMAIL")?
                      getenv("ECCE_JOB_ALLFAILMAIL"): "";
    if (failmail=="true" ||
        (failmail!="" && url.find(failmail)!=string::npos)) {
      string mail =
             "Mail -s 'Eccejobstore Failure' ecce-test@emsl.pnl.gov << EOM\n"
             "Eccejobstore ";
      mail += Ecce::ecceVersion();
      mail += " has terminated due to a monitoring error.\n"
              "Calculation URL: " + url + "\nEOM";
      (void)system(mail.c_str());
    }
  }
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

    // Force a reload of the port from file (because port is cached so we
    // don't have to look it up every time)
    DatagramUtil::loadServerPort();
    vsubscriber = new JMSSubscriber("EcceJobStore");

    bool status = true;
    status &= vsubscriber->subscribe("ecce_url_renamed",calcRenamedMCB);
    status &= vsubscriber->subscribe("ecce_url_removed",calcRemovedMCB);
    status &= vsubscriber->subscribe("ecce_quit",ecceExitMCB);
    status &= vsubscriber->subscribe("ecce_ejs_kill",ejsKillMCB);
    // Now tell JMSDispatcher that we are ready to start the subscriber
    // connection for these subscriptions 
    // (must happen AFTER all subscribe calls):
    status &= vsubscriber->startSubscriber();

    if (!status)
      logErr("Messaging", "Unable to subscribe to one or more message types");

    int msgsocket = vsubscriber->getSocketID();

    if (msgsocket == -1)
      // Should this result in calling fail()?
      logErr("Messaging", "Subscriber socket is -1");

    vXtInputId = XtAppAddInput(appContext, msgsocket, 
                               (XtPointer)XtInputReadMask,
                               (XtInputCallbackProc)msgDispatch,
                               NULL);
  }
}

void exitMessaging(void)
{
  if (vsubscriber != (JMSSubscriber*)0) {
    // Have to remove from event loop
    if (vXtInputId != (XtInputId)0) {
      XtRemoveInput(vXtInputId);
    }
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
  localconn = new RCommand("system");
  if (localconn->isOpen())
    logMessage("System", "Started local csh RCommand connection");
  else
    restart("System", "Unable to establish local csh RCommand connection");

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

  // Determine whether to use socket or stdio based comms
  // I would do this in envRead but cpServerName has not been set
  // Globus must use stdio comms because ejs sends status values back to ejm
  char* value;
  if (cpRemoteShell.find("Globus")==string::npos &&
      (value = getenv("ECCE_JOB_COMMS"))!=NULL) {
    string comms = value;
    if (comms == "socketlocal")
      socketComms = RCommand::isSameDomain(cpServerName);
    else
      socketComms = (comms == "socket");
  }

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
    string cmd = "perl eccejobmonitor -configFile eccejobmonitor.conf";

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
    // than looking for a "+go+" prompt because eccejobmonitor can run in the
    // background when socket comms fail so I think it would be easy for the
    // remote connection to somehow get "bumped" during normal ejm execution
    // with the prompt being sent but ejm still executing just fine
    cmd += "; echo eccejobmonitor_went_bye_bye";

    // this is a hack for an IRIX 6.5/csh bug where signals aren't trapped
    // if using csh.  I invoke eccejobmonitor under a tcsh shell instead.
    string unameOut;
    if (remoteconn->execout("uname -a", unameOut))
      if (unameOut.find("IRIX")!=string::npos &&
          unameOut.find("6.5")!=string::npos) {
        cmd.insert(0, "tcsh -fc '");
        cmd.append("'");
        logMessage("IRIX 6.5 workaround",
                   "Using tcsh on IRIX 6.5 machine to workaround signal bug");
      }

    // cd to the calculation directory so all paths can be relative
    if (!remoteconn->cd(cpRemoteDir)) {
      message = "Unable to cd on the compute server to calculation directory "+
                cpRemoteDir + " " + remoteconn->commError();
      restart("System", message);
    }

    if (socketComms) {
      cmd += "&";
      if (!remoteconn->expwrite(cmd))
        restart("System", remoteconn->commError());

      if (remoteconn->expect1("\r\n") != 1)
        restart("System", "Did not receive eccejobmonitor command echo");

      if (remoteconn->expect1("\r\n+go+") != 1)
        restart("System", "Did not receive eccejobmonitor background job id");

      message =  "Started job monitor (socket comms) with command: " + cmd;
      logMessage("eccejobmonitor", message); 

      // set file descriptor for reading port from stdio
      fdesc = remoteconn->expfid();

      // get the socket port using the main read/process loop
      // which returns after port is sent by eccejobmonitor
      interactGetOutput();
    }

    if (!socketComms || socketFail) {
      // infinite timeout
      remoteconn->exptimeout(-1);

      if (!socketComms) {
        if (!remoteconn->expwrite(cmd))
          restart("System", remoteconn->commError());

        if (remoteconn->expect1("\r\n") != 1)
          restart("System", "Did not receive eccejobmonitor command echo");
        else {
          message = "Started job monitor (stdio comms) with command: ";
          message += remoteconn->expout();
          logMessage("Job Monitor", message);
        }
      }

      // set file descriptor for stdio monitoring
      fdesc = remoteconn->expfid();
    }
  } else
    restartSystem("System", remoteconn->commError());
}


void setTimeout(const unsigned long& milliseconds)
{
  static XtIntervalId lastTimeOut = 0;

  if (lastTimeOut != 0)
    XtRemoveTimeOut(lastTimeOut);

  if (milliseconds != 0)
    lastTimeOut = XtAppAddTimeOut(appContext, milliseconds,
                                  xtTimeoutHandler, (XtPointer)milliseconds);
}

// ------------------------------------------------------------------------- //
// Get the calculation output from eccejobmonitor.
// Return if successful, exit otherwise.
// ------------------------------------------------------------------------- //
// Declare static data buffer for performance
static char databuf[MAX_BLOCK_LENGTH];

enum ReadState {idState, countState, dataState};

void xtGetJobMonitorInput(XtPointer client_data, int* fid, XtInputId* id)
{
  static enum ReadState readState = idState;
  static int needRead = 1;
  static int readTries = 0;
  static char* dataptr = databuf;
  static string nonMonStr = "";
  static char seqCode;
  static bool ignoreFlag = false;

  int nbytes = read(*fid, dataptr, needRead);
  
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
      } else if (*databuf == '~') {
        // just ignore all data up to \001 completely when "~" found
        // this is data coming from ejs itself (Globus status response)
        // so we don't want to flag it as if there was a problem
        ignoreFlag = true;
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
      else if (strcmp(databuf, "jmUP") == 0)
        interactUp(&databuf[5]);
      else if (strcmp(databuf, "jmSTATUS") == 0)
        interactStatus(&databuf[9]);
      else if (strcmp(databuf, "jmSTATREQ") == 0)
        interactStatusRequest();
      else if (strcmp(databuf, "jmFILE") == 0)
        interactFile(&databuf[7]);
      else if (strcmp(databuf, "jmEOFS") == 0)
        filesFlag = true;
      else if (strcmp(databuf, "jmERROR") == 0)
        interactError(&databuf[8]);
      else if (strcmp(databuf, "jmPORT") == 0) {
        interactPort(&databuf[7]);
        // exit message processing after socket connection is established
        doneFlag = true;
      } else if (strcmp(databuf, "jmNODE") == 0) {
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

  setTimeout(READ_TIMEOUT_MILLISECONDS);
}


void interactGetOutput(void)
{
  XtInputId jobMonitorId = XtAppAddInput(appContext, fdesc,
                                         (XtPointer)XtInputReadMask,
                                         xtGetJobMonitorInput, (XtPointer)0);
  doneFlag = false;
  setTimeout(READ_TIMEOUT_MILLISECONDS);

#ifdef BENCHMARK
  ETimer time;
  time.start();
#endif

  XtInputMask mask = XtIMAlternateInput|XtIMTimer|XtIMSignal;

  while (!doneFlag)
    XtAppProcessEvent(appContext, mask);

#ifdef BENCHMARK
  time.stop();
  message =  "Parsing output file took ";
  message += time.elapsedTime();
  logInfo("Benchmark", message);
#endif

  // clear timeout
  setTimeout(0);

  XtRemoveInput(jobMonitorId);

  // check status of remote shell and prep it for further usage
  // transferring output files, etc.
  (void)remoteconn->expect1("\r\n+go+$");
  if (remoteconn->isOpen())
    logMessage("Benchmark", "eccejobmonitor done--"
               "remote shell connection open");
  else
    logMessage("Benchmark", "eccejobmonitor done--"
               "remote shell connection is not available");
}


// ------------------------------------------------------------------------- //
// Close the current socket.
// ------------------------------------------------------------------------- //
void socketClose(void)
{
  if (close(fdesc) < 0) {
    string errMsg = "Cannot close socket: ";
    errMsg += strErrno();
    logErr("socket", errMsg);
  }
}


// ------------------------------------------------------------------------- //
// Connect to the server on the given port.
// ------------------------------------------------------------------------- //
bool socketConnect(int port)
{
  bool ret = false;

  if ((fdesc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    string errMsg = "Cannot create socket: ";
    errMsg += strErrno();
    logErr("socket", errMsg);

    ret = true;
  }

  string serverAddr = "";
  struct hostent* h = gethostbyname((char*)cpServerName.c_str());
  if ((h != (struct hostent*)0) && (h->h_addr_list[0] != (char*)0)) {
    // extract struct in_addr from struct hostent
    struct in_addr in;
    (void)memcpy((void*)&in, (void*)h->h_addr_list[0], sizeof(in));
 
    // map struct in_addr to d.d.d.d
    serverAddr = inet_ntoa(in);
  }
 
  struct sockaddr_in address;
  (void)memset((void*)&address, 0, sizeof(address));
  address.sin_family      = AF_INET;
  address.sin_addr.s_addr = inet_addr(serverAddr.c_str());
  address.sin_port        = htons(port);
 
  if (connect(fdesc, (struct sockaddr*)&address, sizeof(address)) < 0) {
    string errMsg = "Cannot connect to server " + serverAddr + " on port ";

    char buf[8];
    (void)sprintf(buf, "%d", port);
    errMsg += buf;

    errMsg += ": ";
    errMsg += strErrno();
    logErr("socket", errMsg);

    ret = true;
  }

  return ret;
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
    if (logIsOpen) {
      if (!calculation->appendJobLog(logEntry.c_str())) {
        // don't set logIsOpen to false--make it try to append to the log
        // each time because on calc renames it can fail intermittently
        // logIsOpen = false;
        cerr << programName << ": " << logEntry << endl;
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
}


void interactStatus(char* statusData)
{
  // cleanup temp properties before updating state

  static int lastStatus = -1;

  int status = 0;

  if (statusData==NULL || *statusData=='\0') {
    status = 302;
    logErr("eccejobmonitor",
           "Status message contained no value--setting calculation to killed "
           "(possible file system problem on compute server, disk full?)");
  } else
    status = atoi(statusData);

  if (lastStatus != status) {
    lastStatus = status;
    // update state in database
    if (status == 0)
      calcUpdateState(ResourceDescriptor::STATE_COMPLETED);
    else if (status == 301) {
      calcUpdateState(ResourceDescriptor::STATE_RUNNING);

      // special STTR reaction rate study logic to reset the state of all
      // follow-on tasks to force input file regeneration because it's likely
      // the chemical system has changed (GEOMTRACE property)
      Resource* parent =
                EDSIFactory::getResource(calculation->getURL().getParent());
      if (parent->getApplicationType()==ResourceDescriptor::AT_REACTION_STUDY) {
        calculation->resetReactionTasks();
      }
    } else if (status == 302)
      calcUpdateState(ResourceDescriptor::STATE_KILLED);
    else if (status == 303)
      calcUpdateState(ResourceDescriptor::STATE_LOADED);
    else 
      calcUpdateState(ResourceDescriptor::STATE_UNSUCCESSFUL);
  }

}

void interactStatusRequest(void)
{
  string cmd = "globusrun -status " + cpJobId;
  string statout;
  if (localconn->execout(cmd, statout)) {
    // prefix the state with "~" so ejs knows to ignore the message
    // I wanted to use a control character but those were recognized
    // as two separate characters such as "^" and "A" for \001
    // I don't think this will cause a problem however because logging
    // non ejm message data isn't critical to proper monitoring
    if (statout.find("ACTIVE") != string::npos)
      remoteconn->expwrite("~RUNNING");
    else if (statout.find("PENDING")!=string::npos ||
             statout.find("SUSPENDED")!=string::npos)
      remoteconn->expwrite("~PENDING");
    else if (statout.find("DONE") != string::npos)
      remoteconn->expwrite("~DONE");

    logMessage("eccejobmonitor", cmd);
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
    dbStoreName = true;

    // for imports, create a symbolic link immediately to the specified file
    if (cpImportDir != "") {
      // if the operation fails (file doesn't exist) just ignore
      string cmd = "ln -s " + cpImportDir + fileName + " " + fileName;
      (void)localconn->exec(cmd);
    }
  }
  else if (fileType == MSG_TYPE_F_JOF) {
    dbStoreContents = false;
    dbStoreName = true;
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

void interactPort(char* portData)
{
  string port = firstToken(portData);
  if (port == "")
    restart("eccejobmonitor", "Unexpected end of message (wanted port number)");
  else if (port == "FAILED") {
    socketFail = true;
    logMessage("eccejobmonitor",
           "Unable to create socket from compute server back to local machine");
  } else {
    int sport = atoi(port.c_str());

    // establish socket connection
    socketFail = socketConnect(sport);

    string message;
    if (socketFail) {
      message =  "Unable to connect to eccejobmonitor port " + port;
      logMessage("eccejobmonitor", message); 
    } else {
      message =  "Successfully connected to eccejobmonitor port " + port;
      logMessage("eccejobmonitor", message); 

      // close remote shell since all further comms is over socket
      delete remoteconn;
      remoteconn = (RCommand*)0;
    }
  }
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

