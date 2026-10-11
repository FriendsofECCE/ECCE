#ifndef RUNMGMT_HH
#define RUNMGMT_HH


#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "util/EcceException.H"

class TaskJob;

/**
 * A class that encapsulates some static functions the operate
 * on remote jobs.
 * The purpose of this class is to keep the 'RCommand' code isolated.
 */
class RunMgmt
{
   public:

      static string terminate(const TaskJob *calc);
      static string cleanServerDirs(const TaskJob *calc);
      static bool jobStoreRunning(const string& jobID);

      /**
       * What the compute machine says about a job, over ECCE's own login
       * (#284): whether its process or queue job still exists, when its
       * output was last written, and the job script's status file.  Only
       * reports; the run state is left to the job monitor.
       */
      struct MachineCheck {
        bool reached = false;       // logged in and ran the check
        int present = -1;           // 1 found, 0 not found, -1 unknown
        string what;                // "Process 123", "Slurm job 456"
        string jobLine;             // the ps or queue line that matched
        long long outputTime = 0;   // last written, epoch; 0: no file
        long long outputSize = -1;
        long long machineNow = 0;   // the machine's clock
        string outputName;
        string status;              // .ecce.status, "" until the script ends
        string error;
        // Plain sentences, one per line, for the Progress box and tests.
        vector<string> describe() const;
      };
      static MachineCheck checkOnMachine(const TaskJob *calc);

      /**
       * Register the local machine if not already registered.
       * This allows import to work without user intervention.
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
      static bool registerLocalMachine(string& msg);

      static vector<string> updateCacheFromDataServer(const TaskJob* job,
            const string& cacheDir, const string& filePattern,
            string& errorMessage)
;

      static vector<string> updateCacheFromComputeHost(const TaskJob* job,
            const string& cacheDir, const string& filePattern,
            string& errorMessage)
;

      static vector<string> updateCacheFromDataArchive(const TaskJob* job,
            const string& cacheDir, const string& filePattern,
            string& errorMessage)
;
};

#endif 
