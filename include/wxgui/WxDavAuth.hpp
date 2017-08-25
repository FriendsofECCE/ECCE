/**
 * @file
 *
 *
 */
#ifndef WXDAVAUTH_H
#define WXDAVAUTH_H

class wxWindow;

#include "util/AuthEvent.H"

#include "tdat/AuthCache.H"


/**
 * This class provides default behavior for the abstract AuthEvent
 * interface.  Most/all Wx applications that access dav can inherit
 * from this class for all dav authentication needs.
 *
 * This class is responsible for deciding when maximum retry's have been
 * exceeded.
 *
 * This class is poorly named - it can be used to prompt for non-dav
 * passwords as well.
 *
 * This class can also be used stand-alone in order to put a prompting
 * layer over the AuthCache.
 *
 * @see EDSIFactory::addAuthEventListener

 */
class WxDavAuth : public AuthEventListener
{
   public:
      WxDavAuth(wxWindow *window=0);
      virtual ~WxDavAuth();

      // Need this because if used via inheritance, widget not available at
      // construction time.
      void setAuthDialogParent(wxWindow *window);

      // AuthEvent virtuals
      bool getAuthorization(AuthEvent& event) throw (RetryException);
      void authorizationAccepted(const AuthEvent& event);

      // methods for stand alone usage
      BasicAuth *getAuthorization(const string& url, const string& user);

   protected:
      WxDavAuth(const WxDavAuth& rhs);

      bool prompt(const string& strurl,
            const bool& newUser,
            string& user,
            string& password,
            int retryCount);

      bool createNewUser(const string& strulg, const string& user,
                         const string& password);
      bool changePassword(const string& strulg, const string& user,
                          const string& password, const string& newPassword);

      // The gui dialog
      wxWindow *p_window;

      int p_promptCount;
};
#endif
