/**
* @file
*
*
*/

#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

#include <strstream>
  using std::istrstream;
#include <fstream>
  using std::ofstream;

#include <sstream>
  using std::istringstream;
  using std::ostringstream;

#include <string>
  using std::string;

#include <algorithm>
#include <vector>

#include "util/Host.H"
#include "util/TDateTime.H"
#include "util/TempStorage.H"
#include "util/StringTokenizer.H"
#include "util/BadValueException.H"
#include "util/NullPointerException.H"
#include "util/ResourceUtils.H"
#include "util/TypedFile.H"
#include "util/ETimer.H"
#include "util/Serializer.H"

#include "dsm/CodeFactory.H"
#include "dsm/ActivityLog.H"
#include "dsm/TaskJob.H"
#include "dsm/Session.H"
#include "dsm/PrepareModelXMLizer.H"
#include "dsm/NWChemMDModelXMLizer.H"
#include "dsm/PolyrateModelXMLizer.H"
#include "dsm/NWDirdyModelXMLizer.H"
#include "dsm/EDSIMessage.H"
#include "dsm/EDSI.H"
#include "dsm/VDoc.H"
#include "dsm/EDSIFactory.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"


/**
 * Constructor
 */
TaskJob::TaskJob()
{
  p_vdoc = 0;
}


/**
 * Destructor
 */
TaskJob::~TaskJob()
{
  if (p_vdoc) {
    delete p_vdoc;
    p_vdoc = 0;
  }
}


/**
 * Refreshes the metadata based on the values currently stored on the server.
 * Clears the locally cached metadata.  Searches for VDoc properties
 * (depth = infinity).
 */
void TaskJob::updateProps()
{
  EcceURL originalUrl = getEDSI()->getURL();

  vector<MetaDataRequest> vmdr;
  ResourceMetaDataResult result;

  // Build property query
  vector<string> props;
  getEDSI()->describeServerMetaData(props);
  MetaDataRequest mdr;
  int numProps = props.size();
  for (int idx=0; idx<numProps; idx++) {
    mdr.name = props[idx];
    vmdr.push_back(mdr);
  }

  // Query properties
  getEDSI()->setURL(p_url);
  if (getEDSI()->getMetaData(vmdr,result, true)) {
    clearProps();
    cacheProps(result.metaData);
    p_access = SUCCEEDED;
  }
  else
    p_access = FAILED;

  getEDSI()->setURL(originalUrl);
}


vector<Resource*> * TaskJob::getChildren(bool refresh)
{
  vector<Resource*> *ret;

  ret = Resource::getChildren(refresh);

  if (refresh)
    p_localHideInternal = false;

  bool userFlag = getenv("ECCE_DEVELOPER") == NULL;

  vector<Resource*>::iterator resItor;

  // Hide the Parameters and Props folder if developer flag is not set
  if (userFlag) {
    for (resItor = ret->begin(); resItor != ret->end();) {
      if ((*resItor)->getName() == "Parameters" ||
          (*resItor)->getName() == "Props" ) {
        ret->erase(resItor);
      } else {
        resItor++;
      }
    }
  }

  if (isHideInternal() && !p_localHideInternal) {
    for (resItor = ret->begin(); resItor != ret->end();) {
      if ((*resItor)->getName() == "Inputs" ||
          (*resItor)->getName() == "Outputs" ) {
        ret->erase(resItor);
      }
      else if (!userFlag && ((*resItor)->getName() == "Parameters" ||
                             (*resItor)->getName() == "Props" )) {
        ret->erase(resItor);
      } else {
        resItor++;
      }
    }
    p_localHideInternal = true;
  }
  else if (!isHideInternal() && p_localHideInternal && !refresh) {
    getChildren(true);
  }
  return ret;
}


/**
 * Sets the starting data/time of a task job.
 * The date value must be in GMT.
 *
 * @param date New date value.  Must be in GMT.
 *
 * @return Returns true if date stored successfully, otherwise
 *         flase is returned
 *
 * @todo Validate date value before storing. Convert
 *       to GMT if necessary.
 * @deprecated use setStartDate instead
 */
bool TaskJob::startDate(const string& sdate)
{
   vector<MetaDataResult> results(1);
   results[0].name = VDoc::getEcceNamespace() + ":startdate";
   results[0].value = sdate;
   return addProps(results);
}


/**
 * Sets the starting data/time of a task job.
 * The date value must be in GMT.
 *
 * @param date New date value.  Must be in GMT.
 *
 * @return Returns true if date stored successfully, otherwise
 *         flase is returned
 *
 * @todo Validate date value before storing. Convert
 *       to GMT if necessary.
 */
bool TaskJob::setStartDate(const string& date)
{
   return startDate(date);
}


/**
 * Gets the starting date/time of a task job.
 * The date value is in GMT.
 *
 * @return Returns the starting date/time of a task job.
 *
 * @deprecated use getStartDate instead
 */
string TaskJob::startDate() const
{
  return getProp(VDoc::getEcceNamespace() + ":startdate");
}


/**
 * Gets the starting date/time of a task job.
 * The date value is in GMT.
 *
 * @return Returns the starting date/time of a task job.
 */
string TaskJob::getStartDate() const
{
  return startDate();
}


/**
 * Sets the completion data/time of a task job.
 * The date value must be in GMT.
 *
 * @param date New date value.  Must be in GMT.
 *
 * @return Returns true if date stored successfully, otherwise
 *         flase is returned
 *
 * @todo Validate date value before storing. Convert
 *       to GMT if necessary.
 *
 * @deprecated use setCompletionDate instead
 */
bool TaskJob::completionDate(const string& cdate)
{
  vector<MetaDataResult> results(1);
  results[0].name = VDoc::getEcceNamespace() + ":completiondate";
  results[0].value = cdate;
  return addProps(results);
}


/**
 * Sets the completion data/time of a task job.
 * The date value must be in GMT.
 *
 * @param date New date value.  Must be in GMT.
 *
 * @return Returns true if date stored successfully, otherwise
 *         flase is returned
 *
 * @todo Validate date value before storing. Convert
 *       to GMT if necessary.
 */
bool TaskJob::setCompletionDate(const string& date)
{
  return completionDate(date);
}


/**
 * Gets the completion date/time of a task job.
 * The date value is in GMT.
 *
 * @return Returns the completion date/time of a task job.
 *
 * @deprecated use getCompletionDate instead
 */
string TaskJob::completionDate() const
{
  return getProp(VDoc::getEcceNamespace() + ":completiondate");
}


/**
 * Gets the completion date/time of a task job.
 * The date value is in GMT.
 *
 * @return Returns the completion date/time of a task job.
 */
