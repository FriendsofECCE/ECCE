/**
 * @file
 *
 *
 */

//#define JSPERLTIMER
//#define JSDBTIMER
#define DEBUG

#include <unistd.h>               // getuid()
#include <limits.h>               // MAX_INPUT

#include <iomanip>
  using std::ios;
#include <fstream>
  using std::ofstream;


#include "util/ErrMsg.H"
#include "util/Preferences.H"
#include "util/SFile.H"
#include "util/EcceURL.H"
#include "util/TDateTime.H"
#include "util/TypedFile.H"
#include "util/EcceMap.H"
#include "util/StringTokenizer.H"
#include "util/SDirectory.H"
#include "util/TempStorage.H"

#include "tdat/RefMachine.H"

#include "dsm/IPropCache.H"
#include "dsm/JCode.H"
#include "dsm/CodeFactory.H"
#include "dsm/TaskJob.H"
#include "dsm/ActivityLog.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"

#include "comm/RCommand.H"
#include "comm/Launch.H"
#include "comm/JobParser.H"

#ifdef JSPERLTIMER
extern RWTimer perlTimer;
#endif
#ifdef JSDBTIMER
extern RWTimer dbTimer;
#endif



/**
 * Constructor.
 * Create a TaskJob from the given url and a property parsing object.
 */
JobParser::JobParser(const EcceURL& url) 
{
  p_propcache = EDSIFactory::getIPropCache(url);
  p_task = dynamic_cast<TaskJob*>(EDSIFactory::getResource(url));
}



/**
 * Destructor.
 */
JobParser::~JobParser()
{
  delete p_propcache;
}



/**
 * Changes the url associated with this job parsing activity.
 * Useful if the task changed urls for example.
 */
void JobParser::setURL(const EcceURL& url) 
{
  p_propcache->setURL(url);
}



/**
 * Import a task from an output file.
 */
bool JobParser::importCalculation(const char* parseFile,
                                  TaskJob* task,
                                  const string& importName,
                                  string& message)
{
  bool ret = false;

  string whereami = RCommand::whereami();
  RefMachine* refMachine = RefMachine::abbrevLookup(whereami);
  if (!refMachine) {
    message = whereami + " must be a registered compute server to import.";
    return ret;
  }

  // parse file path into directory and name
  string fullFilePath = parseFile;
  int slash = fullFilePath.find_last_of('/');
  string parseFileName = fullFilePath.substr(slash+1,
                                         fullFilePath.length()-slash-1);
  string importDir = fullFilePath.substr(0, slash+1);

  // Determine the code from the output file
  RCommand localconn("system");
  if (!localconn.isOpen()) {
    message = "Unable to create local command shell for importing.";
    return ret;
  }

  string code;
  string cmd;

  // create a /tmp directory for importing to avoid problems with the
  // parseFile directory not being writable and simultaneous imports
  // taking place in the same directory overwriting files like eccejobmonitor
  SDirectory* dir = TempStorage::getTempJobDirectory(
                                task->getURL().getFilePathTail().c_str());
  if (localconn.cd(dir->path())) {
    cmd = "ln -s " + fullFilePath + " " + parseFileName;
    if (!localconn.exec(cmd)) {
      message = "Cannot create symbolic link to " + fullFilePath +
                " to perform import.";
      return ret;
    }

    vector<string> importCodes = CodeFactory::getImportCodes();
    string pattern;
    for (int idx=0; idx<importCodes.size(); idx++) {
      const JCode *jcode = CodeFactory::lookup(importCodes[idx].c_str());
      pattern = jcode->getParseVerifyPattern();
      if (!pattern.empty()) {
        cmd = "grep \"";
        cmd.append(pattern);
        cmd.append("\" ");
        cmd.append(parseFileName);
        if (localconn.exec(cmd)) {
          code = importCodes[idx];
          break;
        }
      }
    }

    // If we couldn't match to one of the parse files, make sure they
    // aren't trying to import an NWChem output file.  This is left over
    // but harmless hardwired code to try to be helpful to the user.
    // 12/08/04 Not totally harmless now since it only applies to the
    // ICalculation codes but I'll leave it anyway...
    if (code.empty()) {
       cmd = "grep \"Northwest Computational Chemistry Package\" ";
       cmd.append(fullFilePath);
       if (localconn.exec(cmd)) {
            message = "Cannot import NWChem log file--"
                      "must specify ECCE formatted NWChem output file.";
            return ret;
       }
    }

    // If we got here and still don't have a code, give up.
    if (code.empty()) {
       message = "Unrecognized output file format--cannot import.";
       return ret;
    }
  }

  if (!localconn.writable(dir->path())) {
    message = "Directory " + dir->path() + " is not writable--cannot import.";
    return ret;
  }

  // Do the import

  const JCode* jcode = CodeFactory::lookup(code.c_str());
  task->application(jcode);

  string parseSetupScript = jcode->getScript("CalcImport");

  string oput;
  if (!localconn.which(parseSetupScript, oput)) {
    message = "Setup parse script " + parseSetupScript +
              " does not exist or is not executable.";
    return ret;
  }

  cmd = parseSetupScript + " " + parseFileName;
  if (!localconn.execout(cmd, oput)) {
    message = "Failed to execute setup parse script command \"" + cmd + "\"";
    if (oput != "")
      message += ":" + oput + ".";
    else 
      message += ".";

    return ret;
  } else if (oput != "") {
    message = oput + ".";
  }

  try {
    message = task->import(dir->path(), parseFileName);
  } catch (EcceException& ex) {
    message = ex.what();
    return ret;
  }

  struct Launchdata launchdata;
  launchdata.machine = refMachine->refname();
  launchdata.remoteShell = "";
  launchdata.user = "";
  launchdata.nodes = 0;
  launchdata.totalprocs = 0;
  launchdata.maxmemory = 0;
  launchdata.minscratch = 0;
  task->launchdata(launchdata);

  struct Jobdata jobdata;
  jobdata.jobpath = dir->path();
  jobdata.jobid = "";
  task->jobdata(jobdata);

  EcceMap kvargs;
  kvargs["##parse##"] = parseFileName;
  kvargs["##output##"] = parseFileName;

  delete refMachine;

  Launch *launch = new Launch(task, kvargs, false, importDir, importName);
  if (launch->validateLocalDir() &&
      launch->validateRemoteLogin() &&
      launch->generateJobMonitoringFiles() &&
      launch->moveJobMonitoringFiles() &&
      launch->startJobStore(importDir))
    ret = true;
  else {
    message = "Unable to import--";
    message += launch->message();
  }

  delete launch;

  return ret;
}


