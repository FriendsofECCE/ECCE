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
#include <cstdio>
#include <ctime>

#include <iostream>
using namespace std;

// application includes
#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/Host.H"
#include <thread>
#include "util/ProcessMachine.H"
#include "util/TempStorage.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"
#include "util/TypedFile.H"

#include "tdat/RefMachine.H"
#include "tdat/RefQueueMgr.H"
#include "tdat/QueueMgr.H"

#include "dsm/MachinePreferences.H"
#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/CodeFactory.H"

#include "comm/CodePrograms.H"
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
    if (!refMachine) {
      // EE_ASSERT above is EE_WARNING, not EE_FATAL -- it logs (or, in an
      // INSTALL build, does nothing at all) but never stops execution, so
      // this guard is the only thing standing between a legitimately
      // missing machine registration and a null dereference below. Real,
      // reachable case: a job's persisted launch.machine (e.g. an old
      // "127.0.0.1") no longer matches any currently-registered machine
      // refname/fullname (e.g. after renaming the registration to a real
      // hostname) -- refLookup() returns null by design in that case, not
      // as a bug.
      return "Request to terminate job " + id + " not issued--machine \"" +
             launch.machine + "\" is not currently registered";
    }

    {
      const QueueManager* qMgr = 
        QueueManager::lookup(launch.machine);
      if (qMgr) qMgrName = qMgr->queueMgrName();
      const RefQueueManager *rqMgr = 
        RefQueueManager::lookup(qMgrName);

      if (rqMgr) command = rqMgr->cancelCommand();
#ifdef _WIN32
      // MSYS ps has no -o, but the job id is its own process group leader
      // (spawnDetached runs it under setsid), so the group id is the id.
      {
        size_t pg = command.find("-`ps ");
        if (pg != string::npos) {
          size_t end = command.find("`;", pg);
          if (end != string::npos)
            command.replace(pg, end + 1 - pg, "-##id##");
        }
      }
#endif

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
        // ">!" is csh/tcsh-only ("clobber even if noclobber is set");
        // bash doesn't recognize it as part of the redirect operator at
        // all, so "echo 302 >! /path/.ecce.status" was parsed as
        // "redirect to a file literally named '!', then pass the real
        // path as a second, meaningless argument to echo" -- confirmed
        // live: it silently wrote a file named "!" instead of
        // .ecce.status (a different bug than the exec() failure this
        // command sometimes also produces, but real either way). Plain
        // ">" behaves identically in both shells for this purpose --
        // noclobber isn't on by default in either, and even if a user's
        // shell config enabled it, there's no reason this internal,
        // ECCE-controlled status write should ever want to fail because
        // of it.
        // Before the status file: the monitor may report 302 the moment
        // it appears, and eccejobstore calls that a cancel only with this
        // set.  Withdrawn below if the request could not be made.
        const_cast<TaskJob*>(calc)->killRequested(true);
        string fixStatus = "echo 302 > ";
        fixStatus += job.jobpath + "/.ecce.status";
        if (rcmd.exec(fixStatus)) {
          string outstr;
          if (rcmd.execout(command, outstr)) {
            ret = "Request to terminate job " + id + " on " +
                  launch.machine + " has been issued";
          }
          else {
            // If the job has already completed then the kill command
            // will return "No such process" and no $status
            if (outstr.find("such process") != string::npos)
              ret = "Request to terminate job " + id + " on " +
                    launch.machine + " not issued--process not found";
            else {
              ret = rcmd.commError();
              if (ret.find("No status returned") != string::npos)
                ret = "Request to terminate job " + id + " on " +
                      launch.machine + " not issued--process not found";
            }
          }
        } else
          ret = rcmd.commError();
      } else
        ret = "Request to terminate job " + id + " on " + launch.machine +
            " not issued--run directory " + job.jobpath + " does not exist";
    } else
      ret = rcmd.commError();

    // The Shell cancel command ends with a second kill that fails once the
    // first has worked, so "process not found" still means the user's
    // request went through.
    if (ret.find("has been issued") == string::npos &&
        ret.find("process not found") == string::npos)
      const_cast<TaskJob*>(calc)->killRequested(false);

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
// True when `text` holds `id` as a word of its own (a queue id may be
// listed with or without its ".server" suffix).
static bool holdsId(const string& text, const string& id)
{
  const string shortId = id.substr(0, id.find('.'));
  for (const string& want : {id, shortId}) {
    if (want.empty()) continue;
    for (size_t at = text.find(want); at != string::npos;
         at = text.find(want, at + 1)) {
      const bool before = at == 0 || !isalnum((unsigned char)text[at - 1]);
      const size_t end = at + want.size();
      const bool after = end >= text.size() ||
                         !isalnum((unsigned char)text[end]);
      if (before && after) return true;
    }
  }
  return false;
}


