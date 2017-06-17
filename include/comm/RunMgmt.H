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
         throw(EcceException);

      static vector<string> updateCacheFromComputeHost(const TaskJob* job,
            const string& cacheDir, const string& filePattern,
            string& errorMessage)
         throw(EcceException);

      static vector<string> updateCacheFromDataArchive(const TaskJob* job,
            const string& cacheDir, const string& filePattern,
            string& errorMessage)
         throw(EcceException);
};

#endif 
