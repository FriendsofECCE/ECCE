/**
 * @file
 *
 * The Launcher's message handlers on a dispatcher the window's opener owns
 * (#983), so the Launcher can also be a tab or window of the Organizer's
 * process.
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

#include "wxgui/ewxWindowUtils.H"
#include "LauncherMessages.H"
#include "WxLauncher.H"


LauncherMessages::LauncherMessages(LaunchPanel *panel, WxLauncher *host)
    : WxJMSMessageDispatch(LAUNCHER, true), p_panel(panel), p_host(host)
{
}


void LauncherMessages::subscribeMessages()
{
    //  Desktop Messages; handled by WxLauncherApp application class
    subscribe("ecce_invoke", (wxJmsCBFunc)&LauncherMessages::invokeMCB);
    subscribe("ecce_quit", (wxJmsCBFunc)&LauncherMessages::quitMCB);
    subscribe("ecce_identify", (wxJmsCBFunc)&LauncherMessages::identifyMCB);

    // handle return message from a launcher request to start another tool
    subscribe("ecce_invoke_status", (wxJmsCBFunc)&LauncherMessages::invokeStatusMCB, false);

    // Global Preference Messages
    subscribe("ecce_preferences_states", (wxJmsCBFunc)&LauncherMessages::updatePrefsMCB);
    subscribe("ecce_preferences_misc", (wxJmsCBFunc)&LauncherMessages::updatePrefsMCB);

    // Calc/Project Changes; directed to the WxLauncher frame
    subscribe("ecce_url_renamed", (wxJmsCBFunc)&LauncherMessages::calcMoveMCB);
    subscribe("ecce_url_removed", (wxJmsCBFunc)&LauncherMessages::calcDeleteMCB);
    subscribe("ecce_url_code", (wxJmsCBFunc)&LauncherMessages::calcCodeMCB);

    subscribe("ecce_url_state", (wxJmsCBFunc)&LauncherMessages::statusChangeMCB, false);

    // eccejobstore (JobStore.C's cleanup()) sets the calc's final state
    // -- which is what drives statusChangeMCB above -- and only THEN
    // computes and records runStatusReason as a separate metadata
    // property, published on this topic.  A Launcher window already
    // open for the calc would otherwise see the state change with no
    // reason yet to show (Andy, 2026-09-27).
    subscribe("ecce_url_property", (wxJmsCBFunc)&LauncherMessages::propertyChangeMCB, false);

    // Machine Config/Registration Changed
    subscribe("ecce_machconf_changed", (wxJmsCBFunc)&LauncherMessages::machConfChangedMCB);
    subscribe("ecce_machreg_changed", (wxJmsCBFunc)&LauncherMessages::machregChangedMCB);

    subscribe("ecce_auth_changed", (wxJmsCBFunc)&LauncherMessages::msgAuthMCB);
}


 void LauncherMessages::invokeStatusMCB(JMSMessage& msg)
 {
    static const char *errMsg =
                "Unable to start the application.  "
                "Please report it at https://github.com/FriendsofECCE/ECCE/issues .";

    string status = msg.getProperty("status");

    if (status == "failed")
    {
        p_panel->showFeedbackMessage(errMsg, WxFeedback::SYSTEM);
    }
}


string LauncherMessages::identify()
{
    EcceURL url = p_panel->getContext();
    string idnty = LAUNCHER;
    string path = EDSIServerCentral().mapURLtoName(url);

    if (!path.empty())
        idnty += ": " + path;

    return idnty;
}


void LauncherMessages::identifyMCB(JMSMessage& msg)
{
    JMSMessage* resp = newMessage();
    resp->addProperty("info", this->identify());
    publish("ecce_identify_reply", *resp);

    delete resp;
}


void LauncherMessages::invokeMCB(JMSMessage& msg)
{
    if (msg.containsProperty("calcurl"))
    {
        string url = msg.getProperty("calcurl");
        p_panel->setContext(url);
        this->setPollContext(url);

        p_host->Iconize(false);
        p_host->Show(true);
        ewxRaiseWindow(p_host);
        p_host->RequestUserAttention();
    }
}


void LauncherMessages::quitMCB(JMSMessage& msg)
{
   if (msg.getSender().getName() == GATEWAY)
   {
        //  Force the window to close; disallows veto
        p_host->Close(true);
        unsubscribe();
   }
}


/**
 * MCB.
 * Updates the resource pool when a resource is moved/renamed on DAV.
 * @param msg contains the old and the new url for the resource
 */
void LauncherMessages::calcMoveMCB(JMSMessage& msg)
{
  string oldURL = msg.getProperty("oldurl");
  string newURL = msg.getProperty("newurl");
  
  if (p_panel->hasContext() &&
      p_panel->getContext().isChildOrMe(oldURL)) {
    EDSIFactory::renamePoolResource(oldURL, newURL);
    string calcurl = p_panel->getContext().toString();
    p_panel->setFeedbackCaption(EDSIServerCentral().
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
void LauncherMessages::calcDeleteMCB(JMSMessage& msg)
{
    string urlstr = msg.getProperty("url");

    if (p_panel->hasContext() &&
        p_panel->getContext().isChildOrMe(urlstr))
    {
        p_panel->clearContext();
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
void LauncherMessages::calcCodeMCB(JMSMessage& msg)
{
    string urlstr = msg.getProperty("url");

    // Make sure we have the same context

    if (msg.getSender().getID() != getMyID())
    {
        if (p_panel->hasContext())
        {
            EcceURL calcurl = p_panel->getContext();

            if (calcurl.toString() == urlstr)
            {
                EDSIFactory::changePoolResource(urlstr);

                p_panel->validateCode();
            }
        }
    }
}


/**
 * MCB.
 * Updates the resource pool when a resource's metadata is changed on DAV.
 * @param msg contains the url of the resource to update
 */
void LauncherMessages::statusChangeMCB(JMSMessage& msg)
{
    string path = msg.getProperty("url");
    EcceURL url(path);

    if (p_panel->hasContext() && (p_panel->getContext() == url))
    {
        EDSIFactory::changePoolResource(path);

        string statestr = msg.getProperty("state");
        ResourceDescriptor::RUNSTATE state =
                                     ResourceUtils::stringToState(statestr);
        p_panel->updateContext(state);
    }
}


void LauncherMessages::propertyChangeMCB(JMSMessage& msg)
{
    string path = msg.getProperty("url");
    EcceURL url(path);

    if (msg.getProperty("name") != "runStatusReason")
        return;

    if (p_panel->hasContext() && (p_panel->getContext() == url))
    {
        EDSIFactory::changePoolResource(path);
        p_panel->updateContext(p_panel->getContextState());
    }
}


void LauncherMessages::machregChangedMCB(JMSMessage& msg)
{
    p_panel->machRegChanged();
}


void LauncherMessages::msgAuthMCB(JMSMessage& msg)
{
    AuthCache::getCache().msgIn(msg, getMyID());
}


/**
 *  Responds to msg that says the user has changed preferences for States
 *  or for miscellaneous.
*/
void LauncherMessages::updatePrefsMCB(JMSMessage& msg)
{
    p_panel->updateStatePreferences();
}


/**
 *   Set flag indicating that the machine configuration file needs to be
 *   re-read next time it is used.
 */
void LauncherMessages::machConfChangedMCB(JMSMessage& msg)
{
    string id = msg.getSender().getID();

    if ((id != getMyID()) && (id != p_panel->getMyID()))
         p_panel->reloadMachinePreferences();
}