RunMgmt::MachineCheck RunMgmt::checkOnMachine(const TaskJob *calc)
{
  MachineCheck c;
  Jobdata job = calc->jobdata();
  const Launchdata& launch = calc->launchdata();
  if (job.jobid.empty() || job.jobpath.empty()) {
    c.error = "No job information available";
    return c;
  }
  RefMachine* refMachine = RefMachine::refLookup(launch.machine);
  if (!refMachine) {
    c.error = "Machine \"" + launch.machine + "\" is not currently registered";
    return c;
  }
  string qMgrName = "Shell";
  const QueueManager* qMgr = QueueManager::lookup(launch.machine);
  if (qMgr) qMgrName = qMgr->queueMgrName();
  const bool shell = qMgrName == "Shell";
  c.what = (shell ? string("Process ") : qMgrName + " job ") + job.jobid;

  TypedFile out;
  calc->getDataFile(JCode::PRIMARY_OUTPUT, out);
  c.outputName = out.name();

  // One script, one round trip: the run directory, the clock, the output's
  // time and size (perl: the monitor needs it there anyway, and stat's
  // options differ between GNU and BSD), the status file, then the job.
  string script = "cd " + RCommand::quotePath(job.jobpath) +
                  " || { echo 'ecce-check nodir'; exit 0; }\n"
                  "echo \"ecce-check now `date +%s`\"\n";
  if (!c.outputName.empty())
    script += "perl -e '@s = stat($ARGV[0]); "
              "print \"ecce-check out $s[9] $s[7]\\n\" if @s' " +
              RCommand::quotePath(c.outputName) + "\n";
  script += "if [ -f .ecce.status ]; then echo \"ecce-check status "
            "`head -c 32 .ecce.status | tr -d '\\n'`\"; fi\n"
            "echo 'ecce-check job-begin'\n";
  if (shell) {
    script += "ps -p " + job.jobid + " 2>&1\n";
  } else {
    string query;
    const RefQueueManager* rqMgr = RefQueueManager::lookup(qMgrName);
    if (rqMgr) query = rqMgr->queryJobCommand();
    size_t loc;
    while ((loc = query.find("##id##")) != string::npos)
      query.replace(loc, 6, job.jobid);
    while ((loc = query.find("##user##")) != string::npos)
      query.replace(loc, 8, launch.user);
    script += (query.empty() ? string("true") : query) + " 2>&1\n";
  }
  script += "echo 'ecce-check job-end'\n";

  RCommand rcmd(refMachine->fullname(), launch.remoteShell,
                refMachine->shell(), launch.user, "",
                refMachine->frontendMachine(), refMachine->frontendBypass(),
                refMachine->shellPath(), refMachine->libPath(),
                refMachine->sourceFile());
  delete refMachine;
  string output;
  if (!rcmd.isOpen()) {
    c.error = rcmd.commError();
    return c;
  }
  rcmd.execout(script, output, "", 60);
  if (output.find("ecce-check") == string::npos) {
    c.error = rcmd.commError().empty() ? output : rcmd.commError();
    return c;
  }
  c.reached = true;
  if (output.find("ecce-check nodir") != string::npos) {
    c.error = "The run directory " + job.jobpath + " does not exist";
    return c;
  }

  bool inJob = false;
  StringTokenizer lines(output, "\n");
  vector<string> all = lines.tokenize("\n");
  for (string line : all) {
    // A transport may hand the lines back with CRLF.
    while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
      line.pop_back();
    if (line.compare(0, 15, "ecce-check now ") == 0) {
      c.machineNow = atoll(line.c_str() + 15);
    } else if (line.compare(0, 15, "ecce-check out ") == 0) {
      sscanf(line.c_str() + 15, "%lld %lld", &c.outputTime, &c.outputSize);
    } else if (line.compare(0, 18, "ecce-check status ") == 0) {
      c.status = line.substr(18);
    } else if (line == "ecce-check job-begin") {
      inJob = true;
      c.present = 0;
    } else if (line == "ecce-check job-end") {
      inJob = false;
    } else if (inJob && shell) {
      // As eccejobmonitor reads it: the PID column, after an MSYS status
      // letter if there is one.
      StringTokenizer t(line);
      vector<string> f = t.tokenize(" \t");
      if (!f.empty() && !isdigit((unsigned char)f[0][0]) && f[0] != "PID")
        f.erase(f.begin());
      if (!f.empty() && f[0] == job.jobid) {
        c.present = 1;
        c.jobLine = line;
      }
    } else if (inJob && c.present != 1 && holdsId(line, job.jobid)) {
      // "Unknown Job Id 123" names the id too.
      string l;
      StringConverter::toLower(line, l);
      if (l.find("unknown") == string::npos &&
          l.find("invalid") == string::npos &&
          l.find("not found") == string::npos &&
          l.find("error") == string::npos) {
        c.present = 1;
        c.jobLine = line;
      }
    }
  }
  return c;
}


