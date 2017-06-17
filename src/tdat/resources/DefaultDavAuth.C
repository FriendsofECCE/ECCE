#include <iostream>
  using std::endl;
  using std::cout;

#include "util/ErrMsg.H"
#include "util/Ecce.H"

#include "tdat/DefaultDavAuth.H"

/**
  This class implements the AuthEventListener interface by using the
  authorization cache.

  A typical use would be to inherit from this class in any app that
  accesses dav and prompt if no valid credentials are found in cache.
 */

DefaultDavAuth::DefaultDavAuth() 
{
}

DefaultDavAuth::DefaultDavAuth(const DefaultDavAuth& rhs) 
{
  //cout << "DefaultDavAuth copy constructor not implemented" << endl;
}

DefaultDavAuth::~DefaultDavAuth()
{
}

// Get auth from cache and that is all.  If retry count is >1 return false
// Useful for things like jobstore.  So no need to throw exception or
// perhaps use execption to implement this.
bool DefaultDavAuth::getAuthorization(AuthEvent& event) throw (RetryException)
{
   bool ret = false;

   if (event.m_user == "") {
      event.m_user = Ecce::serverUser();
   }

   if (event.m_retryCount == 1)  {

      AuthCache& auth = AuthCache::getCache();
      BasicAuth *ba = NULL;
      ba = auth.getAuthentication(event.m_url,event.m_user,
            event.m_realm, event.m_retryCount);

      if (ba) {
         // Put in password.
         event.m_user = ba->m_user;
         event.m_password = ba->m_pass;
         delete ba;
         ret = true;
      }

   }

   return ret;
}

//
// This classes implementation is a noop.
//
void DefaultDavAuth::authorizationAccepted(const AuthEvent& event)
{

}
