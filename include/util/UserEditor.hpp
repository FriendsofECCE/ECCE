/**
 * @file 
 *
 *
 */
#ifndef USEREDITOR_H
#define USEREDITOR_H

#include <string>
  using std::string;


class SFile;

#include "util/InvalidException.H"

/**
 * A class to provide system information about the users preferred editor.
 * The default implementation makes use of the EDITOR environment variable.
 * This class also has methods to assist with starting a user edit session.
 * Actually starting and tracking editor sessions is typically system 
 * and UI dependent.  This class pulls out some generic support routines. 
 */
class UserEditor 
{
   public:
      UserEditor();
      UserEditor(const UserEditor& rhs);
      virtual ~UserEditor();

      virtual string getPreferredEditor();
      virtual void   getEditCommand(const SFile& file, 
                            string& exe, char *args[], int maxArgs,
                            const string& name, bool readOnly=false)
                            throw(InvalidException);
      void freeArguments(char *args[]);

   protected:

    string getPath(const string& app) const;
    void addArg(char *args[], int& curArg, int maxArg, const string& value);
    void addColorArgs(char *args[], int& curArg, int maxArg, bool readOnly);


    static string p_editor;

};

#endif
