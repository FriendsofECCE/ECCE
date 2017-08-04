/**
 * @file
 *
 *
 */
#include <sys/types.h> // stat
#include <sys/stat.h> // stat
#include <unistd.h> // stat
#include <stdlib.h>
#include <string.h>

#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
  using std::flush;
#include <fstream>
  using std::ofstream;

#include "util/Ecce.H"
#include "util/SFile.H"
#include "util/IndexOutOfRangeException.H"
#include "util/TempStorage.H"
#include "util/Color.H"

#include "util/UserEditor.H"


string UserEditor::p_editor = "";


/**
 * Constructor.
 */
UserEditor::UserEditor()
{
}


/**
 * Copy Constructor.
 */
UserEditor::UserEditor(const UserEditor& rhs)
{
}

 
/**
 * Destructor.
 */
UserEditor::~UserEditor()
{
}


/**
 * Get the name of the users prefered editor.
 * This implementation uses the EDITOR environment variable and defaults
 * to vi if necessary.
 */
string UserEditor::getPreferredEditor()
{
  if (p_editor == "") {
    char *tmp = getenv("ECCE_EDITOR");
    if (tmp != (char*)0) {
      p_editor = tmp;
    } else {
      tmp = getenv("EDITOR");
      if (tmp != (char*)0) {
        p_editor = tmp;
      } else {
        p_editor = "vi";
      }
    }
  }
  return p_editor;
}




void UserEditor::getEditCommand(const SFile& file,
      string& exe, 
      char *args[], int maxArgs,
      const string& name, 
      bool readOnly) throw(InvalidException)
{
  int curArg = 0;

   if (!file.exists() && readOnly) {
      throw InvalidException(file.path() + ": File not found: ", WHERE);
   }

   string quotedName = file.path(true);

   string usersEditor = getPreferredEditor();

   string msg;

   if (usersEditor=="emacs" ||
       (usersEditor.length()>5 &&
        usersEditor.find("/emacs")==usersEditor.length()-6)) {
      exe = getPath(usersEditor);

      if (exe != "") {
         addArg(args,curArg, maxArgs, exe.c_str());
         addColorArgs(args, curArg, maxArgs, readOnly);
         addArg(args, curArg, maxArgs, quotedName);
         if (readOnly) {
            addArg(args,curArg, maxArgs, "-l");
            addArg(args,curArg, maxArgs, string(Ecce::ecceDataPrefPath())+"/readonly.el");
            addArg(args,curArg, maxArgs, "-f");
            addArg(args,curArg, maxArgs, "find-file-read-only-from-command-line");
         }
      } else {
         msg = "Could not find editor command " + usersEditor + " in path.";
      }

   } else if (usersEditor=="vi" ||
              (usersEditor.length()>2 &&
               usersEditor.find("/vi")==usersEditor.length()-3)) {
      exe = getPath("xterm");

      if (exe != "") {
         addArg(args,curArg, maxArgs, exe.c_str());

         if (quotedName.find("amica.out") != string::npos) {
            // determine width of xterm based on longest line of file
            string cmd = "perl -e 'open(INFILE, \"" + quotedName + "\"); "
               "while (<INFILE>) {exit(0) if (length() > 81); "
               "exit(1) if ($lines_in++ > 1000);} exit(1);'";
            int istatus = system(cmd.c_str());
            istatus = istatus >> 8;
            addArg(args,curArg, maxArgs, "-geom");
            if (istatus == 0)
               addArg(args,curArg, maxArgs, "132x40");
            else
               addArg(args,curArg, maxArgs, "80x40");
         } else {
            addArg(args,curArg, maxArgs, "-geom");
            addArg(args,curArg, maxArgs, "80x40");
         }

         addColorArgs(args,curArg, maxArgs, readOnly);

         if (getenv("ECCE_XTERM_FONT")) {
            addArg(args,curArg, maxArgs, "-fn");
            addArg(args,curArg, maxArgs, getenv("ECCE_XTERM_FONT"));
         }

         addArg(args,curArg, maxArgs, "-T");
         addArg(args,curArg, maxArgs, name);
         addArg(args,curArg, maxArgs, "-e");
         addArg(args,curArg, maxArgs, usersEditor);
         if (readOnly) addArg(args,curArg, maxArgs, "-R");

         addArg(args,curArg, maxArgs, quotedName);

      } else {
         msg = "Could not find xterm in path.";
      }
   } else if (usersEditor=="jot" ||
         usersEditor.find("/jot")==usersEditor.length()-4) {
      exe = getPath(usersEditor);

      if (exe != "") {
         addArg(args,curArg, maxArgs, exe.c_str());
         if (readOnly)
            addArg(args,curArg, maxArgs, "-v");

         addArg(args,curArg, maxArgs, quotedName);
      } else {
         msg = "Could not find editor command " + usersEditor + " in path.";
      }
   } else if (usersEditor=="dtpad" ||
         usersEditor.find("/dtpad")==usersEditor.length()-6) {
      exe = getPath(usersEditor);

      if (exe != "") {
         addArg(args,curArg, maxArgs, exe.c_str());
         if (readOnly)
            addArg(args,curArg, maxArgs, "-v");

         addArg(args,curArg, maxArgs, quotedName);
      } else {
         msg = "Could not find editor command " + usersEditor + " in path.";
      }

   } else {
      // Its hard to know what arguments to use.  Just keep it as
      // minimal as possible.
      exe = getPath(usersEditor);

      if (exe != "") {
         addArg(args,curArg, maxArgs, exe);

         addArg(args,curArg, maxArgs, quotedName);
      } else {
         msg = "Could not find editor command " + usersEditor + " in path.";
      }
   }

   args[curArg] = (char*)0;

   if (msg != "") {
      throw (InvalidException(msg,WHERE));
   }

}

