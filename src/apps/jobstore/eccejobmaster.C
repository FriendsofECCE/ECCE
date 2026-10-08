///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: eccejobmaster.C
//
//
// USAGE:
//  eccejobmaster -importDir importDir -calcURL calcURL -jobId jobId
//                -configFile configFile
//  eccejobmaster -remoteDir remoteDir -calcURL calcURL -jobId jobId
//                -configFile configFile
//
// PURPOSE:
//  eccejobmaster.C manages an instance of eccejobstore.  It invokes and
//  waits on the exit of eccejobstore.  Based on the eccejobstore exit
//  status, eccejobmaster will restart eccejobstore.  This design was
//  adopted due to the complexity of eccejobstore communicating with
//  eccejobmonitor while also writing data to the DAV server and
//  sending JMS IPC messages.  The eccejobmaster client is very
//  simple and is a reliable way to insure that eccejobstore is up and
//  running when it should be.
//
// DESCRIPTION:
//  (To be filled in later.)
///////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <iostream>
  using std::cout;
  using std::endl;
  using std::ios;
#include <fstream>
  using std::ofstream;

#include <string>
  using std::string;

#include "util/Ecce.H"
#ifdef _WIN32
#include "comm/DirectTransport.H"
#endif
#include "util/WaitingJobs.H"
#include "tdat/AuthCache.H"

static string logFileName;

void logEntry(const string& entry)
{
  ofstream logFile(logFileName.c_str(), ios::app);
  if (logFile) {
    time_t clk = time(0);
    struct tm tms;
    (void)localtime_r(&clk, &tms);
    char* timestr = asctime(&tms);
    timestr[strlen(timestr)-1] = '\0';

    logFile << "<" << timestr << "> " << entry.c_str() << endl;
    logFile.close();
  }
}

#ifdef _WIN32
// cmd.exe has no nohup and no sh syntax: run the command in the local sh, in
// the foreground, and return its exit status.  The program path comes from
// ecceBinCommand with backslashes, which sh would eat.
static int winShell(string cmd)
{
  if (cmd.compare(0, 6, "nohup ") == 0) cmd.erase(0, 6);
  for (size_t i = 0; i < cmd.size(); i++)
    if (cmd[i] == '\\') cmd[i] = '/';
  DirectTransport t;
  TransportResult r = t.run(cmd, -1);
  return r.status < 0 ? 127 : r.status;
}
#endif

