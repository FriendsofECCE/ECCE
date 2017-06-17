/**
 * @file
 * @author Karen Schuchardt 
 *
 */


#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
using std::cerr;
using std::endl;

#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/StringTokenizer.H"
#include "util/Command.H"
#include "util/ErrMsg.H"
#include "util/SDirectory.H"

#include "tdat/AuthCache.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"

#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WxState.H"
#include "wxgui/ewxMessageDialog.H"

#include "viz/SelectCmd.H"
#include "viz/ValidateStrCmd.H"

#include "BuilderApp.H"
#include "Builder.H"

IMPLEMENT_APP(BuilderApp)



static const string appName()
{
  return (getenv("ECCE_INVOKE_VIEWER")?CALCVIEWER:BUILDER);
}


BuilderApp::BuilderApp()
    : ewxApp(), WxJMSMessageDispatch(appName(), true)
{
  p_builder = NULL;
}


BuilderApp::~BuilderApp()
{
}


string BuilderApp::getName() const
{
  return appName();
}


bool BuilderApp::OnInit()
{
  ewxApp::OnInit();
  
  string context = "";
  string file;
  bool standalone = false;
  if (argc > 1) {
    for (int i=1; i<argc; i++) {
      if (strcmp(argv[i], "-standalone") == 0)
        standalone = true;
      else if (strcmp(argv[i], "-help") == 0) {
        usage();
        exit(-1);
      } else if (strcmp(argv[1], "-pipe") == 0) {
        AuthCache::getCache().pipeIn(argv[++i]);
      } else if (strcmp(argv[i], "-context") == 0) {
        // Might need index out of boundary check
        context = argv[++i];
      } else {
        // assume its a file name to open
        file = argv[i];
      }
    }
  }

  // Determine if this invocation is for a viewer from outside ECCE (standalone)
  bool soloviewer = !getenv("ECCE_INVOKE_FROMECCE") &&
                     getenv("ECCE_INVOKE_VIEWER");

  if (!soloviewer && !standalone) {
    EDSIFactory::addAuthEventListener(this);
  }

  p_builder = new Builder(NULL, standalone);

  p_builder->Show(false);
  SetTopWindow(p_builder);
  registerTopShell(p_builder);

  if (context != "") {
    p_builder->setContext(context);
    setPollContext(context);
  }

  //  Subscribe to Messages
  if (!standalone) {
    subscribeMessages();
    startSubscriber();
  }

  if (file != "") {
     // Let the interpretter report errors
     //p_builder->interpretCommand("open filename " + file);
     p_builder->setContext(file);
  }

  if (soloviewer) {
    // Connect to data server for importing calculation output files
    setAuthDialogParent(p_builder);
    EDSIFactory::addAuthEventListener(this);

    // create the preferences directory if it doesn't already exist so we
    // have someplace to put all the wonderful preference files
    SDirectory directory(Ecce::realUserPrefPath(), 0700);
    if (!directory.exists()) {
      string msg = "Could not create user preferences directory ";
      msg += Ecce::realUserPrefPath();
      EE_RT_ASSERT(FALSE, EE_FATAL, msg);
    }

    try {
      if (!checkServerSetup()) {
        p_builder->quit(false); // canceled
      }
    } catch (RetryException& rex) {
      ewxMessageDialog * dlg =
        new ewxMessageDialog(p_builder, rex.what(), "Retries exceeded!",
                             wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg->ShowModal();
      dlg->Destroy();
      p_builder->quit(false);
    } catch (EcceException& ex) {
      string msg = ex.what();
      msg += "Please contact your ECCE Administrator.";
      ewxMessageDialog * dlg =
        new ewxMessageDialog(p_builder,  msg.c_str(), "ECCE Server Failure",
                             wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg->ShowModal();
      dlg->Destroy();
      p_builder->quit(false);
    }

    try {
      if (!checkServer()) {
        p_builder->quit(false); // canceled
      }
    } catch (RetryException& rex) {
      ewxMessageDialog * dlg =
        new ewxMessageDialog(p_builder, rex.what(), "Retries exceeded!",
                             wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg->ShowModal();
      dlg->Destroy();
      p_builder->quit(false);
    } catch (EcceException& ex) {
      string msg = "An error has occured while attempting to access "
        "the ECCE server.  Please contact your administrator for assistence "
        "if the problem persists.\n";
      msg += ex.what();
      ewxMessageDialog * dlg =
        new ewxMessageDialog(p_builder,  msg.c_str(), "ECCE Server Failure",
                             wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg->ShowModal();
      dlg->Destroy();
      p_builder->quit(false);
    }

    if (!checkUser()) {
      p_builder->quit(false);
    }
  }

  if (!standalone) {
    // Notify execer that I am ready to receive messages:
    notifyReady();
  }

  p_builder->Show(true);
  
  return true;
}


void BuilderApp::subscribeMessages()
{
  //  Desktop Messages
  subscribe("ecce_invoke", (wxJmsCBFunc)&BuilderApp::invokeMCB);
  subscribe("ecce_quit", (wxJmsCBFunc)&BuilderApp::quitMCB);
  subscribe("ecce_identify", (wxJmsCBFunc)&BuilderApp::identifyMCB);

  // handle return message from a launcher request to start another tool
  subscribe("ecce_invoke_status", (wxJmsCBFunc)&BuilderApp::invokeStatusMCB, false);

  // Global Preference Messages
  subscribe("ecce_preferences_states", (wxJmsCBFunc)&BuilderApp::statePrefsMCB);
  subscribe("ecce_preferences_units", (wxJmsCBFunc)&BuilderApp::unitsPrefsMCB);
  subscribe("ecce_preferences_pertab", (wxJmsCBFunc)&BuilderApp::pertabPrefsMCB);
  subscribe("ecce_preferences_misc", (wxJmsCBFunc)&BuilderApp::miscPrefsMCB);

  // Ecce MD messages
  subscribe("ecce_ffconfig_edit", (wxJmsCBFunc)&BuilderApp::ffConfigEditMCB);

  // Calc/Project Changes
  subscribe("ecce_url_renamed", (wxJmsCBFunc)&BuilderApp::urlRenameMCB);
  subscribe("ecce_url_removed", (wxJmsCBFunc)&BuilderApp::urlRemoveMCB);
  subscribe("ecce_url_state", (wxJmsCBFunc)&BuilderApp::urlStateMCB, false);

  // property changes
  subscribe("ecce_url_property", (wxJmsCBFunc)&BuilderApp::propertyChangeMCB);
  subscribe("ecce_url_property_data", (wxJmsCBFunc)&BuilderApp::propertyChangeMCB);

  subscribe("ecce_auth_changed", (wxJmsCBFunc)&BuilderApp::authChangedMCB);

  subscribe("ecce_set_selection", (wxJmsCBFunc)&BuilderApp::setSelectionMCB);
}


int BuilderApp::OnExit()
{
  //  Unsubscribe from Messaging Service
  unsubscribe();

  return 0;
}


string BuilderApp::identify()
{
  EcceURL url = p_builder->getContext();
  string idnty = getenv("ECCE_INVOKE_VIEWER")? "Viewer": "Builder";
  string path = EDSIServerCentral().mapURLtoName(url);

  if (!path.empty())
    idnty += ": " + path;

  return idnty;
}


/**
 * Responds to invoke msg
 */
void BuilderApp::invokeMCB(JMSMessage& msg)
{
  if (msg.containsProperty("calcurl"))
  {
    string urlstr = msg.getProperty("calcurl");
    EcceURL url(urlstr);

    string action = msg.getProperty("actiontopic");
  /*
  cout << "action:" << action;
  cout << " origsender " << msg.getOriginalSenderName();
  cout << " origsenderid " << msg.getOriginalSenderId() << endl;;
  */

    if (action == "") {

      p_builder->setContext(urlstr);
      setPollContext(urlstr);

    } else {

      // Don't setContext if we are already in this calculation context
      if (p_builder->getContext() != urlstr) {
        p_builder->setContext(urlstr);
        setPollContext(urlstr);
      }

      if (action == "getselection") {
        Target t(msg.getOriginalSenderName(), msg.getOriginalSenderId());
        JMSMessage *reply = newMessage(t);
        SGFragment *frag = p_builder->getSceneGraph().getFragment();
        int natoms = frag->m_atomHighLight.size();
        string list; char buf[120];
        if (natoms > 0) {
          sprintf(buf,"%d",frag->m_atomHighLight[0]);
          list.append(buf);
        }
        if (natoms > 1) {
          int i;
          for (i=1; i<natoms; i++) {
            sprintf(buf,"%d",frag->m_atomHighLight[i]);
            list.append(" ");
            list.append(buf);
          }
        }
        reply->addIntProperty("numatoms",natoms);
        reply->addProperty("selection",list.c_str());
        publish("ecce_getselection",*reply);
        delete reply;
      } else if (action == "get_atomlist") {
        Target t(msg.getOriginalSenderName(), msg.getOriginalSenderId());
        JMSMessage *reply = newMessage(t);
        SGFragment *frag = p_builder->getSceneGraph().getFragment();
        int idx;
        int natoms = frag->m_atomHighLight.size();
        reply->addIntProperty("numatoms",natoms);
        string list;
        string symbol;
        char buf[120];
        if (natoms > 0) {
          idx = frag->m_atomHighLight[0];
          symbol = frag->atomRef(idx)->atomicSymbol();
          sprintf(buf,"%s%d",symbol.c_str(),idx+1);
          list.append(buf);
        }
        if (natoms > 1) {
          int i;
          for (i=1; i<natoms; i++) {
            idx = frag->m_atomHighLight[i];
            symbol = frag->atomRef(idx)->atomicSymbol();
            sprintf(buf," %s%d",symbol.c_str(),idx+1);
            list.append(buf);
          }
        }
        reply->addProperty("selection",list.c_str());
        publish("ecce_get_atomlist",*reply);
        delete reply;
      } else if (action == "get_methylgroups") {
        SGFragment *frag = p_builder->getSceneGraph().getFragment();
        int natoms = frag->numAtoms();
        int idx;
        string symbol;
        char buf[120];
        for (idx=0; idx<natoms; idx++) {
          symbol = frag->atomRef(idx)->atomicSymbol();
          if (symbol == "C") {
            vector<TAtm*> *atoms = frag->connectedAtoms(idx);
            int nconnected = atoms->size();
            sprintf(buf,"C%d",frag->atomRef(idx)->index()+1);
            string alist = buf;
            int i;
            int icnt = 0;
            for (i=0; i<nconnected; i++) {
              if ((*atoms)[i]->atomicSymbol() == "H") {
                icnt++;
                sprintf(buf," H%d",(*atoms)[i]->index()+1);
                alist.append(buf);
              }
            }
            if (icnt > 0) {
              Target t(msg.getOriginalSenderName(), msg.getOriginalSenderId());
              JMSMessage *reply = newMessage(t);
              reply->addIntProperty("numatoms",icnt+1);
              reply->addProperty("selection",alist.c_str());
              publish("ecce_get_methyl_constraint",*reply);
              delete reply;
            }
            delete atoms;
          }
        }
      } else if (action == "get_methylhydrogen") {
        SGFragment *frag = p_builder->getSceneGraph().getFragment();
        int natoms = frag->numAtoms();
        int idx;
        string symbol;
        char buf[120];
        for (idx=0; idx<natoms; idx++) {
          symbol = frag->atomRef(idx)->atomicSymbol();
          if (symbol == "C") {
            vector<TAtm*> *atoms = frag->connectedAtoms(idx);
            int nconnected = atoms->size();
            string alist = buf;
            int i;
            int icnt = 0;
            for (i=0; i<nconnected; i++) {
              if ((*atoms)[i]->atomicSymbol() == "H") {
                if (icnt == 0) {
                  sprintf(buf,"H%d",(*atoms)[i]->index()+1);
                  alist = buf;
                } else {
                  sprintf(buf," H%d",(*atoms)[i]->index()+1);
                  alist.append(buf);
                }
                icnt++;
              }
            }
            if (icnt > 1) {
              Target t(msg.getOriginalSenderName(), msg.getOriginalSenderId());
              JMSMessage *reply = newMessage(t);
              reply->addIntProperty("numatoms",icnt);
              reply->addProperty("selection",alist.c_str());
              publish("ecce_get_methyl_constraint",*reply);
              delete reply;
            }
            delete atoms;
          }
        }
      } else if (action == "ecce_set_selection") {
        string selection = msg.getProperty("selection");
        StringTokenizer tkn(selection);
        vector<string> tokens = tkn.tokenize(" ");
        Command *cmd = new SelectCmd("Select Atoms",
                &(p_builder->getSceneGraph()));
        cmd->getParameter("atoms")->setStringList(tokens);
        cmd->getParameter("clear")->setBoolean(true);
        cmd->execute();
      } else if (action == "getNWChemSelection") {
        string selection = p_builder->getSceneGraph()
          .getFragment()->getNWChemStringFromSelection();
        Target t(msg.getOriginalSenderName(), msg.getOriginalSenderId());
        JMSPublisher publisher(appName());
        JMSMessage * reply = publisher.newMessage(t);
        reply->addProperty("selection", selection);
        publisher.publish("ecce_getNWChemSelection", *reply);
        delete reply;
      } else if (action == "showNWChemSelection") {
        p_builder->showSelectionFromNWChemString(msg.getProperty("selection"));
      } else if (action == "getNWChemDistance") {
        wxString valStr;
        double val = p_builder->getSceneGraph()
          .getFragment()->getNWChemPMFDistance(msg.getProperty("selection1"),
                                               msg.getProperty("selection2"));
        valStr << val;
        Target t(msg.getOriginalSenderName(), msg.getOriginalSenderId());
        JMSPublisher publisher(appName());
        JMSMessage * reply = publisher.newMessage(t);
        reply->addProperty("value", valStr.c_str());
        publisher.publish("ecce_getNWChemDistance", *reply);
        delete reply;
      } else if (action == "getNWChemAngle") {
        wxString valStr;
        double val = p_builder->getSceneGraph()
          .getFragment()->getNWChemPMFAngle(msg.getProperty("selection1"),
                                            msg.getProperty("selection2"),
                                            msg.getProperty("selection3"));
        valStr << val;
        Target t(msg.getOriginalSenderName(), msg.getOriginalSenderId());
        JMSPublisher publisher(appName());
        JMSMessage * reply = publisher.newMessage(t);
        reply->addProperty("value", valStr.c_str());
        publisher.publish("ecce_getNWChemAngle", *reply);
        delete reply;
      } else if (action == "getNWChemTorsionAngle") {
        wxString valStr;
        double val = p_builder->getSceneGraph()
          .getFragment()->getNWChemPMFTorsionAngle(
                                                   msg.getProperty("selection1"),
                                                   msg.getProperty("selection2"),
                                                   msg.getProperty("selection3"),
                                                   msg.getProperty("selection4"));
        valStr << val;
        Target t(msg.getOriginalSenderName(), msg.getOriginalSenderId());
        JMSPublisher publisher(appName());
        JMSMessage * reply = publisher.newMessage(t);
        reply->addProperty("value", valStr.c_str());
        publisher.publish("ecce_getNWChemAngle", *reply);
        delete reply;
      } else if (action == "getcharge") {
        Command *cmd = new ValidateStrCmd("Validate Structure",
                                          &(p_builder->getSceneGraph()));
        cmd->execute();
        double charge =
                p_builder->getSceneGraph().getFragment()->getTotalCharge();
        int residueStatus = 
                p_builder->getSceneGraph().getFragment()->checkResidueStatus();
        wxString chargeStr;
        chargeStr << charge;
        Target t(msg.getOriginalSenderName(), msg.getOriginalSenderId());
        JMSPublisher publisher(appName());
        JMSMessage * reply = publisher.newMessage(t);
        reply->addProperty("charge", chargeStr.c_str());
        reply->addIntProperty("residue_status", residueStatus);
        publisher.publish("ecce_getcharge", *reply);
        delete reply;
      } else if (action == "refresh") {
        // set context again to force refresh
        p_builder->setContext(urlstr, true);
        setPollContext(urlstr);
        // new thumbnail needed for solvation editor
        wxCommandEvent event;
        p_builder->OnSaveThumbClick(event);
      } else if (action == "noop") {
        // do nothing
      } else {
        // fall through
        string msg = "Unknown builder action requested: " + action;
        p_builder->showMessage(msg, true);
      }

    }

    p_builder->Iconize(false);
    p_builder->Raise();
    p_builder->Show(true);
    p_builder->SetFocus();
    p_builder->RequestUserAttention();

  } else if (msg.getIntProperty("importcalc") == 1) {
    wxCommandEvent event;
    p_builder->OnImportCalcClick(event);
  }
}


/**
 * Responds to quit msg
 */
void BuilderApp::quitMCB(JMSMessage& msg)
{
  if (msg.getSender().getName() == GATEWAY) {
    //  Force the window to close; disallows veto
    p_builder->quit(false);
    this->OnExit();
  }
}


/**
 * Responds to identify msg
 */
void BuilderApp::identifyMCB(JMSMessage& msg)
{
  JMSMessage* resp = newMessage();
  resp->addProperty("info", this->identify());
  publish("ecce_identify_reply", *resp);

  delete resp;
}


/**
 * Responds to invoke status msg
 */
void BuilderApp::invokeStatusMCB(JMSMessage& msg)
{
  static const char *errMsg =
    "Unable to start the application.  "
    "Please use the ECCE Support tool to report this problem.";

  string status = msg.getProperty("status");

  if (status == "failed")
  {
    wxLogError(errMsg, 0);
  }
}


/**
 * Responds to state color preference change msg
 */
void BuilderApp::statePrefsMCB(JMSMessage& msg)
{
  WxState::resetToUserDefault();
  p_builder->Refresh();
}



/**
 * Responds to unit preference change msg
 */
void BuilderApp::unitsPrefsMCB(JMSMessage& msg)
{
  ewxWindowUtils::processGlobalPreferenceChange(p_builder);
}


/**
 * Responds to pertab preference changed msg
 */
void BuilderApp::pertabPrefsMCB(JMSMessage& msg)
{
  p_builder->pertabPrefsMCB();
}


/**
 * Responds to misc preference changed msg
 */
void BuilderApp::miscPrefsMCB(JMSMessage& msg)
{
  ewxWindowUtils::processGlobalPreferenceChange(p_builder);
}


/**
 * Responds to force field config edit msg
 */
void BuilderApp::ffConfigEditMCB(JMSMessage& msg)
{
}


/**
 * Responds to url rename msg
 */
void BuilderApp::urlRenameMCB(JMSMessage& msg)
{
  p_builder->urlRenameMCB(msg);
  setPollContext(p_builder->getContext());
}


/**
 * Responds to url remove msg
 */
void BuilderApp::urlRemoveMCB(JMSMessage& msg)
{
  p_builder->urlRemoveMCB(msg);
  setPollContext(p_builder->getContext());
}


/**
 * Responds to url state msg
 */
void BuilderApp::urlStateMCB(JMSMessage& msg)
{
  p_builder->urlStateMCB(msg);
}


/**
 * Responds to calculation property changes
 */
void BuilderApp::propertyChangeMCB(JMSMessage& msg)
{
  p_builder->propertyChangeMCB(msg);
}


/**
 * Responds to auth changed msg
 */
void BuilderApp::authChangedMCB(JMSMessage& msg)
{
  AuthCache::getCache().msgIn(msg, getMyID());
}


/**
 * Sets selection in builder
 */
void BuilderApp::setSelectionMCB(JMSMessage& msg)
{
  if (msg.containsProperty("selection")) {
    string selection = msg.getProperty("selection");
    StringTokenizer tkn(selection);
    vector<string> tokens = tkn.tokenize(" ");
    Command *cmd = new SelectCmd("Select Atoms",&(p_builder->getSceneGraph()));
    cmd->getParameter("atoms")->setStringList(tokens);
    cmd->getParameter("clear")->setBoolean(true);
    cmd->execute();
  }
}


/**
 * Checks server to see if it's legit and properly setup.
 */
bool BuilderApp::checkServer() throw (RetryException,EcceException)
{
  EDSIServerCentral servers;
  return servers.checkServer();
}


/**
 * Move this to separate function and command line option such as
 * gateway -check if its too slow
 */
bool BuilderApp::checkServerSetup() throw (RetryException,EcceException)
{
  EDSIServerCentral servers;
  return servers.checkServerSetup();
}


/**
 * At startup time, do some minimal server verification.
 * Its not really clear what needs to be done here since we are
 * attempting to reduce the requirement to have a user directory.
 */
bool BuilderApp::checkUser()
{
  bool ret = true;

  EDSIServerCentral servers;
  EcceURL mount = servers.getDefaultUserHome();

  string directoryPath = mount.getParent().toString();

  // Make virtual connection to URL via EDSI:
  EDSI* connection = EDSIFactory::getEDSI(directoryPath);

  bool userExists = connection->exists();
  string err = connection->m_msgStack.getMessage();
  if (!err.empty()) {
    string msg,title;
    if (connection->m_msgStack.findKey("CANCELED")) {
      msg = "Access controls have been set up on this server.  "
        "You must authenticate to run ECCE.  Contact your "
        "administrator if you do not know your password";
      title = "Authentication Failure";
    } else if (connection->m_msgStack.findKey("TOO_MANY_RETRIES")) {
      msg = "The allowable number of authentication attempts has "
        "been exceeded.  Contact your ECCE administrator for assistance.";
      title = "Authentication Failure";
    } else {
      msg = err;
      title = "ECCE Server Failure";
    }
    ewxMessageDialog* dlg = new ewxMessageDialog(p_builder, msg.c_str(),
                                   title.c_str(), wxOK|wxICON_EXCLAMATION,
                                   wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
    ret = false;

  } else if (!userExists) {
    // Leave this in for now but do we really care?
    string msg = "Web server specified as the first entry in DataServers "
      "is not properly setup as an ECCE DAV server.  Contact "
      "your ECCE site administrator:\n";
    msg += connection->m_msgStack.getMessage();

    ewxMessageDialog* dlg = new ewxMessageDialog(p_builder, msg.c_str(),
                                 "ECCE Server Failure", wxOK|wxICON_EXCLAMATION,
                                 wxDefaultPosition);
    dlg->ShowModal();
    dlg->Destroy();
    ret = false;

  } else {
    // Verify the user has a valid directory.
    // Again leave this in for now but do we really care.
    string userPath = directoryPath + "/" + Ecce::serverUser();
    connection->setURL(userPath);

    userExists = connection->exists();
    err = connection->m_msgStack.getMessage();
    if (!err.empty()) {
      string msg;
      msg += connection->m_msgStack.getMessage();
      ewxMessageDialog* dlg = new ewxMessageDialog(p_builder, msg.c_str(),
                                    "ECCE Server Configuration Error",
                                    wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg->ShowModal();
      dlg->Destroy();
      ret = false;
    } else if (!userExists) {
      string msg = "No top-level user directory exists for ";
      msg += Ecce::serverUser();
      msg += " on the default ECCE server.  The site administrator "
        "must run the add_ecce_user script and create an account for ";
      msg += Ecce::serverUser();
      msg += " in order to run ECCE as this user.";
      ewxMessageDialog* dlg = new ewxMessageDialog(p_builder, msg.c_str(),
                                    "ECCE Server User Not Recognized",
                                    wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg->ShowModal();
      dlg->Destroy();
      ret = false;
    }
  }

  delete connection;

  return ret;
}


/**
 * This usage statement is intended for end users so it does not document
 * options intended for us.  See the argv parsing above for all developer
 * options. 
 */
void BuilderApp::usage()
{
   cout << "Usage: ebuilder [-help] [file]" << endl;
   cout << "\t-help:  this message" << endl;
   cout << "\tfile:  name of file to load at startup" << endl;
}
