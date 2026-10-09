/**
 * @file
 *
 *
 */

#include <strstream>
  using std::ostrstream;

#include <iostream>
  using std::cout;
  using std::endl;

#include <sys/utsname.h> // uname
#include <unistd.h>   // getpid, #120 instrumentation
#include <cstdio>     // fopen/fprintf, #120 instrumentation
#include <cstring>

#include <wx/wx.h>

#include "wxgui/ewxWindowUtils.H"
#include "util/NotImplementedException.H"
#include "util/NullPointerException.H"
#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/LocalData.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"

#include "tdat/AuthCache.H"

#include "dsm/EDSIServerCentral.H"
#include "dsm/EcceDAVClient.H"
#include "dsm/CTLSSocket.H"

#include "wxgui/WxDavAuth.H"
#include "wxgui/WxAuth.H"
#include "wxgui/ewxMessageDialog.H"

bool WxDavAuth::s_switchedToLocal = false;
bool WxDavAuth::s_restartsOnSwitch = false;

/**
 * The login window's "Use this computer instead": what Edit > Change
 * Server... does for that choice (ecce-first-start --apply local), then
 * either `ecce` starts again in local mode (the gateway's login) or the
 * user is told it applies at the next start.  A running session was set
 * up for the server, so it is not switched under the running apps.
 */
static bool useThisComputer()
{
  wxString home = wxString::FromUTF8(Ecce::ecceHome());
#ifdef __WXMSW__
  wxString cmd = "\"" + home + "/python/python3w.exe\" \"" + home +
                 "/bin/ecce-first-start\" --apply local";
#else
  wxString cmd = "\"" + home + "/bin/ecce-first-start\" --apply local";
#endif
  long rc = wxExecute(cmd, wxEXEC_SYNC);
  if (rc == 0 && WxDavAuth::restartsOnSwitch())
    return true;
  ewxMessageDialog dlg(0, rc == 0
      ? "From the next start, ECCE keeps your calculations in a folder on "
        "this computer.\n\nThis start ends now. Start ECCE again to work "
        "there."
      : "ECCE could not switch to this computer. Your setting is unchanged; "
        "use Edit > Change Server... in the Organizer to try again.",
      "Use this computer", wxOK | (rc == 0 ? wxICON_INFORMATION : wxICON_EXCLAMATION));
  dlg.ShowModal();
  return rc == 0;
}

// ECCE_DEBUG_DAVAUTH=<file>: appends one line per getAuthorization()/
// prompt()/authorizationAccepted() decision -- which of cache, session
// file or a real dialog supplied credentials, what retryCount reached
// prompt(), and what knownGood() found. Added to root-cause a live
// report ("even with the correct password I still don't get access to
// my files": the retry dialog showed the wrong wording and a prefilled
// username instead of "<user> has no access...") that a single-process
// static read of this file could not settle on its own -- see the
// long comment on the fix in getAuthorization(AuthEvent&) below.
static void davAuthDebug(const string& msg)
{
   const char *where = getenv("ECCE_DEBUG_DAVAUTH");
   if (where == 0) return;
   FILE *log = fopen(where, "a");
   if (log == 0) return;
   fprintf(log, "[DAVAUTH] pid=%d %s\n", (int)getpid(), msg.c_str());
   fclose(log);
}

/**
 * This class implements the AuthEventListener interface by using the
 * authorization cache and prompting when necessary.
 *
 * A typical use would be to inherit from this class in any app that
 * accesses dav.
 */
WxDavAuth::WxDavAuth(wxWindow *window) 
{
   p_window = window;
   p_prompting = false;
   p_promptCount = 0;
   p_sessionTried = false;
}



WxDavAuth::WxDavAuth(const WxDavAuth& rhs) 
{
   p_window = rhs.p_window;
   p_prompting = false;
   p_promptCount = 0;
   p_sessionTried = false;
   throw NotImplementedException("DavAuth copy constructor!", WHERE);
}



WxDavAuth::~WxDavAuth()
{}



void WxDavAuth::setAuthDialogParent(wxWindow *window)
{
   p_window = window;
   p_prompting = false;
}


/**
 * Get the authentication for the url and user.
 * Unlike the other getAuthorization methods, this one will only try
 * the first match it finds in the cache, then prompt.
 */
