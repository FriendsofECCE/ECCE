///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: SDirectory.H
//
//
// CLASS SYNOPSIS:
//    Wrapper Class for File-System Specific Directories.
//
// DESCRIPTION:
//    Like SFile, SDirectory is designed to allow us to point to
//    and track UNIX system (or theoretically any system)
//    directories.  Much of the behavior is inherited from File.
//    Virtual functions that manipulate directories as opposed to
//    files have been added.
//
// EXAMPLES:
//    testDirectory
//
///////////////////////////////////////////////////////////////////////////////
#ifndef SDIRECTORY_HH
#define SDIRECTORY_HH

#include <iostream>
#include <string>
#include <vector>
#include <functional>
using std::string;
using std::vector;
using std::less;


#include "util/SFile.H"
#include "util/EcceSortedVector.H"

class SDirectory : public SFile
{

public:

  // Constructors
    SDirectory(void);
    SDirectory(const string& name, int create_mode = 0);
    SDirectory(const SDirectory& dir);

  // Virtual Destructor
    virtual ~SDirectory(void);

  // Operators
    SDirectory& operator=(const SDirectory& dir);
    SDirectory& operator=(const string& dir);
    bool  operator==(const SDirectory& dir) const;
    bool  operator!=(const SDirectory& dir) const;

  // Modifiers

  // Accessors
    vector<SFile> get_files(bool recursive = true) const;
    void get_files(vector<SFile>& fileList,
                   bool recursive = true) const;
    void getFileNames(EcceSortedVector<string, less<string> >& fileListE) const;
    int  count(bool recursive = false) const;

  // Operations on actual directories
    bool        remove(void);
    SFile*      copy(const string& new_file);
    bool        move(const string& dest_file);
    static bool create(const string& name, int mode);

protected:


private:

  // State
    // All State is Inherited from File


  // Friends
  friend ostream& operator<< (ostream& os, const SDirectory&);

}; // SDirectory

#endif /* SDIRECTORY_HH */
