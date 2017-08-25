
#include "util/EcceURL.H"

#include "tdat/AuthCache.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"

#include "SolvateEdApp.H"
#include "SolvateEd.H"

IMPLEMENT_APP(SolvateEdApp)


SolvateEdApp::SolvateEdApp()
  : ewxApp(),
    WxJMSMessageDispatch(SOLVATE, true),
    p_solvate(NULL)
{

}


SolvateEdApp::~SolvateEdApp()
{

}


bool SolvateEdApp::OnInit()
{
  ewxApp::OnInit();

  if (argc>2 && strcmp(argv[1],"-pipe")==0) {
    AuthCache::getCache().pipeIn(argv[2]);
  }

  p_solvate = new SolvateEd(NULL);

  string context("");

  if ((this->argc > 1) && (strcmp(this->argv[1], "-context") == 0))
      context = this->argv[2];

  if (!context.empty())
      p_solvate->setContext(context);

  // Show the main window
  p_solvate->Show();
  SetTopWindow(p_solvate);
  registerTopShell(p_solvate);

  //  Subscribe to Messages
  subscribeMessages();
  startSubscriber();
  notifyReady();

  return true;
}


int SolvateEdApp::OnExit()
{
  //  Unsubscribe from Messaging Service
  unsubscribe();

  return 0;
}


string SolvateEdApp::getName() const
{
  return SOLVATE;
}



void SolvateEdApp::subscribeMessages()
{
  //  Desktop Messages
  subscribe("ecce_invoke", (wxJmsCBFunc)&SolvateEdApp::invokeMCB);
  subscribe("ecce_quit", (wxJmsCBFunc)&SolvateEdApp::quitMCB);
  subscribe("ecce_identify", (wxJmsCBFunc)&SolvateEdApp::identifyMCB);

  // Calc/Project Changes
  subscribe("ecce_url_renamed",(wxJmsCBFunc)&SolvateEdApp::calcMoveMCB);
  subscribe("ecce_url_removed",(wxJmsCBFunc)&SolvateEdApp::calcDeleteMCB);
  subscribe("ecce_url_state",(wxJmsCBFunc)&SolvateEdApp::calcStateMCB, false);
  subscribe("ecce_url_subject",(wxJmsCBFunc)&SolvateEdApp::calcSubjectMCB);

  subscribe("ecce_auth_changed",
            (wxJmsCBFunc)&SolvateEdApp::msgAuthMCB);
}



void SolvateEdApp::invokeMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("calcurl");
  if (!urlstr.empty()) {
    p_solvate->setContext(urlstr);
    setPollContext(urlstr);

    p_solvate->Iconize(false);
    p_solvate->Raise();
    p_solvate->Show(true);
    p_solvate->SetFocus();
    p_solvate->RequestUserAttention();
  }
}


void SolvateEdApp::quitMCB(JMSMessage& msg)
{
  if (msg.getSender().getName() == GATEWAY) {
    // Force the window to close; disallows veto
    p_solvate->Close(true);
    this->OnExit();
  }
}


void SolvateEdApp::identifyMCB(JMSMessage& msg)
{
  JMSMessage *reply = newMessage();
  reply->addProperty("info", this->identify());
  publish("ecce_identify_reply", *reply);
  delete reply;
}


void SolvateEdApp::calcMoveMCB(JMSMessage& msg)
{
  string oldurl = msg.getProperty("oldurl");
  string newurl = msg.getProperty("newurl");
  if (EcceURL(p_solvate->getContext()).isChildOrMe(oldurl)) {
    EDSIFactory::renamePoolResource(oldurl, newurl);

    wxCommandEvent event;
    p_solvate->moveMCB(event);
  }
}


void SolvateEdApp::calcDeleteMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_solvate->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::removePoolResource(urlstr);

    wxCommandEvent event;
    p_solvate->deleteMCB(event);
  }
}


void SolvateEdApp::calcStateMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (p_solvate->getContext() == urlstr) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    event.SetString(msg.getProperty("state"));
    p_solvate->stateMCB(event);
  }
}


void SolvateEdApp::calcSubjectMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_solvate->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_solvate->subjectMCB(event);
  }
}


void SolvateEdApp::msgAuthMCB(JMSMessage& msg)
{
  AuthCache::getCache().msgIn(msg, getMyID());
}


string SolvateEdApp::identify()
{
  EcceURL url = p_solvate->getContext();
  string identity = "Solvation Editor";
  string path = EDSIServerCentral().mapURLtoName(url);

  if (!path.empty())
    identity += ": " + path;

  return identity;
}
 
