/**
 * @file
 * @author Karen Schuchardt 
 *
 */


#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include <wx/timer.h>

#include <iostream>
using std::cerr;
using std::endl;

#include "util/Ecce.H"
#include "util/LocalData.H"
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

#include "tdat/TPerTab.H"
#include "Builder.H"
#include "BuilderMessages.H"


BuilderMessages::BuilderMessages(const string& name, Builder *builder)
  : WxJMSMessageDispatch(name, true), p_builder(builder)
{
}


void BuilderMessages::subscribeMessages()
{
  //  Desktop Messages
  subscribe("ecce_invoke", (wxJmsCBFunc)&BuilderMessages::invokeMCB);
  subscribe("ecce_quit", (wxJmsCBFunc)&BuilderMessages::quitMCB);
  subscribe("ecce_identify", (wxJmsCBFunc)&BuilderMessages::identifyMCB);

  // handle return message from a launcher request to start another tool
  subscribe("ecce_invoke_status", (wxJmsCBFunc)&BuilderMessages::invokeStatusMCB, false);

  // Global Preference Messages
  subscribe("ecce_preferences_units", (wxJmsCBFunc)&BuilderMessages::unitsPrefsMCB);
  subscribe("ecce_preferences_pertab", (wxJmsCBFunc)&BuilderMessages::pertabPrefsMCB);
  subscribe("ecce_preferences_misc", (wxJmsCBFunc)&BuilderMessages::miscPrefsMCB);

  // Ecce MD messages
  subscribe("ecce_ffconfig_edit", (wxJmsCBFunc)&BuilderMessages::ffConfigEditMCB);

  // Calc/Project Changes
  subscribe("ecce_url_renamed", (wxJmsCBFunc)&BuilderMessages::urlRenameMCB);
  subscribe("ecce_url_removed", (wxJmsCBFunc)&BuilderMessages::urlRemoveMCB);
  subscribe("ecce_url_state", (wxJmsCBFunc)&BuilderMessages::urlStateMCB, false);

  // property changes
  subscribe("ecce_url_property", (wxJmsCBFunc)&BuilderMessages::propertyChangeMCB);
  //  The Vina search box drawn in the viewer follows the editor (#455).
  subscribe("ecce_url_details", (wxJmsCBFunc)&BuilderMessages::detailsMCB);
  subscribe("ecce_vina_box", (wxJmsCBFunc)&BuilderMessages::vinaBoxMCB);
  subscribe("ecce_url_property_data", (wxJmsCBFunc)&BuilderMessages::propertyChangeMCB);

  subscribe("ecce_auth_changed", (wxJmsCBFunc)&BuilderMessages::authChangedMCB);

  subscribe("ecce_set_selection", (wxJmsCBFunc)&BuilderMessages::setSelectionMCB);
}