string TaskJob::getCompletionDate() const
{
  return completionDate();
}


/**
 * Sets job data associated with the task.
 *
 * @param jobdata Structure containing job data to set task with.
 * 
 * @return Returns true if the job data was stored, otherwise false.
 */
bool TaskJob::jobdata(Jobdata jdata)
{
  vector<MetaDataResult> results(4);
  results[0].name = VDoc::getEcceNamespace() + ":job_jobid";
  results[1].name = VDoc::getEcceNamespace() + ":job_clientid";
  results[2].name = VDoc::getEcceNamespace() + ":job_path";
  results[3].name = VDoc::getEcceNamespace() + ":job_clienthost";

  results[0].value = jdata.jobid;
  results[1].value = jdata.clientid;
  results[2].value = jdata.jobpath;
  if (jdata.clienthost.empty())
    results[3].value = Host().host_name();
  else
    results[3].value = jdata.clienthost;

  return addProps(results);
}


/**
 * Gets job data associated with the task.  Always gets fresh version from
 * server.
 *
 * @return Returns job data associated with the task.
 */
Jobdata TaskJob::jobdata() const
{
  Jobdata ret;

  ret.jobid = getProp(VDoc::getEcceNamespace() + ":job_jobid");
  ret.clientid = getProp(VDoc::getEcceNamespace() + ":job_clientid");
  ret.jobpath = getProp(VDoc::getEcceNamespace() + ":job_path");
  ret.clienthost = getProp(VDoc::getEcceNamespace() + ":job_clienthost");

  return ret;
}


/**
 * Sets launch data associated with the task.
 *
 * @param launchinfo Structure containing launch data to set task with.
 * 
 * @return Returns true if the job data was stored, otherwise false.
 */
bool TaskJob::launchdata(const Launchdata& launchinfo)
{
  char buf[80];
  vector<MetaDataResult> results(12);
  results[0].name = VDoc::getEcceNamespace() + ":launch_machine";
  results[1].name = VDoc::getEcceNamespace() + ":launch_nodes";
  results[2].name = VDoc::getEcceNamespace() + ":launch_rundir";
  results[3].name = VDoc::getEcceNamespace() + ":launch_scratchdir";
  results[4].name = VDoc::getEcceNamespace() + ":launch_totalprocs";
  results[5].name = VDoc::getEcceNamespace() + ":launch_user";
  results[6].name = VDoc::getEcceNamespace() + ":launch_queue";
  results[7].name = VDoc::getEcceNamespace() + ":launch_priority";
  results[8].name = VDoc::getEcceNamespace() + ":launch_maxmemory";
  results[9].name = VDoc::getEcceNamespace() + ":launch_minscratch";
  results[10].name = VDoc::getEcceNamespace() + ":launch_maxwall";
  results[11].name = VDoc::getEcceNamespace() + ":launch_remoteShell";

  results[0].value = launchinfo.machine;
  sprintf(buf, "%lu", launchinfo.nodes);
  results[1].value = buf;
  results[2].value = launchinfo.rundir;
  results[3].value = launchinfo.scratchdir;
  sprintf(buf, "%lu", launchinfo.totalprocs);
  results[4].value = buf;
  results[5].value = launchinfo.user;
  results[6].value = launchinfo.queue;
  results[7].value = launchinfo.priority;
  sprintf(buf, "%lu", launchinfo.maxmemory);
  results[8].value = buf;
  sprintf(buf, "%lu", launchinfo.minscratch);
  results[9].value = buf;
  results[10].value = launchinfo.maxwall;
  results[11].value = launchinfo.remoteShell;
  
  return addProps(results);
}


/**
 * Gets launch data associated with the task.
 *
 * @param useInputProvider  when true, queries input provider metadata as well
 * @return Returns launch data associated with the task.
 *         Null is returned if an error is encountered with the Dav server.
 */
Launchdata TaskJob::launchdata(const bool &useInputProvider) const
{
  Launchdata ret;
  string tmp;

  EcceMap *props = 0;
  if (useInputProvider) {
    props = getProps(true);
  } else {
    props = getProps();
  }
  NULLPOINTEREXCEPTION(props, "getProps(...) failed to return valid pointer");

  props->findValue(VDoc::getEcceNamespace() + ":launch_machine", ret.machine);
  props->findValue(VDoc::getEcceNamespace() + ":launch_rundir", ret.rundir);
  props->findValue(VDoc::getEcceNamespace() + ":launch_scratchdir", ret.scratchdir);
  props->findValue(VDoc::getEcceNamespace() + ":launch_user", ret.user);
  props->findValue(VDoc::getEcceNamespace() + ":launch_queue", ret.queue);
  props->findValue(VDoc::getEcceNamespace() + ":launch_priority", ret.priority);
  props->findValue(VDoc::getEcceNamespace() + ":launch_maxwall", ret.maxwall);
  props->findValue(VDoc::getEcceNamespace() + ":launch_remoteShell", ret.remoteShell);

  tmp = "";
  props->findValue(VDoc::getEcceNamespace() + ":launch_nodes", tmp);
  if (!tmp.empty()) ret.nodes = atoi(tmp.c_str());
  tmp = "";
  props->findValue(VDoc::getEcceNamespace() + ":launch_totalprocs", tmp);
  if (!tmp.empty()) ret.totalprocs = atoi(tmp.c_str());
  tmp = "";
  props->findValue(VDoc::getEcceNamespace() + ":launch_maxmemory", tmp);
  if (!tmp.empty()) ret.maxmemory = atoi(tmp.c_str());
  tmp = "";
  props->findValue(VDoc::getEcceNamespace() + ":launch_minscratch", tmp);
  if (!tmp.empty()) ret.minscratch = atoi(tmp.c_str());

  // clean up memeory
  delete props;

  return ret;
}


/**
 * Sets job log data associated with the task.
 *
 * @param log Job log data to set for task.
 * 
 * @return Returns true if the job log data was stored, otherwise false.
 */
bool TaskJob::joblog(const char* log)
{
  vector<MetaDataResult> log_results;
  log_results[0].name = VDoc::getEcceNamespace() + ":joblog";
  log_results[0].value = log;
  return addProps(log_results);
}


/**
 * Gets job log  data associated with the task.  Always gets fresh version
 * from server.
 *
 * @return Returns job log data associated with the task.
 */
string TaskJob::joblog(void) const
{
  return getProp(VDoc::getEcceNamespace() + ":joblog");
}


/**
 * Starts a new job log for the task.  A backup is created for 
 * existing job log -  for debugging job monitoring.  Only "monitor error" 
 * job logs aren't deleted before this point by either a 
 * "reset to rerun" or "reconnect job monitoring" so only those
 * will be saved.  The suffix ".last" is appended to the backup
 * job log name.
 *
 * @param rootName Name of job log for task
 * 
 * @return Returns true if the new job log was created, otherwise
 *         false is returned.
 */
