/**
 * @file
 *
 *
 */
#ifndef JOBPARSER_HH
#define JOBPARSER_HH


#include <string>
#include <map>
#include <vector>

using std::string;
using std::map;
using std::vector;
using std::less;


#include "dsm/PropertyInterpreter.H"
#include "dsm/IPropCache.H"

class TaskJob;
class PropertyInterpreter;
class EcceURL;
class RCommand;


/**
 * Class for running scripts to parse data from code output files.
 * Also includes method to import an output file since that uses the
 * same mechanism.
 */
class JobParser
{
   public:
      JobParser(const EcceURL& url);
      ~JobParser(void);

      void setURL(const EcceURL& url);

      static string importCode(const string& path, string& message);

      static bool importCalculation(const char* parseFile,
            TaskJob* task,
            const string& importName,
            string& message);

      // Import the finished run staged in stageDir as the GROMACS MD task
      // `task`: its output files under the names a live run brings back
      // (gromacs.gmxlog, <sys>_<calc>.gro, <sys>.gmxtop, *.itp,
      // ecce-parse-traj.txt) go through the job monitor and store as for a
      // live run.  Used by the GROMACS run importer only.
      static bool importGromacsTask(TaskJob* task,
            const string& stageDir,
            string& message);

      void cacheParseArgs(const string& tmpDir);

      void storeProperty(RCommand* localconn,
            const string& codeName,
            const string& propKey,
            propertyMapPairToValue&   properties,
            unsigned long callCount,
            const char* strmBuf,
            string& log, 
            bool fromFile = false);

   protected:

      IPropCache *p_propcache;
      TaskJob    *p_task;

      string p_parseArgs;
      string p_parseIn;
      string p_parseOut;
      string p_absoluteParseIn;
      string p_absoluteParseOut;

      // Use this to convert output file date format into the format
      // that the TDateTime constructor can use
      static string reformatDate(const string& dateFromOutputFile);

}; // JobParser

#endif // JOBPARSER_HH

