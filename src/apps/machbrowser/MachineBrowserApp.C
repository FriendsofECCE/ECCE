/**
 *  @file
 *  @author Ken Swanson
 *
 *  Parent application for the MachineBrowser utility.
 *
 */


#include "util/Ecce.H"

#include "tdat/AuthCache.H"

#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxWindowUtils.H"

#include "MachineBrowserApp.H"
#include "MachineBrowserGUI.H"

IMPLEMENT_APP(MachineBrowserApp)

MachineBrowserApp::MachineBrowserApp()
            : WxAppJMSMessageDispatch(MACHBROWSER)
{
    p_brwsrFrame = NULL;
}


MachineBrowserApp::~MachineBrowserApp()
{
}


bool MachineBrowserApp::OnInit()
{
    ewxApp::OnInit();

    if (argc>2 && strcmp(argv[1],"-pipe")==0) {
      AuthCache::getCache().pipeIn(argv[2]);
    }

    p_brwsrFrame = new MachineBrowser(NULL);
    p_brwsrFrame->Show();
    SetTopWindow(p_brwsrFrame);
    registerTopShell(p_brwsrFrame);

    subscribeMessages();
    startSubscriber();
    notifyReady();

    return true;
}


void MachineBrowserApp::subscribeMessages()
{
    subscribe("ecce_machconf_changed",
                    (wxJmsCBFunc)&MachineBrowserApp::machineConfigChangedMCB);
    subscribe("ecce_machreg_changed",
                    (wxJmsCBFunc)&MachineBrowserApp::machineRegChangedMCB);
    subscribe("ecce_auth_changed", (wxJmsCBFunc)&MachineBrowserApp::authMCB);

    WxAppJMSMessageDispatch::subscribeMessages();
}


int MachineBrowserApp::OnExit()
{
    unsubscribe();

    return 0;
}


string MachineBrowserApp::identify()
{
    string dscpn = getName();
    string machName = "";

    if (p_brwsrFrame != NULL)
    {
        machName = p_brwsrFrame->getSelectionName();

        if (machName.size() > 0)
            dscpn.append(":  " + machName);
    }

    return dscpn;
}


void MachineBrowserApp::machineConfigChangedMCB(JMSMessage &msg)
{
    string id = msg.getSender().getID();

    if ((id != getMyID()) && (id != p_brwsrFrame->getMyID()))
    {
        RefMachine::markUpdateUserPrefs();
        MachinePreferences::refresh(true);
        p_brwsrFrame->refreshList();
    }
}


void MachineBrowserApp::machineRegChangedMCB(JMSMessage &msg)
{
    RefMachine::markUpdateUserPrefs();
    MachinePreferences::refresh(true);
    p_brwsrFrame->refreshList();
}


void MachineBrowserApp::authMCB(JMSMessage &msg)
{
    AuthCache::getCache().msgIn(msg, getMyID());
}


string MachineBrowserApp::getName()
{
    return "Machine Browser";
}