/**
 * Deletes memory for args.
 * The args list is presumed to be 0 pointers if not in use and
 * terminated by a 0 pointer.
 */
void UserEditor::freeArguments(char *args[])
{
   // Free up argument list
   for (int idx=0; args[idx] != 0; idx++) {
      delete [] args[idx];
   }
}



/**
 * Add arguments for setting ECCE foreground and background colors
 * on the command line for readonly vs editable.
 */
void UserEditor::addColorArgs(char *args[], 
                              int& curArg, 
                              int maxArg, 
                              bool readOnly)
{
  addArg(args,curArg,maxArg,"-bg");

  if (readOnly) {
    addArg(args, curArg, maxArg, Color::READONLY);
  } else {
    addArg(args, curArg, maxArg, Color::INPUT);
  }

  addArg(args, curArg, maxArg, "-fg");
  addArg(args, curArg, maxArg, Color::TEXT);
}



/**
 * Add the string to the argument list.
 */
void UserEditor::addArg(char *args[], int& curArg, int maxArg, const string& value)
{
  INDEXOUTOFRANGEEXCEPTION(curArg < maxArg, 0, maxArg, curArg);
  args[curArg] = new char[value.length()+1];
  strcpy(args[curArg],value.c_str());
  curArg++;
}






/**
 * Returns the path of the editor application.  The path is determined 
 * through the "which" command.
 */
string UserEditor::getPath(const string& app) const
{
  // WARNING:  this method used to be implemented by doing a popen and
  // then issuing the "which" command.  This proved not to work when
  // Ecce was invoked in the background.  There is something in the nature
  // of which requiring terminal access so it won't work in the background.
  // For an example, try a "which which&" which won't work :)

  struct stat statbuf;
  string path = app;

  if (path[0] != '/') {
    // WARNING:  must copy PATH because otherwise strtok corrupts it
    // which is not recommended if we want to keep Ecce happy
    char* pathvar = strdup(getenv("PATH"));

    if (pathvar != NULL) {
      char* tok;
      string trypath;
      bool foundFlag = false;

      for (tok = strtok(pathvar, ":"); tok!=NULL && !foundFlag;
           tok = strtok(NULL, ":")) {
        trypath = tok;
        trypath += "/" + app;
        foundFlag = stat(trypath.c_str(), &statbuf) == 0;
      }

      if (foundFlag)
        path = trypath;
      else
        path = "";

      free(pathvar);
    } else if (stat(path.c_str(), &statbuf) != 0)
      path = "";
  } else if (stat(path.c_str(), &statbuf) != 0)
      path = "";

  return path;
}

