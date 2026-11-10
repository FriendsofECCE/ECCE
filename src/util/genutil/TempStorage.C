/**
 * @file
 *
 *
 */
#include <stdio.h>   // tempnam
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <iterator>
#include <vector>
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
#include "util/WaitingJobs.H"

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


TempStorage::JobDirGuard::~JobDirGuard()
{
   if (p_path.empty()) return;
   SDirectory dir(p_path.c_str());
   if (dir.exists())
      (void)dir.remove();
}


namespace {

bool statNoFollow(const string& path, struct stat& st)
{
#ifdef _WIN32
   return stat(path.c_str(), &st) == 0;
#else
   return lstat(path.c_str(), &st) == 0;
#endif
}

// The mkstemp names TempStorage hands out: <prefix>__XXXXXX for job
// folders, ECCEXXXXXX for files. Nothing else in these folders is touched.
bool endsInUniqueTail(const string& name, size_t at)
{
   if (name.size() != at + 6) return false;
   for (size_t i = at; i < name.size(); i++)
      if (!isalnum((unsigned char)name[i])) return false;
   return true;
}

// Newest change in the folder or its own entries: a monitor still writing
// its logs keeps the folder young.
time_t newestChange(const string& dir, time_t mtime)
{
   time_t newest = mtime;
   DIR* d = opendir(dir.c_str());
   if (!d) return newest;
   while (struct dirent* e = readdir(d)) {
      // ".." is the jobs folder itself, which every new job makes young.
      if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
         continue;
      struct stat st;
      if (statNoFollow(dir + "/" + e->d_name, st) && st.st_mtime > newest)
         newest = st.st_mtime;
   }
   closedir(d);
   return newest;
}

string confValue(const string& conf, const string& key)
{
   std::ifstream in(conf.c_str());
   for (string line; std::getline(in, line); )
      if (line.compare(0, key.size() + 2, key + ": ") == 0) {
         string v = line.substr(key.size() + 2);
         while (v.size() > 1 && v.back() == '/') v.pop_back();
         return v;
      }
   return "";
}

// Linux: the working directories and command lines of the user's own
// processes, so a folder a live process still names is never removed.
vector<string> liveProcessPaths()
{
   vector<string> out;
#ifdef __linux__
   DIR* d = opendir("/proc");
   if (!d) return out;
   while (struct dirent* e = readdir(d)) {
      if (!isdigit((unsigned char)e->d_name[0])) continue;
      const string pid = string("/proc/") + e->d_name;
      char buf[4096];
      ssize_t n = readlink((pid + "/cwd").c_str(), buf, sizeof buf - 1);
      if (n > 0) out.push_back(string(buf, n) + "/");
      std::ifstream cl((pid + "/cmdline").c_str(), std::ios::binary);
      string args((std::istreambuf_iterator<char>(cl)),
                  std::istreambuf_iterator<char>());
      for (char& c : args) if (c == '\0') c = ' ';
      if (!args.empty()) out.push_back(args);
   }
   closedir(d);
#endif
   return out;
}

}  // namespace


int TempStorage::sweepStale(int days)
{
   const time_t cutoff = time(0) - (time_t)days * 24 * 3600;
   const string root = getTempRootPath();
   const string jobs = root + "/jobs";
   int removed = 0;

   struct Folder { string path; time_t newest; };
   vector<Folder> old;
   vector<string> inUse;   // folders a running job monitor still owns
   if (DIR* d = opendir(jobs.c_str())) {
      while (struct dirent* e = readdir(d)) {
         const string name = e->d_name;
         const size_t sep = name.rfind("__");
         if (sep == string::npos || sep == 0 || !endsInUniqueTail(name, sep + 2))
            continue;
         const string path = jobs + "/" + name;
         struct stat st;
         if (!statNoFollow(path, st) || !S_ISDIR(st.st_mode)) continue;
         // eccejobmaster holds the calculation's watch lock while it runs;
         // its config names the import folder the monitor runs in.
         const string conf = path + "/eccejobstore.conf";
         const string url = confValue(conf, "calcURL");
         if (!url.empty() && WaitingJobs::watched(url)) {
            inUse.push_back(path);
            inUse.push_back(confValue(conf, "remoteDir"));
            continue;
         }
         const time_t newest = newestChange(path, st.st_mtime);
         if (newest < cutoff) old.push_back({path, newest});
      }
      closedir(d);
   }

   if (!old.empty()) {
      const vector<string> live = liveProcessPaths();
      for (const Folder& f : old) {
         bool used = false;
         for (const string& u : inUse)
            if (u == f.path) used = true;
         const string slash = f.path + "/";
         for (const string& l : live)
            if (l.compare(0, slash.size(), slash) == 0 ||
                l.find(f.path) != string::npos)
               used = true;
         if (used) continue;
         SDirectory dir(f.path.c_str());
         if (dir.remove()) removed++;
      }
   }

   if (DIR* d = opendir(root.c_str())) {
      while (struct dirent* e = readdir(d)) {
         const string name = e->d_name;
         if (name.compare(0, 4, "ECCE") != 0 || !endsInUniqueTail(name, 4))
            continue;
         const string path = root + "/" + name;
         struct stat st;
         if (statNoFollow(path, st) && S_ISREG(st.st_mode) &&
             st.st_mtime < cutoff && unlink(path.c_str()) == 0)
            removed++;
      }
      closedir(d);
   }
   return removed;
}
