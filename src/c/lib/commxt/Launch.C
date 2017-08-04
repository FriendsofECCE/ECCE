//#define DEBUG
///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Launch.C
//
//
// DESIGN:
//   This class has an "iterator" style interface for stepping through
//   the validation, setup, and execution commands of the launch process.
//
///////////////////////////////////////////////////////////////////////////////

#include <string>
  using std::string;

#include <iomanip>
  using std::ios;
#include <fstream>
  using std::ofstream;

#include <iostream>
  using std::cout;
  using std::endl;

#include <stdlib.h>    // getenv
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util/ErrMsg.H"
#include "util/Ecce.H"
#include "util/SFile.H"
#include "util/SDirectory.H"
#include "util/TempStorage.H"
#include "util/EcceRegexp.H"
#include "util/EcceMap.H"
#include "util/TDateTime.H"
#include "util/STLUtil.H"
#include "util/ETimer.H"
#include "util/ResourceUtils.H"


#include "tdat/QueueMgr.H"
#include "tdat/RefMachine.H"
#include "tdat/RefQueueMgr.H"
#include "tdat/AuthCache.H"

#include "dsm/MachineConfig.H"
#include "dsm/NWChemMDModel.H"
#include "dsm/TaskJob.H"
#include "dsm/Session.H"
#include "dsm/JCode.H"
#include "dsm/CodeFactory.H"
#include "dsm/MdTask.H"
#include "dsm/EDSIFactory.H"
#include "dsm/DirDyVTSTTask.H"
#include "dsm/ResourceType.H"
#include "dsm/ResourceDescriptor.H"


#include "comm/RCommand.H"
#include "comm/Launch.H"

#define MAXLINE 256

// When instanced, submit scripts get renamed to submitPrefix
// + calcname.  If you change this, also change eccejobstore.C
// cleanup code.
static const string submitPrefix = "submit__";

///////////////////////////////////////////////////////////////////////////////
// Class Statics
///////////////////////////////////////////////////////////////////////////////
const LaunchData Launch::p_opData[] = {
  // Remote login first to catch problems there right away
  {  "Verifying remote login...",
     (launchOperation)&Launch::validateRemoteLogin},

  // Local operations
  {  "Validating local directory...",
     (launchOperation)&Launch::validateLocalDir},
  {  "Validating job...",
     (launchOperation)&Launch::validateCalculation},
  {  "Generating job submission script...",
     (launchOperation)&Launch::generateJobSubmissionFile},
  {  "Generating job monitoring configuration files...",
     (launchOperation)&Launch::generateJobMonitoringFiles},

  // Remote operations
  {  "Verifying remote directory...",
     (launchOperation)&Launch::validateRemoteDir},
  {  "Verifying scratch directory...",
     (launchOperation)&Launch::validateScratchDir},
  {  "Verifying remote perl...",
     (launchOperation)&Launch::validateRemotePerl},

  // Local=>Remote copy before submit
  {  "Transferring files...",
     (launchOperation)&Launch::moveFiles},

  // Remote submit
  {  "Running submit script...\nStarting eccejobstore...",
     (launchOperation)&Launch::doLaunch}
};

class CalcInfo {
public:
  string calcURL;
  string machineName;
  string fullMachineName;
  string scriptName;
  string appName;
  string codeName;
  string directory;
  string userName;
  string remoteShell;
  string remoteDir;
  string scratchDir;
  string jobId;
  string importDir;
  string importName;
  bool   frontendFlag;
  bool   checkScratchFlag;
  bool   isMd;
  string mdSystemName;
  string mdCalcName;
  int    mdStoreTrj;
  bool   mdBatchOutput;
  bool   mdPMFOutput;
  const  RefQueueManager* mgr;
};

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Constructor.  Changes made to the SingleJob after this point will
//    NOT be picked up.
//
//  Implementation
//    The RCommand is instanced only when it is first needed.
//    The SingleJob is cached in a local structure because we wanted to
//    eliminate ALL transactions during the launch process.  Transactions
//    during the validation process are ok.
//
///////////////////////////////////////////////////////////////////////////////
Launch::Launch(TaskJob* task,
               EcceMap& kvargs,
               const bool& fullLaunchFlag,
               const string& importDir, const string& importName)
{
  p_restartProvider = 0;
  p_topologyProvider = 0;
  p_fragmentProvider = 0;
  p_isPrepareTask = false;
  p_isMetadynamicsTask = false;
  p_taskjob = task;
  Session *session = task->getSession();

  if (session != 0) {
    // Get restart file - May have to search through several tasks
    p_restartProvider = 0;
    TaskJob *searchTask = dynamic_cast<TaskJob*>(EDSIFactory::getResource(task->getURL()));
    bool finishedSearch = false;
    while (!finishedSearch && searchTask != 0) {
      p_restartProvider = dynamic_cast<TaskJob*>(searchTask->getInputProvider(session));
      if (p_restartProvider == 0) {
        finishedSearch = true;
        searchTask = 0;
      }
      else {
        vector<EcceURL> restartFiles = p_restartProvider->getDataFiles(JCode::RESTART_OUTPUT);
        if (restartFiles.size() > 0) {
          finishedSearch = true;
          searchTask = 0;
        }
        else {
          searchTask = p_restartProvider;
          p_restartProvider = 0;
        }
      }
    }

    // Get topology file - May have to search through several tasks
    p_topologyProvider = 0;
    searchTask = dynamic_cast<TaskJob*>(EDSIFactory::getResource(task->getURL()));
    finishedSearch = false;
    while (!finishedSearch && searchTask != 0) {
      p_topologyProvider = dynamic_cast<TaskJob*>(searchTask->getInputProvider(session));
      if (p_topologyProvider == 0) {
        finishedSearch = true;
        searchTask = 0;
      }
      else {
        vector<EcceURL> topologyFiles = p_topologyProvider->getDataFiles(JCode::TOPOLOGY_OUTPUT);
        if (topologyFiles.size() > 0) {
          finishedSearch = true;
          searchTask = 0;
        }
        else {
          searchTask = p_topologyProvider;
          p_topologyProvider = 0;
        }
      }
    }

    // Get fragment provider - may have to search through several tasks.
    // If this is the first Prepare task, then there will not be a fragment
    // provider (i.e. the originalsource fragment will be used.
    if (task->getContentType() == ResourceDescriptor::CT_MDPREPARE) {
      p_isPrepareTask = true;
      searchTask = dynamic_cast<TaskJob*>(EDSIFactory::getResource(task->getURL()));
      finishedSearch = false;
      while (!finishedSearch && searchTask != 0) {
        p_fragmentProvider = dynamic_cast<TaskJob*>(searchTask->getInputProvider(session));
        if (p_fragmentProvider == 0) {
          finishedSearch = true;
          searchTask = 0;
        }
        else {
          vector<EcceURL> fragmentFiles = p_fragmentProvider->getDataFiles(JCode::FRAGMENT_OUTPUT);
          if (fragmentFiles.size() > 0) {
            finishedSearch = true;
            searchTask = 0;
          }
          else {
            searchTask = p_fragmentProvider;
            p_fragmentProvider = 0;
          }
        }
      }
    }

    // For condensed phase reaction study metadynamics tasks--used to bring
    // back extra output files in order to allow task chaining
    if (task->getApplicationType() == ResourceDescriptor::AT_METADYNAMICS)
      p_isMetadynamicsTask = true;
  }

  p_valid = true;
  p_connection = (RCommand*)0;
  p_localconn = (RCommand*)0;
  p_curOp = 0;
  cacheCalcInfo(importDir, importName);

  // Copy the options
  p_options =new EcceMap(kvargs);

  p_numOps = sizeof(p_opData)/sizeof(LaunchData);
  if (!fullLaunchFlag)
    p_numOps--;

}


// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Destructor.
//
//  Implementation
//    Close connection if there is an open one.
//
///////////////////////////////////////////////////////////////////////////////
Launch::~Launch(void)
{
  p_options->clear();
  delete p_options;

  if (!p_cache->frontendFlag) {
    // deleting the remote connection completely hoses job monitoring when it
    // is done as a child process
    if (p_connection != (RCommand*)0)
      delete p_connection;
  }

  if (p_localconn != (RCommand*)0)
    delete p_localconn;
}

// ------------ Modifiers ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Executes the next operation.  If there has already been an
//    error, the next operation will not be executed and this method
//    will return False.  Use message() to retrieve the message
//    for the operation that failed.  Also use description() for a
//    description of what the current operation does.
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
bool Launch::nextOperation(void)
{
#if (!defined(INSTALL) && defined(DEBUG))
cout << "operation: " << p_opData[p_curOp].description << endl;
#endif

  if (p_connection!=NULL && !p_connection->isOpen()) {
    p_lastMessage = p_connection->commError();
    p_valid = false;
  }
  else if (p_localconn!=NULL && !p_localconn->isOpen()) {
    p_lastMessage = p_localconn->commError();
    p_valid = false;
  }
  else if (p_valid && p_curOp<p_numOps) {
    p_lastMessage = "";
    p_infoMessage = "";
    p_valid = (this->*p_opData[p_curOp].operation)();
    if (p_valid)
      p_curOp++;
  }

  return p_valid;
}

