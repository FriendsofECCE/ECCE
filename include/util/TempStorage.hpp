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

      static    string getJobDirectoryRoot();

      static    SDirectory *getTempJobDirectory(const char *prefix="ECCE");

      static    string getTrjDirectoryRoot();

      static    string getJobRunDirectoryPath(const EcceURL& url);


};

#endif // TEMPSTORAGE_HH
