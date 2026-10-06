/////////////////////////////////////////////////////////////////////////////
// Name:        GatewayApp.C
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// Created:     Wed 09 Mar 2005 02:12:56 PM PST
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#include <iomanip>
using std::ios;
#include <fstream>
#include <set>
using std::ofstream;

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "GatewayApp.H"
#endif

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/stopwatch.h"

#include <atomic>
#include <signal.h>
#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util/Ecce.H"
#include "util/LocalData.H"
#include "util/ErrMsg.H"
#include "util/SDirectory.H"
#include "util/Preferences.H"
#include "util/MqttLink.H"
#include "util/SessionLease.H"
#include "util/PreferenceLabels.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/EDSI.H"

#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/WxJMSSubscriber.H"
#include "wxgui/ewxMessageDialog.H"

#include "GatewayApp.H"
#include "Gateway.H"

#ifdef EMSL
#include "EMSLAuth.H"
#endif


IMPLEMENT_APP( GatewayApp )


class SessionWatch : public wxTimer
{
public:
  SessionWatch(GatewayApp *app) : p_app(app) {}
  void Notify() { p_app->checkSessionEnd(); }
private:
  GatewayApp *p_app;
};


GatewayApp::GatewayApp()
  : WxJMSMessageDispatch(GATEWAY),
    p_gateway(NULL),
    p_sessionWatch(NULL),
    p_sessionSeen(false),
    p_idleTicks(0),
    p_loginFromDashL(false),
    p_sessionLoginFinalized(false)
{
}


string GatewayApp::getName() const
{
   return GATEWAY;
}


//  The Gateway frame is constructed and then left unmapped since #93
//  removed the Gateway window.  A dialog that is transient-for an UNMAPPED
//  parent is never granted keyboard focus by the compositor -- pointer
//  events still arrive, so its buttons work while not one field accepts a
//  keystroke.  Use this instead of p_gateway directly when parenting a
//  dialog, so a hidden frame becomes "no parent" rather than "a parent
//  that cannot take focus".
wxWindow* GatewayApp::dialogParent()
{
  return eccGatewayWindowEnabled() ? (wxWindow*) p_gateway : (wxWindow*) NULL;
}


// A move of the local data folder asked for in Preferences (#216) happens
// here, at the start of a session and before any of its processes opens
// the folder; never while ECCE is using it.  Only for a folder that came
// from the preference: an explicit ECCE_LOCAL_DATA is left alone.
static void offerLocalDataMove()
{
  const char *env = getenv("ECCE_LOCAL_DATA");
  if (!getenv("ECCE_LOCAL_DATA_FROM_PREF") || !env || !*env) return;
  string from = env;
  string to = LocalData::prefMoveTo();
  if (to.empty()) return;
  string use = from, pending, note;
  if (to == from) {
    pending = "";
  } else if (LocalData::isEmptyOrMissing(to)) {
    ewxMessageDialog dlg(0,
        "Move your calculations from\n    " + from + "\nto\n    " + to + " ?\n\n"
        "Start empty: use the new folder with no calculations in it; "
        "they stay in the old one.\n"
        "Cancel: keep using the old folder.",
        "ECCE data folder", wxICON_QUESTION);
    dlg.AddButton(wxID_CANCEL, "Cancel");
    dlg.AddButton(wxID_NO, "Start empty");
    dlg.AddButton(wxID_YES, "Move")->SetDefault();
    int answer = dlg.ShowModal();
    if (answer == wxID_YES) {
      string msg;
      LocalData::MoveResult r = LocalData::move(from, to, msg);
      if (r == LocalData::MOVED || r == LocalData::SAME) {
        use = to;
        if (!msg.empty()) note = msg;
      } else if (r == LocalData::IN_USE) {
        pending = to;
        note = msg + "\n\nECCE uses " + from + " for this session and "
               "will offer the move again at the next start.";
      } else {
        note = msg + "\n\nECCE keeps using " + from + ".";
      }
    } else if (answer == wxID_NO) {
      use = to;
      note = "Your calculations stay in " + from + ".";
    } else {
      note = "ECCE keeps using " + from + ".";
    }
  } else {
    ewxMessageDialog dlg(0,
        to + " is not empty, and ECCE does not merge two data folders.\n\n"
        "Use it as it is: work with what is already in " + to +
        "; your calculations in " + from + " stay there.\n"
        "Cancel: keep using " + from + ".",
        "ECCE data folder", wxICON_QUESTION);
    dlg.AddButton(wxID_CANCEL, "Cancel");
    dlg.AddButton(wxID_OK, "Use it as it is");
    if (dlg.ShowModal() == wxID_OK) {
      use = to;
      note = "Your calculations in " + from + " stay there.";
    } else {
      note = "ECCE keeps using " + from + ".";
    }
  }
  LocalData::setPref(true, use, pending);
  setenv("ECCE_LOCAL_DATA", use.c_str(), 1);
  if (!note.empty()) {
    ewxMessageDialog info(0, note, "ECCE data folder", wxOK | wxICON_INFORMATION);
    info.ShowModal();
  }
}

