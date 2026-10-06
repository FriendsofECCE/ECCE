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
#include <stdlib.h>              // getenv
#include <unistd.h>              // access
#include <locale.h>
#include <cstdint>
#include <random>


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
// The remembered login is kept separately per mode: a name recalled
// for the central server (ECCE_REMOTE_SERVER) must never become the
// default for the local data server, and vice versa. Shared by
// serverUser() (reads it) and rememberServerUser() (writes it).
static const char* serverLoginPrefFile()
{
  return (getenv("ECCE_REMOTE_SERVER") != (const char*)0)
       ? "ServerLogin.remote" : "ServerLogin";
}

const char* Ecce::serverUser(void)
{
  // `ecce -l NAME`, or a name typed into the first login dialog of the
  // session (see setSessionServerUser()), is session-only: exported
  // rather than written to a file, so it can never leak into a later
  // plain `ecce` as a remembered login for the wrong server.
  const char* sessionLogin = getenv("ECCE_SERVER_LOGIN");
  if (sessionLogin != (const char*)0 && sessionLogin[0] != '\0') {
    return sessionLogin;
  }

  Preferences prefs(serverLoginPrefFile());
  static string login;
  if (prefs.getString("Login", login)) {
    return login.c_str();
  } else {
    return Ecce::realUser();
  }
}

// Called once, by whichever process owns the session's first successful
// data-server login (the gateway), when the name that actually
// authenticated differs from what serverUser() had assumed -- e.g. the
// user changed the name in the login dialog. Every later serverUser()
// call in THIS process, and in every process this one goes on to spawn
// (environment is inherited), returns `name` from then on.
void Ecce::setSessionServerUser(const string& name)
{
  setenv("ECCE_SERVER_LOGIN", name.c_str(), 1);
}

// Remembers `name` as the server login to default to next time, in the
// file for the current mode (see serverLoginPrefFile()) -- or, if it is
// the same as the Unix username, clears any existing override so that
// default (no file needed) applies again. Never call this for a name
// that came from `ecce -l`: that stays session-only by design, and
// letting it reach this file is exactly the bug this pair of functions
// exists to prevent (a central-server login becoming a later local
// session's default, or vice versa).
void Ecce::rememberServerUser(const string& name)
{
  Preferences prefs(serverLoginPrefFile());
  if (name == string(Ecce::realUser())) {
    prefs.remove_entry("Login");
  } else {
    prefs.setString("Login", name);
  }
  prefs.saveFile();
}
bool Ecce::validSessionId(const string& id)
{
  if (id.size() != 16) return false;
  for (size_t i = 0; i < id.size(); i++)
    if (!((id[i] >= '0' && id[i] <= '9') || (id[i] >= 'a' && id[i] <= 'f')))
      return false;
  return true;
}

string Ecce::sessionId()
{
  const char* id = getenv("ECCE_SESSION_ID");
  return (id && validSessionId(id)) ? string(id) : string();
}

// ECCE_HOST, else HOST, else the host name: ecce_session_host's order.
string Ecce::sessionHost()
{
  const char* host = getenv("ECCE_HOST");
  if (!host || !*host) host = getenv("HOST");
  if (host && *host) return host;
  char buf[256];
  if (gethostname(buf, sizeof(buf) - 1) != 0) return "localhost";
  buf[sizeof(buf) - 1] = '\0';
  return buf;
}

// tr -c 'A-Za-z0-9._-' '_' in the C locale, byte by byte.
static string sessionSanitize(string s)
{
  for (size_t i = 0; i < s.size(); i++) {
    unsigned char ch = s[i];
    bool keep = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' ||
                ch == '-';
    if (!keep) s[i] = '_';
  }
  return s;
}

string Ecce::sessionKeyFor(const string& id)
{
  if (!validSessionId(id)) return "";
  return sessionSanitize(sessionHost() + "_" + id);
}

string Ecce::sessionKey()
{
  return sessionKeyFor(sessionId());
}

string Ecce::sessionPointerFile()
{
  const char* home = getenv("ECCE_REALUSERHOME");
  if (!home || !*home) return "";
  return string(home) + "/.ECCE/session_" + sessionSanitize(sessionHost());
}

string Ecce::newSessionId()
{
  std::random_device rd;
  static const char hex[] = "0123456789abcdef";
  string id;
  for (int i = 0; i < 2; i++) {          // 2 x 32 bits = 16 hex digits
    uint32_t word = rd();
    for (int j = 0; j < 4; j++) {
      id += hex[(word >> (8 * j + 4)) & 0xf];
      id += hex[(word >> (8 * j)) & 0xf];
    }
  }
  return id;
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

//  The helper programs in bin (autosym, passdialog, eccejobmaster, ...)
//  are not on the user's PATH, and the applications no longer run with
//  their working directory set to the bin directory.  Every historical
//  "./<name>" invocation of one therefore found nothing, and each of
//  them failed silently in its own way (GitHub #134).
//
//  Falls back to the bare name rather than asserting, so a developer
//  with the helpers on PATH still works and an unset ECCE_HOME degrades
//  to the old behavior instead of taking the application down.
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
 

