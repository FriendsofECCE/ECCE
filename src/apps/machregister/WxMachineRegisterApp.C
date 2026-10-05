/**
 *  @file
 *  @author Ken Swanson
 *
 *  Parent application for the MachineRegister utility.  Provides JMS
 *  listening services.
 *
 */

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/Ecce.H"

#include "tdat/AuthCache.H"

#include "dsm/ResourceDescriptor.H"

#include "WxMachineRegisterApp.H"
#include "WxMachineRegisterScript.H"
#include "WxMachineRegister.H"


IMPLEMENT_APP(WxMachineRegisterApp)

WxMachineRegisterApp::WxMachineRegisterApp()
    : ewxApp(), WxJMSMessageDispatch(MACHREGISTER, true)
{
    p_rgstrFrame = NULL;
}


bool WxMachineRegisterApp::OnInit()
{
    ewxApp::OnInit();

    bool admin = ((this->argc > 1) && (strcmp(this->argv[1], "-admin") == 0));

    if (!admin && this->argc>2 && strcmp(this->argv[1],"-pipe")==0) {
      AuthCache::pipeClean(argv[2].ToStdString());
    }

    p_rgstrFrame = new WxMachineRegister(NULL, admin);

    p_rgstrFrame->Show();
    SetTopWindow(p_rgstrFrame);

    //  Test hook (see WxMachineRegisterScript.H): drive the real frame
    //  without a broker.
    const char* script = getenv("ECCE_MACHREG_SCRIPT");
    if (script != NULL)
    {
        fprintf(stderr, "[MACHREG] binary=%s ECCE_HOME=%s admin=%d\n",
                (const char*)wxString(this->argv[0]).c_str(),
                Ecce::ecceHome(), (int)admin);
        MachRegScript* run = new MachRegScript(p_rgstrFrame, script);
        run->start();
        return true;
    }

    registerTopShell(p_rgstrFrame);

    subscribeMessages();
    startSubscriber();
    notifyReady();

    return true;
}


int WxMachineRegisterApp::OnExit()
{
    //  Unsubscribe from Messaging Service
    unsubscribe();

    return 0;
}


void WxMachineRegisterApp::subscribeMessages()
{
    subscribe("ecce_invoke", (wxJmsCBFunc)&WxMachineRegisterApp::invokeMCB);
    subscribe("ecce_quit", (wxJmsCBFunc)&WxMachineRegisterApp::quitMCB);
    subscribe("ecce_identify", (wxJmsCBFunc)&WxMachineRegisterApp::identifyMCB);
    subscribe("ecce_auth_changed", (wxJmsCBFunc)&WxMachineRegisterApp::authMCB);
}


string WxMachineRegisterApp::identify()
{
    return getName();
}


string WxMachineRegisterApp::getName()
{
    return MACHREGISTER; // "Machine Registration";
}


void WxMachineRegisterApp::invokeMCB(JMSMessage& msg)
{
    string refname = msg.getProperty("initmachine");

    if (!refname.empty())
    {
        p_rgstrFrame->selectMachine(refname);
    }

    string page = msg.getProperty("initpage");
    if (!page.empty())
        p_rgstrFrame->showPage(page);

    p_rgstrFrame->Iconize(false);
    p_rgstrFrame->Raise();
    p_rgstrFrame->Show(true);
    p_rgstrFrame->SetFocus();
    p_rgstrFrame->RequestUserAttention();
}


void WxMachineRegisterApp::quitMCB(JMSMessage& msg)
{
    if (msg.getSender().getName() == GATEWAY)
    {
       p_rgstrFrame->Close(true);
       this->OnExit();
    }
}


void WxMachineRegisterApp::identifyMCB(JMSMessage&)
{
    string value = identify();

    JMSMessage *msg = newMessage();
    msg->addProperty("info", value);
    publish("ecce_identify_reply", *msg);

    delete msg;
 }

void WxMachineRegisterApp::authMCB(JMSMessage& msg)
{
    AuthCache::getCache().msgIn(msg, getMyID());
}

