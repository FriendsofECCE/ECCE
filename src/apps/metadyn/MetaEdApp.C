
#include "util/EcceURL.H"

#include "tdat/AuthCache.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ResourceDescriptor.H"

#include "MetaEdApp.H"
#include "MetaEd.H"

IMPLEMENT_APP(MetaEdApp)


MetaEdApp::MetaEdApp()
  : ewxApp(),
    WxJMSMessageDispatch(METADYNAMICS, true),
    p_metaed(NULL)
{
}


MetaEdApp::~MetaEdApp()
{
}


bool MetaEdApp::OnInit()
{
  ewxApp::OnInit();

  if (argc>2 && strcmp(argv[1],"-pipe")==0) {
    AuthCache::getCache().pipeIn(argv[2]);
  }

  p_metaed = new MetaEd(NULL);

  string context("");

  if ((this->argc > 1) && (strcmp(this->argv[1], "-context") == 0))
      context = this->argv[2];

  if (!context.empty())
      p_metaed->setContext(context);

  // Show the main window
  p_metaed->Show();
  SetTopWindow(p_metaed);
  registerTopShell(p_metaed);

  //  Subscribe to Messages
  subscribeMessages();
  startSubscriber();
  notifyReady();

  return true;
}


int MetaEdApp::OnExit()
{
  //  Unsubscribe from Messaging Service
  unsubscribe();

  return 0;
}


string MetaEdApp::getName() const
{
  return METADYNAMICS;
}



void MetaEdApp::subscribeMessages()
{
  //  Desktop Messages
  subscribe("ecce_invoke", (wxJmsCBFunc)&MetaEdApp::invokeMCB);
  subscribe("ecce_quit", (wxJmsCBFunc)&MetaEdApp::quitMCB);
  subscribe("ecce_identify", (wxJmsCBFunc)&MetaEdApp::identifyMCB);

  // handle return message from a launcher request to start another tool
  subscribe("ecce_invoke_status",
            (wxJmsCBFunc)&MetaEdApp::invokeStatusMCB, false);

  // Global Preference Messages
  subscribe("ecce_preferences_states",
            (wxJmsCBFunc)&MetaEdApp::updatePrefsMCB);
  subscribe("ecce_preferences_misc",
            (wxJmsCBFunc)&MetaEdApp::updatePrefsMCB);
  subscribe("ecce_preferences_pertab",
            (wxJmsCBFunc)&MetaEdApp::updatePerTabMCB);

  // Calc/Project Changes
  subscribe("ecce_url_renamed",(wxJmsCBFunc)&MetaEdApp::calcMoveMCB);
  subscribe("ecce_url_removed",(wxJmsCBFunc)&MetaEdApp::calcDeleteMCB);
  subscribe("ecce_url_launch",(wxJmsCBFunc)&MetaEdApp::calcLaunchMCB);
  subscribe("ecce_url_state",(wxJmsCBFunc)&MetaEdApp::calcStateMCB, false);
  subscribe("ecce_url_subject",(wxJmsCBFunc)&MetaEdApp::calcSubjectMCB);
  subscribe("ecce_url_property",(wxJmsCBFunc)&MetaEdApp::calcPropertyMCB);

  subscribe("ecce_auth_changed",
            (wxJmsCBFunc)&MetaEdApp::msgAuthMCB);
}



void MetaEdApp::invokeMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("calcurl");
  if (!urlstr.empty()) {
    string action = msg.getProperty("actiontopic");

    p_metaed->setContext(urlstr);
    setPollContext(urlstr);

    if (action == "noop") {
      // do nothing
    } else {
      // error message
    }

    p_metaed->Iconize(false);
    p_metaed->Raise();
    p_metaed->Show(true);
    p_metaed->SetFocus();
    p_metaed->RequestUserAttention();
  }
}


void MetaEdApp::quitMCB(JMSMessage& msg)
{
  if (msg.getSender().getName() == GATEWAY) {
    // Force the window to close; disallows veto
    p_metaed->Close(true);
    this->OnExit();
  }
}


void MetaEdApp::identifyMCB(JMSMessage& msg)
{
  JMSMessage *reply = newMessage();
  reply->addProperty("info", this->identify());
  publish("ecce_identify_reply", *reply);
  delete reply;
}


void MetaEdApp::invokeStatusMCB(JMSMessage& msg)
{
}


void MetaEdApp::calcMoveMCB(JMSMessage& msg)
{
  string oldurl = msg.getProperty("oldurl");
  string newurl = msg.getProperty("newurl");
  if (EcceURL(p_metaed->getContext()).isChildOrMe(oldurl)) {
    EDSIFactory::renamePoolResource(oldurl, newurl);

    wxCommandEvent event;
    p_metaed->moveMCB(event);
  }
}


void MetaEdApp::calcDeleteMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_metaed->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::removePoolResource(urlstr);

    wxCommandEvent event;
    p_metaed->deleteMCB(event);
  }
}


void MetaEdApp::calcLaunchMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_metaed->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_metaed->launchMCB(event);
  }
}


void MetaEdApp::calcStateMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (p_metaed->getContext() == urlstr) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    event.SetString(msg.getProperty("state"));
    p_metaed->stateMCB(event);
  }
}


void MetaEdApp::calcSubjectMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_metaed->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    wxCommandEvent event;
    p_metaed->subjectMCB(event);
  }
}


void MetaEdApp::calcPropertyMCB(JMSMessage& msg)
{
  // This could be an annotation change which we care about when
  // generating the input file
  string urlstr = msg.getProperty("url");
  if (EcceURL(p_metaed->getContext()).isChildOrMe(urlstr)) {
    EDSIFactory::changePoolResource(urlstr);

    string name = msg.getProperty("name");
    if (name.find("reviewed") == string::npos) {
      wxCommandEvent event;
      p_metaed->propertyMCB(event);
    }
  }
}


void MetaEdApp::updatePrefsMCB(JMSMessage& msg)
{
}


void MetaEdApp::updatePerTabMCB(JMSMessage& msg)
{
}



void MetaEdApp::msgAuthMCB(JMSMessage& msg)
{
  AuthCache::getCache().msgIn(msg, getMyID());
}



string MetaEdApp::identify()
{
  EcceURL url = p_metaed->getContext();
  string path = EDSIServerCentral().mapURLtoName(url);

  string identity = "Metadynamics Editor";
  if (!path.empty())
    identity += ": " + path;

  return identity;
}

