///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: SFile.H
//
//
// CLASS SYNOPSIS:
//    Encapsulate operating system files to provide file system
//    independence.
//
// EXPORTED TYPES:
//    Class SFile
//
// DESCRIPTION:
//    A SFile can be instantiated with an absolute or relative path
//    or with a path that includes one or more environment variables.
//    Nested environment variable definitions are supported.
//    No explicit support is provided for relative filenames (i.e.,
//    there is no mechanism to construct the absolute path) so be
//    careful when using operator== and operator!=.
//
//    When any of the methods that return part of the file name or
//    return information about the file are invoked, the environment
//    variables are interpreted dynamically.  If a variable is
//    undefined, it is replaced with the 0 length string.
//
//    File status information is taken from stat(3).
//
//    IMPORTANT:
//    The results of pathroot, pathtail, and extension are undefined if
//    exists() returns FALSE since the results are dependent  upon
//    whether the file is a directory or regular file etc.
//
//    Filenames like .cshrc are considered hidden files.  The path
//    and extension in this example are "".
//
//    The following are considered valid uses of environment
//    variables.  However, it is the users responsibility to
//    ensure they are semantically correct.
//	    SFile("$USER");
//	    SFile("/tmp/$USER");
//	    SFile("$HOME$USER")
//
///////////////////////////////////////////////////////////////////////////////
#ifndef SFILE_HH
#define SFILE_HH

#include <iostream>
  using std::ostream;

#include <stdio.h>

#include <string>
  using std::string;

#include "util/TDateTime.H"

class SFile 
{

public:
  // Constructors
      SFile(void);
      SFile(const string& name, int create_mode = 0);
      SFile(const char *name, int create_mode = 0);
      SFile(const SFile& file);

  // Virtual Destructor
      virtual ~SFile(void);

  // Operators
      SFile&     operator=(const SFile& file);
      SFile&     operator=(const string& name);
      bool operator==(const SFile& file) const;
      bool operator!=(const SFile& file) const;

  // Modifiers

  // Accessors
      // File Attributes
      TDateTime    lastModified(void) const;
      bool non_zero() const;
      off_t     size() const;
      // File Access
      bool exists() const;
      bool is_writable() const;
      bool is_readable() const;
      // File Type Information
      bool is_dir() const;
      bool is_link() const;
      bool is_regular_file() const;
      // File Path and Name Access
      string extension() const;
      string path(const bool expand = false) const;
      string pathroot() const;
      string pathtail() const;
      string filename(void) const;

  // Static Utilities
      static bool is_dir(const string& path);
      static bool is_link(const string& path);
      static bool is_regular_file(const string& path);
      static string make_name_valid(const string& name);
      static string pathtail(const string& path);
      static bool create(const string& path,int mode=0644);

  // Operations on Operating System Files
      virtual bool remove(void);
      virtual SFile*     copy(const string& new_file);
      virtual bool move(const string& dest_file);

protected:

  // Inheritable Utilities
      string expanded_name(void) const;
      static string expanded_name(const string& path);
      static void pushSystemError(int errnum, const char *file);

private:

  // State
      string     p_name;             // User Supplied File Name

  // Encapsulated Behavior

  // Friends
     friend ostream& operator<<(ostream& os, const SFile& file);

}; // SFile

#endif /* SFILE_HH */
