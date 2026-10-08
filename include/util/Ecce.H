///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Ecce.H
//
//
// CLASS SYNOPSIS:
//    The Ecce class has the following uses: manage version information
//    for ECCE aplication (including parsing -v -V from the command-line),
//    and static data members and functions to manage required, non-
//    ObjectStore environment variables.
//
//      
// DESCRIPTION:
//    Ecce statics provide access to environment variable names as static
//    data members, and static functions that get the values of those
//    environment variables, or defaults if they are not defined.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef ECCE_HH
#define ECCE_HH

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#endif
using std::string;


// application includes

class Ecce
{

   public:

      // Generic initialization to be called by every app.
      static void initialize();

      static const char *ecceVersion();

      // Environment Variable Wrappers
      static const char*  realUserVar;        // ECCE_REALUSER
      static const char*  realUser();         // Accessor with Default
      static const char*  serverUser();       // Accessor with Default

      // Set once by the gateway, when the session's first successful
      // data-server login turns out to be a different name than
      // serverUser() had assumed (see Ecce.C for the full rationale).
      static void setSessionServerUser(const string& name);
      static void rememberServerUser(const string& name);
      // The session (#233): ECCE_SESSION_ID, 16 lower-case hex characters
      // made by `ecce` and inherited by every process of the session.
      // sessionKey() is "<host>_<id>" as packaging/gateway/ecce-session-
      // lib.sh derives it (the broker and credential file names, the
      // session topic level); both are "" without a well-formed id.
      static string       sessionId();
      static string       sessionKey();
      static string       sessionKeyFor(const string& id);
      // ~/.ECCE/session_<host>: the id of the newest session started on
      // this host by this account; "" without ECCE_REALUSERHOME.
      static string       sessionPointerFile();
      static string       sessionHost();
      static string       newSessionId();
      static bool         validSessionId(const string& id);
      // Whether a dialog can be shown at all (X/Wayland on Linux, always on
      // macOS and Windows), e.g. to ask for an ssh host key or password.
      static bool         guiAvailable();
      static const char*  realUserHome();     // HOME
      static const char*  realUserPrefPath(); // $HOME/.ECCE* Prefs Directory
      // Where configuration files are looked up, highest priority first
      // (#192).  Local mode: [~/.ECCE, install siteconfig].  -remote with a
      // cache of the server's files: [the user's server copy, the server's
      // site copy, install siteconfig]; a cache that does not exist is left
      // out, so with none this is the local list.  A layer from the server
      // has fromServer set.  Every dir ends in '/'.
      //
      // "localhost" is always the client: for machine == "localhost" the
      // server layers are dropped and the user layer is ~/.ECCE.
      struct SiteLayer {
        string dir;
        bool user;         // registrations of the user, not of the site
        bool fromServer;   // a cache of what the server publishes
      };
      static std::vector<SiteLayer> siteConfigLayers(const string& machine = "");
      // Path of `name` in the first site layer (not the user's) that has
      // it, else "".
      static string       siteConfigFile(const string& name,
                                         const string& machine = "");
      // The layer directories joined with ':', for gensub's
      // ECCE_SITECONFIG_DIRS.
      static string       siteConfigDirs(const string& machine = "");
      // Where the user's own registrations (MyMachines, Queues, <m>.Q,
      // CONFIG.<m>, MachPrefs) are read and written; trailing '/'.
      static string       userRegistrationDir();
      // "<host>_<port>" of the -remote data server; "" in local mode.
      static string       siteCacheKey();
      static const char*  ecceHomeVar;        // ECCE_HOME
      static const char*  ecceHome();         // Accessor with Default
      static const char*  ecceDataPathVar;    // ECCE_DATA - Data Directory
      static const char*  ecceDataPath();     // Accessor with Default
      static const char*  ecceDataLoadPath(); // ECCE_DATA/refload
      static const char*  ecceDataPrefPath(); // ECCE_DATA/config
      static const char*  ecceDataGBSPath();  // ECCE_DATA/basissets
      static const char*  ecceDataControllersPath();

