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

#include <signal.h>

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/SDirectory.H"
#include "util/Preferences.H"
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


GatewayApp::GatewayApp()
  : WxJMSMessageDispatch(GATEWAY)
{
}


string GatewayApp::getName() const
{
   return GATEWAY;
}


bool GatewayApp::OnInit()
{
  ewxApp::OnInit();

  string compileVersion = Ecce::ecceVersion();
  int idx;
  for (idx=0; idx<argc; idx++) {
    if (strncmp(argv[idx],"-V",2) == 0) {
      cmdLineVersion = argv[idx];
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
  subscribe("ecce_activity",(wxJmsCBFunc)&GatewayApp::activityMCB, false);
  subscribe("ecce_identify",(wxJmsCBFunc)&GatewayApp::identifyMCB);
  subscribe("ecce_identify_reply",(wxJmsCBFunc)&GatewayApp::identifyReplyMCB);
  subscribe("ecce_gateway_raise",(wxJmsCBFunc)&GatewayApp::raiseMeMCB);
  subscribe("ecce_invoke_status",(wxJmsCBFunc)&GatewayApp::toolStartStatusMCB, false);
  subscribe("ecce_preferences_gateway",(wxJmsCBFunc)&GatewayApp::preferenceMCB, false);
  subscribe("ecce_auth_changed",(wxJmsCBFunc)&GatewayApp::authMCB, false);

  startSubscriber();

  setAuthDialogParent(p_gateway);
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

  try {
    if (!checkServerSetup()) {
      p_gateway->quit(false); // canceled
    }
  } catch (RetryException& rex) {
    ewxMessageDialog * dlg =
      new ewxMessageDialog(p_gateway, rex.what(), "Retries exceeded!",
                           wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
    p_gateway->quit(false);
  } catch (EcceException& ex) {
    string msg = ex.what();
    msg += "Please contact your ECCE Administrator.";
    ewxMessageDialog * dlg =
      new ewxMessageDialog(p_gateway,  msg.c_str(), "ECCE Server Failure",
                           wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
    p_gateway->quit(false);
  }

  try {
    if (!checkServer()) {
      p_gateway->quit(false); // canceled
    }
  } catch (RetryException& rex) {
    ewxMessageDialog * dlg =
      new ewxMessageDialog(p_gateway, rex.what(), "Retries exceeded!",
                           wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
    p_gateway->quit(false);
  } catch (EcceException& ex) {
    string msg = "An error has occured while attempting to access "
      "the ECCE server.  Please contact your administrator for assistence "
      "if the problem persists.\n";
    msg += ex.what();
    ewxMessageDialog * dlg =
      new ewxMessageDialog(p_gateway,  msg.c_str(), "ECCE Server Failure",
                           wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
    p_gateway->quit(false);
  }

  if (!checkUser()) {
    p_gateway->quit(false);
  }

  // notify pertinent eccejobstore processes to reconnect tooltalk messaging
  reconnectJobStoreMessaging();

  // Notify execer that I am ready to receive messages:

  notifyReady();
  p_gateway->Show(true);

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
        ewxMessageDialog* userDlg = new ewxMessageDialog(p_gateway,
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
      ewxMessageDialog* startDlg = new ewxMessageDialog(p_gateway,
                  startMsg.c_str(), "ECCE Startup Message",
                  wxOK|wxICON_INFORMATION|wxSTAY_ON_TOP, wxDefaultPosition);
      startDlg->ShowModal();
      startDlg->Destroy();
    }
  }

  return true;
}


int GatewayApp::OnExit()
{
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
    ewxMessageDialog * dlg = new ewxMessageDialog(p_gateway,
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
bool GatewayApp::checkServer() throw (RetryException,EcceException)
{
  EDSIServerCentral servers;
  return servers.checkServer();
}


/**
 * Move this to separate function and command line option such as
 * gateway -check if its too slow
 */
bool GatewayApp::checkServerSetup() throw (RetryException,EcceException)
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
    ewxMessageDialog* dlg = new ewxMessageDialog(p_gateway, msg.c_str(),
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
    
    ewxMessageDialog* dlg = new ewxMessageDialog(p_gateway, msg.c_str(),
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

    string userPath = directoryPath + "/" + Ecce::serverUser();
    connection->setURL(userPath);

    userExists = connection->exists();
    err = connection->m_msgStack.getMessage();
    if (!err.empty()) {
      string msg;
      msg += connection->m_msgStack.getMessage();
      ewxMessageDialog* dlg = new ewxMessageDialog(p_gateway, msg.c_str(),
                                  "ECCE Server Configuration Error",
                                  wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg->ShowModal();
      dlg->Destroy();
      ret = false;
    } else if (!userExists) {
      string msg = "No top-level user directory exists for ";
      msg += Ecce::serverUser();
      msg += " on the default ECCE server.  The site administrator "
        "must run the add_ecce_user script and create an account for ";
      msg += Ecce::serverUser();
      msg += " in order to run ECCE as this user.";
      ewxMessageDialog* dlg = new ewxMessageDialog(p_gateway, msg.c_str(),
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
