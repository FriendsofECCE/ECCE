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
#include <sstream>
#include <vector>
  using std::vector;

#include "util/Ecce.H"
#include "util/SFile.H"
#include "util/IndexOutOfRangeException.H"
#include "util/TempStorage.H"
#include "util/Color.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"

#include "util/UserEditor.H"


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
 * The editor command, which may carry arguments ("emacs -nw").  Order:
 * ECCE_EDITOR, then Edit > Preferences, then VISUAL and EDITOR, then vi
 * (on macOS "open -t", the default text editor).
 * Read on every call so a preference change needs no restart.
 */
string UserEditor::getPreferredEditor()
{
  const char *tmp = getenv("ECCE_EDITOR");
  if (tmp != (const char*)0 && *tmp != '\0') return tmp;

  Preferences pref(PrefLabels::GLOBALPREFFILE);
  string fromPref;
  if (pref.getString(PrefLabels::EDITOR, fromPref) && !fromPref.empty()) {
    return fromPref;
  }

  static const char* const vars[] = { "VISUAL", "EDITOR" };
  for (unsigned i = 0; i < sizeof(vars)/sizeof(vars[0]); i++) {
    tmp = getenv(vars[i]);
    if (tmp != (const char*)0 && *tmp != '\0') return tmp;
  }
#ifdef __APPLE__
  return "open -t";
#else
  return "vi";
#endif
}


/**
 * The terminal command used for editors and remote shells; ECCE_TERMINAL,
 * then the preference, else xterm, or on macOS ecce-macos-terminal, which
 * runs "-e command" in Terminal.app.  May carry arguments.
 */
string UserEditor::getTerminal()
{
  const char *env = getenv("ECCE_TERMINAL");
  if (env != (const char*)0 && *env != '\0') return env;

  Preferences pref(PrefLabels::GLOBALPREFFILE);
  string term;
  if (pref.getString(PrefLabels::TERMINAL, term) && !term.empty()) return term;
#ifdef __APPLE__
  const char *home = getenv("ECCE_HOME");
  if (home != (const char*)0 && *home != '\0' && !strchr(home, ' ')) {
    return string(home) + "/scripts/ecce-macos-terminal";
  }
  return "ecce-macos-terminal";
#else
  return "xterm";
#endif
}


static vector<string> splitWords(const string& s)
{
  vector<string> words;
  std::istringstream is(s);
  string w;
  while (is >> w) words.push_back(w);
  return words;
}

static string baseName(const string& path)
{
  string::size_type slash = path.rfind('/');
  return slash == string::npos ? path : path.substr(slash+1);
}

static bool hasAny(const vector<string>& words, const char* a, const char* b)
{
  for (size_t i = 1; i < words.size(); i++) {
    if (words[i] == a || (b != NULL && words[i] == b)) return true;
  }
  return false;
}


