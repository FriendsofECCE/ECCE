#ifndef AUTHEVENT_H
#define AUTHEVENT_H

#include <string>
  using std::string;

#include "util/RetryException.H"

struct  AuthEvent {
  string m_realm;   // as in WWW-Authenticate
  string m_url;
  string m_user;
  string m_password;
  int    m_retryCount;
  bool   m_newUser;
};

class AuthEventListener
{
  public:
  // Implement this method and add self as listener to providers of this
  // event.  Returning false means no password was retrieved.  Otherwise,
  // should return user and password (unencrypted).
  virtual bool getAuthorization(AuthEvent& event) throw (RetryException) = 0;

  // When getAuthorization returns true, we still don't know if that
  // info was valid.  This method gets called when it has been determined
  // to be valid.
  virtual void authorizationAccepted(const AuthEvent& event) = 0;
};
#endif
