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