int main(int argc, char** argv)
{
  // cache authentications to pass to jobstore instances
  AuthCache::getCache().pipeIn(argv[2]);

  // default settings
  int MAX_RESTART_TRIES = 25;
  int MAX_QUICK_TRIES = 5;
  int MAX_QUICK_TIME = 60;
  // a run this long is not a rapid failure: a login node that kills the
  // monitor now and then must not use up the restart budget of a long job
  int RESTART_RESET = 600;

  // possibly overriden by siteconfig/site_runtime settings
  if (getenv("ECCE_JOB_MAXCONNECTS")) {
    MAX_RESTART_TRIES = (int)strtol(getenv("ECCE_JOB_MAXCONNECTS"), NULL, 10);
  }
  if (getenv("ECCE_JOB_MAXQUICKCONNECTS")) {
    MAX_QUICK_TRIES = (int)strtol(getenv("ECCE_JOB_MAXQUICKCONNECTS"), NULL, 10);
  }
  if (getenv("ECCE_JOB_MAXQUICKTIME")) {
    MAX_QUICK_TIME = (int)strtol(getenv("ECCE_JOB_MAXQUICKTIME"), NULL, 10);
  }
  if (getenv("ECCE_JOB_RESTARTRESET")) {
    RESTART_RESET = (int)strtol(getenv("ECCE_JOB_RESTARTRESET"), NULL, 10);
  }

  int it;
  // nohup was added because it seems to fix an sh/bash problem with
  // closing a shell after exitting ECCE. The full path, because the
  // working directory is whatever the launching app had (#107).
  string ejsStart = "nohup " + Ecce::ecceBinCommand("eccejobstore");

  for (it=9; it<argc; it++) {
    ejsStart += " ";
    ejsStart += argv[it];
  }

  // the last argument is the config filename -- use it to determine the
  // directory to specify for log files
  string cacheDir = argv[argc-1];
  string::size_type slash = cacheDir.rfind('/');
  if (slash != string::npos)
    cacheDir.resize(slash);

  string ejsEnd = " > " + cacheDir;
  ejsEnd += "/eccejobstore.log";

  logFileName = cacheDir + "/eccejobmaster.log";
  ofstream logFile(logFileName.c_str(), (ios::out | ios::trunc));
  if (!logFile)
    return 1;

  logFile.close();
  logEntry("eccejobmaster started");

  // Tells session-start catch-up that this calculation is being watched.
  const string calcURL = argc > 6 ? argv[6] : "";
  const int watchFd = WaitingJobs::watch(calcURL);

  string ejsCmd;
  string entry;
  char buf[64];
  int status = 2;
  time_t starttime;
  int is;

  // initialize to something large enough not to trigger an exit
  // until enough restarts have been accumulated.
  int runTimes[MAX_QUICK_TRIES];
  for (it=0; it<MAX_QUICK_TRIES; it++)
    runTimes[it] = 999;
  int sumTimes = 999;

  string tryStr = argv[8];
  int maxTries = (int)strtol(tryStr.c_str(), NULL, 10);
  if (maxTries <= 0)
    maxTries = MAX_RESTART_TRIES;

  pid_t pid;

  // it numbers the runs (and their log files); tries is what maxTries limits
  int tries = 0;
  // 0 done, 3 failed, 5 parked waiting for login (#208): none is retried.
  for (it=0; tries<maxTries && status!=0 && status!=3 && status!=5 &&
       sumTimes>MAX_QUICK_TIME; it++, tries++) {

    ejsCmd = ejsStart;

    string authPipeName = AuthCache::pipeName();
    ejsCmd += " -pipe " + authPipeName;

    if (it > 0) {
      ejsCmd += " -restart";
      ejsCmd += ejsEnd;
      sprintf(buf, ".restart_%d", it);
      ejsCmd += buf;

      sprintf(buf, "%d", it);
      entry = "eccejobstore restart count is ";
      entry += buf;
      logEntry(entry);
    } else
      ejsCmd += ejsEnd;

    ejsCmd += " 2>&1";

    entry = "eccejobstore invoked with system(" + ejsCmd + ")";
    logEntry(entry);

#ifdef _WIN32
    // No fork: the pipe is a plain file here, written before the store runs.
    AuthCache::getCache().pipeOut(authPipeName);
    pid = 1;
    starttime = time(0);
    status = winShell(ejsCmd);
    runTimes[it % MAX_QUICK_TRIES] = time(0) - starttime;
    if (status != 0 && status != 3 && runTimes[it % MAX_QUICK_TRIES] >= RESTART_RESET) {
      sprintf(buf, "%d", (int)runTimes[it % MAX_QUICK_TRIES]);
      entry = "eccejobstore ran for ";
      entry += buf;
      entry += " seconds; restart count reset";
      logEntry(entry);
      tries = 0;
    }
    entry = "eccejobstore exited with status value ";
    sprintf(buf, "%d", status);
    entry += buf;
    logEntry(entry);
    for (sumTimes=0, is=0; is<MAX_QUICK_TRIES; sumTimes+=runTimes[is], is++);
    continue;
#endif
    if ((pid = fork()) == 0) {
      // newly created child process

      // pass authentication cache to jobstore
      AuthCache::getCache().pipeOut(authPipeName);

      _exit(0);
    } else if (pid > 0) {
      // parent/current process
      starttime = time(0);

      // here's where eccejobmaster starts ejs and just patiently waits for
      // it to complete
      status = system(ejsCmd.c_str());
      status = status >> 8;

      runTimes[it % MAX_QUICK_TRIES] = time(0) - starttime;

      if (status != 0 && status != 3 && runTimes[it % MAX_QUICK_TRIES] >= RESTART_RESET) {
        sprintf(buf, "%d", (int)runTimes[it % MAX_QUICK_TRIES]);
        entry = "eccejobstore ran for ";
        entry += buf;
        entry += " seconds; restart count reset";
        logEntry(entry);
        tries = 0;
      }

      entry = "eccejobstore exited with status value ";
      sprintf(buf, "%d", status);
      entry += buf;
      logEntry(entry);

      for (sumTimes=0, is=0; is<MAX_QUICK_TRIES; sumTimes+=runTimes[is], is++);
    }
  }

  entry = "eccejobmaster exited with final status value ";
  entry += buf;

  if (sumTimes <= MAX_QUICK_TIME) {
    sprintf(buf, "\nRestart attempts aborted after %d restarts in %d seconds!",
            MAX_QUICK_TRIES, sumTimes);
    entry += buf;
  }

  // A store that lost the job (status 4) left the calculation waiting for
  // login, so running out of restarts here is not a failure of the job.
  if (status == 4)
    logEntry("Restart budget used up; the calculation stays waiting for "
             "login and is caught up at the next session start");

  logEntry(entry);

  WaitingJobs::unwatch(calcURL, watchFd);

  // delete cacheDir when appropriate
  char* value;
  // check both the status and whether any restarts were done--
  // want to save files whenever a restart was needed
  // note that "it" will be one more than the number of restarts
  bool deleteFlag = status==0 && it<=1;
  if ((value = getenv("ECCE_JOB_LOGMODE")) != NULL) {
    string logmodeStr = value;
    string logmode;
    string::size_type it;
    if ((it = logmodeStr.find(',')) == string::npos)
      logmode = logmodeStr;
    else
      logmode = logmodeStr.substr(0, it);

    if (logmode.find("yes")!=string::npos ||
        logmode.find("true")!=string::npos)
      deleteFlag = false;
    else if (logmode.find("no")!=string::npos ||
             logmode.find("false")!=string::npos)
      deleteFlag = true;
  }

  if (!cacheDir.empty() && cacheDir.length()>5 && deleteFlag) {
    string setdir = cacheDir + "/..";
    // tricky, tricky, tricky
    // must change working directory up a level to succeed in deleting
    // the temporary cache directory
    (void)chdir(setdir.c_str());

    string rmcmd = "/bin/rm -rf " + cacheDir;
#ifdef _WIN32
    (void)winShell("rm -rf " + cacheDir);
#else
    (void)system(rmcmd.c_str());
#endif
  }

  string import = argv[3];
  string importDir; 
  deleteFlag = status==0 && it<=1 && import=="-importDir";
  if (import == "-importDir") {
    importDir = argv[4];

    if (value != NULL) {
      string logmodeStr = value;
      string logmode;
      string::size_type it;
      if ((it = logmodeStr.find(',')) == string::npos)
        logmode = logmodeStr;
      else
        logmode = logmodeStr.substr(it+1, logmodeStr.length()-it-1);

      if (logmode.find("yes")!=string::npos ||
          logmode.find("true")!=string::npos)
        deleteFlag = false;
      else if (logmode.find("no")!=string::npos ||
               logmode.find("false")!=string::npos)
        deleteFlag = true;
    }
  }

  if (!importDir.empty() && importDir.length()>5 && deleteFlag) {
    string rmcmd = "/bin/rm -rf " + importDir;
#ifdef _WIN32
    (void)winShell("rm -rf " + importDir);
#else
    (void)system(rmcmd.c_str());
#endif
  }

  return status;
}