bool TaskJob::startJobLog(const char* rootName)
{
  bool ret = true;

  string runLogName = getVDoc()->getRunLogName();
  try {
    // check for existence of run log instead of just throwing an
    // exception when it doesn't exist (since it won't most of the time)
    vector<EcceURL> urls = getVDoc()->getOutputs();
    int it;
    for (it = 0; it<urls.size() &&
         urls[it].getFilePathTail()!=runLogName; it++);
    if (it < urls.size())
      getVDoc()->moveOutput(runLogName, runLogName + ".last");
  }
  catch (DavException& davException) {
    p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                    davException.what());
  }

  // Start new job Log
  string log = ActivityLog::startLog(rootName);
  istrstream is(log.c_str());
  try {
    getVDoc()->addOutput(runLogName, &is);
  }
  catch (DavException& davException) {
    p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                    davException.what());
    ret = false;
  }
  return ret;
}


/**
 * Appends a character string to the existing job log
 * for a task.
 * 
 * @param log Character string to append to the job log
 *
 * @return Returns true if the character string was appended
 *         to the job log, otherwise false is returned.
 */
bool TaskJob::appendJobLog(const char* log)
{
  EcceURL originalUrl = getEDSI()->getURL();

  bool ret = true;
  string runLogName = getVDoc()->getRunLogName();

  getEDSI()->setURL(getVDoc()->getOutput(runLogName));
  ret = getEDSI()->appendDataSet(log);

  getEDSI()->setURL(originalUrl);
  return ret;
}


/**
 * Appends the closing xml element tag for the job log.
 *
 * @param rootName Name of the closing xml element tag.
 *                 This should be the name of the job log.
 *
 * @return Returns true if the closing xml element tag was
           appended, otherwise false is returned.
 */
bool TaskJob::endJobLog(const char* rootName)
{
  EcceURL originalUrl = getEDSI()->getURL();

  bool ret = true;
  string runLogName = getVDoc()->getRunLogName();
  string log = ActivityLog::endLog(rootName);

  getEDSI()->setURL(getVDoc()->getOutput(runLogName));
  ret = getEDSI()->appendDataSet(log.c_str());

  getEDSI()->setURL(originalUrl);
  return ret;
}


/**
 * Removes the current job log.
 *
 * @return Returns true is the job log was removed, otherwise 
 *         false is returned.
 */
bool TaskJob::removeJobLog(void)
{
  EcceURL originalUrl = getEDSI()->getURL();

  bool ret = true;
  string runLogName = getVDoc()->getRunLogName();

  getEDSI()->setURL(getVDoc()->getOutput(runLogName));
  ret = getEDSI()->removeResource();

  getEDSI()->setURL(originalUrl);
  return ret;
}



/** 
 * Determines if the input file is new by comparing its :lastmodified value 
 * with that of the molecule and basis set files.
 *
 * @return Returns true if the :lastmodified time for the input file is newer
 *         than the molecule and basis set files.
 */
bool TaskJob::isInputFileNew() const 
{
  EcceURL originalUrl = getEDSI()->getURL();

  bool ret = false;
  MetaDataRequest request;
  vector<MetaDataRequest> requests;
  vector<MetaDataResult> molecule_results; 
  vector<MetaDataResult> gbs_results; 
  vector<MetaDataResult> infile_results; 

  request.name = "DAV:getlastmodified";
  requests.push_back(request); 

  getEDSI()->setURL(getVDoc()->getConfiguration(getVDoc()->getMoleculeName()));
  getEDSI()->getMetaData(requests, molecule_results);
  getEDSI()->setURL(getVDoc()->getConfiguration(getVDoc()->getBasisSetName()));
  getEDSI()->getMetaData(requests, gbs_results);

  vector<EcceURL> inputfile = getDataFiles(JCode::INPUT);
  if (inputfile.size() > 0) {
    EcceURL iUrl = inputfile[0];  
    getEDSI()->setURL(iUrl);
    getEDSI()->getMetaData(requests, infile_results); 
  }

  if (molecule_results.size() > 0 &&
      gbs_results.size() > 0 &&
      infile_results.size() > 0) {
    long inSecs = TDateTime::toSeconds(infile_results[0].value.c_str());
    long molSecs = TDateTime::toSeconds(molecule_results[0].value.c_str());
    long gbsSecs = TDateTime::toSeconds(gbs_results[0].value.c_str());

    ret = inSecs>=molSecs && inSecs>=gbsSecs;
    if (!ret)
      // DAV copy can result in input file being slightly older than molecule
      // and/or basis set.  However, they should all be very close in time so
      // check that the times are all within three seconds meaning that it
      // would be extremely unlikely if they weren't created by some kind of
      // copy operation.
      ret = labs(gbsSecs-molSecs)<4 &&
            labs(molSecs-inSecs)<4 &&
            labs(gbsSecs-inSecs)<4;
  } else if (molecule_results.size() > 0 &&
             infile_results.size() > 0) {
    // This fixes the case for Gaussian Semi-Empirical where there is no
    // basis set.  I think this is safe to do because no input file should
    // exist for a calculation without a basis set unless it is Gaussian SE.
    long inSecs = TDateTime::toSeconds(infile_results[0].value.c_str());
    long molSecs = TDateTime::toSeconds(molecule_results[0].value.c_str());

    ret = inSecs >= molSecs;
    if (!ret)
      ret = molSecs-inSecs < 4;
  }
 
  getEDSI()->setURL(originalUrl);
  return ret;
}




/**
 * Removes all task input files.
 *
 * @return Returns true if all input files were removed, otherwise false.
 */
bool TaskJob::removeInputFiles()
{
  bool ret = true;

  try {
    getVDoc()->removeInputs();
  }
  catch (DavException& davException) {
    p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                    davException.what());
    ret = false;
  }

  return ret;
}


/**
 * Removes all task output files.
 *
 * @return Returns true if all output files were removed, otherwise false.
 */
bool TaskJob::removeOutputFiles()
{
  bool ret = true;

  try {
    getVDoc()->removeOutputs();
  }
  catch (DavException& davException) {
    p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                    davException.what());
    ret = false;
  }

  return ret;
}


/**
 * Adds an input file for a task.  The input file is stored in the 
 * task's virtual document under the supplied name.  If the provided
 * input stream is null then the input file stored under the 
 * supplied name will be removed.
 * 
 * @param name name of new input file -or- name of existing input
 *             file to be removed
 * @param istr pointer to input stream, if null then the named input
 *             file will be removed.
 *
 * @return Returns true if the input file was added/removed, otherwise
 *         false is returned.
 */
