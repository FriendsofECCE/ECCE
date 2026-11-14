/**
 * @file
 *
 * The Calculation Editor window's messages, on a dispatcher of the window
 * instead of the wxApp, so that tool windows can share one process (#983).
 */
#include "wx/timer.h"
#include "wx/app.h"

#include "util/EcceURL.H"

#include "tdat/AuthCache.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxWindowUtils.H"
#include "CalcEdMessages.H"
#include "CalcEd.H"


CalcEdMessages::CalcEdMessages(CalcEd *calced)
  : WxJMSMessageDispatch(CALCED, true),
    p_calced(calced),
    p_shown(false)
{
}


void CalcEdMessages::subscribeMessages()
{
  //  Desktop Messages
  subscribe("ecce_invoke", (wxJmsCBFunc)&CalcEdMessages::invokeMCB);
  subscribe("ecce_quit", (wxJmsCBFunc)&CalcEdMessages::quitMCB);
  subscribe("ecce_identify", (wxJmsCBFunc)&CalcEdMessages::identifyMCB);

  // handle return message from a launcher request to start another tool
  subscribe("ecce_invoke_status",
            (wxJmsCBFunc)&CalcEdMessages::invokeStatusMCB, false);

  // Global Preference Messages
  subscribe("ecce_preferences_states",
            (wxJmsCBFunc)&CalcEdMessages::updatePrefsMCB);
  subscribe("ecce_preferences_misc",
            (wxJmsCBFunc)&CalcEdMessages::updatePrefsMCB);
  subscribe("ecce_preferences_pertab",
            (wxJmsCBFunc)&CalcEdMessages::updatePerTabMCB);

  // Calc/Project Changes
  subscribe("ecce_url_renamed",(wxJmsCBFunc)&CalcEdMessages::calcMoveMCB);
  subscribe("ecce_url_removed",(wxJmsCBFunc)&CalcEdMessages::calcDeleteMCB);
  subscribe("ecce_url_basis",(wxJmsCBFunc)&CalcEdMessages::calcBasisMCB);
  subscribe("ecce_url_code",(wxJmsCBFunc)&CalcEdMessages::calcCodeMCB);
  subscribe("ecce_url_details",(wxJmsCBFunc)&CalcEdMessages::calcDetailsMCB);
  subscribe("ecce_url_launch",(wxJmsCBFunc)&CalcEdMessages::calcLaunchMCB);
  subscribe("ecce_url_runtype",(wxJmsCBFunc)&CalcEdMessages::calcRuntypeMCB);
  subscribe("ecce_url_spin",(wxJmsCBFunc)&CalcEdMessages::calcSpinMCB);
  subscribe("ecce_url_state",(wxJmsCBFunc)&CalcEdMessages::calcStateMCB, false);
  subscribe("ecce_url_subject",(wxJmsCBFunc)&CalcEdMessages::calcSubjectMCB);
  subscribe("ecce_url_theory",(wxJmsCBFunc)&CalcEdMessages::calcTheoryMCB);
  subscribe("ecce_url_property",(wxJmsCBFunc)&CalcEdMessages::calcPropertyMCB);

  subscribe("ecce_auth_changed",
            (wxJmsCBFunc)&CalcEdMessages::msgAuthMCB);
}



void CalcEdMessages::invokeMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("calcurl");
  if (!urlstr.empty()) {
    string action = msg.getProperty("actiontopic");
    string codeName = msg.getProperty("codename");

    p_calced->setContext(urlstr, codeName);
    setPollContext(urlstr);

    if (action == "noop") {
      // do nothing
    } else {
      // error message
    }

    p_calced->Iconize(false);
    showEditor();
    ewxRaiseWindow(p_calced);
    p_calced->RequestUserAttention();
  } else {
    showEditor();
  }
}


//  The first Show centres the window at the size its content was fitted to.
//  It is registered as a top shell only then: the dispatcher Show()s every
//  top shell before it hands ecce_invoke to invokeMCB.
void CalcEdMessages::showEditor()
{
  if (!p_shown) {
    p_shown = true;
    p_calced->Centre();
    registerTopShell(p_calced);
  }
  p_calced->Show(true);
}


void CalcEdMessages::quitMCB(JMSMessage& msg)
{
  if (msg.getSender().getName() == GATEWAY) {
    // In one process the Organizer may have closed this window already.
    if (wxTheApp->IsScheduledForDestruction(p_calced)) return;
    // Force the window to close; disallows veto
    p_calced->Close(true);
    unsubscribe();
  }
}


void CalcEdMessages::identifyMCB(JMSMessage& msg)
{
  JMSMessage *reply = newMessage();
  reply->addProperty("info", this->identify());
  publish("ecce_identify_reply", *reply);
  delete reply;
}


void CalcEdMessages::invokeStatusMCB(JMSMessage& msg)
{
  
}


void CalcEdMessages::calcMoveMCB(JMSMessage& msg)
{
  string oldurl = msg.getProperty("oldurl");
  string newurl = msg.getProperty("newurl");
  if (EcceURL(p_calced->getContext()).isChildOrMe(oldurl)) {
    EDSIFactory::renamePoolResource(oldurl, newurl);

    wxCommandEvent event;
    p_calced->moveMCB(event);
  }
}


void CalcEdMessages::calcDeleteMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::removePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->deleteMCB(event);
  }
}


void CalcEdMessages::calcBasisMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->basisMCB(event);
  }
}


void CalcEdMessages::calcCodeMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->codeMCB(event);
  }
}


void CalcEdMessages::calcDetailsMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->detailsMCB(event);
  }
}


void CalcEdMessages::calcLaunchMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->launchMCB(event);
  }
}


void CalcEdMessages::calcRuntypeMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->runtypeMCB(event);
  }
}


void CalcEdMessages::calcSpinMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->spinMCB(event);
  }
}


void CalcEdMessages::calcStateMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (p_calced->getContext() == urlstr) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    event.SetString(msg.getProperty("state"));
    p_calced->stateMCB(event);
  }
}


void CalcEdMessages::calcSubjectMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->subjectMCB(event);
  }
}


void CalcEdMessages::calcTheoryMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->theoryMCB(event);
  }
}


void CalcEdMessages::calcPropertyMCB(JMSMessage& msg)
{
  // This could be an annotation change which we care about when
  // generating the input file
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    string name = msg.getProperty("name");
    if (name.find("reviewed") == string::npos) {
      wxCommandEvent event;
      p_calced->propertyMCB(event);
    }
  }
}


void CalcEdMessages::updatePrefsMCB(JMSMessage& msg)
{
}


void CalcEdMessages::updatePerTabMCB(JMSMessage& msg)
{
}



void CalcEdMessages::msgAuthMCB(JMSMessage& msg)
{
  AuthCache::getCache().msgIn(msg, getMyID());
}



string CalcEdMessages::identify()
{
  EcceURL url = p_calced->getContext();
  string identity = p_calced->getCodeName() + " Editor";
  string path = EDSIServerCentral().mapURLtoName(url);

  if (!path.empty())
    identity += ": " + path;

  return identity;
}