/**
 * Get the command line args that will be passed to each parse script.
 * The arguments are assumed to be the same for all script invocations
 * for a given calculation so they are retrieved once and stored.
 */
void JobParser::cacheParseArgs(const string& tmpDir)
{

   // Each TaskJob class is responsible for specifying parse script args
   p_parseArgs = p_task->getParseScriptArgs();

   p_parseIn = "parseIn";
   p_parseOut = "parseOut";
   p_absoluteParseIn = tmpDir + "/" + p_parseIn;
   p_absoluteParseOut = tmpDir + "/" + p_parseOut;
}



void JobParser::storeProperty(RCommand* localconn, const string& codeName,
                              const string& propKey,
                              propertyMapPairToValue& properties,
                              unsigned long callCount, const char* strmBuf,
                              string& log , bool fromFile)
{
  string realPropKey(propKey);
  if (propKey.find(']') != string::npos)
    realPropKey  = realPropKey.substr(0, propKey.find(']'));

  for (int i = 0; i < realPropKey.length(); i++) {
    realPropKey[i] = tolower(realPropKey[i]);
  }

  const JCode* jcode = CodeFactory::lookup(codeName.c_str());
  string command = jcode->parseScript(realPropKey);
  if (command != "") {
    // parseArgs has leading space character
    command += p_parseArgs;
    command += " >" + p_parseOut + " <";
    if (!fromFile) {
      if (strmBuf != NULL) {
        ofstream input(p_absoluteParseIn.c_str(), (ios::out | ios::trunc));
        input << strmBuf;
        input.close();
      }
      command += p_parseIn;
    } else {
      command += strmBuf;
    }

#if (!defined(INSTALL) && defined(DEBUG)) 
    string debugCmd = "cat ";

    if (fromFile)
      debugCmd += strmBuf;
    else
      debugCmd += p_parseIn;

    debugCmd += " >> " + p_parseIn + "Debug";
    (void)localconn->exec(debugCmd);
#endif

#ifdef JSPERLTIMER
perlTimer.start();
#endif

    // the real parsing is done here
    // allow user override of timeout because perl parse scripts can
    // take a long time
    int timeout = 300;
    char *parseto;
    if ((parseto = getenv("ECCE_JOB_PARSE_TIMEOUT")) != NULL) {
      int ipto = atoi(parseto);
      if (ipto != 0)
        timeout = 60*ipto;
    }

    // bump up the timeout because perl parse scripts can take a long time
    if (localconn->exec(command, "", timeout)) {
#ifdef JSPERLTIMER
perlTimer.stop();
#endif
#if (!defined(INSTALL) && defined(DEBUG)) 
      debugCmd = "cat " + p_parseOut + " >> " + p_parseOut + "Debug";
      (void)localconn->exec(debugCmd);
#endif
#ifdef JSDBTIMER
dbTimer.start();
#endif

      ifstream output(p_absoluteParseOut.c_str());
      if (output) {
        if (p_propcache)  
          p_propcache->put(output, properties, log); 
        output.close();
      } else  {
        // This should never happen unless we run out of disk space or
        // something like that.
        TDateTime date;
        string message = "Unable to open parse output\n";
        message += localconn->commError();
    
        log += ActivityLog::entry(ActivityLog::ERROR, "Parser", date,
                                  message.c_str());
      }
    } else {
      string msg = "Couldn't parse property: ";
      msg += localconn->commError();
      msg += "\nCommand [" + command;
      msg += "] Codename [" + codeName;
      msg += "] PropKey [" + realPropKey;
      msg += "]";
      TDateTime date;
      log += ActivityLog::entry(ActivityLog::ERROR, "Parser", date,msg.c_str());
    }
  } else {
    string msg = "Couldn't map property parse script: Codename [" + codeName;
    msg += "] PropKey [" + realPropKey;
    msg += "]";
    TDateTime date;
    log += ActivityLog::entry(ActivityLog::ERROR, "Parser", date, msg.c_str());
  }

#ifdef JSDBTIMER
dbTimer.stop();
#endif
}
