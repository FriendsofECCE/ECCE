///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: RunMgmt.C
//
//
// DESIGN:
//
///////////////////////////////////////////////////////////////////////////////

// system includes
#include <signal.h>
#include <unistd.h>
#include <vector>

#include <iostream>
using namespace std;

// application includes
#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/Host.H"
#include "util/TempStorage.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"

#include "tdat/RefMachine.H"
#include "tdat/RefQueueMgr.H"
#include "tdat/QueueMgr.H"

#include "dsm/MachinePreferences.H"
#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/CodeFactory.H"

#include "comm/RCommand.H"
#include "comm/RunMgmt.H"

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Kill a job launched by ecce.
//
//    The following unique cases must be handled at this time:
//      . single executable code (NWChem) on single node machine
//      . single executable code (NWChem) on multi-node node machine
//        (ie running Parallel)
//      . any batch code on queued system (Maui, LL, NQE, NQS)
//      . linked executable code (g94) on single node machine
//      . linked executable code (g94) on multiple node machine
//
//  Implementation
//    NWChem catches and handles the -INT signal.  If you do not use
//    -INT (say -TERM), then parallel will not exit gracefully and
//    shared memory will not be freed up.  However since we send the
//    signal to the entire process group, things seem to work properly
//    so I have converted to using SIGTERM.
//
//    NO LONGER TRUE - TERM now handled properly.
//
//    Gaussian apparently catches and ignores -INT.  To kill you must
//    use -TERM.  NOTE that -TERM is not catchable in csh scripts
//    so the job script itself cannot handle this case gracefully.
//
///////////////////////////////////////////////////////////////////////////////
string RunMgmt::terminate(const TaskJob *calc)
{
  string ret;
  string id;
  string signal = "TERM";
  string SIGNAL = "SIGTERM";
  Jobdata const job =  calc->jobdata();
  
  id = job.jobid;
  if (id != "") {
    // Need to get the Queue Manager command to do the terminate
    string command;
    string qMgrName = "Shell";

    const Launchdata& launch = calc->launchdata();

    RefMachine* refMachine = RefMachine::refLookup(launch.machine);
    EE_ASSERT(refMachine,EE_WARNING,"No RefMachine object");

    bool isGlobus = RCommand::isRemote(refMachine->fullname(),
                     launch.remoteShell, launch.user) && 
                     launch.remoteShell=="Globus";

    if (!isGlobus) {
      const QueueManager* qMgr = 
        QueueManager::lookup(launch.machine);
      if (qMgr) qMgrName = qMgr->queueMgrName();
      const RefQueueManager *rqMgr = 
        RefQueueManager::lookup(qMgrName);

      if (rqMgr) command = rqMgr->cancelCommand();

      // Now if you have a command, do id substitutions. 
      int loc;
      while ((loc=command.find("##id##")) != string::npos)
        command.replace(loc,6,id);

      if ((loc=command.find("##signal##")) != string::npos)
        command.replace(loc,10,signal);
      if ((loc=command.find("##SIGNAL##")) != string::npos)
        command.replace(loc,10,SIGNAL);
    }

    // Finally issue the command
    RCommand rcmd(refMachine->fullname(), launch.remoteShell,
                  refMachine->shell(), launch.user, "",
                  refMachine->frontendMachine(),refMachine->frontendBypass(),
                  refMachine->shellPath(), refMachine->libPath(),
                  refMachine->sourceFile());

    // Write .ecce.status file with killed state.  This should someday all
    // be moved to a killed script.  There are possible timing problems
    // with this strategy.
    if (rcmd.isOpen()) {
      if (rcmd.directory(job.jobpath)) {
        string fixStatus = "echo 302 >! ";
        fixStatus += job.jobpath + "/.ecce.status";
        if (rcmd.exec(fixStatus)) {
          string outstr;
          if (!isGlobus) {
            if (rcmd.execout(command, outstr))
              ret = "Request to terminate job " + id + " on " +
                    launch.machine + " has been issued";
            else {
              // If the job has already completed then the kill command
              // will return "No such process" and no $status
              if (outstr.find("such process") != string::npos)
                ret = "Request to terminate job " + id + " on " +
                      launch.machine + " not issued--process not found";
              else {
                ret = rcmd.commError();
                if (ret.find("No status returned") != string::npos);
                  ret = "Request to terminate job " + id + " on " +
                        launch.machine + " not issued--process not found";
              }
            }
          } else {
            command = "globusrun -kill ";
            command += id;
            RCommand lcmd;
            if (lcmd.isOpen() &&
                lcmd.execout(command, outstr))
              ret = "Request to terminate Globus job " + id + " on " +
                    launch.machine + " has been issued";
            else
              ret = "Request to terminate Globus job " + id + " on " +
                    launch.machine+" could not be issued:\nError output: "
                    + outstr;
          }
        } else
          ret = rcmd.commError();
      } else
        ret = "Request to terminate job " + id + " on " + launch.machine +
            " not issued--run directory " + job.jobpath + " does not exist";
    } else
      ret = rcmd.commError();

    delete refMachine;
  } else
    ret = "Request to terminate job not issued--unable to lookup job id";

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//
//  Implementation
//    It may be better to require the caller to send in the dirs
//    to shorten the txn in the caller.
///////////////////////////////////////////////////////////////////////////////
string RunMgmt::cleanServerDirs(const TaskJob *calc)
{
  string ret;

  Jobdata job = calc->jobdata();
  if (!job.jobpath.empty()) {

    string remoteDir = job.jobpath;

    const Launchdata& launch = calc->launchdata();

    // Check to see if the "tail" of the remote directory path has been set.
    // If not deleting would be very bad as Theresa Windus found out the hard
    // way when the rm used to be done recursively.
    // With the changed model, just look to see of jobpath is set.
    if (remoteDir != "") {
      string output;
      string cmd1 = "/bin/rm -f " + remoteDir + "/{*,.ecce.status}";
      string cmd2 = "/bin/rmdir " + remoteDir;
      RefMachine* refMachine = RefMachine::refLookup(launch.machine);

      EE_ASSERT(refMachine,EE_WARNING,"No RefMachine object");
      RCommand rcmd(launch.machine, launch.remoteShell, refMachine->shell(),
                    launch.user, "", refMachine->frontendMachine(),
                    refMachine->frontendBypass());
      if (rcmd.isOpen()) {
        if (rcmd.directory(remoteDir)) {
          if (!rcmd.execout(cmd1, output)) {
            ret = "Could not remove calculation directory contents--";
            ret += output;
          } else if (!rcmd.execout(cmd2, output)) {
            ret = "Could not rmdir calculation directory--";
            ret += output;
          }
        } else {
          ret = "Calculation directory does not exist";
        }
      }
    } else {
      ret = "Could not remove calculation directory contents--"
        "machine information not available";
    }
  }

  return ret;
}

bool RunMgmt::jobStoreRunning(const string& jobID)
{
  bool ret = false;

  RCommand lcmd;
  if (lcmd.isOpen())
    ret = lcmd.exec("ps -eo args | grep 'eccejobstore -jobId " + jobID +
                    " ' | grep -v grep");

  return ret;
}


/**
 * Register the local machine if not already registered.
 * This allows import to work without user intervention.
 * Attempts to register paths to codes such as the bundled version of NWchem
 * for users outside PNNL.
 *
 * Algorithm is:
 *   . try to get fully qualified name
 *   . if successful
 *      lookup registration
 *      if successful then done and return true with empty message
 *      else
 *        use hostname only
 *        lookup registration
 *        if successful then done
 *        else add it and return true together with a message
 *          if failed to add, return false together with a message
 */
bool RunMgmt::registerLocalMachine(string& msg)
{
  bool succeeded = true;

  Host myhost;
  string whereami = myhost.host_name();
  string fullwhereami = myhost.fullyQualifiedName();
  string architecture = myhost.machine();

  RefMachine* refMachine = (RefMachine*)0;

  // Lookup fully qualified name if we think we have one
  if (fullwhereami.find(whereami)==0  && fullwhereami!=whereami)
    refMachine = RefMachine::fullLookup(fullwhereami);
  
  // If we don't have a hit, try the plain old hostname
  if (refMachine == (RefMachine*)0)
    refMachine = RefMachine::abbrevLookup(whereami);

  // If we still don't have a hit, time to add it
  if (refMachine == (RefMachine*)0) {
    // Bail if no valid machine name was find to avoid corrupting
    // the MyMachines file
    if (fullwhereami != "") {

      // Assume fullwhereami is machine name if no domain name available
      string machine = fullwhereami;
      string refname = machine;
      int idx = refname.find('.');
      if (idx != string::npos)
        refname = refname.substr(0, idx);

      string settings = "type=accept&siteconfig=false&machine=";
      settings += machine;

      settings += "&name=";
      settings += refname;

      string model = "Unspecified";
      string vendor = "Unspecified";

      string sysname = myhost.system_name();
      if (sysname.find("Linux") != string::npos) 
        vendor = "Linux";
      else if (sysname.find("IRIX") != string::npos) 
        vendor = "SGI";
      
      // See if it is an O2 (uname machine will be IP32)
      if (architecture == "IP32")
        model = "O2";

      settings += "&vendor=" + vendor + "&model=" + model;
      settings += "&processor=Unspecified&procs=1&nodes=1&rsh=false&ssh=true&sshftp=false&telnet=false&gsh=false";

      // Within PNNL, use the AFS installations of codes as defined in the
      // CONFIG.<vendor> files.
      bool emslFlag = false;
      // Even though a machine has a .pnl.domain does not mean it is using
      // the EMSL maintained version of ECCE or has AFS access.  GDB 3/24/12
      if (machine.find(".pnl.gov")==machine.length()-8 &&
          RefMachine::configFile("LINUX")!="") {
        string code;
        string paths = "";
        settings += "&registeredcodes=";

        vector <string> codes = CodeFactory::getFullySupportedCodes();

        for (int idx=0; idx<codes.size(); idx++) {
          code = codes[idx];
          if (RefMachine::exePath(code, machine.c_str(),
				  vendor, (model=="Unspecified")? "": model.c_str()) != "") {
            emslFlag = true;
            settings += code + ",";
            paths += "&" + code + "=EMSL";
          }
        }
        settings += paths;
      } else {
        // Outside PNNL, register the bundled version of NWChem
        string path = Ecce::ecceHome();
        path += "/";
        path += getenv("ECCE_SYSDIR");
        path += "3rdparty/";
        path += "nwchem/bin/nwchem";

        settings += "&registeredcodes=NWChem,&NWChem=" + path;
      }

      settings += "&qmgrPath=&AA=false&qmgr=None&numQueues=0";

      string cmd = "echo \"";
      cmd += settings + "\" | processmachine";

      char buf[32];
      sprintf(buf,"CONTENT_LENGTH=%zu", settings.length());
      char *string = strdup(buf);
      putenv(string);

      int status = system(cmd.c_str());
      status = status >> 8;

      if (status == 0) {
        msg = "The machine you are running on, ";
        msg += machine;
        msg += ", has just been added to your list of registered machines as a "
            "single processor " + vendor + " workstation using ssh.  ";
        if (emslFlag)
          msg += "Paths to codes have been set based on EMSL defaults.  ";
        else
          msg += "The NWChem distribution bundled with ECCE will be used.  ";
        msg += "These settings can be changed using the \"Register "
            "Machines...\" option from the Launcher Job menu.\n";

        RefMachine::markUpdateUserPrefs();
      } else {
        msg = "Attempt to register the machine you are on, ";
        msg += machine;
        msg += ", failed.  Please register it using the "
            "\"Register Machines...\" option from the Launcher Job menu "
            "if you wish to import or launch jobs locally.";
        succeeded = false;
      }
    }
  }
  return succeeded;
}


/**
 * This method will sync files associated with a job, to a local cache.
 * @param job the running job
 * @param cacheDir local cacheDir
 * @param filePattern files to sync
 * @return list of all matching files in local cache after update
 */
vector<string> RunMgmt::updateCacheFromDataServer(const TaskJob* job,
      const string& cacheDir, const string& filePattern, string& errorMessage)
throw(EcceException)
{
  // assume no problems come up?
  errorMessage = "";
  vector<string> ret;

#define usepid
#ifdef usepid
  // One cache per pid
  char buf[32];
  sprintf(buf, "%d", getpid());
  string key = buf;
#else
  // One cache per calc
  string key = hash(job->getURL().toString().c_str());
#endif

  string filePatternExt = filePattern.substr(filePattern.size() - 4);
  string patternLower;
  string patternUpper;
  StringConverter::toLower(filePatternExt, patternLower);
  StringConverter::toUpper(filePatternExt, patternUpper);

  vector<EcceURL> files = job->getDataFiles(JCode::OUTPUT);
  for (vector<EcceURL>::iterator it = files.begin(); it != files.end(); it++) {
    if (it->toString().find(patternLower) != string::npos
            || it->toString().find(patternUpper) != string::npos) {
      string path = cacheDir + "/" + key + it->getFilePathTail();
      SFile *local = new SFile(path);
      if (!local->exists()) {
        local = job->getAnyFile(it->getFilePathTail(), local);
      }
      delete local;
    }
  }

  return ret;
}


/**
 * This method will sync files associated with a job, to a local cache.
 * @param job - the job that is running.
 * @param cacheDir - local cacheDir
 * @param filePattern - files to sync
 * @return list of all matching files in local cache after update
 */
vector<string> RunMgmt::updateCacheFromComputeHost(const TaskJob* job,
      const string& cacheDir, const string& filePattern, string& errorMessage) 
throw(EcceException)
{
   // assume no problems come up :)
   errorMessage = "";

   vector<string> ret;

   Launchdata ldata = job->launchdata();
   Jobdata jdata = job->jobdata();

   // jdata.jobpath contains full calculation path to the files
   RefMachine* refMachine = RefMachine::refLookup(ldata.machine);

   if (refMachine) {
      RCommand* rcmd = new RCommand(refMachine->fullname(),
                                    ldata.remoteShell, refMachine->shell(),
                                    ldata.user, "",
                                    refMachine->frontendMachine(),
                                    refMachine->frontendBypass());

      if (rcmd->commError() == "") {
         if (rcmd->cd(jdata.jobpath)) {
            string hopMachine = "";
            if (job->getState()==ResourceDescriptor::STATE_RUNNING &&
                rcmd->exists("hopnode.out")) {
              if (rcmd->execout("cat hopnode.out", hopMachine)) {
                rcmd->hop(hopMachine, refMachine->shell(), ldata.user,
                          "");
                if (rcmd->commError() != "") {
                  // could not connect
                  errorMessage = "Unable to transfer trajectory files "
                          "from head compute node " + hopMachine + ": " +
                          rcmd->commError();
                } else if (!rcmd->cd("/scratch")) {
                  // could not cd
                  errorMessage = "Unable to transfer trajectory files "
                                 "from head compute node " + hopMachine +
                                 ": change directory to /scratch failed";
                }
              }
            }

            if (errorMessage != "") {
              delete rcmd;
              return ret;
            }

            string lloutput;
            if (rcmd->execout("/bin/ls -l " + filePattern, lloutput)) {
              string rname, rsize;
              string::size_type rpos;
              EcceMap fileMap;
              EcceMap::iterator it;

              char buf[32];
#define usepid
#ifdef usepid
              // One cache per pid
              sprintf(buf, "%d", getpid());
              string key = buf;
#else
              // One cache per calc
              string key = hash(job->getURL().toString().c_str());
#endif

              // Build hash map of remote file sizes
              // Can't nest StringTokenizer instances because internal
              // strtok implementation gets confused and doesn't work right,
              // so the outer tokenizer must be done another way
              string rline = lloutput;
              while (!rline.empty()) {
                StringTokenizer rfields(rline, " \t\n\r");
                // skip passed the fields of no interest
                rfields.next(); rfields.next();
                rfields.next(); rfields.next();
                rsize = rfields.next();

                // more fields to ignore
                rfields.next(); rfields.next(); rfields.next();
                rname = rfields.next();
                if (rname!="" && rsize!="")
                  fileMap[key + rname] = rsize;

                // get the next line
                if ((rpos = rline.find('\n')) != string::npos)
                  rline = rline.substr(rpos+1, rline.size()-rpos);
                else
                  rline = "";
              }
              int remoteFileCount = fileMap.size();

              // Iterate through local cached files and delete entries
              // from the remote file hash map where there is a local
              // file with the same name and size
              RCommand lcmd("system");
              if (lcmd.cd(cacheDir)) {
                if (lcmd.execout("/bin/ls -l "+filePattern, lloutput)) {
                  string lname, lsize;
                  string::size_type lpos;
                  string lline = lloutput;
                  while (!lline.empty()) {
                    StringTokenizer lfields(lline, " \t\n\r");
                    // skip passed the fields of no interest
                    lfields.next(); lfields.next();
                    lfields.next(); lfields.next();
                    lsize = lfields.next();

                    // more fields to ignore
                    lfields.next(); lfields.next(); lfields.next();
                    lname = lfields.next();

                    // delete files from map with the same local/remote size
                    if ((it = fileMap.find(lname)) != fileMap.end()) {
                      if (it->second == lsize) {
                        fileMap.erase(it);

                        // it is important to return the full list
                        // of local files and not just the new ones
                        // transferred over
                        // Therefore, make sure to add the ones being
                        // dropped from the transfer to the list!
                        ret.push_back(cacheDir + "/" + lname);
                      }
                    }

                    // get the next line
                    if ((lpos = lline.find('\n')) != string::npos)
                      lline = lline.substr(lpos+1, lline.size()-lpos);
                    else
                      lline = "";
                  }
                }
              }

              // add the remaining elements into the list of files
              // to be transferred
              vector<string> fullFiles;
              vector<string> baseFiles;
              string noPidName;
              string fullFile;
              int fullFileLength = 0;

              for (it = fileMap.begin(); it != fileMap.end(); it++) {
                noPidName = it->first.substr(key.size(),
                                             it->first.size() - key.size());
                fullFile = jdata.jobpath + "/" + noPidName;
                fullFileLength += fullFile.size();
                fullFiles.push_back(fullFile);
                baseFiles.push_back(noPidName);
                ret.push_back(cacheDir + "/" + it->first);
              }

              // do the file transfer of those that have changed since
              // the last update
              if (fullFiles.size() > 0) {
                string err, cmd;
                bool copyGoodFlag;
                int idx;

                // for a "hop" based file transfer, first do a local copy
                // of the files back to the run directory
                if (hopMachine != "") {
                  string cmd;
                  for (idx=0; idx < baseFiles.size(); idx++) {
                    cmd = "/bin/cp -f " + baseFiles[idx] + " " +
                          jdata.jobpath;
                    (void)rcmd->exec(cmd);
                  }
                }

                bool frontendFlag = refMachine->singleConnect() ||
                                   (refMachine->frontendMachine()!="" &&
                                    (refMachine->frontendBypass()=="" ||
                      !RCommand::isSameDomain(refMachine->frontendBypass())));

                // check whether just a subset of files are to be
                // transferred or if the total length of the file names
                // to be transferred is more than 18,000 characters.  The
                // former indicates that it's a refresh operation rather
                // than the calcviewer first coming up while the latter
                // is because on SGI at least scp (and actually even
                // wildcard file listings) fails somewhere between 18,000
                // and 19,000 characters when the file names are expanded
                // in my testing.  Since transferring more than 100 batch
                // files, which allows 180 characters per file, is rare,
                // this doesn't seem that likely to cause much of a
                // real-world issue.
                if (fullFiles.size()<remoteFileCount ||
                    fullFileLength > 18000) {
                  if (ldata.remoteShell.find("shellcp")!=string::npos ||
                      frontendFlag)
                    copyGoodFlag = rcmd->shellget(fullFiles, cacheDir);
                  else
                    copyGoodFlag = RCommand::get(err, refMachine->fullname(),
                                          ldata.remoteShell, ldata.user,
                                          "", fullFiles, cacheDir);
                } else {
                  // use a wildcard specification which greatly speeds up
                  // file transfer if all files are being copied back and
                  // the total length of the files names is 18,000 chars
                  vector<string> starFiles;
                  starFiles.push_back(jdata.jobpath + "/" + filePattern);
                  if (ldata.remoteShell.find("shellcp")!=string::npos ||
                      frontendFlag)
                    copyGoodFlag = rcmd->shellget(starFiles, cacheDir);
                  else
                    copyGoodFlag = RCommand::get(err, refMachine->fullname(),
                                          ldata.remoteShell, ldata.user,
                                          "", starFiles, cacheDir);
                }

                if (copyGoodFlag) {
                   // rename the files with the pid prepended to
                   // guarantee unique names per calcviewer session
                   for (idx = 0; idx < baseFiles.size(); idx++) {
                     cmd = "/bin/mv -f " + baseFiles[idx] + " " +
                                           key + baseFiles[idx];
                     (void)lcmd.exec(cmd);
                   }
                } else {
                   // could not get files
                   errorMessage = "Unable to transfer trajectory files: " + err;
                }
              }
            } else {
               // could not ls
               errorMessage = "Unable to transfer trajectory files: "
                              "directory listing of " + filePattern + " failed";
            }
         } else {
            // could not cd
            errorMessage = "Unable to transfer trajectory files: "
                           "change directory to " + jdata.jobpath + " failed";
         }
      } else {
         // could not connect
         errorMessage = "Unable to connect to compute host "
                        "to transfer trajectory files: " +
                        refMachine->fullname() + ": " + rcmd->commError();
      }

      delete rcmd;
   }

   return ret;
}


vector<string> RunMgmt::updateCacheFromDataArchive(const TaskJob* job,
      const string& cacheDir, const string& filePattern, string& errorMessage) 
throw(EcceException)
{
  vector<string> ret;

  // Using 'create' here assures us prefs will not be null, even
  // if it is empty.
  MachinePreferences *prefs = MachinePreferences::lookup("nwfs");
  if (prefs != NULL) {
    string cpArchiveName = "n2.emsl.pnl.gov";
    string cpArchiveShell = "sftp";
    string cpArchiveDir = prefs->getRemoteDirectory();
    string cpArchiveUserName = prefs->getUsername();
    cpArchiveDir += TempStorage::getJobRunDirectoryPath(job->getURL());
    cpArchiveDir += "/" + filePattern;

    char *fileStrs[2];
    fileStrs[0] = (char*)cpArchiveDir.c_str();
    fileStrs[1] = NULL;

    string err;
    bool goodGet = RCommand::get(err, cpArchiveName, cpArchiveShell,
                                 cpArchiveUserName, "",
                                 (const char**)fileStrs, cacheDir);

    if (goodGet) {
      RCommand lcmd("system");
      if (lcmd.cd(cacheDir)) {
        string lsoutput;
        if (lcmd.execout("/bin/ls -1 " + filePattern, lsoutput)) {
          // Surprisingly, data has \r character
          StringTokenizer tokenizer(lsoutput, " \n\r\t");
          string file;
          while (!(file=tokenizer.next()).empty()) {
            ret.push_back(cacheDir + "/" + file);
          }
        }
      }
    } else {
      // could not get files
      errorMessage = "Unable to transfer " + filePattern + " files: " + err;
    }
  } else {
    // must set an errorMessage for the caller to know operation failed
    errorMessage = "Unable to transfer " + filePattern +
                   " files: NWFS access not configured";
  }

  return ret;
}

