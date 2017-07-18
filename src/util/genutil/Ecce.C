///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Ecce.C
//
//
// DESIGN:
//	This class is intended for generic Ecce functions like
//	argument processing and version number management.
//
//	Currently, it process the arguments -v and -V.  This should
//	be pulled from the argument stack but currently are not.
//
//	Most of the information about the version number is passed in
//	at instantiation time.  Thus this class is really intended to
//	be instantiated once at the beginning of each program.
//
//	The operating system and machine architecture are defined
//	statically within this class at compile time.  This class
//	will not compile if ECCE_OS is not defined.
//
//	Each program must define the global version data necessary
//	to instantiate this class.  A more desirable strategy would
//	be to do everything through the build process but this will
//	be impractical on ECCE' where a single build or tape cut
//	can include more than one tool with differing versions.
//
//      The environment variable static data members and methods
//      handle defaults if non-essential environment varaibles are
//      not set.
//
///////////////////////////////////////////////////////////////////////////////

#include <strstream>
using std::ostrstream;

#include <string.h>
#include <locale.h>


#include "util/EcceException.H"
#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/KeyValueReader.H"
#include "util/SFile.H"
#include "util/Preferences.H"


// ECCE_VERSION must be supplied by the Makefile.
const char* Ecce::p_ecceVersion = ECCE_VERSION;


// -----------------------
// Public Member Functions
// -----------------------

// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
// man
//  Description
//    Virtual destructor.
///////////////////////////////////////////////////////////////////////////////
Ecce::~Ecce(void)
{ }



///////////////////////////////////////////////////////////////////////////////
// Perform standard initialization required by all applications.
///////////////////////////////////////////////////////////////////////////////
void Ecce::initialize()
{
   // Force use of standard locale.  According to the man pages:
   // The locale "C"  or  "POSIX"  is  a  portable  locale;  its
   // LC_CTYPE  part  corresponds  to  the 7-bit ASCII character set.
   // We do this so that date strings are in english and not chineese or
   // some other language that messes up dav calls.
   // Alternatively we could use setlocale(LC_TIME,"") and use an
   // environment variable if C turns out not to be portable.
   // We also had problems with LC_NUMERIC for french locales so just
   // try setting LC_ALL rather than piece meal them.
   // Note that the "" argument causes setlocale to check env variables 
   // which should get set in the Ecce script.  This seemed to a better idea
   // than hardwiring it in case we run across special problems.
   setlocale(LC_ALL, "");

   // Define a default exception handler that dumps core instead of just exit.
   EcceException::setDefaultHandler();

}



///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//	Returns the complete version name.
///////////////////////////////////////////////////////////////////////////////
const char * Ecce::ecceVersion() 
{
  return p_ecceVersion;
}



///////////////////////////////////////////////////////////////////////////////
//  man
//  Description
//    These static methods and the corresponding static data members
//    handle the retrieval of environment variable values.  They also
//    support default values if the environment variables are optional.
//    Run-Time errors are signalled if required environment variables
//    are not defined.
///////////////////////////////////////////////////////////////////////////////
const char* Ecce::realUser(void)
{
  static const char* realUser = getenv(Ecce::realUserVar);
  EE_RT_ASSERT(realUser !=  (const char*)0, EE_FATAL,
	       string("You Must Define ") + Ecce::realUserVar);
  return realUser;
}
const char* Ecce::serverUser(void)
{
  Preferences prefs("ServerLogin");
  static string login;
  if (prefs.getString("Login", login)) {
    return login.c_str();
  } else {
    return Ecce::realUser();
  }
}
const char* Ecce::realUserHome(void)
{
  static const char* userHome = getenv("ECCE_REALUSERHOME");
  EE_RT_ASSERT(userHome !=  (const char*)0, EE_FATAL,
	       "You Must Define ECCE_REALUSERHOME");
  return userHome;
}
const char* Ecce::ecceHome(void)
{
  static const char* ecceHome = getenv(Ecce::ecceHomeVar);
  EE_RT_ASSERT(ecceHome !=  (const char*)0, EE_FATAL,
	       string("You Must Define ") + Ecce::ecceHomeVar);
  return ecceHome;
}
const char* Ecce::realUserPrefPath(void)
{
  static string result;

  // Initailize this way for g++
  if (result.empty()) {
    result = Ecce::realUserHome();
    result += "/.ECCE/";
  }
  return result.c_str();
}
const char* Ecce::ecceDataPath(void)
{
  static const char* environment = getenv(Ecce::ecceDataPathVar);
  static string defaultDataPath;
  
  // Initailize this way for g++
  if (defaultDataPath.empty()) {
   defaultDataPath = Ecce::ecceHome();
   defaultDataPath += "/data";
  }

  if (environment != (const char*)0) {
    return environment;
  } else {
    return defaultDataPath.c_str();
  }
}
const char* Ecce::ecceDataLoadPath(void)
{
  static string result;

  // Initailize this way for g++
  if (result.empty()) {
    result = Ecce::ecceDataPath();
    result += "/admin/refload";
  }
  return result.c_str();
}
const char* Ecce::ecceDataPrefPath(void)
{
  static string result;
  // Initailize this way for g++
  if (result.empty()) {
    result = Ecce::ecceDataPath();
    result +=  "/client/config";
  }
  return result.c_str();
}

