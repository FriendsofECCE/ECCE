///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: SDirectory.C
//
//
// DESIGN:
//	Inherit most behavior from SFile, most notably the handling
//	of environment variables and path name parsing.  Implement
//	virtual functions like copy, move, etc in manner consistent
//	with directory files.
//
///////////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>		// mkdir
#include <errno.h>
#include <unistd.h>		// rmdir
#include <dirent.h>		// access to directory files
#include <string.h>

// application includes
#include "util/ErrMsg.H"
#include "util/SDirectory.H"
#include "util/EcceSortedVector.H"
#include "util/StringTokenizer.H"


// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Default constructor should only be used when creating vectors of
//    SDirectory objects.
///////////////////////////////////////////////////////////////////////////////
SDirectory::SDirectory() : SFile()
{}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Create object that refers to physical directory.  If the
//    create mode is not 0 and the directory doesn't exist, it
//    will be created.  Environment variables can be embedded in
//    the name.  Use the ErrMsg class to confirm success.
//  Implementation
///////////////////////////////////////////////////////////////////////////////
SDirectory::SDirectory(const string& name, int create_mode) : SFile(name,0)
{
  // Create the directory if necessary
  if (create_mode != 0) {
    SDirectory::create(path(true),create_mode);
  }
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Copy constructor.
//
///////////////////////////////////////////////////////////////////////////////
SDirectory::SDirectory(const SDirectory& dir) : SFile(dir)
{ }

 
// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Destructor.
///////////////////////////////////////////////////////////////////////////////
SDirectory::~SDirectory(void) {}


// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Modify the SDirectory to refer to a new physical directory.
//  Implementation
//    Use the superclasses behavior, making no checks for existence here.
///////////////////////////////////////////////////////////////////////////////
SDirectory& SDirectory::operator=(const string& dir)
{
  SFile::operator=(dir);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Assignment operator.  The directory name is copied to the
//    lhs object.  Note that this does not cause a directory copy
//    to occur.
///////////////////////////////////////////////////////////////////////////////
SDirectory& SDirectory::operator=(const SDirectory& dir)
{
  if (this != &dir) {
    SFile::operator=(dir);
  }
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    operator== and operator!= compare the fully expanded path
//    names of the two objects.  The comparisons are case sensitive.
//  Implementation
//    Again, rely on the superclasses operators (coercing the SDirectory
//    objects to a SFile objects).
///////////////////////////////////////////////////////////////////////////////
bool SDirectory::operator==(const SDirectory& dir) const
{ return (SFile::operator==(dir)); }

bool SDirectory::operator!=(const SDirectory& dir) const
{ return (SFile::operator!=(dir)); }


// ------------ Modifiers ------------

// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Returns a vector of SFiles for the directory object.  If the
//    directory doesn't exist, the vector will be empty.  If the argument
//    is TRUE the directory is searched recursively.  The first form copies
//    the results into a vector that is returned, the second form adds
//    new files to the vector argument.
//  Implementation
//    Note that dirent.h is considered proprietary and is probably
//    highly system dependent.
///////////////////////////////////////////////////////////////////////////////
vector<SFile> SDirectory::get_files(bool recursive) const
{
  vector<SFile> ret;
  DIR *myDIR = opendir(path(true).c_str());
  if (myDIR) {
    struct dirent *mydirent = 0;
    string path = pathroot() + "/";
    for (;;) {
      mydirent = readdir(myDIR);
      if (mydirent == 0) break;
      if (strcmp(mydirent->d_name,".") != 0 &&
          strcmp(mydirent->d_name,"..") != 0) {
        SFile file(path+mydirent->d_name);
        ret.push_back(file);
        if (recursive && file.is_dir()) {
          SDirectory subdir(file.path());
          subdir.get_files(ret, recursive);
        }
      }
    }
    closedir(myDIR);
  }
  return ret;
}
void SDirectory::get_files(vector<SFile>& fileList,
			  bool recursive) const
{
  DIR *myDIR = opendir(path(true).c_str());
  if (myDIR) {
    struct dirent *mydirent = 0;
    string path = pathroot() + "/";
    for (;;) {
      mydirent = readdir(myDIR);
      if (mydirent == 0) break;
      if (strcmp(mydirent->d_name,".") != 0 &&
          strcmp(mydirent->d_name,"..") != 0) {
        SFile file(path+mydirent->d_name);
        fileList.push_back(file);
        if (recursive && file.is_dir()) {
          SDirectory subdir(file.path());
          subdir.get_files(fileList, recursive);
        }
      }
    }
    closedir(myDIR);
  }
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Returns a vector of file (and directory) namdes for the directory object.
//    If the directory doesn't exist, the vector will be empty.
//    If the argument is TRUE the directory is searched recursively.
///////////////////////////////////////////////////////////////////////////////
void SDirectory::getFileNames(
    EcceSortedVector<string, less<string> >& fileList) const
{
  DIR *myDIR = opendir(path(true).c_str());
  if (myDIR) {
    struct dirent *mydirent = 0;
    for (;;) {
      mydirent = readdir(myDIR);
      if (mydirent == 0) break;
      if (strcmp(mydirent->d_name,".") != 0 &&
          strcmp(mydirent->d_name,"..") != 0) {
        fileList.push_back(mydirent->d_name);
      }
    }
    closedir(myDIR);
  }
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Return the number of files (including directories) contained by
//    this SDirectory.  If the argument is TRUE, the count includes
//    recursive files and directories.  A value of -1 is returned if
//    the directory does not exist.
///////////////////////////////////////////////////////////////////////////////
int SDirectory::count(bool recursive) const
{
  int ret = -1;
  DIR *myDIR = opendir(path(true).c_str());
  if (myDIR) {
    ret = 0;
    string path = pathroot() + "/";
    struct dirent *mydirent = 0;
    for (;;) {
      mydirent = readdir(myDIR);
      if (mydirent == 0) break;
      ret++;
      if (recursive) {
        SFile file(path+mydirent->d_name);
        if (file.is_dir()) {
          SDirectory subdir(file.path());
          ret += subdir.count(recursive);
        }
      }
    }
    closedir(myDIR);
  }
  return ret;
}


// ---------- General Methods  ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Removes the directory and all files recursively.  If any file or
//    directory wasn't removed, FALSE will be returned.  If succecssful,
//    TRUE is returned.  A partial removal may occur.
//
//  Implementation
//    See the man pages for rmdir(3).  Note that rmdir does not operate
//    recursively.
///////////////////////////////////////////////////////////////////////////////
bool SDirectory::remove(void)
{
  int status = 0;
  vector<SFile> files;
  get_files(files, false);
  for (int idx=files.size()-1; (idx>=0) && (status == 0); idx--) {
    if (files[idx].is_regular_file() || files[idx].is_link()) {
      status = (files[idx].remove() ? 0 : -1);
    } else {
      SDirectory tmp(files[idx].path(true));
      status = (tmp.remove() ? 0 : -1);
    }
  }
  if (status == 0) {
    if (is_regular_file() || is_link()) {
      status = SFile::remove();
    } else {
      status = rmdir(path(true).c_str());
      if (status != 0) {
        pushSystemError(errno,path(true).c_str());
      }
    }
  }
  return (status == 0);
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Copies a directory and its contents recursively.  The new
//    directory root is at new_dir.  Errors are placed on the queue.
//    As soon as an error is detected, the copy aborts which means that
//    partial copies of directory trees can occur.
//
//  Implementation
//    This function relies on SDirectory::create() to place it's appropriate
//    errors on the error queue.
//
//    IMPORTANT: This method is not really tested since it is not used
//               by the calcmgr copy at this time.  This is especially
//               true of the recursive copy code block.
///////////////////////////////////////////////////////////////////////////////
SFile* SDirectory::copy(const string& new_dir)
{
  SFile* result = new SFile;
  ErrMsg errs;
  if (!exists() || !is_dir()) {
    errs.message("EE_FNF", EE_FATAL, WHERE, path().c_str());
  } else {
    *result = new_dir;
    if (result->exists()) {
      errs.message("EE_FILE_EXISTS", EE_FATAL, WHERE, new_dir.c_str());
    } else {
      vector<SFile> files;
      get_files(files, true);
      int mode = 0755;
      string new_path;
      bool errors = false;
      if (SDirectory::create(new_dir, mode)) {
        for (size_t idx=0; idx<files.size() && !errors; idx++) {
          new_path = new_dir + "/" + files[idx].filename();
          if (files[idx].is_dir()) {
            SDirectory subdir(this->path(true) + "/" + files[idx].filename());
            SFile *subsubdir = subdir.copy(new_path);
            if (!subsubdir->exists()) errors=true;
            delete subsubdir;
          } else {
	    files[idx].copy(new_path);
	  }
	}
      }
    }
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Moves a directory.  Errors are placed on the queue and FALSE is
//    returned if errors occur.
//
//  Implementation
//    Use rename(2).
///////////////////////////////////////////////////////////////////////////////
bool SDirectory::move(const string& dest_dir)
{
  int status = 0;  // return value

  // check if dest_dir is a subdir of this
  // Use temporary SFile object to properly handle env variables in dest_dir
  SFile targetDir(dest_dir);

  // rename() lets us do things we want tot trap so we have to do the
  // error checking ourselves first.
  ErrMsg errs;
  if (path(true) == targetDir.path(true)) {
      errs.message("EE_FILE_EXISTS", EE_FATAL, WHERE,dest_dir.c_str());
  } else if (targetDir.exists()) {
      errs.message("EE_FILE_EXISTS", EE_FATAL, WHERE,dest_dir.c_str());
  } else {
    status = rename(path(true).c_str(), targetDir.path(true).c_str());

    // If successful, reset the current file to the new file name
    if (status == 0) {
       (*this) = dest_dir;
    } else {
      pushSystemError(errno,path(true).c_str());
    }
  }
  return (status == 0);
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Creates a new directory with the specified access modes.  If the
//    directory can't be created FALSE is returned.  This will attempt
//    to completely create the directory path, including all intermediary
//    directories.  Errors are returned on the queue.
//
//  Implementation
//    Some UNIX variants offer the equivalent of "mkdir -p" to do this
//    work for us, but we can't rely on that so we go directory by directory
//    doing creates.  The implementation is a little messy.  The idea is
//    to skip over existing directories in the path and then perform the
//    mkdirs on the remaining missing parts.  This leads to erroroneous
//    code if the directory to be created already exists so this condition
//    must be checked prior to entering the loop.  All other errors are
//    handled via errno.
///////////////////////////////////////////////////////////////////////////////
bool SDirectory::create(const string& name, int mode)
{
  int status = 0;
  SFile tmp(name);
  ErrMsg errs;
  if (tmp.exists()) {
    errs.message("EE_FILE_EXISTS", EE_FATAL, WHERE,name.c_str());
    status = -1;
  } else {

    StringTokenizer next(name);
    string dirName;
    if (name.substr(0,1) == "/") {
      dirName += "/";
    }
    string token;

    // skip dir parts that already exist to avoid bad error messages.
    // Note that this will pass through the entire path if the dir already
    // exists.  See method header comments.
    for (dirName += (token = next.next("/"));
       !token.empty() && !(access(dirName.c_str(), F_OK|X_OK));
       dirName += "/" + (token = next.next("/")));

    while (!token.empty() && (status == 0)) {
      if ((status = mkdir(dirName.c_str(), mode)) != 0) {
        pushSystemError(errno,dirName.c_str());
      } else {
        dirName += "/" + (token = next.next("/"));
      }
    }
  }
  return (status == 0);
}

// ------------------------
// Private Member Functions
// ------------------------

// ---------- Friends ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Dumps the  SDirectory object to the stream.  Since the
//    abstraction chosen for SFiles and Directories is a name or
//    pointer the the object, all we dump is the name.
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const SDirectory& dir)
{
  os << (SFile&)dir;
  return os;
}