bool GatewayApp::OnInit()
{
  ewxApp::OnInit();

  // Captured before anything else runs: whether `-l` set this session's
  // login is what decides, once the session's first login succeeds
  // below, whether that name is ever written to ~/.ECCE/ServerLogin[.
  // remote] -- see authorizationAccepted().
  {
    const char *l = getenv("ECCE_SERVER_LOGIN");
    p_loginFromDashL = (l != (const char*)0 && l[0] != '\0');
  }

  string compileVersion = Ecce::ecceVersion();
  int idx;
  for (idx=0; idx<argc; idx++) {
    if (strncmp(argv[idx].ToStdString().c_str(),"-V",2) == 0) {
      cmdLineVersion = argv[idx].ToStdString();
      cmdLineVersion.erase(0,2);
      break;
    }
  }
  if (cmdLineVersion == "")
    cmdLineVersion = compileVersion;
  else if (cmdLineVersion != compileVersion) {
    string msg =  "Compiled version, \"";
    msg += compileVersion;
    msg += "\", and command line version, \"";
    msg += cmdLineVersion;
    msg += "\", don't match.";
    EE_RT_ASSERT(FALSE, EE_WARNING, msg);
    cmdLineVersion = compileVersion;
  }

  Ecce::initialize();
  offerLocalDataMove();

  // A refused broker login would otherwise only reach this process's
  // stderr, which nobody sees; the other apps' refusals are the same one.
  MqttLink::setRefusalHandler(
    [](const string& account, const string& host, int port, const string& why) {
      static std::atomic<bool> shown(false);
      if (shown.exchange(true)) return;
      string msg = "The ECCE message broker";
      if (!host.empty())
        msg += " (" + host + ":" + std::to_string(port) + ")";
      if (why.compare(0, 4, "TLS:") == 0)
        msg += " could not be trusted: its certificate is not the one this "
               "installation was set up with, so ECCE did not send your "
               "login to it.\n\nAsk your ECCE administrator.";
      else
      msg += " refused the login of '" + account + "'.\n\n"
             "Your data server login was accepted, but the message broker "
             "did not know this account or its password, so jobs cannot "
             "report back and ECCE's windows will not update each other.\n\n"
             "Ask your ECCE administrator to check the account on the message broker.";
      wxTheApp->CallAfter([msg]() {
        ewxMessageDialog dlg(0, msg.c_str(), "Message broker refused the login",
                             wxOK | wxICON_EXCLAMATION);
        dlg.ShowModal();
      });
    });

  // Get rid of the leading "v" because it reads better and takes up
  // less space when the gateway is oriented vertically
  if (cmdLineVersion[0] == 'v')
    cmdLineVersion.erase(0,1);
  string title = "ECCE " + cmdLineVersion;

#ifdef EMSL
  EMSLAuth::getCache().loadCache();
#endif

  p_gateway = new Gateway(this, NULL, SYMBOL_GATEWAY_IDNAME, title.c_str());
  p_gateway->Show(false);

  SetTopWindow(p_gateway);
  registerTopShell(p_gateway);
  registerMyselfAsAppExecer(); // only gateway calls this
  //  Only parent the authentication dialog to the Gateway frame when that
  //  frame is actually on screen.  Since #93 removed the Gateway window,
  //  p_gateway is constructed and then left unmapped -- and a modal dialog
  //  that is transient-for an UNMAPPED parent is never granted keyboard
  //  focus by the compositor.  Pointer events still arrive, so the buttons
  //  work and not one field accepts a keystroke, which is exactly how this
  //  presented on a Wayland session.  With no parent the dialog is a
  //  top-level in its own right and takes focus normally.
  setAuthDialogParent(dialogParent());
  EDSIFactory::addAuthEventListener(this);

  // create the preferences directory if it doesn't already exist so we
  // have someplace to put all the wonderful preference files
  SDirectory directory(Ecce::realUserPrefPath(), 0700);
  if (!directory.exists()) {
    string msg = "Could not create user preferences directory ";
    msg += Ecce::realUserPrefPath();
    EE_RT_ASSERT(FALSE, EE_FATAL, msg);
  }
  
  // @todo Need to implement a wx version of EcceHelp
  //  EcceHelp().registerWidgetHelp(getGateway());

  bool newUserFlag = false;

  // Every quit() below must be followed by returning false: quit() tears
  // down p_timer and the target list, and OnInit had no `return` after
  // any of these, so a failed check fell through into the NEXT try block
  // and could call quit() again on the now-torn-down gateway --
  // dereferencing the already-deleted, NULLed p_timer (found live
  // 2026-10-02, closing the auth dialog's window via its own X button:
  // checkServerSetup() reports canceled, quit()s, falls through to
  // checkServer(), which also reports canceled and quit()s again,
  // crashing on p_timer->Stop()).
  try {
    if (!checkServerSetup()) {
      p_gateway->quit(false); // canceled
      return false;
    }
  } catch (RetryException& rex) {
    ewxMessageDialog * dlg =
      new ewxMessageDialog(dialogParent(), rex.what(), "Retries exceeded!",
                           wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
    p_gateway->quit(false);
    return false;
  } catch (EcceException& ex) {
    string msg = ex.what();
    msg += "Please contact your ECCE Administrator.";
    ewxMessageDialog * dlg =
      new ewxMessageDialog(dialogParent(),  msg.c_str(), "ECCE Server Failure",
                           wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
    p_gateway->quit(false);
    return false;
  }

  try {
    if (!checkServer()) {
      p_gateway->quit(false); // canceled
      return false;
    }
  } catch (RetryException& rex) {
    ewxMessageDialog * dlg =
      new ewxMessageDialog(dialogParent(), rex.what(), "Retries exceeded!",
                           wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
    p_gateway->quit(false);
    return false;
  } catch (EcceException& ex) {
    string msg = "An error has occured while attempting to access "
      "the ECCE server.  Please contact your administrator for assistence "
      "if the problem persists.\n";
    msg += ex.what();
    ewxMessageDialog * dlg =
      new ewxMessageDialog(dialogParent(),  msg.c_str(), "ECCE Server Failure",
                           wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
    p_gateway->quit(false);
    return false;
  }

  if (!checkUser()) {
    p_gateway->quit(false);
    return false;
  }

  // Subscribing opens the broker connection, and a central or shared broker
  // takes the data server login as its account: the login is made above.
  // A broker of this user's own (Unix socket) takes no login.
  subscribe("ecce_activity",(wxJmsCBFunc)&GatewayApp::activityMCB, false);
  subscribe("ecce_identify",(wxJmsCBFunc)&GatewayApp::identifyMCB);
  subscribe("ecce_identify_reply",(wxJmsCBFunc)&GatewayApp::identifyReplyMCB);
  subscribe("ecce_gateway_raise",(wxJmsCBFunc)&GatewayApp::raiseMeMCB);
  subscribe("ecce_invoke_status",(wxJmsCBFunc)&GatewayApp::toolStartStatusMCB, false);
  subscribe("ecce_preferences_gateway",(wxJmsCBFunc)&GatewayApp::preferenceMCB, false);
  subscribe("ecce_auth_changed",(wxJmsCBFunc)&GatewayApp::authMCB, false);

  startSubscriber();

  // notify pertinent eccejobstore processes to reconnect tooltalk messaging
  reconnectJobStoreMessaging();

  // Notify execer that I am ready to receive messages:

  notifyReady();

  //  Issue #93: hidden by default; ECCE_GATEWAY_WINDOW=1 restores it.
  //  See Gateway.H. Everything else about the frame is unchanged -- it is
  //  still the top window, still owns JMS and the session lifecycle, and
  //  is still what launches the other apps.
  if (eccGatewayWindowEnabled()) {
    p_gateway->Show(true);
  } else {
    //  With the window hidden the Gateway has no visible presence at all,
    //  so something has to open or `ecce` looks like it did nothing. The
    //  Organizer becomes the front door, which is the point of #93.
    //
    //  Same message the toolbar button publishes (see
    //  Gateway::toolActivate), so the launch path, the dataserver check
    //  and the app-ready handshake are all the existing ones -- nothing
    //  here is a second way to start an app.
    //
    //  forcenew is 0: if an Organizer is somehow already up, raise it
    //  rather than opening a second one.
    Target myself("", getMyID());
    JMSMessage *startMsg = newMessage(myself);
    startMsg->addProperty("appname", "Organizer");
    startMsg->addIntProperty("forcenew", 0);
    publish("ecce_get_app", *startMsg);
    delete startMsg;

    //  With no Gateway window there is no Quit button, so the session
    //  ends when its last app closes (#185).
    p_sessionWatch = new SessionWatch(this);
    p_sessionWatch->Start(1000);
  }

  static const int BUFSIZE=512;
  char buf[BUFSIZE];

  // if they are a new user, show the new user message, if any
  if (newUserFlag) {
    string userFileName = Ecce::ecceHome();
    userFileName += "/siteconfig/NewUserMessage";
    ifstream newUserFile(userFileName.c_str());
    if (newUserFile) {
      string userMsg = ""; 
      while (newUserFile.getline(buf, BUFSIZE-1)) {
        userMsg += buf;
        userMsg += "\n";
      }

      if (userMsg != "") {
        ewxMessageDialog* userDlg = new ewxMessageDialog(dialogParent(),
                    userMsg.c_str(), "New ECCE User Message",
                    wxOK|wxICON_INFORMATION|wxSTAY_ON_TOP, wxDefaultPosition);
        userDlg->ShowModal();
        userDlg->Destroy();
      }
    }
  }

  // show the startup message, if any
  string startFileName = Ecce::ecceHome();
  startFileName += "/siteconfig/StartupMessage";
  ifstream startFile(startFileName.c_str());
  if (startFile) {
    string startMsg = ""; 
    while (startFile.getline(buf, BUFSIZE-1)) {
      startMsg += buf;
      startMsg += "\n";
    }

    if (startMsg != "") {
      ewxMessageDialog* startDlg = new ewxMessageDialog(dialogParent(),
                  startMsg.c_str(), "ECCE Startup Message",
                  wxOK|wxICON_INFORMATION|wxSTAY_ON_TOP, wxDefaultPosition);
      startDlg->ShowModal();
      startDlg->Destroy();
    }
  }

  // ecce-csh2sh (run by the `ecce` launcher, once per version) leaves a note
  // when it converted the user's csh job-script snippets or found some it
  // cannot convert.  Shown once.
  string noticeName = Ecce::realUserHome();
  noticeName += "/.ECCE/csh2sh-notice";
  ifstream noticeFile(noticeName.c_str());
  if (noticeFile) {
    string noticeMsg, noticeLine;
    while (getline(noticeFile, noticeLine)) {
      noticeMsg += noticeLine;
      noticeMsg += "\n";
    }
    noticeFile.close();
    unlink(noticeName.c_str());

    if (noticeMsg != "") {
      ewxMessageDialog* noticeDlg = new ewxMessageDialog(dialogParent(),
                  noticeMsg.c_str(), "Job scripts are now sh",
                  wxOK|wxICON_INFORMATION|wxSTAY_ON_TOP, wxDefaultPosition);
      noticeDlg->ShowModal();
      noticeDlg->Destroy();
    }
  }

  return true;
}


int GatewayApp::OnExit()
{
  if (p_sessionWatch) {
    p_sessionWatch->Stop();
    delete p_sessionWatch;
    p_sessionWatch = NULL;
  }

  // This wxTimer logic allows all the other apps in the session to exit
  // cleanly before Gateway tries to unsubscribe messaging.
  // Otherwise, some nastiness could result, although this is a theory.
  // It needs to be in a while loop because timers (like the sleep() call)
  // are interrupted by signals and these happen when other apps exit
  // so this is the only way to guarantee it actually holds up execution
  // of the unsubscribe for the full time
  wxStopWatch swatch;
  
  while (swatch.Time() < 3000)
    wxMilliSleep(3000);

  unsubscribe();

#ifdef EMSL
  EMSLAuth::getCache().writeCache();
#endif

  return wxApp::OnExit();
}


/**
 * Process a message that indicates either the start or end of an
 * ECCE' system activity.  The first argument of "START" will be
 * processed as the start of a new activity.  All other values are
 * processed as the end of an activity.
 */
void GatewayApp::activityMCB(JMSMessage& msg)
{
  string action = msg.getProperty("action");

  // use a case-insensitive comparison method
  if (action == "start") {
    p_gateway->startActivity();
  } else {
    p_gateway->endActivity();
  }
}


/**
 * This message gets delivered to this process too so we need to catch
 * and ignore it so that default processing doesn't ocurr.
 */
void GatewayApp::identifyMCB(JMSMessage&)
{ }


/**
 * Message handler for the ecce_identify_reply message.  What it
 * does is add a button to the windows pulldown menu.  This button
 * represents the responding tool.
 *
 * There was a sporadic but very repeatable bug where the buttons
 * popup normally and fully functional BUT without their labels.  I tried
 * all sorts of strategyies for moving code around and inserting 
 * XmUpdateDisplay calls but none of them seemed to work.  The bug
 * appears to be some sort of timing issue.  The sleep() call that is
 * commented out where the identify message is sent fixed the problem
 * but was too slow.  So instead, I put an x timer delay before posting
 * the buttons.  A 0 length timer didn't work but 25 seems to be ok
 * To make this all work, the vectors p_queued* were added (just for this).
 */
void GatewayApp::identifyReplyMCB(JMSMessage& msg)
{
  p_gateway->addMenuEntry(msg);
}


/**
 * This message gets sent just to the gateway from all other apps so they
 * can tell the gateway to come out of hiding.
 */
void GatewayApp::raiseMeMCB(JMSMessage&)
{
  p_gateway->Iconize(false);
  p_gateway->Show(true);
}


/**
 * Popup an error dialog if a tool failed to start for some reason.
 */
void GatewayApp::toolStartStatusMCB(JMSMessage& msg)
{
  string status = msg.getProperty("status");
  p_gateway->endActivity();
  
  if (status == "failed") {
    //  A launch that failed still counts as the session having started,
    //  or an Organizer that never came up would leave this hidden process
    //  waiting for an app that will never appear.
    p_sessionSeen = true;
    ewxMessageDialog * dlg = new ewxMessageDialog(dialogParent(),
                    "Unable to start the application.\n"
                    "Please use the ECCE Support tool to report this problem.",
                    "System Error", wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
  }
}


void GatewayApp::preferenceMCB(JMSMessage& msg)
{
  p_gateway->preferenceChanged();
}


void GatewayApp::authMCB(JMSMessage& msg)
{
  AuthCache::getCache().msgIn(msg, getMyID());
}


/**
 * The gateway is the FIRST thing in a session to authenticate to the
 * data server (checkServer()/checkServerSetup()/checkUser(), all called
 * from OnInit() before any other app is spawned) -- so the very first
 * time this fires, event.m_user is the name that actually authenticated
 * this session, which is not necessarily the name serverUser() assumed
 * going in (the user can change it in the login dialog: live report,
 * "even with the correct user and password I still don't get access to
 * my files" -- the typed name authenticated fine, but Ecce::serverUser()
 * kept returning the saved/-l name for the rest of the session, so the
 * home-folder check below and every app it spawned used the wrong one).
 *
 * Fixed by making the FIRST accepted name the session's server user
 * from here on (setSessionServerUser() exports ECCE_SERVER_LOGIN,
 * inherited by every app this process goes on to spawn), and, unless
 * `-l` is what set it (session-only by design), remembering it on disk
 * for next time. p_sessionLoginFinalized makes this a one-shot: a LATER
 * auth success -- e.g. the no-access retry finding a folder's real
 * owner -- must not re-point the session at yet another user.
 */
void GatewayApp::authorizationAccepted(const AuthEvent& event)
{
  WxDavAuth::authorizationAccepted(event);

  if (p_sessionLoginFinalized || event.m_user.empty()) {
    return;
  }
  p_sessionLoginFinalized = true;

  // Always set: cheap, and guarantees ECCE_SERVER_LOGIN is exactly the
  // name that just authenticated for the rest of this process and
  // everything it spawns, regardless of what it was set to going in.
  Ecce::setSessionServerUser(event.m_user);

  if (!p_loginFromDashL) {
    Ecce::rememberServerUser(event.m_user);
  }
}


/**
 * Tell other tools to quit.  If Netscape is running from help, just
 * let it keep on running.
 *
 * Regarding old HH code: Hyperhelp is dealt with separately.
 * The HELP_FORCE_QUIT is necessary.  I tried just the regular quit
 * but it didn't work.  The HH manual discusses this a little - basically
 * it just says on some systems, you need to use HELP_FORCE_QUIT.
 */
void GatewayApp::notifyExit()
{
  JMSMessage* msg = newMessage();
  publish("ecce_quit", *msg);
  delete msg;
}


/**
 * Send a message telling tools to iconify or raise depending on
 * the value of iconicState.  Also iconify or raise the preference
 * dialog if there is one.
 */
void GatewayApp::sendIconify(int iconicState)
{
  JMSMessage* msg = newMessage();
  msg->addIntProperty("state", iconicState);
  publish("ecce_set_iconified", *msg);
  delete msg;
}


/**
 * Checks server to see if it's legit and properly setup.
 */
bool GatewayApp::checkServer()
{
  EDSIServerCentral servers;
  return servers.checkServer();
}


/**
 * Move this to separate function and command line option such as
 * gateway -check if its too slow
 */
bool GatewayApp::checkServerSetup()
{
  EDSIServerCentral servers;
  return servers.checkServerSetup();
}


/**
 * At startup time, do some minimal server verification.
 * Its not really clear what needs to be done here since we are
 * attempting to reduce the requirement to have a user directory.
 */
bool GatewayApp::checkUser()
{
  bool ret = true;

  EDSIServerCentral servers;
  EcceURL mount = servers.getDefaultUserHome();

  string directoryPath = mount.getParent().toString();

  // Make virtual connection to URL via EDSI:
  EDSI* connection = EDSIFactory::getEDSI(directoryPath);

  bool userExists = connection->exists();
  string err = connection->m_msgStack.getMessage();
  if (!err.empty()) {
    string msg,title;
    if (connection->m_msgStack.findKey("CANCELED")) {
      msg = "Access controls have been set up on this server.  "
        "You must authenticate to run ECCE.  Contact your "
        "administrator if you do not know your password";
      title = "Authentication Failure";
    } else if (connection->m_msgStack.findKey("TOO_MANY_RETRIES")) {
      msg = "The allowable number of authentication attempts has "
        "been exceeded.  Contact your ECCE administrator for assistance.";
      title = "Authentication Failure";
    } else {
      msg = err;
      title = "ECCE Server Failure";
    }
    ewxMessageDialog* dlg = new ewxMessageDialog(dialogParent(), msg.c_str(),
                                   title.c_str(), wxOK|wxICON_EXCLAMATION,
                                   wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
    ret = false;

  } else if (!userExists) {
    // Leave this in for now but do we really care?
    string msg = "Web server specified as the first entry in DataServers "
      "is not properly setup as an ECCE DAV server.  Contact "
      "your ECCE site administrator:\n";
    msg += connection->m_msgStack.getMessage();
    
    ewxMessageDialog* dlg = new ewxMessageDialog(dialogParent(), msg.c_str(),
                                "ECCE Server Failure",
                                wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
    ret = false;

  } else {
    // Verify the user has a valid directory.
    // Again leave this in for now but do we really care.
    // This used to do a list collection.  This seems expensive so I
    // switched it to an exists call.  The only potential problem is that
    // it exists and is a file but not a directory.

    // Local mode's home is users/local, not users/<account>.
    string userPath = LocalData::dir().empty() ?
      directoryPath + "/" + Ecce::serverUser() : mount.toString();
    connection->setURL(userPath);

    userExists = connection->exists();
    err = connection->m_msgStack.getMessage();
    if (!err.empty()) {
      string msg;
      msg += connection->m_msgStack.getMessage();
      ewxMessageDialog* dlg = new ewxMessageDialog(dialogParent(), msg.c_str(),
                                  "ECCE Server Configuration Error",
                                  wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg->ShowModal();
      dlg->Destroy();
      ret = false;
    } else if (!userExists) {
      string msg = "There is no account for ";
      msg += Ecce::serverUser();
      msg += " on this ECCE data server.  Create one there with "
        "ecce-dataserver-adduser, or start ECCE as another user with "
        "\"ecce -l NAME\" (the name is remembered for later sessions).";
      ewxMessageDialog* dlg = new ewxMessageDialog(dialogParent(), msg.c_str(),
                                  "ECCE Server User Not Recognized",
                                  wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg->ShowModal();
      dlg->Destroy();
      ret = false;
    }
  }

  delete connection;

  return ret;
}


void GatewayApp::reconnectJobStoreMessaging()
{
  // send SIGXPCU to any running jobstore, launcher, organizer processes for
  // this user so JMS messaging will be reconnected

  string headcmd = "ps -u ";
  headcmd += Ecce::realUser();
  headcmd += " -o 'pid args' | grep ";
  string tailcmd = " | grep -v 'sh -c' | grep -v eccejobmaster | grep -v grep";
  FILE* psPtr;
  char buf[1024];
  int pid;

  string cmd = headcmd + "eccejobstore" + tailcmd;
  if ((psPtr = popen(cmd.c_str(), "r")) != NULL) {
    while (fgets(buf, sizeof(buf), psPtr) != NULL) {
      // strtol is smart about stopping at white-space
      pid = (int)strtol(buf, NULL, 10);
      if (pid > 0)
        // originally SIGUSR1 was sent but this killed vim sessions
        (void)kill(pid, SIGCONT);
    }
    // close the pipe
    pclose(psPtr);
  }

  cmd = headcmd + "launcher" + tailcmd;
  if ((psPtr = popen(cmd.c_str(), "r")) != NULL) {
    while (fgets(buf, sizeof(buf), psPtr) != NULL) {
      pid = (int)strtol(buf, NULL, 10);
      if (pid > 0)
        (void)kill(pid, SIGCONT);

    }
    pclose(psPtr);
  }

  cmd = headcmd + "organizer" + tailcmd;
  if ((psPtr = popen(cmd.c_str(), "r")) != NULL) {
    while (fgets(buf, sizeof(buf), psPtr) != NULL) {
      pid = (int)strtol(buf, NULL, 10);
      if (pid > 0)
        (void)kill(pid, SIGCONT);
    }
    pclose(psPtr);
  }
}


/**
 * Called once a second while the Gateway frame is hidden (#185). Quits
 * once an app of this session has been seen and none is left.
 *
 * Idle must hold on two successive ticks: an app launched just before
 * the last one closed is briefly only a /bin/sh between fork and exec.
 */
void GatewayApp::checkSessionEnd()
{
  if (p_sessionWatch == NULL || p_gateway == NULL) return;

  // Never from under a dialog: timers still fire in a modal loop.
  for (wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();
       node; node = node->GetNext()) {
    wxWindow *win = node->GetData();
    if (win != p_gateway && win->IsShown()) return;
  }

  if (otherSessionApps() > 0) {
    p_sessionSeen = true;
    p_idleTicks = 0;
    return;
  }
  if (!p_sessionSeen || ++p_idleTicks < 2) return;

  p_sessionWatch->Stop();
  p_gateway->quit(true);
}


/**
 * The ECCE apps of this session still running: this user's processes
 * running a binary from $ECCE_HOME/bin with this session's
 * ECCE_SESSION_ID (#233), the same rule ecce-gateway-reap uses, so the
 * two agree on when a session is over.
 *
 * A binary counts if $ECCE_HOME/bin/<name> resolves to it, so a bin/
 * of symlinks into a build tree is recognised too. Job monitoring is
 * excluded: eccejobstore/eccejobmaster outlive the session by design,
 * and ecmd is a command runner, not a window.
 *
 * The evidence is the programs' session leases (SessionLease.H) and, on
 * Linux, the process table; ECCE_SESSION_LIVENESS selects one of them.
 */
int GatewayApp::otherSessionApps() const
{
  static const char *notApps[] =
    { "gateway", "eccejobstore", "eccejobmaster", "ecmd", NULL };

  string id = Ecce::sessionId();
  if (id.empty()) return 0;
  pid_t self = getpid();
  std::set<long> apps;

  if (SessionLease::useLease()) {
    std::vector<SessionLease::Holder> held =
      SessionLease::live(string(Ecce::realUserHome()) + "/.ECCE",
                         Ecce::sessionKey());
    for (size_t h = 0; h < held.size(); h++) {
      bool skip = held[h].pid == (long)self;
      for (int i = 0; notApps[i]; i++)
        if (held[h].name == notApps[i]) skip = true;
      if (!skip) apps.insert(held[h].pid);
    }
  }
  if (SessionLease::useProc()) procSessionApps(notApps, apps);
  return (int)apps.size();
}


// The same, from /proc: this user's processes of an $ECCE_HOME/bin binary
// with this session's id in their environment.
void GatewayApp::procSessionApps(const char *const *notApps,
                                 std::set<long>& apps) const
{
  string want = "ECCE_SESSION_ID=" + Ecce::sessionId();
  string bindir = string(Ecce::ecceHome()) + "/bin/";
  pid_t self = getpid();
  uid_t uid = getuid();

  DIR *proc = opendir("/proc");
  if (proc == NULL) return;

  struct dirent *entry;
  while ((entry = readdir(proc)) != NULL) {
    if (!isdigit((unsigned char)entry->d_name[0])) continue;
    if ((pid_t)atoi(entry->d_name) == self) continue;

    string dir = string("/proc/") + entry->d_name;
    struct stat st;
    if (stat(dir.c_str(), &st) != 0 || st.st_uid != uid) continue;

    char buf[PATH_MAX];
    ssize_t len = readlink((dir + "/exe").c_str(), buf, sizeof(buf) - 1);
    if (len <= 0) continue;          // zombie, kernel thread, or not ours
    buf[len] = '\0';
    string exe = buf;
    static const string deleted = " (deleted)";   // rebuilt while running
    if (exe.size() > deleted.size() &&
        exe.compare(exe.size() - deleted.size(), deleted.size(), deleted) == 0)
      exe.erase(exe.size() - deleted.size());

    string name = exe.substr(exe.rfind('/') + 1);
    bool skip = false;
    for (int i = 0; notApps[i]; i++)
      if (name == notApps[i]) skip = true;
    if (skip) continue;

    char real[PATH_MAX];
    if (realpath((bindir + name).c_str(), real) == NULL || exe != real)
      continue;

    ifstream env((dir + "/environ").c_str());
    string var;
    while (std::getline(env, var, '\0')) {
      if (var.compare(0, 16, "ECCE_SESSION_ID=") == 0) {
        if (var == want) apps.insert(atol(entry->d_name));
        break;
      }
    }
  }
  closedir(proc);
}
