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

#include <cstdlib>
#include <string>
#include <unistd.h>
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
      static string       sessionHost();
      static string       newSessionId();
      static bool         validSessionId(const string& id);
      static const char*  realUserHome();     // HOME
      static const char*  realUserPrefPath(); // $HOME/.ECCE* Prefs Directory
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
      static string ecceBinCommand(const string& name)
      {
        const char* home = getenv("ECCE_HOME");
        if (home == (const char*)0 || *home == '\0') return name;

        string path = string(home) + "/bin/" + name;
        if (access(path.c_str(), X_OK) == 0) return path;
        return name;
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
