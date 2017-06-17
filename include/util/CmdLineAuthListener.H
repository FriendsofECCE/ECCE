#ifndef CMDLINEAUTHLISTENER_H
#define CMDLINEAUTHLISTENER_H

#include "util/AuthEvent.H"

//
// This implementation of AuthEventListener gets authentication
// information by prompting the user at the command line.  It does NOT
// deal with a cache at all.  Its useful for test programs and other
// stand alone maintenance programs like os_to_dav.
// 
//
class CmdLineAuthListener : public AuthEventListener
{
public:
  CmdLineAuthListener();
  virtual ~CmdLineAuthListener();

   // AuthEvent virtuals
   bool getAuthorization(AuthEvent& event) throw (RetryException);
   void authorizationAccepted(const AuthEvent& event);

protected:
  CmdLineAuthListener(const CmdLineAuthListener& rhs);

};
#endif
