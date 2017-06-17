///////////////////////////////////////////////////////////////////////////////
// SOURCE SFileNAME: SFile.C
//
//
// DESIGN:
//    For the purposes of size efficiency, the p_name attribute is
//    a pointer.  By creating the string via the string copy
//    constructor, the string size is only as large as it needs to be
//    (I think).
//
//    The access() and stat() functions do not interpret env vars
//    so expanded_name must always be called first to expand the
//    variables.  To eliminate redundant calls to expanded_name,
//    and to provide convenience functions without requiring a user
//    to always create SFile objects, is_dir and is_regular_file
//    are member funcions and static functions.
//
//    Note that if the SFilename is "", routines like access and
//    stat seem to interpret that as the current directory (.).
//    The method exists() checks this condition.  Other methods
//    do not under the assumption that if a SFile doesn't exist,
//    the values returned form is_directory etc are undefined.
//
//    This class has started to expand into operations on the actual
//    SFile system objects.  Examples of this are remove, move, and copy.
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
using std::ios;
#include <fstream>
using std::ofstream;
using std::ifstream;

#include <stdlib.h>	/* getenv()  system() */
#include <stdio.h>
#include <unistd.h>	/* access() getpid() */
#include <string.h>
#include <errno.h>	/* access() getpid() */
#include <sys/types.h>
#include <sys/stat.h>


