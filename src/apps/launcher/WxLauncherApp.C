/**
 * @file
 * @author Ken Swanson
 *
 */


#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/Ecce.H"
#include "util/ResourceUtils.H"
#include "util/EcceURL.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ResourceDescriptor.H"

#include "WxLauncherApp.H"
#include "WxLauncherGUI.H"
#include "WxLauncher.H"
#include "WxLauncherScript.H"

IMPLEMENT_APP(WxLauncherApp)
//IMPLEMENT_CLASS(WxLauncherApp, ewxApp);

int WxLauncherApp::p_indentLevel = 0;



WxLauncherApp::WxLauncherApp()
    : ewxApp(), WxJMSMessageDispatch(LAUNCHER, true)
{
    p_launchFrame = NULL;
}


string WxLauncherApp::getName() const
{
    return LAUNCHER;
}


bool WxLauncherApp::OnInit()
{
    string cntxt = "";

    ewxApp::OnInit();

   if (argc>2 && strcmp(argv[1].ToStdString().c_str(),"-pipe")==0) {
     AuthCache::getCache().pipeIn(argv[2].ToStdString());
   }

    p_launchFrame = new WxLauncher(NULL);
    EDSIFactory::addAuthEventListener(p_launchFrame);

    //  "-context URL" may follow "-pipe FILE" (tests/apps/qe_walkthrough_test.py).
  for (int a = 1; a + 1 < this->argc; a++) {
    if (strcmp(this->argv[a].ToStdString().c_str(), "-context") == 0) {
      cntxt = this->argv[a + 1];
      break;
    }
  }

    p_launchFrame->setContext(cntxt);
    p_launchFrame->loadSettings("Launcher");

    // Show the main window
    p_launchFrame->Show();
    SetTopWindow(p_launchFrame);
    registerTopShell(p_launchFrame);

    //  Test hook (WxLauncherScript.H): drive the real frame without a broker.
    const char* script = getenv("ECCE_LAUNCHER_SCRIPT");
    if (script != NULL)
    {
        fprintf(stderr, "[LAUNCHER] binary=%s ECCE_HOME=%s\n",
                (const char*)wxString(this->argv[0]).c_str(), Ecce::ecceHome());
        LauncherScript* run = new LauncherScript(p_launchFrame, script);
        run->start();
        return true;
    }

    //  Subscribe to Messages
    subscribeMessages();
    startSubscriber();
    notifyReady();

    return true;
}




void WxLauncherApp::subscribeMessages()
{
    //  Desktop Messages; handled by WxLauncherApp application class
    subscribe("ecce_invoke", (wxJmsCBFunc)&WxLauncherApp::invokeMCB);
    subscribe("ecce_quit", (wxJmsCBFunc)&WxLauncherApp::quitMCB);
    subscribe("ecce_identify", (wxJmsCBFunc)&WxLauncherApp::identifyMCB);

    // handle return message from a launcher request to start another tool
    subscribe("ecce_invoke_status", (wxJmsCBFunc)&WxLauncherApp::invokeStatusMCB, false);

    // Global Preference Messages
    subscribe("ecce_preferences_states", (wxJmsCBFunc)&WxLauncherApp::updatePrefsMCB);
    subscribe("ecce_preferences_misc", (wxJmsCBFunc)&WxLauncherApp::updatePrefsMCB);

    // Calc/Project Changes; directed to the WxLauncher frame
    subscribe("ecce_url_renamed", (wxJmsCBFunc)&WxLauncherApp::calcMoveMCB);
    subscribe("ecce_url_removed", (wxJmsCBFunc)&WxLauncherApp::calcDeleteMCB);
    subscribe("ecce_url_code", (wxJmsCBFunc)&WxLauncherApp::calcCodeMCB);

    subscribe("ecce_url_state", (wxJmsCBFunc)&WxLauncherApp::statusChangeMCB, false);

    // eccejobstore (JobStore.C's cleanup()) sets the calc's final state
    // -- which is what drives statusChangeMCB above -- and only THEN
    // computes and records runStatusReason as a separate metadata
    // property, published on this topic.  A Launcher window already
    // open for the calc would otherwise see the state change with no
    // reason yet to show (Andy, 2026-09-27).
    subscribe("ecce_url_property", (wxJmsCBFunc)&WxLauncherApp::propertyChangeMCB, false);

    // Machine Config/Registration Changed
    subscribe("ecce_machconf_changed", (wxJmsCBFunc)&WxLauncherApp::machConfChangedMCB);
    subscribe("ecce_machreg_changed", (wxJmsCBFunc)&WxLauncherApp::machregChangedMCB);

    subscribe("ecce_auth_changed", (wxJmsCBFunc)&WxLauncherApp::msgAuthMCB);
}


int WxLauncherApp::OnExit()
{
    //  Unsubscribe from Messaging Service
    unsubscribe();

    return 0;
}


 void WxLauncherApp::invokeStatusMCB(JMSMessage& msg)
 {
    static const char *errMsg =
                "Unable to start the application.  "
                "Please use the ECCE Support tool to report this problem.";

    string status = msg.getProperty("status");

    if (status == "failed")
    {
        p_launchFrame->showFeedbackMessage(errMsg, WxFeedback::SYSTEM);
    }
}


