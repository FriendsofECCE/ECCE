/**
 * @file
 * @author Lisong Sun
 *
 * Calculation Manager Main Application Class
 * 
 *
 */

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "CalcMgrApp.H"
#endif

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

//#include <iostream>
//using std::cerr;

#include "tdat/RefMachine.H"

#include "dsm/MachineConfig.H"
#include "dsm/EDSIFactory.H"

#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/WxJMSSubscriber.H"
#include "wxgui/WxState.H"

#include "CalcMgrApp.H"
#include "CalcMgr.H"


IMPLEMENT_APP(CalcMgrApp)

IMPLEMENT_CLASS(CalcMgrApp, ewxApp)

/**
 * Default constructor
 *
 * Subclassed from WxJMSMessageDispatch
 */
CalcMgrApp::CalcMgrApp()
  : ewxApp(), WxJMSMessageDispatch(ORGANIZER.c_str())
{
  p_calcMgr = NULL;
}

string CalcMgrApp::getName() const
{
   return "Organizer";
}


/**
 * Initialize CalcMgr application
 *
 * Generate a new CalcMgr object and show it.
 * Subscribe to the misc message event.
 */
bool CalcMgrApp::OnInit()
{
  ewxApp::OnInit();

  if (argc>2 && strcmp(argv[1],"-pipe")==0) {
    AuthCache::getCache().pipeIn(argv[2]);
  }

  p_calcMgr = new CalcMgr(NULL);
  p_calcMgr->Show(true);
  SetTopWindow(p_calcMgr);
  registerTopShell(p_calcMgr);

  // Desk Top Messages
  subscribe("ecce_quit", (wxJmsCBFunc)&CalcMgrApp::quitMCB);
  subscribe("ecce_identify", (wxJmsCBFunc)&CalcMgrApp::identifyMCB);
  
  // Preference Messages
  subscribe("ecce_preferences_states", (wxJmsCBFunc)&CalcMgrApp::statePrefChangeMCB);
  subscribe("ecce_preferences_misc", (wxJmsCBFunc)&CalcMgrApp::miscPrefChangeMCB);

  //  startup status Messages
  subscribe("ecce_invoke", (wxJmsCBFunc)&CalcMgrApp::invokeMCB);
  subscribe("ecce_invoke_status", (wxJmsCBFunc)&CalcMgrApp::msgStatusMCB,false);

  // New ecce 2.0 msgs
  subscribe("ecce_auth_changed", (wxJmsCBFunc)&CalcMgrApp::msgAuthMCB);
  subscribe("ecce_url_renamed", (wxJmsCBFunc)&CalcMgrApp::msgRenameMCB);
  subscribe("ecce_url_created", (wxJmsCBFunc)&CalcMgrApp::msgURLCreatedMCB);
  subscribe("ecce_url_removed", (wxJmsCBFunc)&CalcMgrApp::msgURLRemovedMCB);
  subscribe("ecce_url_state", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB, false);
  subscribe("ecce_url_property", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_subject", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_code", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_spin", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_theory", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_runtype", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_details", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_basis", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_url_launch", (wxJmsCBFunc)&CalcMgrApp::msgChangedMCB);
  subscribe("ecce_machconf_changed", (wxJmsCBFunc)&CalcMgrApp::msgMachConfChangedMCB);
  subscribe("ecce_machreg_changed", (wxJmsCBFunc)&CalcMgrApp::msgMachRegChangedMCB);
  subscribe("ecce_data_server", (wxJmsCBFunc)&CalcMgrApp::msgDataServerMCB,false);


  // Now tell JMSDispatcher that we are ready to start the subscriber
  // connection for these subscriptions 
  // (must happen AFTER all subscribe calls):
  startSubscriber();
  notifyReady();

  return true;
}


int CalcMgrApp::OnExit()
{
  //  cerr << "Enter CalcMgrApp::OnExit()\n";
  return 0;
}


void CalcMgrApp::quitMCB(JMSMessage& msg)
{
  if (msg.getSender().getName() == GATEWAY) {
    GetTopWindow()->Destroy();
  }
}


void CalcMgrApp::identifyMCB(JMSMessage& msg)
{
  JMSMessage* reply = newMessage();
  string currentNode = p_calcMgr->getTreeSelectionName();
  reply->addProperty("info", "Organizer: " + currentNode);
  publish("ecce_identify_reply", *reply);
  delete reply;
}


void CalcMgrApp::statePrefChangeMCB(JMSMessage& msg)
{
  WxState::resetToUserDefault();
  p_calcMgr->recolorStatePixmaps();
  p_calcMgr->Refresh();
}


void CalcMgrApp::miscPrefChangeMCB(JMSMessage& msg)
{
  p_calcMgr->getFeedback()->readPreferences();
  p_calcMgr->setContextPanel();
}


void CalcMgrApp::invokeMCB(JMSMessage& msg)
{
  p_calcMgr->Iconize(false);
  p_calcMgr->Raise();
  p_calcMgr->Show(true);
  p_calcMgr->SetFocus();
  p_calcMgr->RequestUserAttention();
}


void CalcMgrApp::msgStatusMCB(JMSMessage& msg)
{
  static const char *errMsg =
    "Unable to start the application.  "
    "Please use the ECCE Support tool to report this problem.";

  //  p_cursor->end_activity();
  string status = msg.getProperty("status");
  if (status == "failed") {
    p_calcMgr->getFeedback()->setMessage(errMsg, WxFeedback::SYSTEM);
  }
}


void CalcMgrApp::msgAuthMCB(JMSMessage& msg)
{
  AuthCache::getCache().msgIn(msg, getMyID());
}


void CalcMgrApp::msgRenameMCB(JMSMessage& msg)
{
  EcceURL oldurl(msg.getProperty("oldurl"));
  EcceURL newurl(msg.getProperty("newurl"));
  p_calcMgr->updateName(oldurl, newurl);
}


void CalcMgrApp::msgURLCreatedMCB(JMSMessage& msg)
{
  EcceURL url(msg.getProperty("url"));
  p_calcMgr->updateAddNode(url);
}


void CalcMgrApp::msgURLRemovedMCB(JMSMessage& msg)
{
  EcceURL url(msg.getProperty("url"));
  p_calcMgr->updateRemoveNode(url);
}


void CalcMgrApp::msgChangedMCB(JMSMessage& msg)
{
  EcceURL url(msg.getProperty("url"));
  p_calcMgr->updateUrl(url);
}


void CalcMgrApp::msgMachConfChangedMCB(JMSMessage& msg)
{
  MachineConfig::markUpdateUserPrefs();
}


void CalcMgrApp::msgMachRegChangedMCB(JMSMessage& msg)
{
  RefMachine::markUpdateUserPrefs();
}


void CalcMgrApp::msgDataServerMCB(JMSMessage& msg)
{
  string name = msg.getProperty("name");
  string urlstr = msg.getProperty("url");
  p_calcMgr->addRoot(name, urlstr);
}