static string clockTime(long long t)
{
  time_t tt = (time_t)t;
  struct tm tms;
  localtime_r(&tt, &tms);
  char buf[16];
  strftime(buf, sizeof buf, "%H:%M", &tms);
  return buf;
}


static string agoText(long long seconds)
{
  if (seconds < 0) seconds = 0;
  if (seconds < 120) return std::to_string(seconds) + " s ago";
  if (seconds < 7200) return std::to_string(seconds / 60) + " min ago";
  return std::to_string(seconds / 3600) + " h ago";
}


vector<string> RunMgmt::MachineCheck::describe() const
{
  vector<string> out;
  if (!reached) {
    out.push_back("Could not check the machine: " +
                  (error.empty() ? string("no reply") : error));
    return out;
  }
  if (!error.empty()) {
    out.push_back(error + ".");
    return out;
  }
  if (present == 1)
    out.push_back(what + " is still on the machine.");
  else if (present == 0)
    out.push_back(what + " is no longer on the machine.");
  else
    out.push_back("Whether " + what + " is still there is not known.");
  if (outputTime > 0) {
    string size;
    if (outputSize >= 0)
      size = outputSize < 10240
             ? ", " + std::to_string(outputSize) + " bytes"
             : ", " + std::to_string(outputSize / 1024) + " kB";
    out.push_back("Output " + outputName + " last written " +
                  clockTime(outputTime) + " (" +
                  agoText(machineNow - outputTime) + ")" + size + ".");
  } else if (!outputName.empty()) {
    out.push_back("No output file " + outputName + " yet.");
  }
  if (!status.empty())
    out.push_back("The job script has ended (status " + status + ").");
  if (present == 0 || !status.empty())
    out.push_back("Reconnect reads the output to its end and sets the "
                  "final state.");
  return out;
}


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
      if (!refMachine) {
        // See RunMgmt::terminate() for why this guard is necessary despite
        // the EE_ASSERT above: it's EE_WARNING, not EE_FATAL, so it never
        // stops execution on its own. A job whose persisted launch.machine
        // no longer matches any currently-registered machine (e.g. after a
        // machine rename) hits this legitimately, not as a bug.
        return "Could not remove calculation directory contents--"
               "machine \"" + launch.machine + "\" is not currently registered";
      }
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
#ifdef _WIN32
  // The Windows client runs its jobs here, never over ssh to itself: the
  // machine is "localhost" (siteconfig/Machines ships it), with this
  // computer's processor count if it has to be added.
  whereami = fullwhereami = "localhost";