bool TaskJob::putInputFile(const string& name, istream *istr)
{
  bool ret = true;

  if (istr != (istream*)0) {
    try {
      getVDoc()->addInput(name, istr);
    }
    catch (DavException& davException) {
      p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                      davException.what());
      ret = false;
    }
  }
  else {
    try {
      getVDoc()->removeInput(name);
    }
    catch (DavException& davException) {
      p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                      davException.what());
      ret = false;
    }
  }

  return ret;
}


/**
 * Adds an output file for a task.  The output file is stored in the 
 * task's virtual document under the supplied name.  If the provided
 * input stream is null then the output file stored under the 
 * supplied name will be removed.
 * 
 * @param name name of new output file -or- name of existing output
 *             file to be removed
 * @param istr pointer to input stream, if null then the named output
 *             file will be removed.
 *
 * @return Returns true if the output file was added/removed, otherwise
 *         false is returned.
 */
bool TaskJob::putOutputFile(const string& name, istream *istr)
{
  bool ret = true;



  if (istr != (istream*)0) {
    try {
      getVDoc()->addOutput(name, istr);
    }
    catch (DavException& davException) {

      p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                      davException.what());
      ret = false;
    }
  }
  else {
    try {
      getVDoc()->removeOutput(name);
    }
    catch (DavException& davException) {

      p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                      davException.what());
      ret = false;
    }
  }

  return ret;
}


/** 
 * Stores a task file out on the Dav server.  The file is not documented
 * within the task's virtual document.  This method is used to store misc.
 * files associated with the task that are not, for example,
 * input or output files.
 *
 * @param name name of file
 * @param istr pointer to input stream (file contents)
 *
 * @return Returns true if the file was stored successfully,
 *         otherwise false is returned.
 */
bool TaskJob::putAnyFile(const  string& name, istream* istr)
{
  bool ret = true;

  if (istr != (istream*)0) {
    try {
      getVDoc()->addFile(name, istr);
    }
    catch (DavException& davException) {
      p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                      davException.what());
      ret = false;
    }
  }
  else {
    try {
      getVDoc()->removeFile(name);
    }
    catch (DavException& davException) {
      p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                      davException.what());
      ret = false;
    }
  }

  return ret;
}


/**
 * Gets the names of input or output files of a specic type associated 
 * with the task.
 * 
 * @param type Type of input or output file(s) to get
 *
 * @return Returns a vector containing the names of the data files 
 *         of the specified type.
 * 
 */
vector<string> TaskJob::getDataFileNames(JCode::CodeFileType type) const
{
  vector<string> ret;
  int numFiles, idx;

  vector<EcceURL> dataFiles(getDataFiles(type));
  numFiles = dataFiles.size();
  for (idx = 0; idx < numFiles; idx++) {
    ret.push_back(dataFiles[idx].getFilePathTail());
  }
  return ret;
}


/**
 * Gets the Url's of input or output files of a specic type associated
 * with the task.
 * 
 * @param type Type of input or output file(s) to get
 *
 * @return Returns a vector containing the Url's of the data 
 *         files of the specified type.
 * 
 */
vector<EcceURL> TaskJob::getDataFiles(JCode::CodeFileType type) const
{
  const JCode *codeCap = application();
  vector<TypedFile> codeFiles; 
  vector<EcceURL>::iterator fileIt;
  vector<EcceURL> vdocFiles;
  vector<EcceURL> inputFiles;
  vector<EcceURL> outputFiles;
  vector<EcceURL> dataFiles;

  if (codeCap) {
    codeFiles = codeCap->getCodeFiles(type);
    if (codeFiles.size() > 0) {
      try {
        inputFiles = getVDoc()->getInputs();
        outputFiles = getVDoc()->getOutputs();

        // Add inputs to VDoc files vector
        vdocFiles = inputFiles;

        // Add outputs to VDoc files vector
        int idx;
        for (idx = 0; idx < outputFiles.size(); idx++) {
          fileIt = find(vdocFiles.begin(), vdocFiles.end(), outputFiles[idx]);
          if (fileIt == vdocFiles.end()) {
            vdocFiles.push_back(outputFiles[idx]);
          }
        }

        int numCodeFiles = codeFiles.size();
        int i, j;
        for (i = 0; i < numCodeFiles; i++) {
          for (j = 0; j < vdocFiles.size(); j++) {
            //  Get data files from Dav based on MIME type
            string contentType = getMimeType(vdocFiles[j]);
            if (contentType == codeFiles[i].type()) {
              EcceURL dataFile(vdocFiles[j]);
              fileIt = find(dataFiles.begin(), dataFiles.end(), dataFile);
              if (fileIt == dataFiles.end()) {
                dataFiles.push_back(dataFile);
              }
            }
            // Get data files from Dav based on file name, needed for
            // legacy files that had no MIME type assignment
            if (vdocFiles[j].getFilePathTail() == codeFiles[i].name()) {
              EcceURL dataFile(vdocFiles[j]);
              fileIt = find(dataFiles.begin(), dataFiles.end(), dataFile);
              if (fileIt == dataFiles.end()) {
                dataFiles.push_back(dataFile);
              }
            }
          }
        }

      }
      catch (...) {
        // Return an empty list
        dataFiles.clear();
      }
    }
  }
  return dataFiles; 
}


/**
 * Gets names of all input, output, and misc. files
 * associated with the task. 
 *
 * @return Returns a vector containing the names of all input,
 *         output, and misc. files associated with the task.
 */
vector<string> TaskJob::getAllFileNames() const
{
  vector<string> allfiles;
  vector<string>::iterator fileIt;

  try {
    vector<EcceURL> dataFiles = getVDoc()->getFiles();
    for (int i = 0; i < dataFiles.size(); i++) {
      fileIt = find(allfiles.begin(), allfiles.end(),
                    dataFiles[i].getFilePathTail());
      if (fileIt == allfiles.end()) {
        allfiles.push_back(dataFiles[i].getFilePathTail());
      }
    }
  }
  catch (...) {
    // Return an empty list
    allfiles.clear();
  }

  return allfiles;
}


/**
 * Get data set as stream.  The file stream is retrieved using name.  The file
 * may be either an input, output, or misc. data file associated with
 * the task.
 *
 * @param name  Name of resource
 *
 * @return returns 0 if failed. 
 */
istream *TaskJob::getAnyFile(const string& name) const
{
  EcceURL originalUrl = getEDSI()->getURL();

  istream* ret = 0;

  EcceURL fileUrl = getVDoc()->getFile(name);
  getEDSI()->setURL(fileUrl);
  ret = getEDSI()->getDataSet();

  getEDSI()->setURL(originalUrl);
  return ret;
}