      // Full path to a helper program in $ECCE_HOME/bin.
      //
      // INLINE ON PURPOSE.  MoFragments.C uses it and is compiled
      // standalone by tests/symmetry, which has no util library to
      // link against -- an out-of-line definition here put the whole
      // of Ecce.C, and everything it drags in, on that test's link
      // line.  It is getenv and access; there is nothing to hide.
#ifdef _WIN32
      // A path with a space is quoted so cmd.exe and sh read it as one word.
      static string winQuote(const string& p)
      {
        return p.find(' ') == string::npos ? p : "\"" + p + "\"";
      }
#endif
      static string ecceBinCommand(const string& name)
      {
        const char* home = getenv("ECCE_HOME");
        if (home == (const char*)0 || *home == '\0') return name;

        string path = string(home) + "/bin/" + name;
#ifdef _WIN32
        // Windows programs carry .exe, and a build tree keeps them
        // beside the running program rather than under $ECCE_HOME/bin.
        string exe = path + ".exe";
        if (access(exe.c_str(), 0) == 0) {
          for (size_t i = 0; i < exe.size(); i++) if (exe[i] == '/') exe[i] = '\\';
          return winQuote(exe);   // cmd.exe reads "/" in a command as a switch
        }
        char self[MAX_PATH];
        DWORD n = GetModuleFileNameA(NULL, self, MAX_PATH);
        if (n > 0 && n < MAX_PATH) {
          string dir(self, n);
          size_t slash = dir.find_last_of("\\/");
          if (slash != string::npos) {
            string beside = dir.substr(0, slash + 1) + name + ".exe";
            if (access(beside.c_str(), 0) == 0) return winQuote(beside);
          }
        }
        return name;
#endif
        if (access(path.c_str(), X_OK) == 0) return path;
        return name;
      }

      // The exit status of a system() call.  POSIX returns a wait status
      // (status << 8); Windows returns the exit code itself, which ">> 8"
      // would turn into 0 for every code below 256.
      static int systemExitStatus(int rc)
      {
#ifdef _WIN32
        return rc;
#else
        return rc == -1 ? -1 : (rc >> 8) & 0xff;
#endif
      }

      // A script to run by path in a command line.  cmd.exe cannot run an
      // extensionless script, so on Windows its #! line names the
      // interpreter (perl or python3) and the path is quoted.
      static string scriptCommand(const string& path)
      {
#ifdef _WIN32
        string first;
        if (FILE* f = fopen(path.c_str(), "r")) {
          char buf[256];
          if (fgets(buf, sizeof buf, f)) first = buf;
          fclose(f);
        }
        string quoted = "\"" + path + "\"";
        if (first.compare(0, 2, "#!") == 0) {
          if (first.find("perl") != string::npos) return "perl " + quoted;
          if (first.find("python") != string::npos) return "python3 " + quoted;
        }
        return quoted;
#else
        return path;
#endif
      }

      // system() for a command line of quoted words and redirections;
      // returns the exit status.  On Windows it is cmd.exe /s /c "line"
      // (/s keeps the line's own quotes), with no console window, which a
      // GUI program's system() would flash up, and no inherited handles.
      static int runCommand(const string& cmd)
      {
#ifdef _WIN32
        const char* comspec = getenv("COMSPEC");
        string shell = comspec && *comspec ? comspec : "cmd.exe";
        string line = "\"" + shell + "\" /d /s /c \"" + cmd + "\"";
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        if (!CreateProcessA(NULL, &line[0], 0, 0, FALSE, CREATE_NO_WINDOW,
                            0, 0, &si, &pi))
          return -1;
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD code = 1;
        GetExitCodeProcess(pi.hProcess, &code);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return (int)code;
#else
        return systemExitStatus(system(cmd.c_str()));
#endif
      }

      // site_runtime settings that can't be overriden
      static bool   ecceAutoAccounts();
      static bool   ecceStoreTrajectories();

   protected:

      Ecce();
      Ecce(const Ecce&);
      virtual ~Ecce();


   private:

      static     const char*  p_ecceVersion;



};

#endif /* ECCE_HH */