#endif

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
      // Short name = the first label of a host name; an IP address has
      // no such thing (127.0.0.1 would become "127").
      bool isAddress = refname.find_first_not_of("0123456789.") ==
                       string::npos;
      int idx = refname.find('.');
      if (idx != string::npos && !isAddress)
        refname = refname.substr(0, idx);

      typedef ProcessMachine PM;
      string settings = "type=accept" + PM::field("siteconfig", "false") +
                        PM::field("machine", machine) +
                        PM::field("name", refname);

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

      settings += PM::field("vendor", vendor) + PM::field("model", model);
#ifdef _WIN32
      unsigned cores = std::thread::hardware_concurrency();
      string procs = std::to_string(cores > 0 ? cores : 1);
#else
      string procs = "1";
#endif
      settings += "&processor=Unspecified&procs=" + procs +
                  "&nodes=1&ssh=true&sshftp=false";

      // Within PNNL, use the AFS installations of codes as defined in the
      // CONFIG.<vendor> files.
      bool emslFlag = false;
      vector<string> foundCodes;
      // Even though a machine has a .pnl.domain does not mean it is using
      // the EMSL maintained version of ECCE or has AFS access.  GDB 3/24/12
      if (machine.find(".pnl.gov")==machine.length()-8 &&
          RefMachine::configFile("LINUX")!="") {
        string code;
        string codeList = "";
        string paths = "";

        vector <string> codes = CodeFactory::getFullySupportedCodes();

        for (int idx=0; idx<codes.size(); idx++) {
          code = codes[idx];
          if (RefMachine::exePath(code, machine.c_str(),
				  vendor, (model=="Unspecified")? "": model.c_str()) != "") {
            emslFlag = true;
            codeList += code + ",";
            paths += PM::field(code, "EMSL");
          }
        }
        settings += PM::field("registeredcodes", codeList) + paths;
      } else {
        // Outside PNNL nothing is bundled; register what is on the PATH.
        string codeList = "";
        string paths = "";
        vector<string> codes = CodeFactory::getMachineCodeNames();
        for (size_t i = 0; i < codes.size(); i++) {
          // A code shipped with ECCE (ECCE-QM) has no path to register.
          if (RefMachine::bundledCode(codes[i], "localhost"))
            continue;
#ifdef _WIN32
          string found;  // PATH search is POSIX-only; Windows keeps the Find button
#else
          string found = CodePrograms::findOnPath(codes[i]);
#endif
          if (found.empty())
            continue;
          codeList += codes[i] + ",";
          paths += PM::field(codes[i], found);
          foundCodes.push_back(codes[i] + " (" + found + ")");
        }
        settings += PM::field("registeredcodes", codeList) + paths;
      }

      settings += "&qmgrPath=&AA=false&qmgr=None&numQueues=0";

      int status = PM::run(settings);

      if (status == 0) {
        msg = "The machine you are running on, ";
        msg += machine;
#ifdef _WIN32
        msg += ", has just been added to your list of registered machines; "
            "calculations run on it directly, with up to " + procs +
            " processors.  ";
#else
        msg += ", has just been added to your list of registered machines as a "
            "single processor " + vendor + " workstation using ssh.  ";
#endif
        if (emslFlag)
          msg += "Paths to codes have been set based on EMSL defaults.  ";
        else if (!foundCodes.empty()) {
          msg += "Programs found on the PATH were registered: ";
          for (size_t i = 0; i < foundCodes.size(); i++)
            msg += (i ? ", " : "") + foundCodes[i];
          msg += ".  ";
        } else
          msg += "No computational codes were found pre-installed; register "
              "paths to the codes you have manually.  ";
        msg += "These settings can be changed using \"Register "
            "Machines...\" in the Organizer's Tools menu.\n";

        RefMachine::markUpdateUserPrefs();
      } else {
        msg = "Attempt to register the machine you are on, ";
        msg += machine;
        msg += ", failed.  Please register it using the "
            "\"Register Machines...\" in the Organizer's Tools menu "
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
                    cmd = "/bin/cp -f " + RCommand::quotePath(baseFiles[idx]) + " " +
                          RCommand::quotePath(jdata.jobpath);
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
                     cmd = "/bin/mv -f " + RCommand::quotePath(baseFiles[idx]) + " " +
                                           RCommand::quotePath(key + baseFiles[idx]);
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