BasicAuth *WxDavAuth::getAuthorization(const string& url, const string& user)
{
   // Don't need this, the dialog accepts null for parent
   //NULLPOINTEREXCEPTION(p_window,
         //"Parent widget for authization window not set");

   BasicAuth *ba = 0;

   p_promptCount = 0;


   string username = user;
   if (username == "") {
      username = Ecce::serverUser();
   }

  EcceURL eurl(url);

  string whereami = "";
  struct utsname _uname;
  if (uname(&_uname) != -1)
    whereami = _uname.nodename;

   if (username!=Ecce::serverUser() ||
       eurl.getProtocol()=="http" || eurl.getProtocol()=="https" ||
       eurl.getHost()!=whereami) {
     // Try for the first match in the cache
     ba = AuthCache::getCache().getAuthentication(url, username, "", 1);

     if (ba == 0) {
        // Try prompting just once
        string password;
        if (prompt(url, false, username, password, 1)) {
           ba = AuthCache::createBasicAuth(username, password);
        }
     }
   } else {
     ba = AuthCache::getCache().getBogusAuthentication();
   }

   return ba;
}



/**
 * Get authorization information in response to AuthEvent.
 * All possible matches from the cache will be tried before
 * prompting.
 *
 * The user and password fields of the event may be replacedd
 *
 * @return true if there is a new value to try.
 */
bool WxDavAuth::getAuthorization(AuthEvent& event)
{
   bool ret = false;

   // Don't really need this?
   //NULLPOINTEREXCEPTION(p_window,
         //"Parent widget for authization window not set");


   BasicAuth *ba = NULL;

   // Distinguish between cache retries and prompt retries
   // We might have lots of items in cache that COULD match
   if (event.m_retryCount == 1) {
      p_promptCount = 0;
      p_sessionTried = false;
   }


   // Supply default user name if needed
   // If left blank, matching policies will fail.
   if (event.m_user == "") {
      event.m_user = Ecce::serverUser();
   }

   // If we've started to prompt, no point in looking in cache.
   if (p_promptCount == 0) {
      ba = AuthCache::getCache().getAuthentication(event.m_url,
            event.m_user,
            event.m_realm,
            event.m_retryCount);
   }

   davAuthDebug("getAuthorization url=" + event.m_url +
                " user=" + event.m_user +
                " event.retryCount=" + std::to_string(event.m_retryCount) +
                " p_promptCount=" + std::to_string(p_promptCount) +
                " cacheHit=" + (ba != 0 ? "yes" : "no"));

   // Before asking the user, see whether another process of this session
   // has learned a newer password since this one loaded the session
   // store -- the user changed it elsewhere, or it was reset and typed in
   // another window, and the ecce_auth_changed broadcast did not reach
   // us (#120). Once per request, and only a password other than the one
   // just refused, so a stale store can never cost a prompt attempt or
   // loop. Not counted against the three prompts.
   //
   // This replaces a read of $prefpath/ServerPass that nothing ever
   // wrote: it offered one bare password, unscoped by server or user, to
   // whichever http URL first missed the cache -- a second or central
   // data server included -- and never closed the file.
   if (ba == 0 && !p_sessionTried) {
      p_sessionTried = true;
      string stored;
      if (AuthCache::getCache().sessionLookup(event.m_url, event.m_user,
                                              event.m_realm, stored) &&
          stored != event.m_password) {
         event.m_password = stored;
         return true;
      }
   }

   if (ba == 0) {
      // try prompting then
      p_promptCount++;

      // prompt()'s retryCount must be event.m_retryCount (how many
      // times THE SERVER has rejected an attempt for this operation),
      // not p_promptCount (how many times a DIALOG has been shown for
      // it) -- they are not the same number. AuthCache's key is
      // per-SERVER, not per-path (EcceURL::getRef() carries no path),
      // so a credential already cached from logging in elsewhere on
      // this server is retried automatically on attempt 1 of a fresh
      // operation, with no dialog at all; only once that silent retry
      // is ALSO rejected (event.m_retryCount reaches 2) does the cache
      // finally come up empty and a dialog actually appear -- as
      // p_promptCount's very first prompt, i.e. 1. Passing p_promptCount
      // here made that dialog look like a brand new, never-tried
      // request, so the "no access" wording (which needs retryCount>1)
      // and the "not accepted" wording could never appear on exactly
      // the sequence that produces them: known-good credentials,
      // cached from an earlier login on this server, silently retried
      // and rejected by a DIFFERENT folder's access control. Confirmed
      // live (2026-09-29): "stud1" successfully logged in, then
      // clicking andy's folder in the Organizer showed the plain
      // first-time dialog, prefilled "stud1", instead of "stud1 has no
      // access to this folder...".
      //
      // No cap here: prompt() only returns true on OK (Cancel already
      // ends the loop), so this can't spin on its own, and a client-side
      // limit protects nothing -- anyone can hit Apache directly anyway.
      if (!ret)
        ret = prompt(event.m_url,
              event.m_newUser,
              event.m_user,
              event.m_password,
              event.m_retryCount);
   } else {
      // Got something from cache
      event.m_user = ba->m_user;
      event.m_password = ba->m_pass;
      delete ba;
      ret = true;
   }


   return ret;
}




