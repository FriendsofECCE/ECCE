///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Host.H
//
//
// CLASS SYNOPSIS:
//
// EXPORTED TYPES:
//    Class Host
//
// LOCAL TYPES:
//
// SEE ALSO:
//      
// DESCRIPTION:
//	Host provides an abstraction for machine specific detail
//	information.  Currently, it only provides an interface on
//	top of uname type information.  
//
//	Host can only be instantiated on the local host.  It
//	cannot be used to get information on some arbitrary host.
//
// EXAMPLES:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef HOST_HH
#define HOST_HH

#include <iostream>
using std::ostream;

#include <string>
using std::string;


#include <sys/utsname.h>


class Host
{

  public:
    Host();
    Host(const Host&);

    virtual ~Host(void);

    // Attempts to provide fully qualified domain name as part of the host
    // such as tenenit.emsl.pnl.gov. There are no guarantees
    string fullyQualifiedName() const;

    string system_name() const;
    string host_name() const;
    string release() const;
    string version() const;
    string machine() const;

    // This method gets its info from resolve config files.  The information 
    // is not guaranteed to exist.
    static string domainName(void);

  protected:


  private:

    // State
    struct utsname p_uname;


    friend ostream& operator<< (ostream& os, const Host&);

}; // Host

#endif /* HOST_HH */