/**
 * Get data set as file.  File is retrieved using name.  The file
 * may be either an input, output, or misc. data file associated with
 * the task.
 *
 * @param name name of resource
 * @param dest results destination.  If null a new SFile is allocated 
 *             for return and must be deleted by caller.  
 * @return returns 0 if failed. 
 */
SFile *TaskJob::getAnyFile(const string& name, const SFile *dest) const
{
  EcceURL originalUrl = getEDSI()->getURL();

  SFile *sfile = (SFile*) dest;
  if (sfile == (SFile*)0) sfile = TempStorage::getTempFile();

  EcceURL outputfileUrl = getVDoc()->getFile(name);
  getEDSI()->setURL(outputfileUrl);
  ofstream ofs( sfile->path().c_str() );
  if (! getEDSI()->getDataSet( ofs )) sfile = 0;
  ofs.close();

  getEDSI()->setURL(originalUrl);
  return sfile;
}


/**
 * Get data set as stream.  The file stream is retrieved using Url.  The file
 * may be either an input, output, or misc. data file associated with
 * the task.
 *
 * @param url Url of resource
 *
 * @return returns 0 if failed. 
 */
istream *TaskJob::getAnyFile(const EcceURL& url) const
{
  EcceURL originalUrl = getEDSI()->getURL();

  istream* ret = 0;

  getEDSI()->setURL(url);
  ret = getEDSI()->getDataSet();

  getEDSI()->setURL(originalUrl);
  return ret;
}


/**
 * Get data set as file.  File is retrieved using Url.  The file
 * may be either an input, output, or misc. data file associated with
 * the task.
 *
 * @param url address of file resource
 * @param dest results destination.  If null a new SFile is allocated 
 *             for return and must be deleted by caller.  
 * @return returns 0 if failed. 
 */
SFile *TaskJob::getAnyFile(const EcceURL& url, const SFile *dest) const
{
  EcceURL originalUrl = getEDSI()->getURL();

  SFile *sfile = (SFile*) dest;
  if (sfile == (SFile*)0) sfile = TempStorage::getTempFile();

  getEDSI()->setURL(url);
  ofstream ofs( sfile->path().c_str() );
  if (! getEDSI()->getDataSet( ofs )) sfile = 0;
  ofs.close();

  getEDSI()->setURL(originalUrl);
  return sfile;
}


/** 
 * Gets a stream with the contents of a given data file.  The data
 * file retrieved is determined by the specified type.
 *
 * @param type Type of data file to get.
 * 
 * @return Returns null if there was an error, otherwise the input
 *         stream is returned.  This is an in memory stream!  Use 
 *         an alternative method for large files.
 */
istream *TaskJob::getDataFile(JCode::CodeFileType type) const
{
  EcceURL originalUrl = getEDSI()->getURL();

  istream *ret = 0;
  TypedFile dataFile;

  getDataFile(type, dataFile);

  if ( (!dataFile.name().empty()) &&
       (!dataFile.type().empty())
     ) {
    EcceURL dataFileUrl = getDataFileUrl(type, dataFile.name());
    getEDSI()->setURL(dataFileUrl);
    ret = getEDSI()->getDataSet();
  }

  getEDSI()->setURL(originalUrl);
  return ret;
}


/** 
 * Get the specified file as an SFile object.  If 'dest' is not 
 * specified, a file is created in temp space. The file will be closed
 * before this method returns.
 *
 * @param type Type of data file to get.  
 *
 * @return 0 if the get failed, otherwise path to returned file.
 */
SFile *TaskJob::getDataFile(JCode::CodeFileType type, const SFile *dest) const
{
  EcceURL originalUrl = getEDSI()->getURL();

  SFile *sfile = (SFile*) dest;
  TypedFile dataFile;
  if (sfile == (SFile*)0) sfile = TempStorage::getTempFile();

  getDataFile(type, dataFile);

  if ( (!dataFile.name().empty()) &&
       (!dataFile.type().empty())
     ) {

    EcceURL dataFileUrl = getDataFileUrl(type, dataFile.name());
    getEDSI()->setURL(dataFileUrl);
    ofstream ofs( sfile->path().c_str() );
    if (! getEDSI()->getDataSet( ofs ) ) sfile = 0;
    ofs.close();
  }
  else {
     sfile = 0;
  }

  getEDSI()->setURL(originalUrl);
  return sfile;
}


/**
 * Get the file name and its Mime type for a specific type of
 * input or output file.
 * 
 * @param type Type of file to get.
 * @param dataFile reference to a TypedFile (Out parameter), 
 *                 which will contain name and Mime type of file.
 */
void TaskJob::getDataFile(JCode::CodeFileType type, TypedFile& dataFile) const
{
  vector<EcceURL> dataFiles;

  // Initialize
  dataFile.setName("");
  dataFile.setType("");

  // Get existing data files
  dataFiles = getDataFiles(type);

  // Return first data file
  if (dataFiles.size() > 0) {

    // Get most recent data file, there may be more than one as
    // is the case, for example, for batched trajectory files
    EcceURL url = getLatestDataFile(dataFiles);
    dataFile.setName(url.getFilePathTail());
    dataFile.setType(getMimeType(url));
  }
  // If data file hasn't been created then return
  // default code file.
  else {
    const JCode *codeCap = application();
    if (codeCap) {
      dataFile = codeCap->getCodeFile(type);
    }
  }
}



EcceURL TaskJob::getLatestDataFile(vector<EcceURL> dataFiles) const
{
  int urlIdx = 0;
  int maxSeq = 0;

  // Search data files for max sequence number
  for (int i = 0; i < dataFiles.size(); i++) {

    // get base name w/o extension
    string base = dataFiles[i].getFilePathTail();
    int dot = base.find('.', 0);
    if (dot != string::npos) {
     base = base.substr(0, dot);
    }

    // determine starting position of sequence string
    int idx;
    for (idx = base.length(); ((idx > 0) && (isdigit(base[idx-1]))); idx--);

    // if a sequence found - then acquire and save data file index 
    // if sequence is a maximum
    if ((idx > 0) && (isdigit(base[idx]))) {
      int seq = atoi((char*)&(base.c_str()[idx]));
      if (seq > maxSeq) {
        urlIdx =  i;
        maxSeq = seq;
      }
    }
  }

  return dataFiles[urlIdx];
}
 


/**
 * Retrieves the Url for a named input/output file.
 * 
 * @param type Type of input/output file
 * @param name Name of input/output file
 * 
 * @return Url of input/output file
 */