const char* Ecce::ecceDataGBSPath(void)
{
  static string result;
  // Initailize this way for g++
  if (result.empty()) {
    result = Ecce::ecceDataPath();
    result +=  "/admin/basissets";
  }
  return result.c_str();
}

const char* Ecce::ecceDataControllersPath(void)
{
  static string result;
  // Initailize this way for g++
  if (result.empty()) {
    result = Ecce::ecceHome();
    result +=  "/scripts/parsers";
  }
  return result.c_str();
}

bool Ecce::ecceAutoAccounts(void)
{
  string path;
  if (getenv("ECCE_REMOTE_SERVER")) {
    path = "$ECCE_HOME/siteconfig/RemoteServer/site_runtime";
  } else {
    path = "$ECCE_HOME/siteconfig/site_runtime";
  }
  SFile file(path);
  KeyValueReader reader(file.path(true));
  reader.setSeparator(' ');

  string key, value;
  while (reader.getpair(key, value) && key!="ECCE_AUTO_ACCOUNTS");
  bool status = key == "ECCE_AUTO_ACCOUNTS" &&
                       (value == "yes" || value == "Yes" || value == "YES" ||
                        value == "true" || value == "True" || value == "TRUE");

  return status;
}

bool Ecce::ecceStoreTrajectories(void)
{
  string path;
  if (getenv("ECCE_REMOTE_SERVER")) {
    path = "$ECCE_HOME/siteconfig/RemoteServer/site_runtime";
  } else {
    path = "$ECCE_HOME/siteconfig/site_runtime";
  }
  SFile file(path);
  KeyValueReader reader(file.path(true));
  reader.setSeparator(' ');

  string key, value;
  while (reader.getpair(key, value) && key!="ECCE_STORE_TRAJECTORIES");
  bool status = key == "ECCE_STORE_TRAJECTORIES" && 
                       (value == "yes" || value == "Yes" || value == "YES" ||
                        value == "true" || value == "True" || value == "TRUE");

  return status;
}

//////////////////////////////////////////////////////////////////////////////
//
//  These are so confusing - Here's examples using my trees:
//
//  ECCE_HOME = /files/ecceDir
//  ECCE_DATA = /files/ecceDir/data ($ECCE_HOME/data)
//
//////////////////////////////////////////////////////////////////////////////
const char* Ecce::realUserVar = "ECCE_REALUSER";
const char* Ecce::ecceDataPathVar = "ECCE_DATA";
const char* Ecce::ecceHomeVar = "ECCE_HOME";



// ---------- Hidden Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//  Description
//    Hidden constructors to prevent uninitialized Ecce objects.
///////////////////////////////////////////////////////////////////////////////
Ecce::Ecce(void)
{ }
Ecce::Ecce(const Ecce&)
{ }
 

