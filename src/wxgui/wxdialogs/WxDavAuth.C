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
#include "wxgui/ewxThemeColours.H"

bool WxDavAuth::s_switchedToLocal = false;
bool WxDavAuth::s_restartsOnSwitch = false;
void (*WxDavAuth::s_onSwitch)() = 0;

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


static string perUserAccountFile()
{
  const char *home = getenv("ECCE_REALUSERHOME");
  return string(home && *home ? home : Ecce::realUserHome()) +
         "/.ECCE/dataserver/users";
}

// The data server is this computer's own (per-user) one, whose accounts
// belong to the user: no ECCE_REMOTE_SERVER, not local data, the account
// tool installed and the account file present. A central server's
// accounts are its administrator's, so nothing is offered there.
static bool perUserServer()
{
#ifdef __WXMSW__
  return false;
#else
  if (getenv("ECCE_REMOTE_SERVER") || !LocalData::dir().empty())
    return false;
  string adduser = string(Ecce::ecceHome()) + "/bin/ecce-dataserver-adduser";
  return access(adduser.c_str(), X_OK) == 0 &&
         access(perUserAccountFile().c_str(), R_OK) == 0;
#endif
}

static bool perUserAccountExists(const string& user)
{
  FILE *f = fopen(perUserAccountFile().c_str(), "r");
  if (f == 0) return false;
  bool found = false;
  char line[1024];
  string prefix = user + ":";
  while (!found && fgets(line, sizeof(line), f))
    found = (strncmp(line, prefix.c_str(), prefix.size()) == 0);
  fclose(f);
  return found;
}

// The per-user data server starts with an empty account file, so a fresh
// user's first login can only fail.
static bool perUserServerLacksAccount(const string& user)
{
  return !user.empty() && perUserServer() && !perUserAccountExists(user);
}

// The account tool's own rule; checked here too because the name goes
// into a shell command.
static bool validAccountName(const string& user)
{
  if (user.empty()) return false;
  for (char c : user)
    if (!isalnum((unsigned char)c)) return false;
  return true;
}

// Runs ecce-dataserver-adduser ARGS USER with the password on stdin, never
// argv (ps). On failure `error` holds what the tool said, to show the user.
static bool runAccountTool(const string& args, const string& user,
                           const string& password, string& error)
{
#ifdef __WXMSW__
  error = "Not available on this system.";
  return false;
#else
  if (!validAccountName(user)) {
    error = "A user name has letters and digits only.";
    return false;
  }
  char errPath[] = "/tmp/ecce-adduser-XXXXXX";
  int fd = mkstemp(errPath);
  if (fd < 0) { error = "Could not create a temporary file."; return false; }
  close(fd);
  string cmd = "\"" + string(Ecce::ecceHome()) +
               "/bin/ecce-dataserver-adduser\" " + args + " '" + user +
               "' >/dev/null 2>'" + errPath + "'";
  bool ok = false;
  FILE *p = popen(cmd.c_str(), "w");
  if (p != 0) {
    fprintf(p, "%s\n", password.c_str());
    ok = (pclose(p) == 0);
  }
  error.clear();
  if (FILE *e = fopen(errPath, "r")) {
    char buf[512];
    while (fgets(buf, sizeof(buf), e)) error += buf;
    fclose(e);
  }
  unlink(errPath);
  while (!error.empty() && isspace((unsigned char)error.back()))
    error.pop_back();
  const string tag = "ecce-dataserver-adduser: ";
  if (error.compare(0, tag.size(), tag) == 0) error.erase(0, tag.size());
  if (!ok && error.empty()) error = "ecce-dataserver-adduser failed.";
  return ok;
#endif
}