EcceURL TaskJob::getDataFileUrl(JCode::CodeFileType type, 
                                const string& name) const
{
  EcceURL dataFileUrl;
  switch(type) {
    case JCode::PRIMARY_INPUT :
    case JCode::AUXILIARY_INPUT :
    case JCode::INPUT :
      dataFileUrl = getVDoc()->getInput(name);
      break;
    case JCode::PRIMARY_OUTPUT :
    case JCode::PARSE_OUTPUT :
    case JCode::AUXILIARY_OUTPUT :
    case JCode::PROPERTY_OUTPUT :
    case JCode::FRAGMENT_OUTPUT :
    case JCode::RESTART_OUTPUT :
    case JCode::TOPOLOGY_OUTPUT :
    case JCode::OUTPUT :
      dataFileUrl = getVDoc()->getOutput(name);
    case JCode::ESP_OUTPUT :
    case JCode::MD_PROPERTIES_OUTPUT :
    case JCode::MD_OUTPUT :
      break;
  }
  return dataFileUrl;
}


/**
 * The TaskJob import simply looks for a parameter file and invokes
 * the processImportParameters method.  It is expected that subclasses
 * would override this method.
 */
string TaskJob::import( const string& dir, const string& parseFileName) 
   throw(EcceException)
{
   string message;

   string fullFileRoot = dir + "/" + parseFileName;
   size_t index = fullFileRoot.find_last_of('.');
   if (index != string::npos) fullFileRoot = fullFileRoot.substr(0, index);

   string paramFileName = fullFileRoot + ".param";

   // load .param file
   Preferences param(paramFileName, true);
   if (!param.isValid()) {
      throw EcceException("Setup parse script command "  
         " did not create parameter file.", WHERE);
   }


   // Subclasses may have other stuff in the parameter file.
   processImportParameters(param);

   /* remove temparay files .param .frag files */
   string cmd = "/bin/rm -f ";
   cmd += paramFileName;
   system(cmd.c_str());

   return message;
}



/**
 * Return a string that represents the arguments that should be
 * sent to data parsing scripts.
 * By default an empty string is returned.  Subclasses should
 * override this method if their parse scripts take arguments.
 */
string TaskJob::getParseScriptArgs() const
{
   return " .";
}


/**
 * As part of the import process, a set of key/value pairs may
 * be generated.  It is up to the subclasses to determine which
 * key/value pairs associated scripts will generate and what to
 * do with them when they exist.
 *
 * This method handles the following keys:
 *  <li>StartDate
 *  <li>StartTime
 *  <li>EndDate
 *  <li>EndTime
 * @param params key/value data generated from import script
 */
void   TaskJob::processImportParameters(const Preferences& params)
  throw (IOException)
{

   string date, time;

   // WE NEED TO DOUBLE CHECK THIS DATE STUFF TO MAKE SURE I GOT THE
   // FORMATTING RIGHT FOR THE TDATETIME CONSTRUCTOR
   if (params.getString("StartDate", date) &&
         params.getString("StartTime", time)) {

      string reformattedDate = reformatDate(date);

      TDateTime tdtime(reformattedDate + " " + time); // time format is ok as-is
      if (tdtime.isValid()) {
         addProp(VDoc::getEcceNamespace() + ":startdate", 
                 tdtime.toString());
      }
   }

   if (params.getString("EndDate", date) &&
         params.getString("EndTime", time)) {

      string reformattedDate = reformatDate(date);
      TDateTime tdtime(reformattedDate + " " + time);
      if (tdtime.isValid()) {
         addProp(VDoc::getEcceNamespace() + ":completiondate", 
                 tdtime.toString());
      }
   }
}



/** 
 * This method converts date format found in some output files to 
 * the format ecce uses internally for date strings (TDateTime).
 *
 * StartDate example: "Jan 16, 2002" <-- we need to convert this to
 * day-of-week, 16 Jan 2002
 */
string TaskJob::reformatDate(const string& dateFromOutputFile) 
{
   vector<string> tokens;

   if (dateFromOutputFile != "") {
      StringTokenizer tokenizer(dateFromOutputFile);
      tokens = tokenizer.tokenize(" ,");
      string msg = "Bad date input string: " + dateFromOutputFile;
      BADVALUEEXCEPTION(tokens.size() == 3, msg)
   }
   string reformattedDate = "someday, "; // this will get parsed off in
   // TDateTime constructor
   reformattedDate += tokens[1] + " ";
   reformattedDate += tokens[0] + " ";
   reformattedDate += tokens[2];
   return reformattedDate;
}


/**
 * Performs serialization of the Task Model to the DAV
 * server.  If the model pointer is null, then the
 * Task file is removed from the DAV server.
 *
 * @param model Pointer to an Task Model
 *
 * @throw DavException A problem encountered with the Dav server
 * @throw InvalidException Task type no recognized - could not assign
 *                         the correct Serializer.
 *
 * @return Returns true if the Task model was successfully
 *         serialized out to the DAV server, otherwise
 *         false is returned.
 */
void TaskJob::setTaskModel(ITaskModel* model) throw(DavException,
                                                    InvalidException)
{
  EcceMap props;
  string data = "";
  Serializer *serializer = 0;
  string resourceName = "";

  serializer = getSerializer(resourceName);

  if (model != 0) {
    try {
      serializer->serialize(*model, data, props);
      istringstream is(data);
      getVDoc()->addConfiguration(resourceName, &is);
    }
    catch (...) {
      if (serializer != 0) {
        delete serializer;
      }
      throw;
    }
  }
  else {
    getVDoc()->removeConfiguration(resourceName);
  }
  delete serializer;
}


/**
 * Performs deserialization of the Task Model from the DAV
 * server.
 *
 * @param model task Model referenece, deserialized model stored here.
 *
 * @throw DavException A problem encountered with the Dav server
 * @throw ParseException Xerces parsing problem
 * @throw InvalidException Task type no recognized - could not assign
 *                         the correct Serializer.
 *
 * @return Returns true if the Task model was successfully
 *         deserialized from the DAV server, otherwise
 *         false is returned.
 */
void TaskJob::getTaskModel(ITaskModel& model) throw(DavException,
                                                    ParseException,
                                                    InvalidException)
{
  EcceURL originalUrl = getEDSI()->getURL();

  EcceMap props;
  ostringstream os;
  Serializer *serializer = 0;
  string resourceName = "";

  try {

    // Get serializer and name of xml resource
    serializer = getSerializer(resourceName);

    // Set url
    getEDSI()->setURL(getVDoc()->getConfiguration(resourceName));

    // Deserialize xml into model
    if (getEDSI()->exists()) {
      if (getEDSI()->getDataSet(os)) {
        serializer->deserialize(os.str(), model, props);
      }
      else {
        throw DavException("Could not retrieve xml resource "
                           "from Dav server", WHERE);
      }
    }
    delete serializer;
  }
  catch (...) {
    if (serializer != 0 ) {
      delete serializer;
    }
    getEDSI()->setURL(originalUrl);
    throw;
  }

  getEDSI()->setURL(originalUrl);
}