#include "util/ErrMsg.H"
#include "util/Host.H"
#include "util/SFile.H"
#include "util/TDateTime.H"

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Construct an unnamed  SFile object.
//    The object should be named with the assignment operator.
//  Implementation
//    The modification time is set to an illegal value (by assigning to
//    the zero value).
///////////////////////////////////////////////////////////////////////////////
SFile::SFile(void) 
{
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Construct a SFile object of the specified name.
//    If the SFile is created EE_FILE_CREATED is put on the
//    queue for informational purposes.
//  Implementation
//    If environment variables are used within the name, they are not
//    expanded during instantiation.  They will be evaluated dynamically
//    as needed by other member functions.
///////////////////////////////////////////////////////////////////////////////
SFile::SFile(const string& name, int create_mode)
{ 
  p_name = SFile::make_name_valid(name).c_str();

  if (create_mode != 0) {
    SFile::create(p_name,create_mode);
  }
}

SFile::SFile(const char *sname, int create_mode)
{ 
  string name = sname;
  p_name = SFile::make_name_valid(name).c_str();

  if (create_mode != 0) {
    SFile::create(p_name,create_mode);
  }
}

bool SFile::create(const string& name, int create_mode)
{
  bool ret = true;
  SFile file(name);
  if (!file.exists()) {
    // Just open and close the SFile to cause it to exist.
    ofstream tmp(name.c_str(), ios::out);
    if (!tmp) {
      ErrMsg().message("EE_FILE_ACCESS",EE_FATAL,WHERE,name.c_str());
      ret = false;
    }
    tmp.close();
    if (create_mode != 0) {
      chmod(name.c_str(), create_mode);
    }
  } else {
    ErrMsg().message("EE_FILE_EXISTS",EE_FATAL,WHERE,name.c_str());
    ret = false;
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
// man 
// Description
//    Construct a SFile object from an existing one.  This only copies
//    SFile objects.  It does not create a new SFile system SFile.  Use
//    copy() to actually copy the physical SFile.
// Implementation
//    Allocate local string memory and then rely on operator=
//    to do the copy.  This constructor preserves any env variables
//    contained in the RHS SFile object's SFilename.
///////////////////////////////////////////////////////////////////////////////
SFile::SFile(const SFile& SFile) 
{
  *this = SFile;
}
 
// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
// man 
// Description
//    Free up memory used internally by the SFile object.
///////////////////////////////////////////////////////////////////////////////
SFile::~SFile(void)
{ 
}


// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Modify the SFile to refer to a new SFilename or SFile object.
///////////////////////////////////////////////////////////////////////////////
SFile& SFile::operator=(const string& file)
{
  p_name = SFile::make_name_valid(file).c_str();
  return *this;
}
SFile& SFile::operator=(const SFile& file)
{
  if (this != &file) {
    p_name = file.path().c_str();
  }
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Compare SFile objects based on their expanded full names.
//  Implementation
//    It is not necessary for the SFile to exist for this method
//    to return TRUE.
///////////////////////////////////////////////////////////////////////////////
bool SFile::operator==(const SFile& file) const
{ return (expanded_name() == file.path()); }
bool SFile::operator!=(const SFile& file) const
{ return (expanded_name() != file.path()); }



// ------------ Accessors ------------
// ------------ SFile Attributes ------------

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Returns the time the SFile was last modified.  An invalid time
//    is returned if the SFile doesn't exist or is not accessible.
//  Implementation
//    The UNIX time starts Jan 1 1970.
///////////////////////////////////////////////////////////////////////////////
TDateTime SFile::lastModified(void) const
{
  TDateTime ret("INVALID");  // initialize as invalid--this is a special flag
  if (access(expanded_name().c_str(),F_OK) == 0 && exists()) {
    struct stat stat_buf;
    stat(expanded_name().c_str(),&stat_buf);
    ret.setFromTimeT(stat_buf.st_mtime);
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Returns TRUE if the SFile is not empty.  Return FALSE if the SFile
//    is empty, does not exist, or is unreadable.
///////////////////////////////////////////////////////////////////////////////
bool SFile::non_zero() const
{ return  (size() > 0); }

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Returns the size of the SFile in bytes.  Non-existing SFiles have size 0.
//  Implementation
//    Use exists() to make sure the SFile exists.
//    off_t is what stat uses.  off_t can be unsigned so the failure
//    return code has to be 0.
///////////////////////////////////////////////////////////////////////////////
off_t SFile::size() const
{
  off_t size = 0;
  string expanded = expanded_name();

  if (access(expanded.c_str(),F_OK) == 0) {
     struct stat stat_buf;
     stat(expanded.c_str(),&stat_buf);
     size = stat_buf.st_size;
  }
  return size;
}

// ------------ SFile Access ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Returns TRUE if the SFile exits.
//  Implementation
//    Because null strings get interpreted as "." (it seems),
//    we only do SFile name expansion when the path name is not
//    trivial.
///////////////////////////////////////////////////////////////////////////////
bool SFile::exists() const
{ 
  return ((p_name.length() != 0) && !(access(expanded_name().c_str(),F_OK))); 
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Returns TRUE if the SFile is writable, either write permissions
//    if the SFile exists or a writable directory if it doesn't.
//  Implementation
//    See SFile::exists().
///////////////////////////////////////////////////////////////////////////////
bool SFile::is_writable() const
{
  bool retval = false;

  if (p_name.length() != 0) {
    if (exists()) {
      retval = !(access(expanded_name().c_str(),W_OK));
    } else {
      if (pathroot() == "")
        retval = !(access(".",W_OK));
      else
        retval = !(access(pathroot().c_str(),W_OK));
    }
  }
  return retval;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Returns TRUE if the SFile is readable.
//  Implementation
//    See SFile::exists()
///////////////////////////////////////////////////////////////////////////////
bool SFile::is_readable() const
{
  return ((p_name.length() != 0) &&
	  !(access(expanded_name().c_str(),F_OK|R_OK)));
}

// ------------ SFile Type Information ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    These methods check SFile types and return TRUE if the SFile type
//    matches and FALSE otherwise (inaccessible or non-existing).
///////////////////////////////////////////////////////////////////////////////
bool SFile::is_dir() const
{ return SFile::is_dir(expanded_name()); }
bool SFile::is_link() const
{ return SFile::is_link(expanded_name()); }
bool SFile::is_regular_file() const
{ return SFile::is_regular_file(expanded_name()); }


// ------------ SFile Type Information ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Return the SFile's extension.
//    ThelLast "." in the SFile name begins the extension.
///////////////////////////////////////////////////////////////////////////////
string SFile::extension(void) const
{
  string return_val("");
  string expanded = expanded_name();

  size_t start = expanded.find_last_of('.');
  if (start != string::npos) {         // -1 = RW_NPOS, 0 = hidden SFile
    start++;		     	    // increment past "."
    return_val =  expanded.substr(start,expanded.length()-start);
  }
  return return_val;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Return the full path of the SFile.
//    The expand flag determines whether to expand embeded variables.
///////////////////////////////////////////////////////////////////////////////
string SFile::path(const bool expand) const
{
  if (expand)
    return expanded_name();
  else
    return p_name.c_str();
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Return the root portion of the full pathname.
///////////////////////////////////////////////////////////////////////////////
string SFile::pathroot(void) const
{
  string expanded = expanded_name();

  if (!SFile::is_dir(expanded)) {                  // directories are pathroots
    size_t slash = expanded.find_last_of('/');
    if (slash == string::npos)
      expanded = "";
    else
      expanded = expanded.substr(0,slash);
  }
  return expanded;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Return the tail portion of the full path.
///////////////////////////////////////////////////////////////////////////////
string SFile::pathtail(void) const
{
   string expanded = expanded_name();

   size_t pos = expanded.find_last_of('/');
   if (pos != string::npos) {
       pos++;
       expanded = expanded.substr(pos,expanded.length()-pos);
   }

   // Now strip off the SFile extension
   pos = expanded.find_last_of('.');
   if (pos != string::npos)
       expanded = expanded.substr(0,pos);

   return expanded;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Returns the SFile name (including extension).
///////////////////////////////////////////////////////////////////////////////
string SFile::filename(void) const
{
   string expanded = expanded_name();

   size_t pos = expanded.find_last_of('/');
   if (pos != string::npos) {
      pos++;
      expanded = expanded.substr(pos,expanded.length()-pos);
   }

   return expanded;
}


// ---------- Static Utilities  ----------
///////////////////////////////////////////////////////////////////////////////
//  Description
//    Recursively Expand all Variable Names and Return the String
//  Implementation
//    All environment variables are expanded (recursively).  If
//    a string of the form $xxx is not defined, it is removed from
//    the path.
//    The following forms of environment variables are handled:
// 	$ECCE_HOME/file
//	$FILE
//	$ECCE_HOME$FILE
//
//    Not Handled yet.
//	${ECCE_HOME}/file
//	${FILE}
//	${ECCE_HOME}$FILE	other variations possible
//
//    An env var is assumed to start with a $ and end with one of
//    $, /, or end of string.
//  TODO
//    Support {} as in ${HOME}
///////////////////////////////////////////////////////////////////////////////
string SFile::expanded_name(const string& path)
{
   string expanded(path);
   size_t start, end;
   size_t endslash, enddollar;
   while ((start = expanded.find('$')) != string::npos)
   {
     if (expanded[start] != '{') 
     {
	// Find the first of $ or / to terminate this env var.
	enddollar = expanded.find("$",start+1);   // get past current $
	endslash = expanded.find("/",start);
	if (endslash == string::npos && enddollar == string::npos)
	   end = expanded.length()-1;
	else if (endslash != string::npos && enddollar != string::npos)
	   end = (endslash < enddollar) ? endslash-1 : enddollar-1;
	else if (endslash != string::npos)
	   end = endslash-1;
	else
	   end = enddollar-1;

	string envvar;
	if (start < end) envvar = expanded.substr(start+1,end-start);
	if (getenv(envvar.c_str()) != NULL)
	   expanded.replace(start,end-start+1,getenv(envvar.c_str()));
	else
	   expanded.replace(start,end-start+1,"");
     }
     else
     {
	// Some day put code here to handle Ecce::osdbPath type vars
     }
   }

   return expanded;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Return the tail portion of the full path.
///////////////////////////////////////////////////////////////////////////////
string SFile::pathtail(const string& path)
{
   string expanded = expanded_name(path);

   size_t pos = expanded.find_last_of('/');
   if (pos != string::npos) {
       pos++;
       expanded = expanded.substr(pos,expanded.length()-pos);
   }

   return expanded;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Returns TRUE if the argument refers to a directory.
//    Variables are not expanded.
//  Implementation
//    Note that for symbolic links, stat returns info on what the link
//    points to which is not what we intend this function to do so we
//    use lstat instead.
///////////////////////////////////////////////////////////////////////////////
bool SFile::is_dir(const string& path)
{
  bool retval = false;

  // Make sure we can traverse the directory hierarchey
  if (access(path.c_str(),F_OK) == 0) {
    struct stat stat_buf;
    stat(path.c_str(),&stat_buf);
    if (S_ISDIR(stat_buf.st_mode))
      retval = true;
  }
  return retval;
}

bool SFile::is_link(const string& path)
{
  bool retval = false;

  struct stat stat_buf;
  lstat(path.c_str(),&stat_buf);
  if (S_ISLNK(stat_buf.st_mode))
    retval = true;

  return retval;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Returns TRUE if the SFile exists and is a regular SFile.
//    Variable are not expanded.
///////////////////////////////////////////////////////////////////////////////
bool SFile::is_regular_file(const string& path)
{
  bool retval = false;

  // Make sure we can traverse the directory hierarchey
  if (access(path.c_str(),F_OK) == 0) {
    struct stat stat_buf;
    stat(path.c_str(),&stat_buf);
    if (S_ISREG(stat_buf.st_mode))
       retval = true;
  }
  return retval;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Substitutes '_' for characters that are SFile system unfriendly.
//    It is not possible to handle everything without context so
//    the user of this class may have to handle some special cases.
//    $ is left in (env interpretation).
//  Implementation:
//    Add other characters as needed.  I did this the hard way.
//    The string replace functions are not very nice.
///////////////////////////////////////////////////////////////////////////////
string SFile::make_name_valid(const string& name)
{
#ifdef really
  static const string invalid=" \t~!@#%^&*(){}[]|?<>='\";:";

  string ret;
  size_t len = name.length();
  for (int idx=0; idx<len; idx++)
    if (invalid.first(name[idx]) == RW_NPOS)
      ret.append(name[idx]);
    else
      ret.append('_');
  return ret;
#else
  return name;
#endif
}


// ---------- Operations on Operating System SFiles  ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Removes the SFile.  If the SFile cannot be removed, FALSE is
//    returned and EE_FILE_NOT_REMOVED is put on the queue.
//  Implemenation:
//    Causes a decrement to the number of links to a SFile.  If the
//    resulting number of links becomes 0, the SFile is actually
//    removed.  Otherwise it is not.  For many uses, this is
//    not a problem.  See unlink(2).
//    unlink(2) returns 0 if successful, -1 otherwise with an
//    error code in errno.  This method should put a message on
//    the queue when there are errors but currently does not.
///////////////////////////////////////////////////////////////////////////////
bool SFile::remove()
{
  bool ret = true;
  if (unlink(path(true).c_str()) != 0) {
    pushSystemError(errno,path(true).c_str());
    ret = false;
  } else if (exists()) {
    // Other links still exist
    ErrMsg().message("EE_FILE_NOT_REMOVED", EE_FATAL, WHERE, path().c_str());
    ret = false;
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Copies the exiting SFile resulting in a new physical SFile.  The
//    returned SFile object refers to the new SFile.
//  TODO
//    Make sure new SFile has old SFiles access modes.
///////////////////////////////////////////////////////////////////////////////
SFile* SFile::copy(const string& new_file)
{
  SFile* result = new SFile();
  if (exists() && is_regular_file()) {
    ifstream oldf(path(true).c_str());
    if (oldf) {
      *result = new_file;
      ofstream newf(result->path(true).c_str());
      if (newf) {
         char c;
         while (oldf.get(c)) {
           newf << c;
         }
         newf.close();
      } else {
        ErrMsg().message("EE_FILE_ACCESS", EE_FATAL, WHERE, path().c_str());
      }
      oldf.close();
    } else {
        ErrMsg().message("EE_FILE_ACCESS", EE_FATAL, WHERE, new_file.c_str());
    }
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Moves (renames) the SFile to the new SFile name.  If the SFile doe
//    not exist or is not a regular SFile, the move is not performed.
//  Implementation
//    Use temporary SFile object to properly handle env variables in dest_file.
///////////////////////////////////////////////////////////////////////////////
bool SFile::move(const string& dest_file)
{
  bool result = false;
  SFile tmp_file(dest_file);
  if (rename(path(true).c_str(),tmp_file.path(true).c_str()) == 0) {
    p_name = dest_file.c_str();
    result = true;
  } else {
    pushSystemError(errno,path(true).c_str());
  }
  return result;
}


// --------------------------
// Protected Member Functions
// --------------------------

///////////////////////////////////////////////////////////////////////////////
//  Description
//    Currently pop the the system message onto the queue.  The same
//    message key is always used.  errnum should be the system defined
//    errno.  Later we may want to define separate messages for each.
//
//    Limitation: some messages should really have two SFiles names
//                passed in (move for example).
///////////////////////////////////////////////////////////////////////////////
void SFile::pushSystemError(int errnum, const char *file)
{
  ErrMsg().message("EE_SYSFILE_ERR",EE_FATAL,WHERE,strerror(errnum),file);
}

///////////////////////////////////////////////////////////////////////////////
//  Description
//    Recursively Expand all Variable Names and Return the String
//  Implementation
//    All environment variables are expanded (recursively).  If
//    a string of the form $xxx is not defined, it is removed from
//    the path.
//    The following forms of environment variables are handled:
// 	$ECCE_HOME/file
//	$FILE
//	$ECCE_HOME$FILE
//
//    Not Handled yet.
//	${ECCE_HOME}/file
//	${FILE}
//	${ECCE_HOME}$FILE	other variations possible
//
//    An env var is assumed to start with a $ and end with one of
//    $, /, or end of string.
//  TODO
//    Support {} as in ${HOME}
///////////////////////////////////////////////////////////////////////////////
string SFile::expanded_name() const
{
   string expanded = p_name;
   size_t start, end;
   size_t endslash, enddollar;
   while ((start = expanded.find('$')) != string::npos)
   {
     if (expanded[start] != '{') 
     {
	// Find the first of $ or / to terminate this env var.
	enddollar = expanded.find("$",start+1);   // get past current $
	endslash = expanded.find("/",start);
	if (endslash == string::npos && enddollar == string::npos)
	   end = expanded.length()-1;
	else if (endslash != string::npos && enddollar != string::npos)
	   end = (endslash < enddollar) ? endslash-1 : enddollar-1;
	else if (endslash != string::npos)
	   end = endslash-1;
	else
	   end = enddollar-1;

	string envvar;
	if (start < end) envvar = expanded.substr(start+1,end-start);
	if (getenv(envvar.c_str()) != NULL)
	   expanded.replace(start,end-start+1,getenv(envvar.c_str()));
	else
	   expanded.replace(start,end-start+1,"");
     }
     else
     {
	// Some day put code here to handle Ecce::osdbPath type vars
     }
   }

   return expanded;
}

// ------------------------
// Private Member Functions
// ------------------------

// ---------- Friends ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    Print a well structured SFile to a specified stream.
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const SFile& SFile)
{
  os << "[File<";
  os << "Internal Name: " << SFile.p_name.c_str() << ",";
  os << "PathRoot: " << SFile.pathroot() << ",";
  os << "PathTail: " << SFile.pathtail() << ",";
  os << "Extension: " << SFile.extension() << ",";
  os << "Exists: " << SFile.exists() << ",";
  os << "Writable: " << SFile.is_writable() << ",";
  os << "Non_zero: " << SFile.non_zero() << ",";
  os << "Size: " << SFile.size() << ",";
  os << "Directory?: " << SFile.is_dir() << ",";
  os << "Regular SFile?: " << SFile.is_regular_file() << ",";
  os << "Last Modified? " << SFile.lastModified().toString() << ",";
  os << ">]" << endl;
  return os;
}