// Asks for the new password twice; false when cancelled. With
// ECCE_TEST_AUTH_ANSWER=reset:USER:PASS the test fills both and presses OK.
static bool askNewPassword(wxWindow *parent, const string& user,
                           string& password)
{
  wxDialog dlg(parent, wxID_ANY, "Set a new password");
  const int gap = wxSizerFlags::GetDefaultBorder();
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  string intro = "Set a new password for " + user +
                 " on this computer's ECCE data server.";
  top->Add(new wxStaticText(&dlg, wxID_ANY, intro),
           wxSizerFlags().DoubleBorder(wxLEFT|wxRIGHT|wxTOP));
  wxFlexGridSizer *grid = new wxFlexGridSizer(2, gap, gap);
  grid->AddGrowableCol(1);
  const wxSize fieldSize = dlg.FromDIP(wxSize(240, -1));
  wxTextCtrl *first = new wxTextCtrl(&dlg, wxID_ANY, "", wxDefaultPosition,
                                     fieldSize, wxTE_PASSWORD);
  wxTextCtrl *again = new wxTextCtrl(&dlg, wxID_ANY, "", wxDefaultPosition,
                                     fieldSize, wxTE_PASSWORD);
  grid->Add(new wxStaticText(&dlg, wxID_ANY, "New password:"),
            wxSizerFlags().Right().CenterVertical());
  grid->Add(first, wxSizerFlags(1).Expand());
  grid->Add(new wxStaticText(&dlg, wxID_ANY, "Again:"),
            wxSizerFlags().Right().CenterVertical());
  grid->Add(again, wxSizerFlags(1).Expand());
  top->Add(grid, wxSizerFlags().Expand().DoubleBorder(wxLEFT|wxRIGHT|wxTOP));
  wxStaticText *status = new wxStaticText(&dlg, wxID_ANY, "");
  status->SetForegroundColour(ewxThemeColours::statusText(ewxThemeColours::BAD));
  status->Show(false);
  top->Add(status, wxSizerFlags().DoubleBorder(wxLEFT|wxRIGHT|wxTOP));
  top->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
           wxSizerFlags().Expand().DoubleBorder());
  dlg.SetSizerAndFit(top);
  dlg.Centre();
  first->SetFocus();

  // OK closes the dialog only when both fields agree.
  dlg.Bind(wxEVT_BUTTON, [&](wxCommandEvent& ev) {
    if (first->GetValue().empty() || first->GetValue() != again->GetValue()) {
      status->SetLabel(first->GetValue().empty()
                       ? "Enter the new password twice."
                       : "The two passwords differ.");
      status->Show(true);
      dlg.Layout();
      dlg.Fit();
      return;
    }
    ev.Skip();
  }, wxID_OK);

  wxTimer hook(&dlg);
  const char *a = getenv("ECCE_TEST_AUTH_ANSWER");
  if (a && strncmp(a, "reset:", 6) == 0) {
    string rest = a + 6;
    size_t c = rest.find(':');
    if (c != string::npos) {
      first->SetValue(rest.substr(c + 1));
      again->SetValue(rest.substr(c + 1));
      fprintf(stderr, "ECCE_TEST_AUTH_ANSWER: new password asked\n");
      dlg.Bind(wxEVT_TIMER, [&dlg](wxTimerEvent&) {
        wxCommandEvent ok(wxEVT_BUTTON, wxID_OK);
        ok.SetEventObject(dlg.FindWindow(wxID_OK));
        dlg.GetEventHandler()->ProcessEvent(ok);
      });
      hook.StartOnce(1000);
    }
  }
  if (dlg.ShowModal() != wxID_OK) return false;
  password = first->GetValue().ToStdString();
  return true;
}

