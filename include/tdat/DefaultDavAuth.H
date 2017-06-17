#ifndef DEFAULTDAVAUTH_H
#define DEFAULTDAVAUTH_H

#include "util/AuthEvent.H"
#include "tdat/AuthCache.H"

// This class provides default behavior for the abstract AuthEvent
// interface.  Its useful for jobstore or apps that aren't going to
// interact with the user.
// 
// Does not update cache and or send messages - in other words
// authorization accespted is a noop.
//
class DefaultDavAuth : public AuthEventListener
{
public:
  DefaultDavAuth();
  virtual ~DefaultDavAuth();

   // AuthEvent virtuals
   bool getAuthorization(AuthEvent& event) throw (RetryException);
   void authorizationAccepted(const AuthEvent& event);

protected:
  DefaultDavAuth(const DefaultDavAuth& rhs);

};
#endif
