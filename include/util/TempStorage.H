/**
 * @file 
 *
 *
 */
#ifndef TEMPSTORAGE_HH
#define TEMPSTORAGE_HH

#include <string>
  using std::string;

#include "util/EcceURL.H"

class SDirectory;
class SFile;


/**
 * TempStorage provides routines for creating temporary files within
 * Ecce.  All temporary files and directories should be created using
 * these methods so that cleanup can be managed and users can configure
 * what space to use for temp files which can be potentially quite large.
 */
class TempStorage 
{

   public:

      static    SFile* getTempFile(const char* prefix = "ECCE");

      static    SDirectory* getTempDirectory(const char* prefix = "ECCE");

      static    string getTempRootPath();
      // /tmp, or the Windows temp folder with forward slashes.
      static    string systemTempDir();

      static    string getJobDirectoryRoot();

      static    SDirectory *getTempJobDirectory(const char *prefix="ECCE");

      static    string getTrjDirectoryRoot();

      static    string getJobRunDirectoryPath(const EcceURL& url);

      // Removes job folders and ECCE?????? temp files older than `days`
      // that no job monitor or process still uses; returns how many went.
      static    int sweepStale(int days = 3);

      // Removes a job folder on scope exit unless keep() was called: an
      // import or catch-up that fails part way leaves nothing behind, one
      // whose job store started hands the folder to eccejobmaster.
      class JobDirGuard {
         public:
            explicit JobDirGuard(const string& path) : p_path(path) {}
            ~JobDirGuard();
            void keep() { p_path.clear(); }
         private:
            string p_path;
      };


};

#endif // TEMPSTORAGE_HH
