/**
* @file
*
*
*/
#ifndef _TASKJOB_HH
#define _TASKJOB_HH

#include <iostream>
 using std::istream;

#include <string>
  using std::string;

#include <vector>
  using std::vector;

#include "util/SFile.H"
#include "util/EcceURL.H"
#include "util/Preferences.H"
#include "util/IOException.H"
#include "util/EcceException.H"
#include "util/InvalidException.H"
#include "util/DavException.H"
#include "util/ParseException.H"
#include "util/Serializer.H"

#include "tdat/ITaskModel.H"

#include "dsm/Job.H"
#include "dsm/JCode.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"

class VDoc;


/**
 * Structure containing data related to a calculation job.
 */
struct Jobdata {
  string jobid;      /**< Queue id or pid (workstation jobs) of calculation */
  string clienthost; /**< Machine eccejobstore runs on (no domain) */
  string clientid;   /**< Eccejobstore pid */
  string jobpath;    /**< Full path to calculation run directory on compute host */
};



/**
 * Structure containing data used to launch a calulation job. 
 */
struct Launchdata {
  string machine;           /**< Launch machine */
  unsigned long nodes;      /**< Only applies to clusters or mpp */
  unsigned long totalprocs; /**< # processors (ws and/or mpp) */
  string rundir;            /**< Base directory name as entered in launcher gui */
  string scratchdir;        /**< Launcher gui name is same as full scratch path */
  string user;              /**< User account name */
  string queue;             /**< Launch queue name */
  string priority;          /**< Priority setting on job */
  unsigned long maxmemory;  /**< Maximum memory that may be used (Mb) */
  unsigned long minscratch; /**< Minimum available disk space (Mb) */
  string maxwall;           /**< Maximum amount of time for job to finish */
  string remoteShell;       /**< Type of remote communications shell */

  Launchdata()
  {
    nodes=totalprocs=maxmemory=minscratch = 0;
    machine=rundir=scratchdir=user=queue=priority=maxwall=remoteShell="";
  };
};




/**
 * Represents a task that may be submitted/launched to a compute server
 * for processing.   
 */
class TaskJob : public Job,
                public Resource
{
   public:

      virtual ~TaskJob();

      // Resource virtuals
      virtual vector<Resource *> * getChildren(bool refresh = false);
      virtual void getDeleteMessage(bool & canDelete, string & message);

      // Job virtuals
      virtual bool setStartDate(const string& date);
      virtual string getStartDate() const;

      virtual bool setCompletionDate(const string& date);
      virtual string getCompletionDate() const;

      virtual bool canChangeState(string & message) const;
      virtual bool setState(const ResourceDescriptor::RUNSTATE& state);
      virtual ResourceDescriptor::RUNSTATE getState() const;
      virtual bool setRerun(const bool& rerun);
      virtual bool getRerun() const;
      virtual bool resetForRerun(const ResourceDescriptor::RUNSTATE&
                                 toState=ResourceDescriptor::STATE_ILLEGAL);
      virtual void resetReactionTasks();
      virtual void resetCondensedReactionTasks();
      virtual bool resetForRestart();

      // Left-over and/or old TaskJob implementations
      virtual bool startDate(const string& sdate);
      virtual string startDate() const;

      virtual bool completionDate(const string& cdate);
      virtual string completionDate() const;

      virtual bool jobdata(Jobdata jdata);
      virtual Jobdata jobdata() const;

      virtual bool launchdata(const Launchdata& launchinfo);
      virtual Launchdata launchdata(const bool &useInputProvider = false) const;

      virtual bool joblog(const char* log);
      virtual string joblog(void) const;

      virtual bool startJobLog(const char* rootName);
      virtual bool appendJobLog(const char* log);
      virtual bool endJobLog(const char* rootName);
      virtual bool removeJobLog(void);

      virtual bool isInputFileNew() const;

      virtual bool removeInputFiles();
      virtual bool removeOutputFiles();
      virtual bool putInputFile(const string& name, istream *istr);
      virtual bool putOutputFile(const string& name, istream *istr);
      virtual bool putAnyFile(const  string& name, istream* istr);

      virtual vector<string> getDataFileNames(JCode::CodeFileType type) const;
      virtual vector<EcceURL> getDataFiles(JCode::CodeFileType type) const;
      virtual vector<string> getAllFileNames() const;

      virtual istream *getAnyFile(const string& name) const;
      virtual SFile *getAnyFile(const string& name, const SFile *dest) const;
      virtual istream *getAnyFile(const EcceURL& url) const;
      virtual SFile *getAnyFile(const EcceURL& url, const SFile *dest) const;
      virtual istream *getDataFile(JCode::CodeFileType type) const;
      virtual SFile *getDataFile(JCode::CodeFileType type, 
            const SFile *dest) const;
      virtual void getDataFile(JCode::CodeFileType type, 
            TypedFile& dataFile) const;
      virtual vector<string> getOutputTypes();
      virtual EcceURL getLatestDataFile(vector<EcceURL> dataFiles) const;

      virtual EcceURL getDataFileUrl(JCode::CodeFileType type, 
            const string& name) const;


      /**
       * Import an output file.
       * This method is called after the registered import script is called
       * so any intermediate files should exist in the directory.
       * @param dir - path to file to import
       * @param file - file name
       * @return message indicating any problems
       * @throw EcceException - lots of possible errors
       */
      virtual string import( const string& dir, 
                          const string& file) throw(EcceException)=0;

      virtual string getParseScriptArgs() const;


      virtual void setTaskModel(ITaskModel* model)
                   throw(DavException,
                         InvalidException);
      virtual void getTaskModel(ITaskModel& model)
                   throw(DavException,
                         ParseException,
                         InvalidException);
      virtual string serializeModel(ITaskModel* model) throw(InvalidException);

      virtual bool application(const JCode* code);

      virtual const JCode *application() const;

   protected:

      TaskJob();

      // Resource virtuals
      virtual bool setURL(const EcceURL& url);
      virtual void updateProps();

      virtual void   processImportParameters(const Preferences& params)
         throw(IOException);

      static string reformatDate(const string& dateFromOutputFile);

      Serializer* getSerializer(string& resourceName) throw(InvalidException);

      VDoc *getVDoc() const throw(InvalidException);
  
      VDoc *p_vdoc;        /**< Ecce Virtual Document */

};

#endif

