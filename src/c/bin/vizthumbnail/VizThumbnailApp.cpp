/**
 * @file
 *
 */

#include <iostream>
using std::cerr;
using std::endl;

// Remove some of these three once the calls are in SoWx::init
#include "inv/nodes/SoPerspectiveCamera.hpp"
#include "inv/nodes/SoMaterial.hpp"
#include "inv/nodes/SoSeparator.hpp"
#include "inv/nodes/SoDirectionalLight.hpp"
#include "inv/nodes/SoNode.hpp"
#include "inv/nodes/SoCone.hpp"
#include "inv/nodes/SoRotationXYZ.hpp"
#include "inv/manips/SoTrackballManip.hpp"

#include "util/StringConverter.hpp"

#include "dsm/EDSIFactory.hpp"

#include "inv/ChemKit/ChemInit.hpp"

#include "viz/AtomNodesInit.hpp"
#include "viz/NodesInit.hpp"
#include "viz/SGContainer.hpp"
#include "viz/SGFragment.hpp"

#include "inv/SoWx/SoWx.hpp"

#include "wxviz/SGSelection.hpp"
#include "wxviz/VizRender.hpp"

#include "VizThumbnailApp.hpp"

IMPLEMENT_APP(VizThumbnailApp)

/**
 * Default constructor
 *
 * Subclassed from WxJMSMessageDispatch
 */
VizThumbnailApp::VizThumbnailApp()
  : WxJMSMessageDispatch(VIZTHUMBNAIL, true)
{
}


/**
 * Initialize 
 */
bool VizThumbnailApp::OnInit()
{
  ewxApp::OnInit();

  if (argc>2 && strcmp(argv[1],"-pipe")==0) {
    AuthCache::getCache().pipeIn(argv[2]);
  }

  EDSIFactory::addAuthEventListener(this);

  // all wxApps require at least one window... yuck
  // the SoWx::init function also requires a window, oh well
  wxFrame *mainWin = new wxFrame(NULL, wxID_ANY, "Viz Thumbnail");
  SetTopWindow(mainWin);

  // Initialize inventor classes
  // These temporarily copied from SoXt::init 
  // since we don't yet have a SoWx::init
  SoWx::init(GetTopWindow());
  ChemInit::initClasses();
  SGSelection::initClass();
  NodesInit::initClasses();
  AtomNodesInit::initClasses();
  SGFragment::initClass();
  SGContainer::initClass();

  //  Subscribe to Messages
  subscribeMessages();
  startSubscriber();
  notifyReady();

  return true;
}


int VizThumbnailApp::OnExit()
{
  //  Unsubscribe from Messaging Service
  unsubscribe();

  return 0;
}


void VizThumbnailApp::subscribeMessages()
{
  //  Desktop Messages
  subscribe("ecce_invoke",
            (wxJmsCBFunc)&VizThumbnailApp::invokeMCB);
  subscribe("ecce_quit",
            (wxJmsCBFunc)&VizThumbnailApp::quitMCB);
  subscribe("ecce_identify",
            (wxJmsCBFunc)&VizThumbnailApp::identifyMCB);

  // handle return message from a launcher request to start another tool
  subscribe("ecce_invoke_status",
            (wxJmsCBFunc)&VizThumbnailApp::invokeStatusMCB,
            false);

  subscribe("ecce_auth_changed",
            (wxJmsCBFunc)&VizThumbnailApp::msgAuthMCB);
}


void VizThumbnailApp::invokeMCB(JMSMessage& msg)
{
  render(msg);
  ExitMainLoop();
}



void VizThumbnailApp::quitMCB(JMSMessage& msg)
{
  if (msg.getSender().getName() == GATEWAY)
  {
    ExitMainLoop();
  }
}


void VizThumbnailApp::identifyMCB(JMSMessage& msg)
{
  JMSMessage* resp = newMessage();
  resp->addProperty("info", this->identify());
  publish("ecce_identify_reply", *resp);
  delete resp;
}


void VizThumbnailApp::invokeStatusMCB(JMSMessage& msg)
{
  static const char *errMsg = "Unable to start the application.  "
          "Please use the ECCE Support tool to report this problem.";

  string status = msg.getProperty("status");

  if (status == "failed")
  {
    throw EcceException(errMsg, WHERE);
  }
}


void VizThumbnailApp::msgAuthMCB(JMSMessage& msg)
{
  AuthCache::getCache().msgIn(msg, getMyID());
}


string VizThumbnailApp::identify()
{
  return this->getName();
}


void VizThumbnailApp::render(JMSMessage& msg)
{
  int i, idx;
  double d;
  string calcurl, error;
  idx = 0;
  while (! (calcurl = msg.getProperty(
                      "calcurl" + StringConverter::toString(idx))).empty()) {
    idx++;
    if (!VizRender::thumbnail(calcurl,
            StringConverter::toInt(msg.getProperty("width"), i) ? i : 64,
            StringConverter::toInt(msg.getProperty("height"), i) ? i : 64,
            StringConverter::toDouble(msg.getProperty("r"), d) ? d : 0.0,
            StringConverter::toDouble(msg.getProperty("g"), d) ? d : 0.0,
            StringConverter::toDouble(msg.getProperty("b"), d) ? d : 0.0))
    {
      error = calcurl + ": " + VizRender::msg() + "\n";
      cerr << "VizThumbnailApp::render error: " << error << endl;
    }
  }
  notifySuccess(msg.getSender().getName(), error);
}


void VizThumbnailApp::notifySuccess(const string& appName, const string& error)
{
  Target targetApp(appName, "");
  JMSMessage * msg = newMessage(targetApp);
  if (error.empty()) {
    msg->addIntProperty("success", 1);
  } else {
    msg->addIntProperty("success", 0);
    msg->addProperty("error", error);
  }
  publish("ecce_vizthumbnail", *msg);
  delete msg;
}