string BuilderMessages::identify()
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
void BuilderMessages::invokeMCB(JMSMessage& msg)
{
  p_invokeReceived = true;
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

    if (action == "" && urlstr.empty()) {

      // "New Structure": no calculation, keep the default context

    } else if (action == "") {

      p_builder->setContext(urlstr);
      setPollContext(urlstr);

    } else {

      // Don't setContext if we are already in this calculation context
      if (p_builder->getContext() != urlstr) {
        p_builder->setContext(urlstr);
        setPollContext(urlstr);
      }

      if (action == "constraints") {
        p_builder->showConstraints();
      } else if (action == "getselection") {
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
        JMSPublisher publisher(getMyName());
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
        JMSPublisher publisher(getMyName());
        JMSMessage * reply = publisher.newMessage(t);
        reply->addProperty("value", valStr.ToStdString());
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
        JMSPublisher publisher(getMyName());
        JMSMessage * reply = publisher.newMessage(t);
        reply->addProperty("value", valStr.ToStdString());
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
        JMSPublisher publisher(getMyName());
        JMSMessage * reply = publisher.newMessage(t);
        reply->addProperty("value", valStr.ToStdString());
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
        JMSPublisher publisher(getMyName());
        JMSMessage * reply = publisher.newMessage(t);
        reply->addProperty("charge", chargeStr.ToStdString());
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

    registerTopShell(p_builder);
    p_builder->Iconize(false);
#ifdef __WXGTK__
    p_builder->Show(true);
    ewxRaiseWindow(p_builder);
#elif defined(__WXMSW__)
    p_builder->Show(true);
    ewxRaiseWindow(p_builder);
#else
    p_builder->Raise();
    p_builder->Show(true);
    p_builder->SetFocus();
    p_builder->RequestUserAttention();
#endif

  } else if (msg.getIntProperty("importcalc") == 1) {
    registerTopShell(p_builder);
    p_builder->Show(true);
    wxCommandEvent event;
    p_builder->OnImportCalcClick(event);
  }
}


/**
 * Responds to quit msg
 */
void BuilderMessages::quitMCB(JMSMessage& msg)
{
  if (msg.getSender().getName() == GATEWAY) {
    //  In one process the Organizer may have closed this window already.
    if (wxTheApp->IsScheduledForDestruction(p_builder)) return;
    //  Force the window to close; disallows veto
    p_builder->quit(false);
    unsubscribe();
  }
}


/**
 * Responds to identify msg
 */
void BuilderMessages::identifyMCB(JMSMessage& msg)
{
  JMSMessage* resp = newMessage();
  resp->addProperty("info", this->identify());
  publish("ecce_identify_reply", *resp);

  delete resp;
}


/**
 * Responds to invoke status msg
 */
void BuilderMessages::invokeStatusMCB(JMSMessage& msg)
{
  static const char *errMsg =
    "Unable to start the application.  "
    "Please report it at https://github.com/FriendsofECCE/ECCE/issues .";

  string status = msg.getProperty("status");

  if (status == "failed")
  {
    wxLogError("%s", errMsg);
  }
}


/**
 * Responds to unit preference change msg
 */
void BuilderMessages::unitsPrefsMCB(JMSMessage& msg)
{
  ewxWindowUtils::processGlobalPreferenceChange(p_builder);
}


/**
 * Responds to pertab preference changed msg
 */
void BuilderMessages::pertabPrefsMCB(JMSMessage& msg)
{
  TPerTab::applyColourScheme();
  p_builder->pertabPrefsMCB();
}


/**
 * Responds to misc preference changed msg
 */
void BuilderMessages::miscPrefsMCB(JMSMessage& msg)
{
  ewxWindowUtils::processGlobalPreferenceChange(p_builder);
}


/**
 * Responds to force field config edit msg
 */
void BuilderMessages::ffConfigEditMCB(JMSMessage& msg)
{
}


/**
 * Responds to url rename msg
 */
void BuilderMessages::urlRenameMCB(JMSMessage& msg)
{
  p_builder->urlRenameMCB(msg);
  setPollContext(p_builder->getContext());
}


/**
 * Responds to url remove msg
 */
void BuilderMessages::urlRemoveMCB(JMSMessage& msg)
{
  p_builder->urlRemoveMCB(msg);
  setPollContext(p_builder->getContext());
}


/**
 * Responds to url state msg
 */
void BuilderMessages::urlStateMCB(JMSMessage& msg)
{
  p_builder->urlStateMCB(msg);
}


/**
 * Responds to calculation property changes
 */
void BuilderMessages::detailsMCB(JMSMessage& msg)
{
  if (p_builder) p_builder->vinaDetailsChanged(msg.getProperty("url"));
}


void BuilderMessages::vinaBoxMCB(JMSMessage& msg)
{
  if (!p_builder) return;
  std::map<string, string> live;
  static const char *const keys[] = {
    "ES.Runtype.Vina.BoxAuto", "ES.Runtype.Vina.BoxMargin",
    "ES.Runtype.Vina.BoxCenterX", "ES.Runtype.Vina.BoxCenterY",
    "ES.Runtype.Vina.BoxCenterZ", "ES.Runtype.Vina.BoxSizeX",
    "ES.Runtype.Vina.BoxSizeY", "ES.Runtype.Vina.BoxSizeZ",
    "ES.Runtype.Docking.LigandAtoms" };
  for (const char *k : keys) live[k] = msg.getProperty(k);
  p_builder->vinaDetailsChanged(msg.getProperty("url"), &live);
}


void BuilderMessages::propertyChangeMCB(JMSMessage& msg)
{
  p_builder->propertyChangeMCB(msg);
}


/**
 * Responds to auth changed msg
 */
void BuilderMessages::authChangedMCB(JMSMessage& msg)
{
  AuthCache::getCache().msgIn(msg, getMyID());
}


/**
 * Sets selection in builder
 */
void BuilderMessages::setSelectionMCB(JMSMessage& msg)
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
