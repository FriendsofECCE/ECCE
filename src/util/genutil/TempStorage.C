/**
 * @file
 *
 *
 */
#include <stdio.h>   // tempnam
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
using namespace std;

#include "util/TempStorage.H"
#include "util/LocalData.H"
#include "util/Ecce.H"
#include "util/SDirectory.H"
#include "util/SFile.H"

/**
 * The system's temporary directory: /tmp, or on Windows the user's temp
 * folder (GetTempPath) with forward slashes.
 */
string TempStorage::systemTempDir()
{
#ifdef _WIN32
   char buf[MAX_PATH + 1];
   DWORD n = GetTempPathA(sizeof buf, buf);
   if (n > 0 && n < sizeof buf) {
      string dir(buf, n);
      for (char& c : dir) if (c == '\\') c = '/';
      while (dir.size() > 3 && dir.back() == '/') dir.pop_back();
      return dir;
   }
#endif
   return "/tmp";
}


/**
 * Get path to default ecce temp directory.
 * This method takes into account the username as well as the ECCE_TMPDIR
 * variable.
 */
string TempStorage::getTempRootPath()
{
   string path = getenv("ECCE_TMPDIR")? getenv("ECCE_TMPDIR"): systemTempDir();
   path += "/ecce_";
   path += Ecce::realUser();

   SDirectory dir(path.c_str());

   if (!dir.exists())
      SDirectory::create(path.c_str(), 0755);  // create if it doesn't exist

   return path;
}


/** 
 * Get path to temporary root directory for job monitoring.
 * The path will be based on getTempRootPath.
 */
string TempStorage::getJobDirectoryRoot()
{
   string path = getTempRootPath();
   path += "/jobs";

   SDirectory dir(path.c_str());

   if (!dir.exists())
      SDirectory::create(path.c_str(), 0755);  // create if it doesn't exist

   return path;

}


/** 
 * Get path to temporary root directory for job monitoring.
 * The path will be based on getTempRootPath.
 */
string TempStorage::getTrjDirectoryRoot()
{
   string path = getTempRootPath();
   path += "/trj";

   SDirectory dir(path.c_str());

   if (!dir.exists())
      SDirectory::create(path.c_str(), 0755);  // create if it doesn't exist

   return path;

}


/**
 * Generate a unique temp file name using the given prefix.
 * The file will be in the directory specified by getTempRootPath()
 * The file will be created with permissions 0644.
 * @param prefix - first characters of the temp filename; 
 *                 can be a 0 pointer.
 *
 * @returns a tempnam(3) generated SFile object if successful
 */
SFile* TempStorage::getTempFile(const char* prefix)  
{

   SFile* ret = NULL;
   string nameTemplate = TempStorage::getTempRootPath();
   nameTemplate += "/";

   if (prefix != 0) {
      nameTemplate += prefix;
   }

   nameTemplate += "XXXXXX"; // required by mkstemp

   char* name = new char[nameTemplate.length() +1];
   strcpy (name, nameTemplate.c_str());

   int fd = mkstemp(name);

   if (fd < 0) {
      perror("mkstemp error: ");

   } else {
      // note that file was already created by mkstemp
      ret = new SFile(name, 0644);
      close(fd);
   }
   delete name;

   return ret;
}


/**
 * Generate a unique temp directory name using the given prefix.
 * The directory will be a sub directory of the directory specified 
 * by getTempRootPath()
 * @param prefix - first characters of the temp filename; 
 *                 can be a 0 pointer.
 *
 * @returns a tempnam(3) generated SDirectory object if successful
 */
SDirectory* TempStorage::getTempDirectory(const char *prefix)
{
   SDirectory* ret = NULL;

   string path = TempStorage::getTempRootPath();
   string nameTemplate = path + "/" + prefix + "__XXXXXX";
   char* unique = new char[nameTemplate.length() +1];
   strcpy(unique, nameTemplate.c_str());

   int fd = mkstemp(unique);

   if (fd < 0) {
      perror("mkstemp error: ");

   } else {

      // delete the file (mkstemp creates the file, not just a name);
      // closed first because Windows will not unlink an open file
      close(fd);
      if (unlink(unique)) {
         perror("unlink error: ");
      }

      ret = new SDirectory(unique, 0755);
   }
   delete unique;
   return ret;
}



/** 
 * Get path to temporary root directory for job monitoring.
 * The path will be based on getTempRootPath.
 */
SDirectory *TempStorage::getTempJobDirectory(const char *prefix)
{
   SDirectory* ret = NULL;

   string path = TempStorage::getJobDirectoryRoot();
   string nameTemplate = path + "/" + prefix + "__XXXXXX";
   char* unique = new char[nameTemplate.length() +1];
   strcpy(unique, nameTemplate.c_str());

   int fd = mkstemp(unique);

   if (fd < 0) {
      perror("mkstemp error: ");

   } else {

      // delete the file (mkstemp creates the file, not just a name);
      // closed first because Windows will not unlink an open file
      close(fd);
      if (unlink(unique)) {
         perror("unlink error: ");
      }

      ret = new SDirectory(unique, 0755);
   }
   delete unique;
   return ret;
}


/**
 * Construct the path to a run directory.
 * The method for generating rundir names has been updated over time
 * as requested by users that thought the paths were too long.
 * We were creating all these superflous directories just to be 100%
 * consistent with the paths on the data server.  That is, you'd always
 * have a "user" in your calc run dir path when it really didn't serve
 * any purpose.  Only when running jobs as a different user would it 
 * put in the user name.  Erich advocated the change.
 */
string TempStorage::getJobRunDirectoryPath(const EcceURL& url)
{
   string fullFile = url.getFile();
   string fluff = "/Ecce/users/";
   fluff += Ecce::serverUser();
   fluff += "/";
   // In local mode the user's folder plays the part of /Ecce/users/<user>,
   // so a run directory has the same layout as with a data server.
   string localHome = LocalData::userHome();
   if (!localHome.empty()) localHome += "/";
   // A calculation saved in any other local folder is placed by its path
   // below the user's home directory, as one in a project is below the home.
   const char *userHome = getenv("HOME");
   string homeDir = userHome && *userHome ? string(userHome) + "/" : "";
   if (!localHome.empty() && url.isLocal() && fullFile.find(localHome) == 0)
      fullFile = fullFile.substr(localHome.length()-1);
   else if (!homeDir.empty() && homeDir != "//" && url.isLocal()
            && fullFile.find(homeDir) == 0)
      fullFile = fullFile.substr(homeDir.length()-1);
   else if (fullFile.find(fluff) == 0)
      fullFile = fullFile.substr(fluff.length()-1);
   else if (fullFile.find("/Ecce/users/") == 0)
      fullFile = fullFile.substr(11);
   else if (fullFile.find("/Ecce/") == 0)
      fullFile = fullFile.substr(5);

   if (fullFile.rfind('/') == fullFile.length()-1)
      fullFile.erase(fullFile.length()-1, 1);

  return fullFile;
}
