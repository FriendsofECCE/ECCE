/**
 * @file
 *
 *
 */
#ifndef WXDAVAUTH_H
#define WXDAVAUTH_H

class wxWindow;

#include <set>
#include <map>
#include <ctime>

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
      bool getAuthorization(AuthEvent& event);
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

      // True if this session has already seen `user` accepted by `host`
      // (any URL, via authorizationAccepted()) -- so a 401 for a
      // DIFFERENT url with the same credentials is "no access to this
      // folder", not "wrong password".
      bool knownGood(const string& host, const string& user) const;

      // The gui dialog
      wxWindow *p_window;

      int p_promptCount;

      // Whether this request has already re-read the session credential
      // store (AuthCache::sessionLookup) before prompting.
      bool p_sessionTried;

      // host+user pairs already accepted by the server this session
      // (see knownGood()/authorizationAccepted()).
      std::set<string> p_knownGoodUsers;

      // url+realm this session's user just cancelled a prompt for, and
      // when -- so a second 401 for the same folder arriving right
      // behind the cancel (e.g. a follow-up request from the same user
      // action) does not pop a second dialog. Forgotten after a few
      // seconds, which is enough to cover one user action's requests
      // without suppressing a genuinely new attempt later.
      std::map<string, time_t> p_cancelledAt;

      /**
       * True while an authentication dialog is on screen.
       *
       * ShowModal() runs a NESTED EVENT LOOP, so while the dialog is up a
       * pending DAV authentication event is still dispatched -- straight
       * back into prompt(), opening a second dialog on top of the first.
       * One of the two holds the input grab and the other does not, which
       * presents as a dialog whose buttons work but which accepts no
       * keystrokes in any field.  It only happens on a first run, because
       * once credentials are cached neither path prompts again.
       */
      bool p_prompting;
};
#endif
