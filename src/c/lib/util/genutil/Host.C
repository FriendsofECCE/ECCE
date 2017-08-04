///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Host.C
//
//
// DESIGN:
//	Simple interface to uname().  See man 2 uname.  It's worth
//	putting in a class since the structure is pretty OS dependent
//	and we can hide that behind the class interface.  The key
//	difference seems to be the length of strings in the 
//	struct uname structure and the number of reserved attributes
//	of the structure.
//
///////////////////////////////////////////////////////////////////////////////

#include <strstream>
using std::ostrstream;
#include <fstream>
using std::ifstream;

#include <string.h>

#include <netdb.h>        // gethostbyname

#include "util/ErrMsg.H"
#include "util/Host.H"
#include "util/StringTokenizer.H"

// uname is not properly prototyped
extern "C" int uname(struct utsname *name);

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//	Constructs an object for the current local host.  The object then
//	provides a simple interface to uname and other system commands
//	associated with host information.  Failure to construct the
//	object is a fatal error.
//
// Implementation:
//	Call uname once upon construction.  Then the accessors just use
//	the cached structure.
//
///////////////////////////////////////////////////////////////////////////////
Host::Host(void)
{
   int p_status = (short) uname(&p_uname);
   EE_RT_ASSERT(p_status!=-1,EE_FATAL,"uname() failure");
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//	Copy constructor.
//
///////////////////////////////////////////////////////////////////////////////
Host::Host(const Host& host)
{
   strcpy(p_uname.sysname,host.system_name().c_str());
   strcpy(p_uname.nodename,host.host_name().c_str());
   strcpy(p_uname.release,host.release().c_str());
   strcpy(p_uname.version,host.version().c_str());
   strcpy(p_uname.machine,host.machine().c_str());
}

 
// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//	Destructor.
//
///////////////////////////////////////////////////////////////////////////////
Host::~Host(void)
{
}


///////////////////////////////////////////////////////////////////////////////
//  Try to use gethostbyname.  This may return something like
//  localhost.localdomain.  I don't know how standard those strings are but
//  I'm assuming them for the logic at this point.
//  If that fails try the method that looks in /etc/resolve.
///////////////////////////////////////////////////////////////////////////////
string Host::fullyQualifiedName() const
{
  // cout << "fully QName" << endl;
  string ret = host_name();
  // cout << "host " << ret << endl;
  struct hostent* host = gethostbyname(ret.c_str());
  if (host != NULL) {
    // cout << "ok" << host->h_name << endl;
    if (strstr(host->h_name,"localhost") == 0) {
      ret = host->h_name;
    } else {
      string domain = Host::domainName();
      // cout << "fallback " << domain << endl;
      ret.append("." + domain);
    }
  }
  return ret;
}


///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//	Returns the operationg system name.
//
// Implementation:
//
///////////////////////////////////////////////////////////////////////////////
string Host::system_name(void) const
{
   return (string) p_uname.sysname;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//	Returns the host name.  Examples: bike star
//
// Implementation:
//
///////////////////////////////////////////////////////////////////////////////
string Host::host_name(void) const
{
   return (string) p_uname.nodename;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//	Returns the release number which is of the form major.minor[.patch].
//	Examples: 4.1.3 or 5.3
//
// Implementation:
//
///////////////////////////////////////////////////////////////////////////////
string Host::release(void) const
{
   return (string) p_uname.release;
}
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//	Returns the version string.  
//
// Implementation:
//
///////////////////////////////////////////////////////////////////////////////
string Host::version(void) const
{
   return (string) p_uname.version;
}
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//	Return the machine hardware type.  Examples: sun4m mips.
//
// Implementation:
//
///////////////////////////////////////////////////////////////////////////////
string Host::machine(void) const
{
   return (string) p_uname.machine;
}

string Host::domainName(void)
{
  string result;
  ifstream resolv("/etc/resolv.conf");
  EE_RT_ASSERT(resolv, EE_FATAL,
               "No System \"resolv.conf\" File Found.");
  ostrstream temp;
  temp << resolv.rdbuf();
  char* tmpStr = temp.str();
  string resolvStr(tmpStr);
  delete tmpStr;
  
  StringTokenizer token(resolvStr, " \n\t");

   while (!((result=token.next()).empty()) ) {
    if (result == "domain" || result == "search") {
      result = token.next();
      break;
    }
    result = token.next();
  }
  resolv.close();

  return result;
}




///////////////////////////////////////////////////////////////////////////////
// man
//
// Description:
//	Dump the object.  This is mainly a debug/test tool.
//
// Implementation:
//
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const Host& host)
{
  os << "System: " << host.system_name() << endl;
  os << "Host: " << host.host_name() << endl;
  //os << "Node Extension: " << host.node_ext() << endl;
  os << "Release: " << host.release() << endl;
  os << "Version: " << host.version() << endl;
  os << "Machine: " << host.machine() << endl;
  return os;
}

