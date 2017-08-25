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

int main(int argc, char** argv)
{
  // cache authentications to pass to jobstore instances
  AuthCache::getCache().pipeIn(argv[2]);

  // default settings
  int MAX_RESTART_TRIES = 25;
  int MAX_QUICK_TRIES = 5;
  int MAX_QUICK_TIME = 60;

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

  int it;
  // nohup was added because it seems to fix an sh/bash problem with
  // closing a shell after exitting ECCE
  string ejsStart = "nohup ./eccejobstore";

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

  for (it=0; it<maxTries && status!=0 && status!=3 &&
       sumTimes>MAX_QUICK_TIME; it++) {

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

  logEntry(entry);

  // Email failures to ecce-test if ECCE_JOB_FAILMAIL tells us to
  if (status!=0 && status!=4) {
    string url = argv[6];
    string failmail = getenv("ECCE_JOB_FAILMAIL")?
                      getenv("ECCE_JOB_FAILMAIL"): "";
    if (failmail=="true" ||
        (failmail!="" && url.find(failmail)!=string::npos)) {
      string mail =
             "Mail -s 'Job Monitoring Failure' ecce-test@emsl.pnl.gov << EOM\n"
             "Calculation state set to 'monitor error'.\n"
             "Calculation URL: " + url + "\nEOM";
      (void)system(mail.c_str());
    }
  }


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
    (void)system(rmcmd.c_str());
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
    (void)system(rmcmd.c_str());
  }

  return status;
}