// ------------ Accessors ------------
int Launch::percentDone(void) const
{
  return (int)((double)p_curOp/(double)p_numOps * 100);
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns true if all operations have completed and no errors were
//    detected.
//
///////////////////////////////////////////////////////////////////////////////
bool Launch::done(void) const
{ return (p_valid && p_curOp>=p_numOps); }

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//     Returns the description of the current operation.  After all 
//     operations have are "done", the description is an empty string.
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
string Launch::description(void) const
{ 
  string ret;

  if (p_curOp < p_numOps)
    ret = p_opData[p_curOp].description;

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns the error message associated with the last operation executed.
//    If the operation was sucessful, this will be an empty string.
//////////////////////////////////////////////////////////////////////////////
string Launch::message(void) const
{ return p_lastMessage; }

string Launch::info(void) const
{ return p_infoMessage; }

// ---------- General Methods  ----------

//////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Store Necessary Experiment Information in a Local Structure
//
//  Implementation
//    Rather than continually open and close transactions, we locally
//    store needed calculation information in a local structure and 
//    access the member structure (p_cache) in subroutines that do the real
//    work of validating and launching a remote job.
//
////////////////////////////////////////////////////////////////////////////// 
void Launch::cacheCalcInfo(const string& importDir, const string& importName)
{
  p_cache = new CalcInfo;

  p_cache->calcURL = p_taskjob->getURL().toString();

  Launchdata launchdata = p_taskjob->launchdata();
  Jobdata jobdata = p_taskjob->jobdata();

  p_cache->machineName = launchdata.machine;

  p_cache->scriptName = submitPrefix + p_taskjob->getName();

  // Create a temporary staging directory for the input file and other
  // configuration files created by this class.
  SDirectory* dir = TempStorage::getTempJobDirectory(
                                p_taskjob->getURL().getFilePathTail().c_str());
  p_cache->directory = dir->path();

  const JCode* jcode = p_taskjob->application();
  p_cache->appName = jcode->name();
  p_cache->codeName = jcode->getCodeName();
  p_cache->jobId = jobdata.jobid;
  p_cache->userName = launchdata.user;
  p_cache->remoteShell = launchdata.remoteShell;
  p_cache->remoteDir = jobdata.jobpath;

  const QueueManager* queueMgr = 
    QueueManager::lookup(p_cache->machineName);
  string queueMgrName;
  if (queueMgr == (QueueManager*)0)
    queueMgrName = "Shell";
  else
    queueMgrName = queueMgr->queueMgrName();

  p_cache->mgr = RefQueueManager::lookup(queueMgrName.c_str());
  EE_ASSERT(p_cache->mgr != (RefQueueManager*)0, EE_FATAL,
            "No queue manager found for specified machine!");

  p_cache->scratchDir = launchdata.scratchdir;

  p_cache->importDir = importDir;
  p_cache->importName = importName;

  // Set flag for whether a frontend machine is needed, which means
  // to do the single RCommand based job launch and file transfer
  RefMachine* refMachine = RefMachine::refLookup(p_cache->machineName);
  p_cache->frontendFlag = refMachine->singleConnect() ||
                       (refMachine->frontendMachine()!="" &&
                        (refMachine->frontendBypass()=="" ||
                        !RCommand::isSameDomain(refMachine->frontendBypass())));

  p_cache->checkScratchFlag = refMachine->checkScratch();

  // Get system and calc name for md tasks, if MD 
  MdTask *mdTask = 0;

  p_cache->isMd = false;
  p_cache->mdSystemName = "";
  p_cache->mdCalcName = "";
  p_cache->mdStoreTrj = 0;
  p_cache->mdBatchOutput = false;
  p_cache->mdPMFOutput = false;

  try {
    mdTask = dynamic_cast<MdTask*>(EDSIFactory::getResource(p_taskjob->getURL()));

    if (mdTask != 0) {
      p_cache->isMd = true;
      p_cache->mdSystemName = mdTask->getSystemName();
      p_cache->mdCalcName = mdTask->getCalcName();

      if (!p_isPrepareTask) {
        // The logic to populate the model correctly is pretty tricky
        // Panels that are to be referenced must be "pushed" so the model
        // fields are populated.
        vector<NWChemMDModel::GUIPanel> panels;
        panels.push_back(NWChemMDModel::FILES);
        panels.push_back(NWChemMDModel::THERMODYNAMICS);
        NWChemMDModel taskModel(panels);
        try {
          mdTask->getTaskModel(taskModel);

          FilesModel *fmodel = taskModel.getFilesModel();
          if (fmodel) {
            p_cache->mdStoreTrj = fmodel->getTrajectoryStorage();
            p_cache->mdBatchOutput = fmodel->getUseBatches();
          }
          ThermodynamicsModel *tmodel = taskModel.getThermodynamicsModel();
          if (tmodel) {
            p_cache->mdPMFOutput = tmodel->getUsePmf();
          }
        } catch (...) {
        }
      }
    }
  }
  catch (...) {
  }
}


//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns true if the remote communication shell is successfully
//    established.
//////////////////////////////////////////////////////////////////////////////
bool Launch::validateRemoteLogin(void)
{
  bool ret = true;

  RefMachine* refMachine = RefMachine::refLookup(p_cache->machineName);
  if (refMachine != (RefMachine*)0) {
    p_cache->fullMachineName = refMachine->fullname();
    string locShell = refMachine->shell();
    string shellPath = refMachine->shellPath();
    string libPath = refMachine->libPath();
    string sourceFile = refMachine->sourceFile();
    string frontendMachine = refMachine->frontendMachine();
    string frontendBypass = refMachine->frontendBypass();

    string password = (*p_options)["##password1##"];

    p_connection = new RCommand(p_cache->fullMachineName,
                                p_cache->remoteShell, locShell,
                                p_cache->userName, password,
                                frontendMachine, frontendBypass,
                                shellPath, libPath, sourceFile);

  } else {
    p_lastMessage = p_cache->machineName;
    p_lastMessage += " is not a registered ECCE machine";
    ret = false;
  }

  if (ret) {
    ret = p_connection->isOpen();
    if (!ret)
      p_lastMessage = p_connection->commError();
  }

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns true if the local directory exists and is a directory.
//////////////////////////////////////////////////////////////////////////////
bool Launch::validateLocalDir(void)
{
  p_localconn = new RCommand("system");

  bool ret = p_localconn->isOpen();
  if (!ret)
    p_lastMessage = p_localconn->commError();
  else {
    ret = p_localconn->directory(p_cache->directory);
    if (!ret)
      p_lastMessage = "Local calculation directory " +
                      p_cache->directory + " does not exist.";
    else {
      ret = p_localconn->writable(p_cache->directory);
      if (!ret)
        p_lastMessage = "Local calculation directory " +
                        p_cache->directory + " is not writable.";
    }
  }

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Validates SingleJob.
//    This is the last line of defense before actually trying to execute
//    a job.  So far, the only thing that can go wrong is if the input
//    file doesn't actually exist.
//////////////////////////////////////////////////////////////////////////////
bool Launch::validateCalculation(void)
{
  bool ret = false;

  if (p_taskjob) {
    if ( (( p_cache->mdSystemName.empty() &&  p_cache->mdCalcName.empty()) &&
         (!p_cache->isMd) ) || 
         ((!p_cache->mdSystemName.empty() && !p_cache->mdCalcName.empty()) &&
         ( p_cache->isMd)) ) {
      string tmpInput = (*p_options)["##input##"];
      EE_RT_ASSERT(!tmpInput.empty(), EE_FATAL, "No Input File Name!");
      string inputFileName = tmpInput;

      istream* is = p_taskjob->getDataFile(JCode::PRIMARY_INPUT);
      if (is) {
        string inputFilePath = p_cache->directory + "/" +inputFileName;
        ofstream ofs(inputFilePath.c_str());

        if (ofs) {
          char buf[256];
          while (is->getline(buf, 255))
            ofs << buf << endl;

          delete is;
          is = 0;
          ofs.close();
          ret = true;

          // process any auxiliary basis set files (Amica)
          vector<EcceURL> basvec = p_taskjob->getDataFiles(JCode::AUXILIARY_INPUT);
          for (int ib=0; ib<basvec.size(); ib++) {
            is = p_taskjob->getAnyFile(basvec[ib]);
            if (is) {
              inputFilePath = p_cache->directory + "/" + basvec[ib].getFilePathTail();
              ofs.open(inputFilePath.c_str());
              if (ofs) {
                while (is->getline(buf, 255))
                  ofs << buf << endl;

                ofs.close();
              } else {
                ret = false;
                p_lastMessage =
                      "Basis set input file could not be written to local disk";
                delete is;
                is = 0;
                break;
              }
              delete is;
              is = 0;
            } else {
              ret = false;
              p_lastMessage = "Basis set input file missing from calculation";
            }
          } /* for */

          //  Restart file
          if (p_cache->isMd && p_restartProvider!=0) {
            vector<EcceURL> restartFiles = p_restartProvider->getDataFiles(
                                                         JCode::RESTART_OUTPUT);
            if (restartFiles.size() > 0) {
              is = p_restartProvider->getDataFile(JCode::RESTART_OUTPUT);
              if (is) {
                inputFilePath = p_cache->directory + "/" +
                     p_cache->mdSystemName + "_" + p_cache->mdCalcName + ".rst";
                ofs.open(inputFilePath.c_str());
                if (ofs) {
                  while (is->getline(buf, 255)) {
                    ofs << buf << endl;
                  }
                  ofs.close();
                }
                delete is;
                is = 0;
              }
            }
          } else if (p_cache->isMd && !p_isPrepareTask) {
            p_lastMessage = "Unable to find restart file from previous MD Study task";
            return false;
          }

          // Topology file
          if (p_cache->isMd && p_topologyProvider!=0) {
            vector<EcceURL> topologyFiles = p_topologyProvider->getDataFiles(
                                                      JCode::TOPOLOGY_OUTPUT);
            if (topologyFiles.size() > 0) {
              is = p_topologyProvider->getDataFile(JCode::TOPOLOGY_OUTPUT);
              if (is) {
                inputFilePath = p_cache->directory + "/" +
                                         p_cache->mdSystemName + ".top";
                ofs.open(inputFilePath.c_str());
                if (ofs) {
                  while (is->getline(buf, 255)) {
                    ofs << buf << endl;
                  }
                  ofs.close();
                }
                delete is;
                is = 0;
              }
            }
          } else if (p_cache->isMd && !p_isPrepareTask) {
            p_lastMessage = "Unable to find topology file from previous MD Study task";
            return false;
          }

          if (p_isPrepareTask) {
            // Fragment Provider
            if (p_fragmentProvider != 0)  {
              vector<EcceURL> topologyFiles = p_fragmentProvider->getDataFiles(JCode::FRAGMENT_OUTPUT);
              if (topologyFiles.size() > 0) {
                is = p_fragmentProvider->getDataFile(JCode::FRAGMENT_OUTPUT);
              }
            }
            // Original Fragment
            else {
              MdTask *mdTask = 0;
              mdTask = dynamic_cast<MdTask*>(EDSIFactory::getResource(p_taskjob->getURL()));
              if (mdTask != 0) {
                is = mdTask->getInputFragmentAsFile();
                mdTask = 0;
              }
            }
            // Save fragment
            if (is) {
              inputFilePath = p_cache->directory + "/" + p_cache->mdSystemName + ".pdb";
              ofs.open(inputFilePath.c_str());
              if (ofs) {
                while (is->getline(buf, 255)) {
                  ofs << buf << endl;
                }
                ofs.close();
              } else {
                ret = false;
                p_lastMessage = "Unable to write chemical system pdb file for MD Prepare task";
              }
              delete is;
              is = 0;
            } else {
              ret = false;
              p_lastMessage = "Unable to find source for chemical system pdb file for MD Prepare task";
            }
          }
        } 
        else {
          p_lastMessage = "Input file could not be written to local disk";
          delete is;
          is = 0;
        }

      } else
        p_lastMessage = "Input file missing from calculation";
    } else 
      p_lastMessage = "Could not determine MD System and Calc names";
  } else
    p_lastMessage = "Not a valid calculation";

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns true if the remote dir exists or can be created,
//    is writable, and is not in afs.
//////////////////////////////////////////////////////////////////////////////
bool Launch::validateRemoteDir(void)
{
  bool ret = checkRemoteDir(p_cache->remoteDir, true) &&
             checkDiskSpace(p_cache->remoteDir);

  if (ret) {
    string parseFileName = (*p_options)["##parse##"];
    string outputFileName = (*p_options)["##output##"];
    EE_RT_ASSERT(!parseFileName.empty() && !outputFileName.empty(),
                 EE_FATAL, "No Parse/Output File Names!");
    string propertyFileName = (*p_options)["##property##"];
    string auxiliaryFileName = (*p_options)["##auxiliary##"];

    substituteRealFileName(parseFileName);
    substituteRealFileName(outputFileName);
    substituteRealFileName(propertyFileName);
    substituteRealFileName(auxiliaryFileName);

    string cmd;
    string file = p_cache->remoteDir + "/" + outputFileName;
    if (p_connection->exists(file)) {
      cmd = "touch " + file + ".old";
      (void)p_connection->exec(cmd);
      cmd = "cat " + file + " >> " + file + ".old";
      (void)p_connection->exec(cmd);
      cmd = "rm -f " + file;
      (void)p_connection->exec(cmd);
    }
    if (parseFileName!="" && outputFileName!=parseFileName) {
      file = p_cache->remoteDir + "/" + parseFileName;
      if (p_connection->exists(file)) {
        cmd = "touch " + file + ".old";
        (void)p_connection->exec(cmd);
        cmd = "cat " + file + " >> " + file + ".old";
        (void)p_connection->exec(cmd);
        cmd = "rm -f " + file;
        (void)p_connection->exec(cmd);
      } else {
        // special logic to remove batch sequenced parse files
        // otherwise the job monitoring will think the job is already
        // running when really the parse files are from a previous run
        string wildfile = file;
        int pos = wildfile.find_last_of('.');
        if (pos != string::npos) {
          wildfile.insert(pos, "001");
          if (p_connection->exists(wildfile)) {
            cmd = "touch " + file + ".old";
            (void)p_connection->exec(cmd);
            wildfile = file;
            wildfile.insert(pos, "*");
            cmd = "cat " + wildfile + " >> " + file + ".old";
            (void)p_connection->exec(cmd);
            cmd = "rm -f " + wildfile;
            (void)p_connection->exec(cmd);
          }
        }
      }
    }
    if (propertyFileName != "") {
      file = p_cache->remoteDir + "/" + propertyFileName;
      if (p_connection->exists(file)) {
        cmd = "rm -f " + file;
        (void)p_connection->exec(cmd);
      }
    }
    if (auxiliaryFileName != "") {
      file = p_cache->remoteDir + "/" + auxiliaryFileName;
      if (p_connection->exists(file)) {
        cmd = "rm -f " + file;
        (void)p_connection->exec(cmd);
      }
    }

    cmd = "rm -f " + p_cache->remoteDir + "/.ecce.status";
    (void)p_connection->exec(cmd);
    cmd = "rm -f " + p_cache->remoteDir + "/*.desc";
    (void)p_connection->exec(cmd);
  }

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns true if the scratch dir exists or can be created,
//    is writable, and is not in afs.
//////////////////////////////////////////////////////////////////////////////
bool Launch::validateScratchDir(void)
{
  bool ret = true;
  // On some machines (chinook, mitcluster) we don't want to validate the
  // scratch directory because it's not writable on the login nodes so the
  // check would always fail
  if (p_cache->scratchDir!="" && p_cache->checkScratchFlag) {
    ret = p_cache->scratchDir==p_cache->remoteDir ||
          checkRemoteDir(p_cache->scratchDir, false);
  }
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    First, check if the directory already exists.  If it does we
//    need to make sure it's writable.  If it doesn't, we have to
//    do a recursive mkdir.
//////////////////////////////////////////////////////////////////////////////
bool Launch::checkRemoteDir(const string& remoteDir, const bool& rerunCheck)
{
  bool ret = remoteDir[0]=='/' || remoteDir[0]=='~';

  // First see if it already exists
  if (ret) {
    string cmd;
    ret = p_connection->directory(remoteDir);
    if (ret && rerunCheck && p_taskjob->getRerun()) {
      // if a reset for rerun operation was done delete all the existing
      // run directory files and clear the rerun flag
      cmd = "rm -f " + p_cache->remoteDir + "/*";
      (void)p_connection->exec(cmd);
      cmd = "rm -f " + p_cache->remoteDir + "/.ecce.status";
      (void)p_connection->exec(cmd);
      p_taskjob->setRerun(false);
    } else if (!ret) {
      cmd = "mkdir -p " + remoteDir;
      string err = "The remote directory " + remoteDir +
                      " on " + p_cache->machineName +
                      " cannot be created (no write permission?)";
      ret = p_connection->exec(cmd, err);
      if (!ret) {
        p_lastMessage = p_connection->commError();
      }
    }
  
    if (ret) {
      ret = p_connection->writable(remoteDir);
      if (!ret) {
        p_lastMessage = "The remote directory " + remoteDir + 
                        " on " + p_cache->machineName +
                        " is not writable.";
      }
    }
  } else {
    p_lastMessage = "The remote directory " + remoteDir + 
                    " on " + p_cache->machineName +
                    " must begin with / or ~ (absolute path).";
  }

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns true if there is enough disk space to at least start the job
//////////////////////////////////////////////////////////////////////////////
bool Launch::checkDiskSpace(const string& dir)
{
  bool ret = true;

  if (p_connection->executable("/usr/sbin/mkfile") ||
      p_connection->executable("/etc/mkfile") ||
      p_connection->executable("/bin/mkfile")) {

    string testFile = dir + "/test_mkfile";
    string cmd = "mkfile 2m " + testFile;
    string err = "The remote directory " + dir +
                    " on " + p_cache->machineName +
                    " does not have enough available disk space.";

    ret = p_connection->exec(cmd, err);
    if (!ret) {
      p_lastMessage = p_connection->commError();
      if (p_lastMessage.find("The remote directory ") != 0) {
        p_lastMessage = "";
        ret = true;
      }
    }

    cmd = "rm -f " + testFile;
    (void)p_connection->exec(cmd);
  }

  return ret;
}


///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns true if the remote command executes successfully.
//
///////////////////////////////////////////////////////////////////////////////
bool Launch::checkRemoteCmd(const string& command,
                                 const string& msg)
{
  bool ret = p_connection->exec(command, msg);
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "cmd " << command << endl;
#endif
  if (!ret)
    p_lastMessage = p_connection->commError();

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns true if the local command succeeds.
//
///////////////////////////////////////////////////////////////////////////////
bool Launch::checkLocalCmd(const string& command,
                                const string& msg)
{
  bool ret = p_localconn->exec(command, msg);
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "cmd " << command << endl;
#endif
  if (!ret)
    p_lastMessage = p_localconn->commError();

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Returns true if perl (hopefully v5) is found on the remote server.
//
//////////////////////////////////////////////////////////////////////////////
bool Launch::validateRemotePerl(void)
{
  return (checkRemoteCmd("perl -v", "perl not found on remote machine."));
}

///////////////////////////////////////////////////////////////////////////////
//
// Description
//    Change the local working directory.
//    If the directory is illegal an error message is issued and
//    no changes are made.
//
///////////////////////////////////////////////////////////////////////////////
void Launch::changeWD(const string& workingDir)
{
  if (!p_localconn->cd(workingDir))
    ErrMsg().message("EE_FILE_ACCESS",EE_FATAL,WHERE,workingDir.c_str());
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//    Create an eccejobstore.conf configuration file.
//    This routine will overwrite previous configurations.
//
// Implementation
//    Using the experiment information we set all parameters relevant to
//    an electronic structure calculation.
//    We also specify that eccejobstore should parse all properties
//    for the specified type of run.
//
///////////////////////////////////////////////////////////////////////////////
bool Launch::generateJobMonitoringFiles(void)
{
  bool ret = true;

  // eccejobstore.conf
  string configName = p_cache->directory;
  configName += "/eccejobstore.conf";

  ofstream storeConfigFile(configName.c_str());

  // used below
  const JCode* jcode = p_taskjob->application();

#if (!defined(INSTALL) && defined(DEBUG))
  cout << "launch: writing Config File \"eccejobstore.conf\"" << endl;
#endif
  if (!storeConfigFile)
    ErrMsg().message("EE_FILE_ACCESS", EE_FATAL, WHERE, "eccejobstore.conf");
  else {

    // construct date object with current date/time
    TDateTime date;
    storeConfigFile << "# eccejobstore configuration file, created "
                    << date.toString(TDateTime::AmericanLocale,
				     TDateTime::HMS24)
		    << endl << endl;
    storeConfigFile << "host: " << p_cache->machineName << endl;
    storeConfigFile << "calcURL: " << p_cache->calcURL << endl;
    storeConfigFile << "codeName: " << p_cache->appName << endl;
    storeConfigFile << "remoteDir: " << p_cache->remoteDir << endl;
    storeConfigFile << "remoteShell: " << p_cache->remoteShell << endl;
    storeConfigFile << "userName: " << p_cache->userName << endl;
    storeConfigFile << "importDir: " << p_cache->importDir << endl;
    if (p_cache->mdStoreTrj == 1)
      storeConfigFile << "mdStoreTrj: archive" << endl;
    else if (p_cache->mdStoreTrj==2 && Ecce::ecceStoreTrajectories())
      storeConfigFile << "mdStoreTrj: server" << endl;
    else
      storeConfigFile << "mdStoreTrj: none" << endl;
    storeConfigFile << "rxnMetadynamicsTask: " <<
                       (p_isMetadynamicsTask? "yes": "no") << endl;

    storeConfigFile.close();
  }

  // eccejobmonitor.conf
  configName = p_cache->directory;
  configName += "/eccejobmonitor.conf";

  ofstream monitorConfigFile(configName.c_str());

#if (!defined(INSTALL) && defined(DEBUG))
  cout << "launch: writing Config File \"eccejobmonitor.conf\"" << endl;
#endif
  if (!monitorConfigFile)
    ErrMsg().message("EE_FILE_ACCESS", EE_FATAL, WHERE, "eccejobmonitor.conf");
  else {
    monitorConfigFile << "host " << p_cache->machineName << endl;

    if (p_cache->isMd) {
      monitorConfigFile << "systemName " << p_cache->mdSystemName << endl;
      monitorConfigFile << "calcName " << p_cache->mdCalcName << endl;
    } else {
      if (p_cache->importName != "") {
        // for an import the correct name is passed in to us
        monitorConfigFile << "calcName " << p_cache->importName << endl;
      } else {
        // for a live calculation, extract the name from the input file.
        // This only works for an NWChem calculation that has a start/restart
        // directive, but that may be the only time we have output files
        // based on the calculation name.  Fallback to stripping the ECCE
        // calculation name of any trailing dashes/numbers if all else fails.
        string calcName = "";
        string inputFilePath = p_cache->directory + "/" +
                               (*p_options)["##input##"];

        RCommand localconn("system");
        if (localconn.isOpen()) {
          string cmd = "grep -i \"^start \" ";
          cmd.append(inputFilePath);
          string oput;
          if (localconn.execout(cmd, oput)) {
            // Strip up to second token
            oput = oput.substr(oput.find_first_not_of(" \t",6));
            // Strip trailing whitespace and newlines
            oput = oput.substr(0, oput.find_last_not_of(" \t\n\r")+1 );
            if (oput != "")
              calcName = oput;
          } else {
            cmd = "grep -i \"^restart \" ";
            cmd.append(inputFilePath);
            if (localconn.execout(cmd, oput)) {
              // Strip up to second token
              oput = oput.substr(oput.find_first_not_of(" \t",8));
              // Strip trailing whitespace and newlines
              oput = oput.substr(0, oput.find_last_not_of(" \t\n\r")+1 );
              if (oput != "")
                calcName = oput;
            }
          }
        }

        if (calcName == "") {
          // need to strip off the unique extensions to the name added by ECCE
          // to get the proper "base" calculation name
          calcName = p_taskjob->getName();
          // assumes that the user does not use dashes in the name or else this
          // will not behave as desired
          string::size_type pos;
          if ((pos = calcName.find_first_of('-')) != string::npos) {
            calcName = calcName.substr(0, pos);
          }
        }

        monitorConfigFile << "calcName " << calcName << endl;
      }
    }

    monitorConfigFile << "ecceVersion " << Ecce::ecceVersion() << endl;

    if (p_cache->importDir != "") {
      monitorConfigFile << "monitoringMode post" << endl;
      monitorConfigFile << "importDir " << p_cache->importDir << endl;
    } else
      monitorConfigFile << "monitoringMode live" << endl;

    char* value;

    if (p_cache->remoteShell.find("Globus")!=string::npos &&
        RCommand::isRemote(p_cache->fullMachineName,
                           p_cache->remoteShell, p_cache->userName)) {
      monitorConfigFile << "jobQ globus" << endl;

      // Globus must use stdio because ejm writes to ejs over stdio
      monitorConfigFile << "commType stdio" << endl;
    } else {
      monitorConfigFile << "jobQ " << p_cache->mgr->name() << endl;

      string comms = "stdio";
      if ((value = getenv("ECCE_JOB_COMMS")) != NULL)
        comms = value;

      if (comms == "socketlocal") {
        if (RCommand::isSameDomain(p_cache->fullMachineName)) {
          monitorConfigFile << "commType socket" << endl;
          monitorConfigFile << "portFile ecce.port" << endl;
        } else
          monitorConfigFile << "commType stdio" << endl;
      }
      else if (comms == "socket") {
        monitorConfigFile << "commType socket" << endl;
        monitorConfigFile << "portFile ecce.port" << endl;
      }
      else
        monitorConfigFile << "commType stdio" << endl;
    }

    string parseFileName = (*p_options)["##parse##"];
    string outputFileName = (*p_options)["##output##"];
    EE_RT_ASSERT(!parseFileName.empty() && !outputFileName.empty(),
                 EE_FATAL, "No Parse/Output File Names!");

    substituteRealFileName(parseFileName);
    substituteRealFileName(outputFileName);

    monitorConfigFile << "jobOutputFile " << parseFileName << endl;
    monitorConfigFile << "jobOutputFile2 " << outputFileName << endl;

    string fragmentFileName = (*p_options)["##output_frag##"];
    substituteRealFileName(fragmentFileName);
    if (!fragmentFileName.empty()) {
      monitorConfigFile << "jobOutputFile3 " << fragmentFileName << endl;
    }
    string restartFileName = (*p_options)["##restart##"];
    substituteRealFileName(restartFileName);
    if (!restartFileName.empty()) {
      monitorConfigFile << "jobOutputFile4 " << restartFileName << endl;
    }
    string topologyFileName = (*p_options)["##topology##"];
    substituteRealFileName(topologyFileName);
    if (!topologyFileName.empty()) {
      monitorConfigFile << "jobOutputFile5 " << topologyFileName << endl;
    }
    string mdOutputFileName = (*p_options)["##md_output##"];
    substituteRealFileName(mdOutputFileName);
    if (!mdOutputFileName.empty()) {
      monitorConfigFile << "jobOutputFile6 " << mdOutputFileName << endl;
    }

    monitorConfigFile << "parseTypes ALL" << endl;

    monitorConfigFile << "mdTask " <<
                         (p_cache->isMd? "yes": "no") << endl;
    monitorConfigFile << "mdPrepareTask " <<
                         (p_isPrepareTask? "yes": "no") << endl;
    monitorConfigFile << "mdBatchOutput " <<
                         (p_cache->mdBatchOutput? "yes": "no") << endl;
    monitorConfigFile << "mdPMFOutput " <<
                         (p_cache->mdPMFOutput? "yes": "no") << endl;

    string logModeJobMonitor = "rmifok";
    if ((value = getenv("ECCE_JOB_LOGMODE")) != NULL) {
      // parse value
      string modes = value;
      int it;
      if ((it = modes.find_first_of(',')) != string::npos) {
        logModeJobMonitor = modes.substr(it + 1, modes.length() - it - 1);
        if (logModeJobMonitor == "false")
          logModeJobMonitor = "no";
        else if (logModeJobMonitor == "true")
          logModeJobMonitor = "yes";

        // verify value
        if (!((logModeJobMonitor == "no") ||
              (logModeJobMonitor == "rmifok") ||
              (logModeJobMonitor == "yes")))
          // silently reset to default
          logModeJobMonitor = "rmifok";
      }
    }
    monitorConfigFile << "logMode " << logModeJobMonitor << endl;

    monitorConfigFile << "parseDescriptorFile "
                      << jcode->parseDescriptor() << endl;

    monitorConfigFile.close();
  }

  // .desc file
  string sourceDescName = Ecce::ecceDataControllersPath();
  sourceDescName += "/" + jcode->parseDescriptor();

  string targetDescName = p_cache->directory;
  targetDescName += "/" + jcode->parseDescriptor();

  ret = generateDescriptorFile(sourceDescName, targetDescName);

  return ret;
}

bool Launch::generateDescriptorFile(const string& source, const string& target)
{
  bool ret = false;

  char* discardScripts = getenv("ECCE_JOB_PARSE_IGNORE");

  if (discardScripts == NULL) {
    string cmd = "cp -f " + source + " " + target;
    ret = p_localconn->exec(cmd);
    if (!ret)
      p_lastMessage = "Unable to copy parse descriptor file to staging area";
  } else {

    ifstream sourceDescFile(source.c_str());

    if (sourceDescFile) {
      ofstream targetDescFile(target.c_str());

      if (targetDescFile) {
        static const int BUFSIZE=512;
        char buf[BUFSIZE];
        char bufup[BUFSIZE];
        string blockText;
        bool blockFlag = false;
        bool discardFlag = false;
        int i;

        while (sourceDescFile.getline(buf, BUFSIZE-1)) {
          // make an upper case copy for comparisons
          strcpy(bufup, buf);
          for (i=0; i<strlen(bufup); i++)
            bufup[i] = toupper(bufup[i]);

          if (!blockFlag && strstr(bufup,"[")!=NULL &&
                            strstr(bufup,"[END]")==NULL) {
            blockFlag = true;
            blockText = buf;
            blockText += "\n";
          } else if (blockFlag) {
            blockText += buf;
            blockText += "\n";

            if (strstr(bufup, "[END]") != NULL) {
              if (!discardFlag)
                targetDescFile << "\n" << blockText;

              blockFlag = false;
              discardFlag = false;
            } else if (strstr(bufup, "SCRIPT") != NULL) {
              char* script = strchr(buf, '=');
              if (script != NULL) {
                script++;
                while (*script==' ' || *script=='\t')
                  script++;
              }

              if (script!=NULL && *script!='\0')
                discardFlag = strstr(discardScripts, script) != NULL;
            }
          }
        }

        targetDescFile.close();

      } else {
        ret = false;
        p_lastMessage = "Unable to open staged parse descriptor file";
      }

      sourceDescFile.close();

      ret = true;
    } else {
      ret = false;
      p_lastMessage = "Unable to open existing parse descriptor file";
    }
  }

  return ret;
}


void Launch::substituteRealFileName(string& fileName)
{
  int pos;

  // For MD jobs, the SystemName can be used to parameterize file names
  if ((pos = fileName.find("##SystemName##")) != string::npos) {
    fileName.replace(pos, 14, p_cache->mdSystemName);
  }
  if ((pos = fileName.find("##CalcName##")) != string::npos) {
    fileName.replace(pos, 12, p_cache->mdCalcName);
  }
}


bool Launch::generateJobSubmissionFile(void)
{
  bool ret = Launch::postProcessInput();
  if (ret) ret = Launch::instanceScript(*p_options);

  return ret;
}


//////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Transfer experiment setup files to remote machine.
//
////////////////////////////////////////////////////////////////////////////// 
bool Launch::moveFiles(void)
{
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Launch::moveFiles" << endl;
#endif

  bool ret;
  int restartIdx = 0;
  string restartFileName;
  int topologyIdx = 0;
  string topologyFileName;
  int pdbIdx = 0;
  string pdbFileName;
  string tmpInput = (*p_options)["##input##"];
  EE_RT_ASSERT(!tmpInput.empty(), EE_FATAL, "No Input File Name!");

  // Get system and calc name for md tasks, if MD 
  if ( (( p_cache->mdSystemName.empty() &&  p_cache->mdCalcName.empty()) &&
       (!p_cache->isMd) ) || 
       ((!p_cache->mdSystemName.empty() && !p_cache->mdCalcName.empty()) &&
       ( p_cache->isMd)) ) {

    // allocate array for all files to move
    vector<string> prvec = p_taskjob->getDataFileNames(JCode::PRIMARY_INPUT);
    vector<string> auxvec = p_taskjob->getDataFileNames(JCode::AUXILIARY_INPUT);
    vector<string> invec;
    if (prvec.size() > 0) {
      invec.push_back(prvec[0]);
    }
    for (int i=0; i < auxvec.size(); i++) {
      invec.push_back(auxvec[i]);
    }

    int auxCount = invec.size();
    int incount = invec.size();

    // NWDirDyVTST/POLYRATE file30 File
    string fu30FilePath = "";
    Session *session = p_taskjob->getSession();
    // Condensed phase reaction metadynamics task chaining files
    vector<string> metaFiles;
    if (session != 0) {
      // Get the DirdyVTST task
      DavCalculation *calc =
        dynamic_cast<DavCalculation*>(p_taskjob->getInputProvider(session));
      if (calc != 0) {
        vector<string> fu30vec=calc->getDataFileNames(JCode::AUXILIARY_OUTPUT);
        if (fu30vec.size() > 0) {

          istream* is = calc->getDataFile(JCode::AUXILIARY_OUTPUT);
          if (is) {
            // string fu30File = fu30vec[0];
            // fu30FilePath = p_cache->directory + "/" + fu30File;
            // rename the file while making a local copy
            fu30FilePath = p_cache->directory + "/poly.fu30";
            ofstream ofs(fu30FilePath.c_str());

            if (ofs) {
              char buf[256];
              while (is->getline(buf, 255))
                ofs << buf << endl;

              delete is;
              ofs.close();
              incount++;
            } else
              fu30FilePath = "";
          }
        } else if (p_isMetadynamicsTask) {
          vector<string> allFiles = calc->getAllFileNames();
          string metaFileName, metaFilePath;
          SFile *local;
          for (int it=0; it<allFiles.size(); it++) {
            metaFileName = allFiles[it];
            metaFilePath = "";
            // files where the base name uses the task name need to be renamed
            // for the chained calculation to restart correctly
            if (STLUtil::endsWith(metaFileName, ".db")) {
              metaFilePath = p_cache->directory + "/" + p_taskjob->getName() +
                             ".db";
            } else if (STLUtil::endsWith(metaFileName, ".movecs")) {
              metaFilePath = p_cache->directory + "/" + p_taskjob->getName() +
                             ".movecs";
            } else if (STLUtil::endsWith(metaFileName, ".vmovecs")) {
              metaFilePath = p_cache->directory + "/" + p_taskjob->getName() +
                             ".vmovecs";
            } else if (STLUtil::endsWith(metaFileName, ".vpp") ||
                       STLUtil::endsWith(metaFileName, ".psp") ||
                       metaFileName=="meta.dat" ) {
              metaFilePath = p_cache->directory + "/" + metaFileName;
            }

            if (metaFilePath != "") {
              local = new SFile(metaFilePath);
              if (!local->exists()) {
                local = calc->getAnyFile(metaFileName, local);
                incount++;
                metaFiles.push_back(metaFilePath);
              }
              delete local;
            }
          } /* for */
        }
      }
    }

    // Restart File
    if (p_cache->isMd && p_restartProvider!=0) {
      vector<string> restartFile = p_restartProvider->getDataFileNames(
                                                      JCode::RESTART_OUTPUT);
      if (restartFile.size() > 0) { 
        //restartFileName = restartFile[0];
        restartFileName = p_cache->mdSystemName + "_" +
                          p_cache->mdCalcName + ".rst";
        incount++;
        restartIdx = incount - 1;
      }
    } else if (p_cache->isMd && !p_isPrepareTask) {
      p_lastMessage = "Unable to find restart file from previous MD Study task";
      return false;
    }

    // Topology File
    if (p_cache->isMd && p_topologyProvider!=0) {
      vector<string> topologyFile = p_topologyProvider->getDataFileNames(
                                                        JCode::TOPOLOGY_OUTPUT);
      if (topologyFile.size() > 0) { 
        //topologyFileName = topologyFile[0];
        topologyFileName = p_cache->mdSystemName + ".top";
        incount++;
        topologyIdx = incount -1;
      }
    } else if (p_cache->isMd && !p_isPrepareTask) {
      p_lastMessage = "Unable to find topology file from previous MD Study task";
      return false;
    }

    // PDB File
    if (p_cache->isMd && p_isPrepareTask) {
      pdbFileName = p_cache->mdSystemName + ".pdb";
      incount++;
      pdbIdx = incount -1;
    }

    char** fromFiles = (char**)malloc((incount+5) * sizeof(char*));
    fromFiles[incount+4] = NULL;

    // primary input file
    string inputFileName = tmpInput;
    string inputFilePath = p_cache->directory + "/" + inputFileName;
    int inc = 0;
    fromFiles[inc++] = (char*)inputFilePath.c_str();

    if (fu30FilePath != "") {
      fromFiles[inc++] = (char*)fu30FilePath.c_str();
    }

    // Condensed phase reaction metadynamics task chaining files
    int in;
    for (in=0; in<metaFiles.size(); in++)
      fromFiles[inc++] = (char*)metaFiles[in].c_str();

    // auxiliary basis set files (Amica)
    // first file in vector is the primary input file -- skip that
    vector<string> bases;
    string basis;
    for (in=1; in<auxCount; in++) {
      basis = p_cache->directory;
      basis += "/" + invec[in];
      bases.push_back(basis);
    }
    for (in=0; in<bases.size(); in++)
      fromFiles[inc++] = (char*)bases[in].c_str();

    // Restart, Topology, and PDB Files - if any
    string restartFilePath = p_cache->directory + "/" + restartFileName;
    if (restartIdx > 0) {
      fromFiles[restartIdx] = (char*)restartFilePath.c_str();
    }

    string topologyFilePath = p_cache->directory + "/" + topologyFileName;
    if (topologyIdx > 0) {
      fromFiles[topologyIdx] = (char*)topologyFilePath.c_str();
    }

    string pdbFilePath = p_cache->directory + "/" + pdbFileName;
    if (pdbIdx > 0) {
      fromFiles[pdbIdx] = (char*)pdbFilePath.c_str();
    }

    string submit = p_cache->directory + "/";
    submit += p_cache->scriptName;
    fromFiles[incount] = (char*)submit.c_str();

    string config = p_cache->directory + "/eccejobmonitor.conf";
    fromFiles[incount+1] = (char*)config.c_str();

    const JCode* jcode = p_taskjob->application();
    string desc = p_cache->directory;
    desc += "/" + jcode->parseDescriptor();
    fromFiles[incount+2] = (char*)desc.c_str();

    string monitor = Ecce::ecceHome();
    monitor += "/scripts/eccejobmonitor";
    fromFiles[incount+3] = (char*)monitor.c_str();

    string password = (*p_options)["##password1##"];
    string toFile = p_cache->remoteDir;

    string errMessage = "";
    if (p_cache->remoteShell.find("shellcp")==string::npos &&
        !p_cache->frontendFlag) {
      ret = RCommand::put(errMessage, p_cache->fullMachineName,
                          p_cache->remoteShell, p_cache->userName,
                          password, (const char**)fromFiles, toFile);
    } else {
      ret = p_connection->shellput((const char**)fromFiles, toFile);
    }

    if (ret) {
      // make files writable so they can be copied over
#if (!defined(INSTALL) && defined(DEBUG))
      cout << "launch: Executing cd " << p_cache->remoteDir << endl;
#endif
      ret = p_connection->cd(p_cache->remoteDir);
      if (!ret) {
        p_lastMessage = "Unable to 'cd' to run directory";
#if (!defined(INSTALL) && defined(DEBUG))
        cout << "launch: Aborting After \"cd\" Attempt!" << endl;
#endif
      } else {
        (void)p_connection->exec(
              "chmod u+w eccejobmonitor eccejobmonitor.conf *.desc submit__*");
      } 
    } else {
      if (!p_cache->frontendFlag) 
        p_lastMessage = errMessage.c_str();
      else
        p_lastMessage = p_connection->commError();
    }

    delete fromFiles;
  }
  else {
    ret = false;
    p_lastMessage = "Could not determine MD System and Calc names";
  }

  return ret;
}


//////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Transfer job monitoring setup files to remote machine.
//
////////////////////////////////////////////////////////////////////////////// 
bool Launch::moveJobMonitoringFiles(void)
{
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "Launch::moveJobMonitoringFiles" << endl;
#endif

  bool ret = p_connection->directory(p_cache->remoteDir);
  if (!ret)
    p_lastMessage = "The remote directory " + p_cache->remoteDir +
                    " on " + p_cache->machineName + " does not exist.  "
                    "Cannot start job monitoring.";
  else {
    string cmd = "rm -f " + p_cache->remoteDir + "/*.desc";
    (void)p_connection->exec(cmd);

    string config = p_cache->directory + "/eccejobmonitor.conf";
    const JCode* jcode = p_taskjob->application();
    string desc = p_cache->directory;
    desc += "/" + jcode->parseDescriptor();

    string monitor = Ecce::ecceHome();
    monitor += "/scripts/eccejobmonitor";

    string password = (*p_options)["##password1##"];

    string errMessage = "";
    if (p_cache->remoteShell.find("shellcp")==string::npos &&
        !p_cache->frontendFlag) {
      ret = RCommand::put(errMessage, p_cache->fullMachineName,
                          p_cache->remoteShell, p_cache->userName,
                          password, 4, (char*)config.c_str(),
                          (char*)desc.c_str(), (char*)monitor.c_str(),
                          (char*)p_cache->remoteDir.c_str());
    } else {
      const char *fromFiles[4];
      fromFiles[0] = (char*)config.c_str();
      fromFiles[1] = (char*)desc.c_str();
      fromFiles[2] = (char*)monitor.c_str();
      fromFiles[3] = NULL;
      ret = p_connection->shellput(fromFiles, p_cache->remoteDir);
    }

    if (ret) {
      // make files writable so they can be copied over
#if (!defined(INSTALL) && defined(DEBUG))
      cout << "launch: Executing cd " << p_cache->remoteDir << endl;
#endif
      ret = p_connection->cd(p_cache->remoteDir);
      if (!ret) {
        p_lastMessage = "Unable to 'cd' to run directory";
#if (!defined(INSTALL) && defined(DEBUG))
        cout << "launch: Aborting After \"cd\" Attempt!" << endl;
#endif
      } else {
        (void)p_connection->exec(
                    "chmod u+w eccejobmonitor eccejobmonitor.conf *.desc");
      }
    } else
      p_lastMessage = errMessage;
  }

  return ret;
}


//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    The main routine for launching a remote calculation.
//    This cannot be called from within a transaction.
//
////////////////////////////////////////////////////////////////////////////// 
bool Launch::doLaunch(void)
{
  bool ret = true;

  // First move to the run directory because that is where we want
  // to submit from.
#if (!defined(INSTALL) && defined(DEBUG))
  cout << "launch: Executing cd " << p_cache->remoteDir << endl;
#endif
  ret = p_connection->cd(p_cache->remoteDir);
  if (!ret) {
     p_lastMessage = "Unable to 'cd' to run directory";
#if (!defined(INSTALL) && defined(DEBUG))
     cout << "launch: Aborting After \"cd\" Attempt!" << endl;
#endif
  } else {
    if (!p_connection->executable(p_cache->scriptName)) {
      string cmd = "chmod u+x " + p_cache->scriptName;
      ret = p_connection->exec(cmd);
      if (!ret) {
        p_lastMessage = "Unable to 'chmod u+x' " + p_cache->scriptName;
#if (!defined(INSTALL) && defined(DEBUG))
        cout << "launch: Aborting After \"chmod u+x\" Attempt!" << endl;
#endif
      }
    }

    if (ret) {
      // Warning:  the return status from the submission command that
      // follows only indicates that it was possible to run the command.
      // It does not indicate whether the job was successfully submitted.
      // It is also the responsibility of the developer to insure that
      // any command that is executed to invoke the job submission does
      // set the $status variable appropriately.

      // Check if this machine requires users to submit the job
      RefMachine* refMachine = RefMachine::refLookup(p_cache->machineName);
      if (refMachine->userSubmit()) {
        // Prompt the user for the job id
        string jobCmd = "./msgdialog prompt 'Enter Job ID' 'After submitting the job, enter the job ID that was returned:'";
        FILE* jobPtr;
        char jobBuf[MAXLINE];

        ret = false;
        if ((jobPtr = popen(jobCmd.c_str(), "r")) != NULL) {
          if (fgets(jobBuf, sizeof(jobBuf), jobPtr) != NULL) {
            // strip off the trailing newline
            jobBuf[strlen(jobBuf)-1] = '\0';
            // handle job id dialog cancel button
            if (strcmp(jobBuf, "") != 0) {
              // update Jobdata structure with the id given by the user
              p_cache->jobId = jobBuf;

              p_infoMessage = "Job id is " + p_cache->jobId;

              Jobdata jobdata = p_taskjob->jobdata();
              jobdata.jobid = p_cache->jobId;
              p_taskjob->jobdata(jobdata);

              ret = true;
            }
          }
        }
        if (!ret) {
          p_lastMessage = "Job id must be entered.";
        }
      } else {
        if (p_cache->remoteShell.find("Globus") != string::npos &&
            RCommand::isRemote(p_cache->fullMachineName,
                               p_cache->remoteShell, p_cache->userName))
          ret = launchGlobus();
        else
          ret = launchNormal();
      }

      if (ret)
        ret = startJobStore("");
    }
  }

  return ret;
}

bool Launch::launchNormal(void)
{
  // This substitutes in the name of the submit script (currently hardwired
  // to submit) plus the nice level into the submission command
  string command = p_cache->mgr->submitCommand();
  instanceCommand(command, *p_options);

  // Figure out where the nice command lives for this platform
  if (command.substr(0,5) == "nice ") {
    if (p_connection->executable("/bin/nice"))
      command.insert(0, "/bin/");
    else if (p_connection->executable("/usr/bin/nice"))
      command.insert(0, "/usr/bin/");
  }

#if (!defined(INSTALL) && defined(DEBUG))
    cout << "launch: Executing Command: " << command << endl;
#endif

  // The timeout can now be adjusted by setting an ECCE_SUBMIT_TIMEOUT
  // environment var.  If this is not set, it defaults to 120.
  int timeOut = 120;
  if (getenv("ECCE_SUBMIT_TIMEOUT") != NULL) 
    timeOut = (int)strtol(getenv("ECCE_SUBMIT_TIMEOUT"), (char**)NULL, 10);

  string output;

  bool ret;
  bool bgFlag = false;
  if (command[command.length()-1] != '&')
    ret = p_connection->execout(command, output, "", timeOut);
  else {
    command.resize(command.length()-1);
    ret = p_connection->execbg(command, output);
    bgFlag = true;
  }

#if (!defined(INSTALL) && defined(DEBUG))
    cout << "launch: Executed " << command << endl;
    cout << "        Submit timeout: " << timeOut << " seconds" << endl;
#endif

  if (!ret) {
    p_lastMessage = p_connection->commError();
    if (output != "") {
      p_lastMessage += "\nJob submission output: ";
      p_lastMessage += output;
    }
  } else if (bgFlag) {
    // shell job id parsing hardwired to allow for variability in output
    string id = output;
    string::size_type pos = id.find(']');
    if (pos != string::npos)
      id = id.substr(pos+1, id.length()-1);

    pos = id.find_first_of("\r\n");
    if (pos != string::npos)
      id = id.substr(0, pos);

    STLUtil::stripLeadingAndTrailingWhiteSpace(id);

    if (id == "")  {
      ret = false;
      p_lastMessage = "Unable to parse job id.  Cannot monitor job.";
    } else {
      p_infoMessage = "Job id is " + id;

      Jobdata jobdata = p_taskjob->jobdata();
      jobdata.jobid = id;
      p_taskjob->jobdata(jobdata);

      p_cache->jobId = id;
    }
  } else {
    // queue management system job id parsing
    string parseout = output;
    // Strip trailing characters '\n' and '\r'
    parseout = parseout.substr(0, parseout.find_last_not_of('\n')+1 );
    parseout = parseout.substr(0, parseout.find_last_not_of('\r')+1 );

    // Extract and save job/process id
    string expression = p_cache->mgr->jobIdParseExpression();
    string leading = p_cache->mgr->jobIdParseLeadingText();
    string trailing = p_cache->mgr->jobIdParseTrailingText();

    string fullExp = leading + expression + trailing;

    // MAKE REG EXP CLASS
    EcceRegexp re(fullExp);
    int length, foundIdx;
    string id = re.match(parseout, foundIdx, length);
 
    if (id == "")  {
      ret = false;
      p_lastMessage = "Unable to parse job id.  Cannot monitor job.";
    } else {
      // strip leading and trailing text if applicable
      // the nice thing here is that we know that the leading and trailing
      // text matched or the regular expression match would have failed
      if (leading != "")
        id.replace(0, leading.length(), "");
      if (trailing != "")
        id.resize(id.length()-trailing.length());

      // strip leading whitespace
      STLUtil::stripLeadingWhiteSpace(id);

      // finally, strip from the first whitespace character to the end
      if (id.find_first_of(" \t") != string::npos)
        id.replace(id.find_first_of(" \t"), string::npos, "");
    
      p_infoMessage = "Job id is " + id;

      Jobdata jobdata = p_taskjob->jobdata();
      jobdata.jobid = id;
      p_taskjob->jobdata(jobdata);

      p_cache->jobId = id;
    }
  }

  return ret;
}

bool Launch::launchGlobus(void)
{
  string command = p_cache->remoteDir + "/" + p_cache->scriptName;
  string queueRSL = "";
  string tmpVal;

  if (p_cache->mgr->name() == "Shell") {
    tmpVal = (*p_options)["##priority##"];
    if (!tmpVal.empty()) {
      command.insert(0, tmpVal + " ");

      // Figure out where the nice command lives for this platform
      if (command.substr(0,5) == "nice ") {
        if (p_connection->executable("/bin/nice"))
          command.insert(0, "/bin/");
        else if (p_connection->executable("/usr/bin/nice"))
          command.insert(0, "/usr/bin/");
      }
    }
  } else {
    tmpVal = (*p_options)["##queue##"];
    if (!tmpVal.empty())
      queueRSL += "(queue=" + tmpVal + ")";

    tmpVal = (*p_options)["##wall_clock_seconds##"];
    if (tmpVal != "0" && tmpVal != "") {
      unsigned long seconds;
      char buf[12];
      sscanf(tmpVal.c_str(),"%lu",&seconds);
      sprintf(buf,"%lu",seconds/60);
      queueRSL += "(maxWallTime=";
      queueRSL += buf;
      queueRSL += ")";
    }

    tmpVal = (*p_options)["##maxmemory##"];
    if (tmpVal != "0" && tmpVal != "") {
      unsigned long mwords;
      char buf[12];
      sscanf(tmpVal.c_str(),"%lu",&mwords);
      sprintf(buf,"%lu",mwords*8);
      queueRSL += "(maxMemory=";
      queueRSL += buf;
      queueRSL += ")";
    }

    tmpVal = (*p_options)["##minscratch##"];
    if (!tmpVal.empty())
      queueRSL += "(minScratch=" + tmpVal + ")";

    queueRSL += "(directory=" + p_cache->remoteDir + ")";

    queueRSL += "(stdout=";
    queueRSL += p_cache->remoteDir + "/" + (*p_options)["##output##"] +")";

    // Need this so that our submit script runs non-parallel and
    // our job runs parallel.
    queueRSL += "(jobType=single)";

    tmpVal = (*p_options)["##account_no##"];
    if (!tmpVal.empty())
      queueRSL += "(project=" + tmpVal + ")";
  }

  tmpVal = (*p_options)["##numProcs##"];
  if (!tmpVal.empty())
    queueRSL += "(count=" + tmpVal + ")";

  tmpVal = (*p_options)["##numNodes##"];
  if (!tmpVal.empty())
    queueRSL += "(hostCount=" + tmpVal + ")";

#if (!defined(INSTALL) && defined(DEBUG))
  cout << "launch: Executing Command: " << command << endl;
#endif

  bool ret;

  string output;
  string errMessage;

  const MachineConfig *conf = MachineConfig::userPref(p_cache->machineName);
  if (conf != (MachineConfig*)0) {
    string password;
    if (p_cache->remoteShell=="Globus-ssh" ||
        p_cache->remoteShell.find("Globus-ssh/")==0)
      password = (*p_options)["##password2##"];
    else
      password = (*p_options)["##password1##"];

    RefMachine* refMachine = RefMachine::refLookup(p_cache->machineName);

    ret = RCommand::globusrun(command.c_str(), output, errMessage,
                              p_cache->fullMachineName,
                              refMachine->globusContact(), password, queueRSL);
    if (!ret) {
      p_lastMessage = errMessage;
      if (output != "") {
        p_lastMessage += "\nJob submission output: ";
        p_lastMessage += output;
      }
    }
  } else {
    p_lastMessage = "No current " + p_cache->remoteShell;
    p_lastMessage += " machine configuration for " + p_cache->userName;
    p_lastMessage += " on " + p_cache->machineName;
    ret = false;
  }

#if (!defined(INSTALL) && defined(DEBUG))
  cout << "launch: Executed " << command << endl;
#endif

  if (ret) {
    // Extract and save Globus job manager URL
    string id = output;
    p_infoMessage = "Globus Job URL is " + id;

    Jobdata jobdata = p_taskjob->jobdata();
    jobdata.jobid = id;
    p_taskjob->jobdata(jobdata);

    p_cache->jobId = id;
  }

#if (!defined(INSTALL) && defined(DEBUG))
  cout << "launch: Executed " << command << endl;
#endif

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Start eccejobstore on local machine after submit.
//
////////////////////////////////////////////////////////////////////////////// 
bool Launch::startJobStore(const string& importDir)
{
  bool ret = true;

  // make files writable so they can be copied over if the job is killed
  // this is called whenever job monitoring is being done (full job
  // launch, reconnect, etc.) so this is the common location for this code
  // rather then in doLaunch.
  (void)p_connection->exec(
              "chmod u+w eccejobmonitor eccejobmonitor.conf *.desc submit__*");

  if (p_cache->frontendFlag) {
    extern void jobstore_main(RCommand* remoteconn, const string& jobId,
                const string& configFileName, const bool& restartFlag);

    pid_t pid;
    string configFileName =  p_cache->directory + "/eccejobstore.conf";

    switch (pid = fork()) {
      case -1:
        p_lastMessage =
          "Unable to fork main launch process for job monitoring";
        return false;

      case 0:
        jobstore_main(p_connection, p_cache->jobId, configFileName, false);
        // should never get here because jobstore_main exits itself rather
        // than returning
        _exit(0);

      default:
        // don't wait on child because it is going to be monitoring the
        // job and that normally completes after the parent launcher is closed
        ret = true;
    }
  } else {
    // nohup added to fix to sh/bash shell issue with exitting ECCE
    string clientCmd = "nohup ./eccejobmaster ";

    string pipeName = AuthCache::pipeName();
    clientCmd += "-pipe " + pipeName + " ";

    if (importDir != "")
      clientCmd += "-importDir ";
    else
      clientCmd += "-remoteDir ";
    clientCmd += p_cache->remoteDir;

    clientCmd += " -calcURL ";
    clientCmd += p_cache->calcURL;

    clientCmd += " -restartTries ";

    // No restarts if it is a prepare task because it is usually local and
    // we want only one debug popup to be generated by the MDPrepare application
    string tries = "0";
    MdTask *mdTask = 0;
    mdTask = dynamic_cast<MdTask*>(EDSIFactory::getResource(p_taskjob->getURL()));
    if (mdTask != 0) {
      ResourceType *rtype = mdTask->getDescriptor();
      if (rtype != (ResourceType*)0) {
        string ctype = rtype->getContentType();
        if (ResourceUtils::stringToContentType(ctype) ==
            ResourceDescriptor::CT_MDPREPARE)
          tries = "1";
      }
      mdTask = 0;
    } 

    // "0" means the default number of restarts and "1" means just run jobstore
    // once 
    clientCmd += tries;

    if (p_cache->jobId != "") {
      clientCmd += " -jobId ";
      clientCmd += p_cache->jobId;
    }

    clientCmd += " -configFile ";
    clientCmd +=  p_cache->directory + "/eccejobstore.conf &";


#if (!defined(INSTALL) && defined(DEBUG))
    cout << "Start eccejobmaster with system(" << clientCmd << ")" << endl;
#endif

    int status = system((char*)clientCmd.c_str());
    if (status<<8 != 0) {
      ret = false;
      p_lastMessage = "Unable to start eccejobmaster with: " + clientCmd;
    }

    updateState();

    // Write the current authentication cache to a pipe for eccejobmaster
    AuthCache::getCache().pipeOut(pipeName);
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Description
//   Update the job state to submitted.
///////////////////////////////////////////////////////////////////////////////
void Launch::updateState(void)
{
  // Check state before setting to submitted, calc may have
  // already failed, or completed.
  ResourceDescriptor::RUNSTATE currentState = p_taskjob->getState();

  // Also switch "created" calcs because that's what they are for imports
  // Lisong 07/24/06: Not sure if setState sending notification will
  // cause problem or not.
  if (currentState <= ResourceDescriptor::STATE_READY)
    p_taskjob->setState(ResourceDescriptor::STATE_SUBMITTED);

    // special STTR reaction rate study logic to reset the state of all
    // follow-on tasks to force input file regeneration because it's likely
    // the chemical system has changed (GEOMTRACE property)
    Resource* parent =EDSIFactory::getResource(p_taskjob->getURL().getParent());
    if (parent->getApplicationType() == ResourceDescriptor::AT_REACTION_STUDY) {
      p_taskjob->resetReactionTasks();
    }

}


// ------------------------
// Private Member Functions
// ------------------------

/////////////////////////////////////////////////////////////////////////////
// Description
//   Some options that can be set in the interface need to be added
//   after the input file is generated.  These have to do with the computing
//   environment more than the chemistry (which is handled in the normal
//   input file generation).  This method, calls the relevent post-processing
//   script to get the job done.  The script takes a command line argument
//   that is a file containing parameters such as the input file name,
//   number of processors and nodes, run directory, machine name, etc.
//
/////////////////////////////////////////////////////////////////////////////
bool Launch::postProcessInput(void)
{
  bool ret = true;

  // generate the file of parameters for the post-processing script
  string paramf = p_cache->directory + "/postParams";
  ofstream os(paramf.c_str(), (ios::out | ios::trunc));

  if (os) {
    p_inputFile = (*p_options)["##input##"];

    os << "inputFile: " << p_inputFile << endl;

    // Number of processors
    string procs = (*p_options)["##numProcs##"];
    if (procs.empty())  procs = "1";
    os << "numProcs: " << procs << endl;

    // Number of nodes
    procs = (*p_options)["##numNodes##"];
    if (procs.empty())  procs = "1";
    os << "numNodes: " << procs << endl;

    os << "runDir: " << p_cache->remoteDir << endl;

    if (p_cache->scratchDir != "")
      os << "scratchDir: " << p_cache->scratchDir << endl;

    os << "machineName: " << p_cache->fullMachineName << endl;

    os.close();

    const JCode* jcode = p_taskjob->application();
    string cmd = jcode->launchPPScript();
    cmd += " -p postParams";

    // cd to the local calculation directory where the input file is
    changeWD(p_cache->directory);

    string output;
    string err = "Unable to execute launch post-processing command: " + cmd;
    ret = p_localconn->execout(cmd, output, err);
    if (!ret) {
      p_lastMessage = p_localconn->commError();
      if (output != "") {
        p_lastMessage += "\nPost-processing output: ";
        p_lastMessage += output;
      }
    } else {
       // Write it back out to dav.
       // I had to use the full path or I got corrupted data.  I don't know why.
       ifstream ifs((p_cache->directory + "/" + p_inputFile).c_str());
       if (!p_taskjob->putInputFile(p_inputFile, &ifs)) {
          p_lastMessage += "Unable to write input file to data server "
                           "after launch time post-processing.";
          ret = false;
       }
       ifs.close();
    }

    // Be tidy and remove the parameter file
    SFile tmp(paramf);
    tmp.remove();
  } else {
    p_lastMessage = "Unable to create param file for input post-processing "
                    "script.";
    ret = false;
  }

  return ret;
}

//////////////////////////////////////////////////////////////////////////
// Description
//   Generate submit script using "gensub".  If there are any errors
//   the are put in the return string.  The dictionary is used so that
//   not all gui attributes have to be stored persistently.
//
// Since ecce paths are so long, now we send all the args in a 
// param file.   This ensures that we won't fail due to shell cmd length
// limitations UNLESS that directory path is so long that we can't even
// specify the param file name with its path.
//////////////////////////////////////////////////////////////////////////
bool Launch::instanceScript(EcceMap& kv)
{
  bool ret = true;
  
  // generate the parameter file
  string paramf = p_cache->directory + "/subParams";
  ofstream os(paramf.c_str(), (ios::out | ios::trunc));
  if (os) {
    string tmpVal;
    os << " -Q " + p_cache->mgr->name() << "\n";
    os << " -H " + p_cache->machineName << "\n";
    os << " -d " + p_cache->fullMachineName << "\n";
    os <<  " -c " + p_cache->codeName << "\n";

    if (kv.count("##queue##") == 1) {
      tmpVal = kv["##queue##"];
    }
    if (!tmpVal.empty()) {
      os << " -q " + tmpVal << "\n";
    }
    if (kv.count("##numProcs##") == 1) {
      os << " -n " + kv["##numProcs##"] << "\n";
    }
    if (kv.count("##numNodes##") == 1) {
      os << " -N " + kv["##numNodes##"] << "\n";
    }
    if (kv.count("##runDir##") == 1) {
      os << " -r " + kv["##runDir##"] << "\n";
    }
    if (kv.count("##maxmemory##") == 1) {
      tmpVal = kv["##maxmemory##"];
    } else {
      tmpVal = "";
    }
    if (!tmpVal.empty()) { 
      os << " -m " + tmpVal << "\n";
    }
    if (kv.count("##minscratch##") == 1) {
      tmpVal = kv["##minscratch##"];
    } else {
      tmpVal = "";
    }
    if (!tmpVal.empty()) { 
      os << " -x " + tmpVal << "\n";
    }
    if (kv.count("##scratch##") == 1) {
      tmpVal = kv["##scratch##"];
    } else {
      tmpVal = "";
    }
    if (!tmpVal.empty()) {
      os << " -s " + tmpVal << "\n";
    }
    if (kv.count("##account_no##") == 1) {
      tmpVal = kv["##account_no##"];
    } else {
      tmpVal = "";
    }
    if (!tmpVal.empty()) {
      os << " -a " + tmpVal << "\n";
    }
    // This is for CPU time - not implemented
    //options += "-t " + "

    if (kv.count("##wall_clock_time##") == 1) {
      tmpVal = kv["##wall_clock_time##"];
    } else {
      tmpVal = "";
    }
    if (tmpVal!="" && tmpVal!="0:0:00") {
      os << " -T " + tmpVal << "\n";
    }
    if (kv.count("##wall_clock_hrmin##") == 1) {
      tmpVal = kv["##wall_clock_hrmin##"];
    } else {
      tmpVal = "";
    }
    if (tmpVal!="" && tmpVal!="0:0") {
      os << " -w " + tmpVal << "\n";
    }

    if (kv.count("##input##") == 1) {
      os << " -i " + kv["##input##"] << "\n";
    }
    if (kv.count("##output##") == 1) {
      os << " -o " + kv["##output##"] << "\n";
    }
    // This is for memory - not yet implemented in gui
    // options += "-m " + "

    os << " -f " + p_cache->directory + "/" + p_cache->scriptName << "\n";

    if (p_cache->isMd) {
      os << " -S " + p_cache->mdSystemName << "\n";
      os << " -C " + p_cache->mdCalcName << "\n";
    }

    // Oh boy this is getting so ugly.  We do this so that gensub will know
    // whether or not to include keywords which it must not do if using globus.
    if (kv.count("##globus##") == 1 && 
        kv["##globus##"] == "true") {
      os <<  " -g\n";
    }
    os.close();

    // Now run gensub and return the results
    string cmd = "gensub -v ";
    cmd += " -p subParams";
    string output;
    ret = p_localconn->execout(cmd, output);
    #if (!defined(INSTALL) && defined(DEBUG))
      cout << "cmd " << cmd << endl;
    #endif
    if (!ret) {
      p_lastMessage = p_localconn->commError();
      if (output != "") {
        p_lastMessage += "\nSubmit script generation failed: ";
        p_lastMessage += output;
      }
    }

    // Be tidy and remove this file.
    SFile tmp(paramf);
    tmp.remove();
  }
  else {
    p_lastMessage="Unable to create param file for submit script generation.";
    ret = false;
  }

  return ret;
}

void Launch::instanceCommand(string& command,
                             EcceMap& kv)
{
  EcceMap::iterator it;
  static EcceRegexp re(".*");

  int foundIdx, substrLength;

  // Pulled this out of the dictionary - VERY clumsy!
  if (command.find("##script##") != string::npos) {
    re = "##script##";
    re.match(command,foundIdx, substrLength);
    command.replace(foundIdx, substrLength, p_cache->scriptName);
  }

  if (command.find("##queueMgr##") != string::npos) {
    re = "##queueMgr##";
    re.match(command, foundIdx, substrLength);
    command.replace(foundIdx, substrLength,  p_cache->mgr->name());
  }

  for (it = kv.begin(); it != kv.end(); it++) {

    // if command contains key
    if (command.find((*it).first) != string::npos) {
      re = (*it).first;  // regex = key
      re.match(command, foundIdx, substrLength);

      // replace matching substring with value
      command.replace(foundIdx, substrLength, (*it).second);
    }
  }
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                      Class CalcInfo
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const CalcInfo& calcInfo)
{
  os << "machineName: " << calcInfo.machineName << endl;
  os << "scriptName: " << calcInfo.scriptName << endl;
  os << "codeName: " << calcInfo.codeName << endl;
  os << "directory: " << calcInfo.directory << endl;
  os << "userName: " << calcInfo.userName << endl;
  os << "remoteDir: " << calcInfo.remoteDir << endl;
  os << "scratchDir: " << calcInfo.scratchDir << endl;
  os << "mgr: " << *(calcInfo.mgr) << endl;
  return os;
}