string WxLauncherApp::identify()
{
    EcceURL url = p_launchFrame->getContext();
    string idnty = this->getName();
    string path = EDSIServerCentral().mapURLtoName(url);

    if (!path.empty())
        idnty += ": " + path;

    return idnty;
}


void WxLauncherApp::identifyMCB(JMSMessage& msg)
{
    JMSMessage* resp = newMessage();
    resp->addProperty("info", this->identify());
    publish("ecce_identify_reply", *resp);

    delete resp;
}


void WxLauncherApp::invokeMCB(JMSMessage& msg)
{
    if (msg.containsProperty("calcurl"))
    {
        string url = msg.getProperty("calcurl");
        p_launchFrame->setContext(url);
        this->setPollContext(url);

        p_launchFrame->Iconize(false);
        p_launchFrame->Raise();
        p_launchFrame->Show(true);
        p_launchFrame->SetFocus();
        p_launchFrame->RequestUserAttention();
    }
}


void WxLauncherApp::quitMCB(JMSMessage& msg)
{
   if (msg.getSender().getName() == GATEWAY)
   {
        //  Force the window to close; disallows veto
        p_launchFrame->Close(true);
        this->OnExit();
   }
}


/**
 * MCB.
 * Updates the resource pool when a resource is moved/renamed on DAV.
 * @param msg contains the old and the new url for the resource
 */
void WxLauncherApp::calcMoveMCB(JMSMessage& msg)
{
  string oldURL = msg.getProperty("oldurl");
  string newURL = msg.getProperty("newurl");
  
  if (p_launchFrame->hasContext() &&
      p_launchFrame->getContext().isChildOrMe(oldURL)) {
    EDSIFactory::renamePoolResource(oldURL, newURL);
    string calcurl = p_launchFrame->getContext().toString();
    p_launchFrame->setFeedbackCaption(EDSIServerCentral().
                                      mapURLtoName(calcurl));
    setPollContext(calcurl);
  }
}


/**
 * MCB.
 * Updates the resource pool when a resource is removed from DAV.
 * @param msg contains the url of the resource that no longer exists
 * @todo Lock the launcher when it's calculation is removed elsewhere
 */
void WxLauncherApp::calcDeleteMCB(JMSMessage& msg)
{
    string urlstr = msg.getProperty("url");

    if (p_launchFrame->hasContext() &&
        p_launchFrame->getContext().isChildOrMe(urlstr))
    {
        p_launchFrame->clearContext();
        EDSIFactory::removePoolResource(urlstr);
    }
}


/**
 *  MCB
 *  Handle calc modification message from calced.  We only care about
 *  this message if it applies to one of our calculations and only if it is
 *  a code change.  Since we keep an internal vector of all the codes
 *  required for each calc, it must be kept up to date.
 *
 *  Since we also send a message that the calc was modified when the
 *  user changes the machine name, we want to ignore our own messages!
 */
void WxLauncherApp::calcCodeMCB(JMSMessage& msg)
{
    string urlstr = msg.getProperty("url");

    // Make sure we have the same context

    if (msg.getSender().getID() != getMyID())
    {
        if (p_launchFrame->hasContext())
        {
            EcceURL calcurl = p_launchFrame->getContext();

            if (calcurl.toString() == urlstr)
            {
                EDSIFactory::changePoolResource(urlstr);

                p_launchFrame->validateCode();
            }
        }
    }
}


/**
 * MCB.
 * Updates the resource pool when a resource's metadata is changed on DAV.
 * @param msg contains the url of the resource to update
 */
void WxLauncherApp::statusChangeMCB(JMSMessage& msg)
{
    string path = msg.getProperty("url");
    EcceURL url(path);

    if (p_launchFrame->hasContext() && (p_launchFrame->getContext() == url))
    {
        EDSIFactory::changePoolResource(path);

        string statestr = msg.getProperty("state");
        ResourceDescriptor::RUNSTATE state =
                                     ResourceUtils::stringToState(statestr);
        p_launchFrame->updateContext(state);
    }
}


void WxLauncherApp::propertyChangeMCB(JMSMessage& msg)
{
    string path = msg.getProperty("url");
    EcceURL url(path);

    if (msg.getProperty("name") != "runStatusReason")
        return;

    if (p_launchFrame->hasContext() && (p_launchFrame->getContext() == url))
    {
        EDSIFactory::changePoolResource(path);
        p_launchFrame->updateContext(p_launchFrame->getContextState());
    }
}


void WxLauncherApp::machregChangedMCB(JMSMessage& msg)
{
    p_launchFrame->machRegChanged();
}


void WxLauncherApp::msgAuthMCB(JMSMessage& msg)
{
    AuthCache::getCache().msgIn(msg, getMyID());
}


/**
 *  Responds to msg that says the user has changed preferences for States
 *  or for miscellaneous.
*/
void WxLauncherApp::updatePrefsMCB(JMSMessage& msg)
{
    p_launchFrame->updateStatePreferences();
}


/**
 *   Set flag indicating that the machine configuration file needs to be
 *   re-read next time it is used.
 */
void WxLauncherApp::machConfChangedMCB(JMSMessage& msg)
{
    string id = msg.getSender().getID();

    if ((id != getMyID()) && (id != p_launchFrame->getMyID()))
         p_launchFrame->reloadMachinePreferences();
}
