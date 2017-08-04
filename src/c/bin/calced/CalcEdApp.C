
#include "util/EcceURL.H"

#include "tdat/AuthCache.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ResourceDescriptor.H"

#include "CalcEdApp.H"
#include "CalcEd.H"

IMPLEMENT_APP(CalcEdApp)


CalcEdApp::CalcEdApp()
  : ewxApp(),
    WxJMSMessageDispatch(CALCED, true),
    p_calced(NULL)
{

}


CalcEdApp::~CalcEdApp()
{

}


bool CalcEdApp::OnInit()
{
  ewxApp::OnInit();

  if (argc>2 && strcmp(argv[1],"-pipe")==0) {
    AuthCache::getCache().pipeIn(argv[2]);
  }

  p_calced = new CalcEd(NULL);

  string context("");

  if ((this->argc > 1) && (strcmp(this->argv[1], "-context") == 0))
      context = this->argv[2];

  if (!context.empty())
      p_calced->setContext(context);

  // Show the main window
  p_calced->Show();
  SetTopWindow(p_calced);
  registerTopShell(p_calced);

  //  Subscribe to Messages
  subscribeMessages();
  startSubscriber();
  notifyReady();

  return true;
}


int CalcEdApp::OnExit()
{
  //  Unsubscribe from Messaging Service
  unsubscribe();

  return 0;
}


string CalcEdApp::getName() const
{
  return CALCED;
}



void CalcEdApp::subscribeMessages()
{
  //  Desktop Messages
  subscribe("ecce_invoke", (wxJmsCBFunc)&CalcEdApp::invokeMCB);
  subscribe("ecce_quit", (wxJmsCBFunc)&CalcEdApp::quitMCB);
  subscribe("ecce_identify", (wxJmsCBFunc)&CalcEdApp::identifyMCB);

  // handle return message from a launcher request to start another tool
  subscribe("ecce_invoke_status",
            (wxJmsCBFunc)&CalcEdApp::invokeStatusMCB, false);

  // Global Preference Messages
  subscribe("ecce_preferences_states",
            (wxJmsCBFunc)&CalcEdApp::updatePrefsMCB);
  subscribe("ecce_preferences_misc",
            (wxJmsCBFunc)&CalcEdApp::updatePrefsMCB);
  subscribe("ecce_preferences_pertab",
            (wxJmsCBFunc)&CalcEdApp::updatePerTabMCB);

  // Calc/Project Changes
  subscribe("ecce_url_renamed",(wxJmsCBFunc)&CalcEdApp::calcMoveMCB);
  subscribe("ecce_url_removed",(wxJmsCBFunc)&CalcEdApp::calcDeleteMCB);
  subscribe("ecce_url_basis",(wxJmsCBFunc)&CalcEdApp::calcBasisMCB);
  subscribe("ecce_url_code",(wxJmsCBFunc)&CalcEdApp::calcCodeMCB);
  subscribe("ecce_url_details",(wxJmsCBFunc)&CalcEdApp::calcDetailsMCB);
  subscribe("ecce_url_launch",(wxJmsCBFunc)&CalcEdApp::calcLaunchMCB);
  subscribe("ecce_url_runtype",(wxJmsCBFunc)&CalcEdApp::calcRuntypeMCB);
  subscribe("ecce_url_spin",(wxJmsCBFunc)&CalcEdApp::calcSpinMCB);
  subscribe("ecce_url_state",(wxJmsCBFunc)&CalcEdApp::calcStateMCB, false);
  subscribe("ecce_url_subject",(wxJmsCBFunc)&CalcEdApp::calcSubjectMCB);
  subscribe("ecce_url_theory",(wxJmsCBFunc)&CalcEdApp::calcTheoryMCB);
  subscribe("ecce_url_property",(wxJmsCBFunc)&CalcEdApp::calcPropertyMCB);

  subscribe("ecce_auth_changed",
            (wxJmsCBFunc)&CalcEdApp::msgAuthMCB);
}



void CalcEdApp::invokeMCB(JMSMessage& msg)
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
    p_calced->Raise();
    p_calced->Show(true);
    p_calced->SetFocus();
    p_calced->RequestUserAttention();
  }
}


void CalcEdApp::quitMCB(JMSMessage& msg)
{
  if (msg.getSender().getName() == GATEWAY) {
    // Force the window to close; disallows veto
    p_calced->Close(true);
    this->OnExit();
  }
}


void CalcEdApp::identifyMCB(JMSMessage& msg)
{
  JMSMessage *reply = newMessage();
  reply->addProperty("info", this->identify());
  publish("ecce_identify_reply", *reply);
  delete reply;
}


void CalcEdApp::invokeStatusMCB(JMSMessage& msg)
{
  
}


void CalcEdApp::calcMoveMCB(JMSMessage& msg)
{
  string oldurl = msg.getProperty("oldurl");
  string newurl = msg.getProperty("newurl");
  if (EcceURL(p_calced->getContext()).isChildOrMe(oldurl)) {
    EDSIFactory::renamePoolResource(oldurl, newurl);

    wxCommandEvent event;
    p_calced->moveMCB(event);
  }
}


void CalcEdApp::calcDeleteMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::removePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->deleteMCB(event);
  }
}


void CalcEdApp::calcBasisMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->basisMCB(event);
  }
}


void CalcEdApp::calcCodeMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->codeMCB(event);
  }
}


void CalcEdApp::calcDetailsMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->detailsMCB(event);
  }
}


void CalcEdApp::calcLaunchMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->launchMCB(event);
  }
}


void CalcEdApp::calcRuntypeMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->runtypeMCB(event);
  }
}


void CalcEdApp::calcSpinMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->spinMCB(event);
  }
}


void CalcEdApp::calcStateMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (p_calced->getContext() == urlstr) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    event.SetString(msg.getProperty("state"));
    p_calced->stateMCB(event);
  }
}


void CalcEdApp::calcSubjectMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->subjectMCB(event);
  }
}


void CalcEdApp::calcTheoryMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_calced->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_calced->theoryMCB(event);
  }
}


void CalcEdApp::calcPropertyMCB(JMSMessage& msg)
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


void CalcEdApp::updatePrefsMCB(JMSMessage& msg)
{
}


void CalcEdApp::updatePerTabMCB(JMSMessage& msg)
{
}



void CalcEdApp::msgAuthMCB(JMSMessage& msg)
{
  AuthCache::getCache().msgIn(msg, getMyID());
}



string CalcEdApp::identify()
{
  EcceURL url = p_calced->getContext();
  string identity = p_calced->getCodeName() + " Editor";
  string path = EDSIServerCentral().mapURLtoName(url);

  if (!path.empty())
    identity += ": " + path;

  return identity;
}