/**
 * Returns the serialized string for a model - not saved
 * to database.
 */
string TaskJob::serializeModel(ITaskModel* model) throw(InvalidException)
{
  EcceMap props;
  string data = "";
  Serializer *serializer = 0;
  string resourceName = "";

  serializer = getSerializer(resourceName);

  if (model != 0) {
    serializer->serialize(*model, data, props);
    delete serializer;
  }

  return data;
}


/**
 * Based on the type of task, a Serializer is created and returned along
 * with the name of the associated xml resource.
 *
 * @param resourceName Out paramaeter, name of associated xml resource.
 *
 * @throw InvalidException Type of task was not recognized.
 *
 * @return Returns a pointer to a Serializer object , appropriate
 *         for the type of task.  Caller is responsible
 *         for memory management.
 */
Serializer* TaskJob::getSerializer(string& resourceName) throw(InvalidException)
{
  Serializer *serializer = 0;

  // Determine model type - set serializer and resource name
  ResourceDescriptor::CONTENTTYPE taskType = getContentType();
  ResourceDescriptor::APPLICATIONTYPE appType = getApplicationType();
  switch (taskType) {
    case ResourceDescriptor::CT_MDPREPARE:
      serializer = new PrepareModelXMLizer();
      resourceName = getVDoc()->getMdPrepareName();
      break;
    case ResourceDescriptor::CT_MDOPTIMIZE:
      serializer = new NWChemMDModelXMLizer();
      resourceName = getVDoc()->getMdOptimizeName();
      break;
    case ResourceDescriptor::CT_MDENERGY:
      serializer = new NWChemMDModelXMLizer();
      resourceName = getVDoc()->getMdEnergyName();
      break;
    case ResourceDescriptor::CT_MDDYNAMICS:
      serializer = new NWChemMDModelXMLizer();
      resourceName = getVDoc()->getMdDynamicsName();
      break;
    case ResourceDescriptor::CT_CALCULATION:
      if (appType == ResourceDescriptor::AT_DRDVTST) {
        serializer = new NWDirdyModelXMLizer();
        resourceName = getVDoc()->getNwDirdyName();
        break;
      }
      if (appType == ResourceDescriptor::AT_POLYRATE) {
        serializer = new PolyrateModelXMLizer();
        resourceName = getVDoc()->getPolyrateName();
        break;
      }
    default:
      throw InvalidException("Could not determine task type", WHERE);
  }

  return serializer;

}


/**
 * Returns the EDSI interface for this resource.  User should
 * always use this method to access the EDSI data member.  The
 * EDSI will be created upon the first request and cached for
 * subsequent use.
 *
 * @throw InvalidException Indicates that the EDSI interface
 *        creation failed.
 *
 */
VDoc *TaskJob::getVDoc() const throw(InvalidException)
{
  // Create VDoc
  if (p_vdoc == 0) {
    const_cast<TaskJob*>(this)->p_vdoc = EDSIFactory::getVdoc(p_url);
  }

  // Create failed - throw exception
  if (p_vdoc == 0) {
    throw InvalidException("VDoc creation failed.", WHERE);
  }

  return p_vdoc;
}


/**
 * Gets application code object associated with the taskjob.
 *
 * @return Returns pointer to application code object.  Null
 *         is returned if Dav property does not exist or
 *         if a problem was encountered with the Dav server.
 */
const JCode *TaskJob::application() const
{
  const JCode* tcode = 0;

  string result = getProp(VDoc::getEcceNamespace() + ":application");
  if (result != "") {
    tcode = CodeFactory::lookup(result.c_str());
  }

  return tcode;
}


/**
 * Sets name of application associated with the taskjob.
 * This can possibly change this Resource's ResourceType (descriptor).
 *
 * @param code pointer to an application code object
 *
 * @return Returns true if application name was set, otherwise false.
 */
bool TaskJob::application(const JCode* code)
{
  vector<MetaDataResult> results(1);
  results[0].name = VDoc::getEcceNamespace() + ":application";
  ResourceType *descriptor = getDescriptor();

  if ((code) && (descriptor) ) {

    results[0].value = code->name();

    ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
    if (code->name() != descriptor->getApplicationType()) {
      ResourceType *rt = rd.getResourceType(descriptor->getResourceType(),
                                            descriptor->getContentType(),
                                            code->name());
      setDescriptor(rt);
    }
  }

  return addProps(results);
}


void TaskJob::getDeleteMessage(bool & canDelete, string & message)
{
  ResourceDescriptor::RUNSTATE state = getState();
  if (state == ResourceDescriptor::STATE_SUBMITTED ||
      state == ResourceDescriptor::STATE_RUNNING) {
    canDelete = false;
    message = "Calculation \"" + getName() +
      "\" is in submitted or running state and can not be deleted.";
  }
  else {
    canDelete = true;
    message = "Are you sure you want to delete calculation \"" + getName() + "\"?";
  }
}


/**
 * Sets the base url for this task.
 *
 * @param url New base Url for task.
 */
bool TaskJob::setURL(const EcceURL& url)
{
  p_url = url;
  getVDoc()->setURL(url);
  return true;
}


/**
 * Check if the state change is allowed
 */
bool TaskJob::canChangeState(string & message) const
{
  ResourceDescriptor::RUNSTATE state = getState();
  bool ret = (state != ResourceDescriptor::STATE_SUBMITTED &&
              state != ResourceDescriptor::STATE_RUNNING);
  message = ret? "" :"Changing the state of a submitted or running calculation"
    " is not allowed.";
  return ret;
}


/**
 * Sets the runstate of a task job. 
 *
 * @param state New state of task job.
 *
 * @return Returns true if date stored successfully, otherwise
 *         flase is returned
 */
#if 000
#include <execinfo.h>
#endif