/**
 * Prompt user for password.
 * Username can also be changed.
 * @param retryCount the AuthEvent's own m_retryCount: how many times
 *   THE SERVER has rejected an attempt for this operation so far, 1 for
 *   the very first. NOT p_promptCount (how many times a dialog has
 *   actually been shown) -- an earlier rejected attempt can be a
 *   silent cache retry that never showed a dialog at all, and this
 *   still needs to count as "already tried and failed" (see the
 *   comment in getAuthorization(AuthEvent&) on why). Unbounded -- the
 *   user keeps retrying until they succeed or press Cancel.
 */
bool WxDavAuth::prompt(const string& strurl,
      const bool& newUser,
      string& user,
      string& password,
      int retryCount)
{
   // false for anything but http(s): getAuthorization() retries without
   // limit while this returns true, so an unset value could loop with no
   // dialog.
   bool ret = false;

   EcceURL url(strurl);

   // Only prompt for data server passwords because the underlying RCommand
   // code will prompt for any machine passwords it needs
   if (url.getProtocol() == "http" || url.getProtocol() == "https") {
     //  Refuse to stack a second dialog. ShowModal() below runs a nested
     //  event loop, so a DAV authentication event arriving while the first
     //  dialog is up is dispatched straight back into here. The caller
     //  treats this as "no credentials", and the dialog already on screen
     //  will populate the cache for it.
     if (p_prompting) {
       return false;
     }

     // The user already cancelled a prompt for this exact url a moment
     // ago -- a second 401 for it right behind that cancel (a follow-up
     // request from the same user action, e.g. two DAV calls opening one
     // folder) must not pop a second dialog. A few seconds is enough to
     // cover one user action; a later, genuinely new attempt (the user
     // navigates back to the same folder) ages out and prompts again.
     const time_t cancelSuppressSeconds = 5;
     std::map<string, time_t>::iterator cit = p_cancelledAt.find(strurl);
     if (cit != p_cancelledAt.end()) {
       if (time(0) - cit->second < cancelSuppressSeconds) {
         return false;
       }
       p_cancelledAt.erase(cit);
     }

     p_prompting = true;

     //  TEMPORARY, #120: which parent this dialog actually gets, and
     //  whether it's currently mapped -- an unmapped/hidden parent is the
     //  documented cause of "buttons work, keystrokes don't" under
     //  Wayland/XWayland. dialogParent() is supposed to hand back NULL
     //  whenever the (hidden-by-default) Gateway frame would otherwise be
     //  the parent, so p_window should normally be 0 here; if it's ever
     //  non-null AND not shown, that's the live bug.
     {
       const char *where = getenv("ECCE_DEBUG_AUTHPARENT");
       if (where != 0) {
         FILE *log = fopen(where, "a");
         if (log != 0) {
           fprintf(log, "AUTHPROMPT pid=%d retryCount=%d newUser=%d "
             "p_window=%p shown=%d\n", (int)getpid(), retryCount,
             (int)newUser, (void*)p_window,
             p_window ? (int)p_window->IsShown() : -1);
           fclose(log);
         }
       }
     }

     WxAuth authDlg(p_window);

     // A retry with a credential already proven good elsewhere this
     // session (knownGood()) means the server can see the password --
     // it just won't let this user into THIS folder. That is a
     // different problem than a wrong password, and the fix is a
     // different login, not a retyped one.
     bool noAccess = !newUser && retryCount > 1 &&
                     knownGood(url.getHost(), user);

     davAuthDebug("prompt url=" + strurl + " user=" + user +
                  " retryCount=" + std::to_string(retryCount) +
                  " newUser=" + (newUser ? "yes" : "no") +
                  " knownGood=" + (knownGood(url.getHost(), user) ? "yes" : "no") +
                  " -> noAccess=" + (noAccess ? "yes" : "no"));

     if (newUser) {
       authDlg.setPrompt("You do not have an existing data server account!\nPlease enter a new data server password to create one:");
       authDlg.setPasswordLabel("  New\nPassword:");
     } else if (retryCount > 1) {
       // A prior prompt's password was refused by the server -- say
       // so on the error line, rather than silently repeating the dialog.
       if (noAccess) {
         authDlg.setPrompt("Enter its owner's name and password to open it:");
         authDlg.setStatus(user + " has no access to this folder.");
       } else {
         authDlg.setPrompt("Please try again:");
         authDlg.setStatus("The user name or password was not accepted.");
       }
     }

     // No password change or account creation from here: there is no
     // server side for them, and the request put both passwords in the
     // URL, which the data server writes to its access log.
     authDlg.showChangeBtn(false);
     // A data server session can switch to working on this computer.
     authDlg.showUseLocal(!newUser && !noAccess && LocalData::dir().empty());
     {
       string where = url.getProtocol() + "://" + url.getHost();
       const bool secure = (url.getProtocol() == "https");
       if (url.getPort() > 0 && url.getPort() != (secure ? 443 : 80))
         where += ":" + std::to_string(url.getPort());
       authDlg.setServer(where);
       if (secure) {
         authDlg.setEncryption(ipc::CTLSClientSocket::pinnedCertPath().empty()
           ? "Encrypted connection (TLS), certificate checked by the system"
           : "Encrypted connection (TLS), server certificate pinned");
       }
     }
     // Fit() sizes the dialog to the new prompt text's best size, which
     // can come out narrower than the first appearance (the title bar was
     // once truncated); never let it end up smaller than it was.
     {
       wxSize before = authDlg.GetSize();
       authDlg.Layout();
       authDlg.Fit();
       wxSize after = authDlg.GetSize();
       authDlg.SetSize(wxSize(wxMax(before.GetWidth(), after.GetWidth()),
                              wxMax(before.GetHeight(), after.GetHeight())));
     }

     authDlg.setProtocol("http");
     // Prefilling the session user's own name here would invite retyping
     // the same password that already failed for lack of access -- leave
     // it blank so the owner's name has to be entered instead.
     authDlg.setUser(noAccess ? "" : user);

     int status;
     bool done = false;
     bool userCancelled = false;
     while (!done) {
       ret = false;
       done = true;

       //  #120: shown as the first UI action of the process, a plain
       //  Show() can leave the window without keyboard focus under
       //  Mutter; raise it explicitly on every pass (a re-prompt after a
       //  failed password is a fresh Show() and can lose focus too).
       authDlg.Show(true);
       ewxRaiseWindow(&authDlg);

       // Test hook: ECCE_TEST_AUTH_ANSWER=uselocal[:seconds] presses "Use
       // this computer instead" after that long (default 3 s).
       wxTimer hook(&authDlg);
       const char *answer = getenv("ECCE_TEST_AUTH_ANSWER");
       if (answer && strncmp(answer, "uselocal", 8) == 0) {
         int secs = answer[8] == ':' ? atoi(answer + 9) : 3;
         authDlg.Bind(wxEVT_TIMER, [&authDlg](wxTimerEvent&) {
           fprintf(stderr, "ECCE_TEST_AUTH_ANSWER: uselocal\n");
           authDlg.EndModal(WxAuthGUI::ID_BUTTON_AUTH_USE_LOCAL);
         });
         hook.StartOnce(1000 * (secs > 0 ? secs : 3));
       }

       status = authDlg.ShowModal();
       if (status == WxAuthGUI::ID_BUTTON_AUTH_USE_LOCAL) {
         authDlg.Show(false);
         if (useThisComputer()) {
           s_switchedToLocal = true;
           // This start ends; the gateway quits on the refused login, any
           // other app leaves its main loop.
           if (wxTheApp)
             wxTheApp->CallAfter([] { wxTheApp->ExitMainLoop(); });
         }
       }
       if (status != wxID_OK) {
         userCancelled = true;
       }

       if (status == wxID_OK) {
         ret = true;
         user = authDlg.getUser();
         password = authDlg.getPassword();
       }
     }

     if (userCancelled) {
       // Remember it so a second 401 for this same url, arriving right
       // behind this cancel, doesn't prompt again.
       p_cancelledAt[strurl] = time(0);
     }
   }

   p_prompting = false;
   return ret;
}


/**
 * Add to cache because it worked.  The addAuthentication method is
 * responsible for not adding duplicates.
 */
void WxDavAuth::authorizationAccepted(const AuthEvent& event)
{
   AuthCache::getCache().addAuthentication(event.m_url, event.m_user,
                                           event.m_password,
                                           event.m_realm, true);

   EcceURL eurl(event.m_url);
   p_knownGoodUsers.insert(eurl.getHost() + "|" + event.m_user);

   davAuthDebug("authorizationAccepted url=" + event.m_url +
                " user=" + event.m_user +
                " host=" + eurl.getHost() + " -> knownGood recorded");
}

/**
 * Has `user` already succeeded against `host` (any url) this session?
 * If so, a fresh 401 for a different folder is "no access", not a bad
 * password -- the credential itself already proved good.
 */
bool WxDavAuth::knownGood(const string& host, const string& user) const
{
   return p_knownGoodUsers.find(host + "|" + user) != p_knownGoodUsers.end();
}