// ECCE_TEST_AUTH_ANSWER=login:USER:PASS fills the window and presses OK.
static bool testLoginAnswer(string& user, string& pass)
{
  const char *a = getenv("ECCE_TEST_AUTH_ANSWER");
  if (a == 0 || strncmp(a, "login:", 6) != 0) return false;
  string rest = a + 6;
  size_t c = rest.find(':');
  if (c == string::npos) return false;
  user = rest.substr(0, c);
  pass = rest.substr(c + 1);
  return true;
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
         string why = "The user name or password was not accepted.";
         // A central server's accounts are kept by its administrator, who
         // resets a password on the server itself.
         if (getenv("ECCE_REMOTE_SERVER"))
           why += "\nAsk your server administrator to reset the password. "
                  "On the server they run:\n    ecce-dataserver-adduser "
                  "--passwd " + (validAccountName(user) ? user : "NAME");
         authDlg.setStatus(why);
         if (getenv("ECCE_TEST_AUTH_ANSWER"))
           fprintf(stderr, "ECCE_TEST_AUTH_ANSWER: status %s\n", why.c_str());
       }
     }

     // The old Change... button stays hidden: it has no server side, and
     // its request put both passwords in the URL, which the data server
     // writes to its access log. Accounts on this computer's own server
     // are made and reset with ecce-dataserver-adduser instead.
     authDlg.showChangeBtn(false);
     // The start window asks "this computer or a server" at every start,
     // so the login window does not ask it a second time.
     authDlg.showUseLocal(false);
     const bool offerReset = perUserServer();
     authDlg.showReset(offerReset);
     if (getenv("ECCE_TEST_AUTH_ANSWER"))
       fprintf(stderr, "ECCE_TEST_AUTH_ANSWER: reset %s\n",
               offerReset ? "offered" : "not offered");
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

       string tUser, tPass;
       if (testLoginAnswer(tUser, tPass)) {
         fprintf(stderr, "ECCE_TEST_AUTH_ANSWER: login %s\n", tUser.c_str());
         authDlg.setUser(tUser);
         authDlg.setPassword(tPass);
         authDlg.Bind(wxEVT_TIMER, [&authDlg](wxTimerEvent&) {
           authDlg.EndModal(wxID_OK);
         });
         hook.StartOnce(1500);
       }

       // ECCE_TEST_AUTH_ANSWER=reset:USER:PASS presses "Forgot password..."
       // once for USER; askNewPassword() then enters PASS.
       static bool resetPressed = false;
       if (answer && strncmp(answer, "reset:", 6) == 0 && !resetPressed) {
         string rest = answer + 6;
         authDlg.setUser(rest.substr(0, rest.find(':')));
         authDlg.Bind(wxEVT_TIMER, [&authDlg](wxTimerEvent&) {
           fprintf(stderr, "ECCE_TEST_AUTH_ANSWER: reset pressed\n");
           resetPressed = true;
           authDlg.EndModal(WxAuthGUI::ID_BUTTON_AUTH_RESET);
         });
         hook.StartOnce(1500);
       }

       status = authDlg.ShowModal();
       if (status == WxAuthGUI::ID_BUTTON_AUTH_RESET) {
         string who = authDlg.getUser();
         string newPass, error;
         if (!validAccountName(who)) {
           authDlg.setStatus("Enter your user name (letters and digits), "
                             "then press Forgot password... again.");
         } else if (!perUserAccountExists(who)) {
           authDlg.setStatus("There is no account " + who + " on this "
             "computer's data server. Enter a password and press OK to "
             "create it.");
         } else if (askNewPassword(&authDlg, who, newPass)) {
           if (runAccountTool("--passwd", who, newPass, error)) {
             fprintf(stderr, "ECCE_TEST_AUTH_ANSWER: password reset\n");
             authDlg.setPassword(newPass);
             status = wxID_OK;     // logs in with the new password
           } else {
             ewxMessageDialog err(&authDlg, ("The password was not changed:\n"
               + error).c_str(), "Set a new password",
               wxOK | wxICON_EXCLAMATION);
             err.ShowModal();
           }
         }
         if (status != wxID_OK) {
           done = false;
           continue;
         }
       }
       if (status == WxAuthGUI::ID_BUTTON_AUTH_USE_LOCAL) {
         authDlg.Show(false);
         if (useThisComputer()) {
           s_switchedToLocal = true;
           if (s_onSwitch) s_onSwitch();
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
         if (perUserServerLacksAccount(user)) {
           string q = "There is no account for " + user + " on this "
             "computer's ECCE data server yet.\n\nCreate it with the "
             "password you just entered?";
           ewxMessageDialog ask(0, q.c_str(), "Create account",
                                wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
           wxTimer yes(&ask);
           if (testLoginAnswer(tUser, tPass)) {
             fprintf(stderr, "ECCE_TEST_AUTH_ANSWER: create-account asked\n");
             ask.Bind(wxEVT_TIMER, [&ask](wxTimerEvent&) {
               ask.EndModal(wxID_YES);
             });
             yes.StartOnce(1000);
           }
           string error;
           if (ask.ShowModal() == wxID_YES &&
               !runAccountTool("--stdin", user, password, error)) {
             ewxMessageDialog err(0, ("The account could not be created:\n"
               + error).c_str(), "Create account", wxOK | wxICON_EXCLAMATION);
             err.ShowModal();
           }
         }
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

