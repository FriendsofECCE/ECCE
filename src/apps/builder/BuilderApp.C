/**
 * @file
 * @author Karen Schuchardt 
 *
 */


#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include <wx/timer.h>

#include <iostream>
using std::cerr;
using std::endl;

#include "util/Ecce.H"
#include "util/LocalData.H"
#include "util/EcceURL.H"
#include "util/StringTokenizer.H"
#include "util/Command.H"
#include "util/ErrMsg.H"
#include "util/SDirectory.H"

#include "tdat/AuthCache.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"

#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WxState.H"
#include "wxgui/ewxMessageDialog.H"

#include "viz/SelectCmd.H"
#include "viz/ValidateStrCmd.H"

#include "tdat/TPerTab.H"
#include "BuilderApp.H"
#include "Builder.H"
#include "BuilderMessages.H"

IMPLEMENT_APP(BuilderApp)



static const string appName()
{
  return (getenv("ECCE_INVOKE_VIEWER")?CALCVIEWER:BUILDER);
}


BuilderApp::BuilderApp()
    : ewxApp()
{
  p_builder = NULL;
  p_msgs = NULL;
}


BuilderApp::~BuilderApp()
{
}


string BuilderApp::getName() const
{
  return appName();
}


bool BuilderApp::OnInit()
{
  ewxApp::OnInit();
  
  string context = "";
  string file;
  bool standalone = false;
  bool started = false;    // by the Organizer or the gateway (-pipe)
  if (argc > 1) {
    for (int i=1; i<argc; i++) {
      if (strcmp(argv[i].ToStdString().c_str(), "-standalone") == 0)
        standalone = true;
      else if (strcmp(argv[i].ToStdString().c_str(), "-help") == 0) {
        usage();
        exit(-1);
      } else if (strcmp(argv[i].ToStdString().c_str(), "-pipe") == 0) {
        // Was argv[1]: a typo that made -pipe recognised only when it
        // happened to be the FIRST argument. Anywhere else it fell through
        // to the else branch and was swallowed as "a file name to open",
        // silently discarding the credentials and taking the next argument
        // with it.
        AuthCache::getCache().pipeIn(argv[++i].ToStdString());
        started = true;
      } else if (strcmp(argv[i].ToStdString().c_str(), "-context") == 0) {
        // Might need index out of boundary check
        context = argv[++i];
      } else {
        // assume its a file name to open
        file = argv[i];
      }
    }
  }

  // Determine if this invocation is for a viewer from outside ECCE (standalone)
  bool soloviewer = !getenv("ECCE_INVOKE_FROMECCE") &&
                     getenv("ECCE_INVOKE_VIEWER");

  if (!soloviewer && !standalone) {
    EDSIFactory::addAuthEventListener(this);
  }

  p_builder = new Builder(NULL, standalone);
  p_msgs = new BuilderMessages(appName(), p_builder);

  p_builder->Show(false);
  SetTopWindow(p_builder);
  //  Started for a tool request (-pipe), the calculation comes in the
  //  ecce_invoke that follows. Keep the shell hidden until then, since the
  //  dispatcher raises registered shells before delivering that message.
  const bool showOnInvoke = started && !standalone && context.empty() &&
                            file.empty();
  const bool viewerOnInvoke = showOnInvoke && getenv("ECCE_INVOKE_VIEWER");
  if (!showOnInvoke) p_msgs->registerTopShell(p_builder);

  if (context != "") {
    p_builder->setContext(context);
    p_msgs->setPollContext(context);
  }

  //  Subscribe to Messages
  if (!standalone) {
    p_msgs->subscribeMessages();
    p_msgs->startSubscriber();
  }

  if (file != "") {
     // Let the interpretter report errors
     //p_builder->interpretCommand("open filename " + file);
     p_builder->setContext(file);
  }

  if (soloviewer) {
    // Connect to data server for importing calculation output files
    setAuthDialogParent(p_builder);
    EDSIFactory::addAuthEventListener(this);

    // create the preferences directory if it doesn't already exist so we
    // have someplace to put all the wonderful preference files
    SDirectory directory(Ecce::realUserPrefPath(), 0700);
    if (!directory.exists()) {
      string msg = "Could not create user preferences directory ";
      msg += Ecce::realUserPrefPath();
      EE_RT_ASSERT(FALSE, EE_FATAL, msg);
    }

    //  Defensive, not a fix for an observed crash: GatewayApp::OnInit()
    //  had this exact shape (quit() called with no `return`, so a failed
    //  check fell through into the next try block and could call quit()
    //  again on already-torn-down state -- segfault, #120/97d4a862).
    //  Builder::quit() currently can't be hit twice this way in practice
    //  -- with nothing loaded yet (always true this early) it always
    //  reaches its unconditional _exit(0) rather than returning to the
    //  caller -- but that's incidental to how quit() happens to be
    //  written today, not something this startup sequence should rely
    //  on. Same `return false` after every quit() as the Gateway fix,
    //  so this can't become the same live bug if quit() ever changes.
    try {
      if (!checkServerSetup()) {
        p_builder->quit(false); // canceled
        return false;
      }
    } catch (RetryException& rex) {
      ewxMessageDialog * dlg =
        new ewxMessageDialog(p_builder, rex.what(), "Retries exceeded!",
                             wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg->ShowModal();
      dlg->Destroy();
      p_builder->quit(false);
      return false;
    } catch (EcceException& ex) {
      string msg = ex.what();
      msg += "Please contact your ECCE Administrator.";
      ewxMessageDialog * dlg =
        new ewxMessageDialog(p_builder,  msg.c_str(), "ECCE Server Failure",
                             wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg->ShowModal();
      dlg->Destroy();
      p_builder->quit(false);
      return false;
    }

    try {
      if (!checkServer()) {
        p_builder->quit(false); // canceled
        return false;
      }
    } catch (RetryException& rex) {
      ewxMessageDialog * dlg =
        new ewxMessageDialog(p_builder, rex.what(), "Retries exceeded!",
                             wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg->ShowModal();
      dlg->Destroy();
      p_builder->quit(false);
      return false;
    } catch (EcceException& ex) {
      string msg = "An error has occured while attempting to access "
        "the ECCE server.  Please contact your administrator for assistence "
        "if the problem persists.\n";
      msg += ex.what();
      ewxMessageDialog * dlg =
        new ewxMessageDialog(p_builder,  msg.c_str(), "ECCE Server Failure",
                             wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg->ShowModal();
      dlg->Destroy();
      p_builder->quit(false);
      return false;
    }

    if (!checkUser()) {
      p_builder->quit(false);
      return false;
    }
  }

  if (!standalone) {
    // Notify execer that I am ready to receive messages:
    p_msgs->notifyReady();
  }

  //  The timer shows a normal Builder if its invoke never comes. A Viewer
  //  has no useful empty state, so its fallback exits and lets the Gateway
  //  report the failed launch instead of showing a blank Builder window.
  if (showOnInvoke) {
    wxTimer *timer = new wxTimer();
    timer->Bind(wxEVT_TIMER, [this, timer, viewerOnInvoke](wxTimerEvent&) {
      // A Viewer launch must have an ecce_invoke with its calculation.  Do
      // not expose this process as an empty Builder if that request never
      // arrives; exiting lets the Gateway report the failed launch.
      if (viewerOnInvoke && !p_msgs->invokeReceived()) {
        p_builder->quit(false);
        return;
      }
      p_msgs->registerTopShell(p_builder);
      if (!p_builder->IsShown()) p_builder->Show(true);
      p_builder->CallAfter([timer]() { delete timer; });
    });
    //  A busy machine can hold the Viewer's invoke back for seconds (#628);
    //  a Viewer waits much longer before it gives up than a Builder does
    //  before it shows itself empty.
    timer->StartOnce(viewerOnInvoke ? 60000 : 5000);
  } else {
    p_builder->Show(true);
  }

  return true;
}


int BuilderApp::OnExit()
{
  //  Unsubscribe from Messaging Service
  if (p_msgs) p_msgs->unsubscribe();

  return 0;
}


bool BuilderApp::checkServer()
{
  EDSIServerCentral servers;
  return servers.checkServer();
}


/**
 * Move this to separate function and command line option such as
 * gateway -check if its too slow
 */
bool BuilderApp::checkServerSetup()
{
  EDSIServerCentral servers;
  return servers.checkServerSetup();
}


/**
 * At startup time, do some minimal server verification.
 * Its not really clear what needs to be done here since we are
 * attempting to reduce the requirement to have a user directory.
 */
bool BuilderApp::checkUser()
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
    ewxMessageDialog* dlg = new ewxMessageDialog(p_builder, msg.c_str(),
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

    ewxMessageDialog* dlg = new ewxMessageDialog(p_builder, msg.c_str(),
                                 "ECCE Server Failure", wxOK|wxICON_EXCLAMATION,
                                 wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
    ret = false;

  } else {
    // Verify the user has a valid directory.
    // Again leave this in for now but do we really care.
    // Local mode's home is users/local, not users/<account>.
    string userPath = LocalData::dir().empty() ?
      directoryPath + "/" + Ecce::serverUser() : mount.toString();
    connection->setURL(userPath);

    userExists = connection->exists();
    err = connection->m_msgStack.getMessage();
    if (!err.empty()) {
      string msg;
      msg += connection->m_msgStack.getMessage();
      ewxMessageDialog* dlg = new ewxMessageDialog(p_builder, msg.c_str(),
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
      ewxMessageDialog* dlg = new ewxMessageDialog(p_builder, msg.c_str(),
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


/**
 * This usage statement is intended for end users so it does not document
 * options intended for us.  See the argv parsing above for all developer
 * options. 
 */
void BuilderApp::usage()
{
   cout << "Usage: ebuilder [-help] [file]" << endl;
   cout << "\t-help:  this message" << endl;
   cout << "\tfile:  name of file to load at startup" << endl;
}
