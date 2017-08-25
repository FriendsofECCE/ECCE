/**
 * @file
 *
 * Periodic table application main class.
 * 
 *
 */

#include <iostream>
using std::ends;
#include <strstream>
using std::ostrstream;


#include "util/EcceException.H"
#include "util/JMSMessage.H"

#include "tdat/AuthCache.H"

#include "dsm/ResourceDescriptor.H"

#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/WxJMSSubscriber.H"

#include "PerTabApp.H"



IMPLEMENT_APP(PerTabApp)

/**
 * Default constructor
 *
 * Subclassed from WxJMSMessageDispatch
 */
PerTabApp::PerTabApp() : WxAppJMSMessageDispatch(PERTAB.c_str())
{

  p_perTabFrame = NULL;
}


/**
 * Initialize periodic table application
 *
 * Generate a new PerTabFrame object and show it.
 * Subscribe to the following message event:
 *   ecce_invoke
 *   ecce_quit
 *   ecce_preferences_pertab
 *   ecce_identify
 */
bool PerTabApp::OnInit()
{
  ewxApp::OnInit();

  if (argc>2 && strcmp(argv[1],"-pipe")==0) {
    AuthCache::pipeClean(argv[2]);
  }

  p_perTabFrame = new PerTabFrame(NULL);
  p_perTabFrame->Show(true);
  SetTopWindow(p_perTabFrame);
  registerTopShell(p_perTabFrame);

  subscribeMessages();
  subscribe("ecce_preferences_pertab", (wxJmsCBFunc)&PerTabApp::preferenceMCB);
  subscribe("ecce_auth_changed", (wxJmsCBFunc)&PerTabApp::authMCB);

  startSubscriber();
  notifyReady();

  return true;
}


/**
 * Preference changed message call back function
 */
void PerTabApp::preferenceMCB(JMSMessage&) 
{
  p_perTabFrame->resetUserPreferences();
}


/**
 * Authorization cache needs to be updated
 */
void PerTabApp::authMCB(JMSMessage& msg)
{
  AuthCache::getCache().msgIn(msg, getMyID());
}


/**
 * Identify message call back function
 *
 * When gateway sent out identify message, periodic table app will response by sending a ID message back.
 */
string PerTabApp::identify() 
{
  ostrstream os;
  os << "Periodic Table";
  vector<string> atoms = p_perTabFrame->selectedAtomSymbols();
  int card = atoms.size();
  for (int idx=0; idx<card; idx++)
    os << ": " << atoms[idx];
  os << ends;

  return os.str();
}