bool TaskJob::setState(const ResourceDescriptor::RUNSTATE& state)
{
  string stateName = ResourceUtils::stateToString(state);
  vector<MetaDataResult> results((state<=ResourceDescriptor::STATE_READY)?6:2);
  results[0].name = VDoc::getEcceNamespace() + ":state";
  results[0].value = stateName;
  results[1].name = VDoc::getEcceNamespace() + ":rerun";
  results[1].value = "false";
  if (state <= ResourceDescriptor::STATE_READY) {
    results[2].name = VDoc::getEcceNamespace() + ":startdate";
    results[2].value = "";
    results[3].name = VDoc::getEcceNamespace() + ":completiondate";
    results[3].value = "";
    results[4].name = VDoc::getEcceNamespace() + ":reviewed";
    results[4].value = "false";
    results[5].name = VDoc::getEcceNamespace() + ":reviewedDate";
    results[5].value = "";
  }
  
  bool ret = addProps(results);

#if 000
cout << "**** TaskJob STACK TRACE BEGIN" << endl;
void *buffer[64];
size_t size = backtrace(buffer, 64);
char **strings = backtrace_symbols(buffer, size);
size_t i;
for (i=0; i<size; i++)
  cout << "stack trace[" << i << "]: " << strings[i] << endl;
free(strings);
cout << "**** TaskJob STACK TRACE END" << endl;
#endif

  notifyState(stateName);

  return ret;
}


/**
 * Gets the runstate of a task job.
 *
 * @return Returns the runstate of a task job.
 */
ResourceDescriptor::RUNSTATE TaskJob::getState() const
{
  ResourceDescriptor::RUNSTATE state = (ResourceDescriptor::RUNSTATE)-1;
  string result = getProp(VDoc::getEcceNamespace() + ":state");
  if (!result.empty())
    state = ResourceUtils::stringToState(result);
  return state;
}


bool TaskJob::setRerun(const bool& rerun)
{
  vector<MetaDataResult> results(1);
  results[0].name = VDoc::getEcceNamespace() + ":rerun";
  results[0].value = rerun? "true": "false";
  
  bool ret = addProps(results);

  return ret;
}


bool TaskJob::getRerun() const
{
  bool rerun = false;
  string result = getProp(VDoc::getEcceNamespace() + ":rerun");
  if (!result.empty())
    rerun = result=="true";
  return rerun;
}


bool TaskJob::resetForRerun(const ResourceDescriptor::RUNSTATE& toState)
{
  // Logic for detect whether it is ok to change state. Comment out to
  // improve the performance. Caller of this function should always
  // call canChangeState first.
  //   string message;
  //   if (!canChangeState(message)) {
  //     p_msgStack->add(message.c_str());
  //     return false;
  //   }
  ResourceDescriptor::RUNSTATE tostate = toState;

  ResourceDescriptor::RUNSTATE state = getState();
  
  // For debugging purposes we keep run logs for failed (monitor error) jobs
  if (state != ResourceDescriptor::STATE_FAILED)
    removeJobLog();
  
  /* We decided not to bother doing this.  You can get into nasty
   * problems if you duplicate and then call this method.  It cleans
   * out the server dir for the calc you ran.  There is no good thing
   * to do.  And anyway, this shouldn't be necessary.
   string msg = RunMgmt::cleanServerDirs(this);
   if (msg != "") {
   setMessage(msg, WxFeedback::WARNING);
   }
  */
  if (tostate == ResourceDescriptor::STATE_ILLEGAL) {
    // this implies we should figure out the correct state.
    if (state == ResourceDescriptor::STATE_LOADED) {
      // No chance of having an input file.
      tostate = ResourceDescriptor::STATE_CREATED;
    }
    else if (state == ResourceDescriptor::STATE_CREATED) {
      // leave it
      tostate = ResourceDescriptor::STATE_CREATED;
    }
    else {
      tostate = ResourceDescriptor::STATE_READY;
    }
  }
  else if (state != tostate) {
    if (tostate == ResourceDescriptor::STATE_CREATED) {
      // remove input file to maintain calced consistency!!! GDB 8/15/02
      removeInputFiles();
    }
    else if (tostate == ResourceDescriptor::STATE_READY &&
             state == ResourceDescriptor::STATE_CREATED) {
      tostate = ResourceDescriptor::STATE_CREATED;
    }
  }

  bool ret = setState(tostate) && removeOutputFiles();

  // set rerun flag so calculation run directory files are removed
  // if/when the job is launched
  setRerun(true);

  // special STTR logic to reset state of linked reaction study tasks
  Resource* parent = EDSIFactory::getResource(getURL().getParent());
  if (parent->getApplicationType() == ResourceDescriptor::AT_REACTION_STUDY) {
    resetReactionTasks();
  } else if (parent->getApplicationType() ==
             ResourceDescriptor::AT_CONDENSED_REACTION_STUDY) {
    resetCondensedReactionTasks();
  }
  
  return ret;
}


void TaskJob::resetReactionTasks()
{
  // assumes that this task is within a reaction study (caller must check
  // and only invoke this method when that is true)
  if (getApplicationType() != ResourceDescriptor::AT_POLYRATE) {
    ResourceDescriptor::RUNSTATE state = ResourceDescriptor::STATE_CREATED;
    if (getApplicationType() == ResourceDescriptor::AT_DRDVTST) {
      // set state to whatever the DirDyVTST task is being set to--
      // could be either ready or created
      state = getState();
    }

    if (state <= ResourceDescriptor::STATE_READY) {
      vector<Resource *> targets =
              getSession()->getReactionTargets(this, Session::INPUT_PROVIDER);
      vector<Resource *>::iterator targetItor = targets.begin();
      for (; targetItor != targets.end(); targetItor++) {
        Job *job = dynamic_cast<Job*>(*targetItor);
        if (job) {
          (void)job->resetForRerun(state);
        }
      }
    }
  }
}


void TaskJob::resetCondensedReactionTasks()
{
  ResourceDescriptor::RUNSTATE state = getState();

  vector<Resource *> targets = 
          getSession()->getTargets(this, Session::INPUT_PROVIDER);
  vector<Resource *>::iterator targetItor = targets.begin();
  for (; targetItor != targets.end(); targetItor++) {
    Job *job = dynamic_cast<Job*>(*targetItor);
    if (job)
      (void)job->resetForRerun(state);
  }
}


bool TaskJob::resetForRestart()
{
  return setState(ResourceDescriptor::STATE_READY) &&
    setReviewed(false);
}


vector<string> TaskJob::getOutputTypes()
{

  vector<string> ret;
  vector<string> suffixes;
  vector<string>::iterator vecIt;

  vector<EcceURL> urls = getVDoc()->getOutputs();

  // save one file for each unique suffix
  // and then return their mime-types
  for (int i=0; i < urls.size(); i++) {
    string name = urls[i].getFilePathTail();
    int dot = name.rfind('.');
    if (dot != string::npos) {
      string suffix = name.substr(dot);
      if (suffix.size() > 1) {
        suffix = suffix.substr(1);
        vecIt = find(suffixes.begin(), suffixes.end(), suffix);
        if (vecIt == suffixes.end()) {
          suffixes.push_back(suffix);
          ret.push_back(getMimeType(urls[i]));
        }
      }
    }
  }

  return ret;
}