void UserEditor::getEditCommand(const SFile& file,
      string& exe, 
      char *args[], int maxArgs,
      const string& name, 
      bool readOnly)
{
  int curArg = 0;

   if (!file.exists() && readOnly) {
      throw InvalidException(file.path() + ": File not found: ", WHERE);
   }

   string quotedName = file.path(true);

   string usersEditor = getPreferredEditor();
   vector<string> words = splitWords(usersEditor);
   if (words.empty()) words.push_back("vi");
   const string cmd = words[0];
   const string base = baseName(cmd);

   // Editors that need a terminal, and the flag that opens read-only.
   bool terminal = false;
   const char* roFlag = "";
   if (base=="vi" || base=="vim" || base=="nvim") {
      terminal = true;
      roFlag = "-R";
   } else if (base=="view" || base=="pico" || base=="micro") {
      terminal = true;
   } else if (base=="nano") {
      terminal = true;
      roFlag = "-v";
   } else if (base=="emacs" &&
              (hasAny(words, "-nw", "--no-window-system") ||
               hasAny(words, "-t", "--terminal"))) {
      terminal = true;
   }

   string msg;
   string cmdPath = getPath(cmd);

   if (cmdPath == "") {
      msg = "Could not find editor command " + cmd + " in path.";
   } else if (terminal) {
      vector<string> termWords = splitWords(getTerminal());
      if (termWords.empty()) termWords.push_back("xterm");
      const string termBase = baseName(termWords[0]);
      const bool isXterm = (termBase == "xterm");
      exe = getPath(termWords[0]);

      if (exe != "") {
         addArg(args,curArg, maxArgs, exe.c_str());
         for (size_t i = 1; i < termWords.size(); i++) {
            addArg(args,curArg, maxArgs, termWords[i]);
         }

         // Geometry, colours, font and title are xterm options; other
         // terminals get only -e.
         if (isXterm) {
            addArg(args,curArg, maxArgs, "-geom");
            if (quotedName.find("amica.out") != string::npos) {
               // determine width of xterm based on longest line of file
               string pcmd = "perl -e 'open(INFILE, \"" + quotedName + "\"); "
                  "while (<INFILE>) {exit(0) if (length() > 81); "
                  "exit(1) if ($lines_in++ > 1000);} exit(1);'";
               int istatus = system(pcmd.c_str());
               istatus = istatus >> 8;
               addArg(args,curArg, maxArgs, istatus == 0 ? "132x40" : "80x40");
            } else {
               addArg(args,curArg, maxArgs, "80x40");
            }

            addColorArgs(args,curArg, maxArgs, readOnly);

            if (getenv("ECCE_XTERM_FONT")) {
               addArg(args,curArg, maxArgs, "-fn");
               addArg(args,curArg, maxArgs, getenv("ECCE_XTERM_FONT"));
            }

            addArg(args,curArg, maxArgs, "-T");
            addArg(args,curArg, maxArgs, name);
         }
         addArg(args,curArg, maxArgs, "-e");
         for (size_t i = 0; i < words.size(); i++) {
            addArg(args,curArg, maxArgs, words[i]);
         }
         if (readOnly && *roFlag) addArg(args,curArg, maxArgs, roFlag);
         addArg(args,curArg, maxArgs, quotedName);
         if (readOnly && base=="emacs") {
            addArg(args,curArg, maxArgs, "-l");
            addArg(args,curArg, maxArgs, string(Ecce::ecceDataPrefPath())+"/readonly.el");
            addArg(args,curArg, maxArgs, "-f");
            addArg(args,curArg, maxArgs, "find-file-read-only-from-command-line");
         }

      } else {
         msg = "Could not find terminal " + termWords[0] + " in path.";
      }
   } else {
      exe = cmdPath;
      addArg(args,curArg, maxArgs, exe.c_str());
      for (size_t i = 1; i < words.size(); i++) {
         addArg(args,curArg, maxArgs, words[i]);
      }

      if (base=="emacs") {
         addColorArgs(args, curArg, maxArgs, readOnly);
         addArg(args,curArg, maxArgs, quotedName);
         if (readOnly) {
            addArg(args,curArg, maxArgs, "-l");
            addArg(args,curArg, maxArgs, string(Ecce::ecceDataPrefPath())+"/readonly.el");
            addArg(args,curArg, maxArgs, "-f");
            addArg(args,curArg, maxArgs, "find-file-read-only-from-command-line");
         }
         args[curArg] = (char*)0;
         return;
      } else if ((base=="jot" || base=="dtpad") && readOnly) {
         addArg(args,curArg, maxArgs, "-v");
      } else {
         // A single-instance editor hands the file to the running copy and
         // exits at once, which would end the session before any edit.
         const char *flag = NULL, *alt = NULL;
         if (base=="open") {
            // macOS open(1) returns at once; -W waits for the application
            // and -n starts a copy of its own, which ends with the session.
            if (!hasAny(words, "-W", "--wait-apps")) {
               addArg(args,curArg, maxArgs, "-W");
            }
            if (!hasAny(words, "-n", "--new")) {
               addArg(args,curArg, maxArgs, "-n");
            }
         } else if (base=="gedit" || base=="gnome-text-editor" || base=="xed") {
            flag = "--standalone"; alt = "-s";
         } else if (base=="geany") {
            flag = "-i"; alt = "--new-instance";
         } else if (base=="kate") {
            flag = "-n"; alt = "--new";
         }
         if (flag != NULL && !hasAny(words, flag, alt)) {
            addArg(args,curArg, maxArgs, flag);
         }
         if (base=="geany" && readOnly) {
            addArg(args,curArg, maxArgs, "-r");
         }
      }
      addArg(args,curArg, maxArgs, quotedName);
   }

   args[curArg] = (char*)0;

   if (msg != "") {
      // Free what was added; the caller does not get to see args on throw.
      freeArguments(args);
      args[0] = (char*)0;
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

