#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "Polyrate.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
  using std::cerr;
  using std::cout;
  using std::endl;
  using std::ends;
#include <fstream>
  using std::ofstream;
  using std::ifstream;
#include <sstream>
  using std::ostringstream;
#include <strstream>
  using std::istrstream;
  using std::ostrstream;
#include <unistd.h>
#include <values.h>
//#include <stdlib.h>  // abort

#include "util/Ecce.H"
#include "util/CommandWrapper.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/StringConverter.H"
#include "util/SFile.H"
#include "util/TypedFile.H"
#include "util/SDirectory.H"
#include "util/TempStorage.H"
#include "util/EcceException.H"
#include "util/StringConverter.H"
#include "util/ResourceUtils.H"
#include "util/StringTokenizer.H"
#include "util/STLUtil.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/EditEvent.H"

#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "tdat/RefMachine.H"
#include "tdat/NWDirdyModel.H"

#include "dsm/MachineConfig.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/CodeFactory.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"
#include "dsm/DirDyVTSTTask.H"
#include "dsm/PolyrateTask.H"
#include "dsm/VDoc.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/ewxRegexpValidator.H"
#include "wxgui/ewxSingleChoiceDialog.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxUnitHelper.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WxEditSessionMgr.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/WxJMSListener.H"
#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/WxState.H"

#include "Polyrate.H"

/** Used to subscribe to messaging, publish, identify */
static const string PolyrateUnitFamily = "AU";
static const string WHOAMI = "Polyrate";

/**
 * Constructor.
 * Create widgets.  Some widgets like the feedback area are 
 * created in code.  Initialize GUI state and set up validators and
 * unit helpers.
 */
Polyrate::Polyrate(wxWindow* parent, wxWindowID id, const wxString& caption,
                 const wxPoint& pos, const wxSize& size, long style)
  : PolyrateGUI(parent, id, caption, pos, size, style),
    WxDavAuth(0),
    WxJMSListener(),
    EditListener(),
    TaskApp(WHOAMI, 1000),
    CalcDropHandler(),
    FeedbackSaveHandler()
{

   SetIcon(wxIcon(ewxBitmap::pixmapFile(
           ResourceDescriptor::getResourceDescriptor().getTool(POLYRATE)
           ->getIcon()), wxBITMAP_TYPE_XPM));

   EDSIFactory::addAuthEventListener(this);

   initializeMessaging();

   initializeModel();

   p_polyrateTask = 0;

   p_session = 0;

   p_progressDialog = 0;

   p_ignoreCodeEventsFlag = false;

   p_touch = false;

   setState( Polyrate::NO_SYSTEM );
   setState( Polyrate::VALID_SYSTEM );

   p_feedback = ((WxFeedback*)FindWindow(ID_FEEDBACK_PLYRT));
   p_feedback->setSaveHandler(this);
   p_feedback->setDropHandler(this);
   p_feedback->setEditStatus(WxFeedback::EDIT);
}



Polyrate::~Polyrate()
{
  delete p_msgDispatcher;
  if (p_progressDialog != 0) {
    delete p_progressDialog;
  }
}


/**
 * Sets up validators and unit helpers for input fields.
 */
void Polyrate::initializeGUI()
{
   PolyrateModel &model = getPolyrateModel();
   ewxTextCtrl *text = 0;
   ewxStaticText *label = 0;
   ewxCheckBox *check = 0;
   ewxChoice *menu = 0;
   ewxStaticText *units = 0;
   string component;

   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_PLYRT_FROPT));
   check->SetValue(model.FROPT);
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_PLYRT_ANLYS));
   check->SetValue(model.ANALYSIS);
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_PLYRT_SDDL));
   check->SetValue(model.SADDLE);
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SCLRV));
   text->SetValidator(ewxNumericValidator(0, 100, model.RVRXEXP));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SGMF));
   text->SetValidator(ewxNumericValidator(0, 100, model.FRWSYM));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SGMR));
   text->SetValidator(ewxNumericValidator(0, 100, model.REVSYM));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_TMPRTR));
   text->SetValidator(ewxNumericValidator(1.0e-10, 1000000.0, model.TMPLST[0]));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_FSCL));
   text->SetValidator(ewxNumericValidator(1.0e-10, 1000000.0, model.FREQSCAL));

   menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPST));
   initializeMenu(menu);
   menu->SetSelection(0);
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG1));
   text->SetValidator(ewxNumericValidator(0, 1000000, model.DEG1));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG1));
   text->SetValidator(ewxNumericValidator(0.0, 1000000.0, 0.0));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_PLYRT_NRG1));
   text->setUnitHelper(new ewxUnitHelper(PolyrateUnitFamily,"Energy",label));
   if (model.ES1 == 0.0) {
     text->SetValue("");
   } else {
     text->setValueAsDouble(model.ES1);
   }

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG2));
   text->SetValidator(ewxNumericValidator(0, 1000000, model.DEG2));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG2));
   text->SetValidator(ewxNumericValidator(0.0, 1000000.0, 0.0));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_PLYRT_NRG2));
   text->setUnitHelper(new ewxUnitHelper(PolyrateUnitFamily,"Energy",label));
   if (model.ES2 == 0.0) {
     text->SetValue("");
   } else {
     text->setValueAsDouble(model.ES2);
   }

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG3));
   text->SetValidator(ewxNumericValidator(0, 1000000, model.DEG3));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG3));
   text->SetValidator(ewxNumericValidator(0.0, 1000000.0, 0.0));
   label = ((ewxStaticText*)FindWindow(wxID_STATIC_PLYRT_NRG3));
   text->setUnitHelper(new ewxUnitHelper(PolyrateUnitFamily,"Energy",label));
   if (model.ES3 == 0.0) {
     text->SetValue("");
   } else {
     text->setValueAsDouble(model.ES3);
   }

   menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPAX));
   initializeMenu(menu);
   DirDyVTSTTask *dirdyTask = p_polyrateTask->getDirdyTask();
   NWDirdyModel dirdyModel;
   dirdyTask->getTaskModel(dirdyModel);
   if (checkComponent(DirDyVTSTTask::REACT1)) {
     component = convertString("Reactant 1");
     model.setLinearStatus(component,dirdyModel.getLinearReact1());
     model.setNumAtoms(component,dirdyModel.getNumAtomsReact1());
   }
   if (checkComponent(DirDyVTSTTask::REACT2)) {
     component = convertString("Reactant 2");
     model.setLinearStatus(component,dirdyModel.getLinearReact2());
     model.setNumAtoms(component,dirdyModel.getNumAtomsReact2());
   }
   if (checkComponent(DirDyVTSTTask::TRANSITION)) {
     component = convertString("Transition State");
     model.setLinearStatus(component,dirdyModel.getLinearTS());
     model.setNumAtoms(component,dirdyModel.getNumAtomsTS());
   }
   if (checkComponent(DirDyVTSTTask::PROD1)) {
     component = convertString("Product 1");
     model.setLinearStatus(component,dirdyModel.getLinearProd1());
     model.setNumAtoms(component,dirdyModel.getNumAtomsProd1());
   }
   if (checkComponent(DirDyVTSTTask::PROD2)) {
     component = convertString("Product 2");
     model.setLinearStatus(component,dirdyModel.getLinearProd2());
     model.setNumAtoms(component,dirdyModel.getNumAtomsProd2());
   }

   menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
   initializeMenu(menu);

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_MRSMD));
   text->SetValidator(ewxNumericValidator(1, 1000000, 1));

   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_TRSNMD));
   text->SetValidator(ewxNumericValidator(1, 1000000, 1));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DMIN));
   text->SetValidator(ewxNumericValidator(1, 1000000, 1));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SYMNUM));
   text->SetValidator(ewxNumericValidator(1, 1000000, 1));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SBGRP));
   text->SetValidator(ewxNumericValidator(1, 1000000, 1));

   units = ((ewxStaticText*)FindWindow(wxID_STATIC_PLYRT_DVALS));
   units->SetFont(ewxStyledWindow::getUnitFont());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DVALS));
   text->SetValidator(ewxGenericValidator());

   

   refreshGUI();

}


PolyrateModel& Polyrate::getPolyrateModel() const
{
   return (PolyrateModel&)*p_model;
}


/**
 * Process drop into feedback drop site.
 */
void Polyrate::processDrop(const string& url)
{
   setContext(url);
}


void Polyrate::setState(PolyrateState state)
{

   // Visually reflect the state
   ewxButton *remoteBtn = ((ewxButton*)FindWindow(ID_BUTTON_PLYRT_LAUNCH));
   ewxButton *editBtn = ((ewxButton*)FindWindow(ID_BUTTON_PLYRT_FINAL_EDIT));

   if (state == NO_SYSTEM) {
      // disable everything except builder access
      remoteBtn->Disable();
      editBtn->Disable();

   } else if (state == VALID_SYSTEM)  {
      remoteBtn->Enable(true);
      editBtn->Enable(true);

   } else if (state == PROVIDED_SYSTEM)  {
      remoteBtn->Enable(true);
      editBtn->Enable(true);

   } else if (state == VALID_TOPOLOGY)  {
      remoteBtn->Enable(true);
      editBtn->Enable(true);

   } else {
      cerr << "Unknown POLYRATE state " << state << endl;
   }

   p_state = state;
}



Polyrate::PolyrateState Polyrate::getState() const
{
   return p_state;
}


bool Polyrate::Show( bool show)
{
   p_ignoreCodeEventsFlag = true;
   wxCommandEvent event;
   p_ignoreCodeEventsFlag = false;

   if (!show) {
      // save current settings
      Preferences prefs(WHOAMI);
      saveWindowSettings(WHOAMI, prefs, false);
      prefs.saveFile();
   } else if (show) {
      // restore settings - could optimize this to only do it the first time
      // but this is ok for now
      Preferences prefs(WHOAMI);
      restoreWindowSettings(WHOAMI, prefs, false);
   }

   return PolyrateGUI::Show(show);
}


/**
 * Restore all GUI fields from the data model object.
 */
void Polyrate::refreshGUI()
{
   // Change to default units since the model is always in these units
   ewxWindowUtils::setUnitFamily(this, PolyrateUnitFamily );

   PolyrateModel &model = getPolyrateModel();
   ewxTextCtrl *text = 0;
   ewxCheckBox *check = 0;
   ewxListBox *list = 0;
   ewxChoice *menu = 0;

   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_PLYRT_FROPT));
   check->SetValue(model.getFrwrdRevOpt());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_PLYRT_ANLYS));
   check->SetValue(model.getAnalysisFlag());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_PLYRT_SDDL));
   check->SetValue(model.getSaddleFlag());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SCLRV));
   text->setValueAsInt(model.getReverseRxScaleExp());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SGMF));
   text->setValueAsInt(model.getForwardSymmetryFactor());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SGMR));
   text->setValueAsInt(model.getReverseSymmetryFactor());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_FSCL));
   text->setValueAsDouble(model.getFrequencyScale());
   list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_TMPLST));
   list->Clear();
   int ntemp = model.getTemperatureListSize();
   int idx;
   if (ntemp > 0) {
     char buf[32];
     for (idx=0; idx<ntemp; idx++) {
       sprintf(buf,"%18.6f",model.getTemperature(idx));
       list->Append(buf);
     }
   }
   menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPST));
   menu->SetSelection(0);
   string component = convertString(menu->GetStringSelection().c_str());
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG1));
   text->setValueAsInt(model.getDegeneracy1(component));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG1));
   text->setValueAsDouble(model.getElectronicEnergy1(component));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG2));
   text->setValueAsInt(model.getDegeneracy2(component));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG2));
   if (model.getElectronicEnergy2(component) == 0.0) {
     text->SetValue("");
   } else {
     text->setValueAsDouble(model.getElectronicEnergy2(component));
   }
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG3));
   text->setValueAsInt(model.getDegeneracy3(component));
   text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG3));
   if (model.getElectronicEnergy3(component) == 0.0) {
     text->SetValue("");
   } else {
     text->setValueAsDouble(model.getElectronicEnergy3(component));
   }

   menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPAX));
   menu->SetSelection(0);

   menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
   menu->SetSelection(0);

   // Now change to user prefered units
   ewxWindowUtils::processGlobalPreferenceChange(this);
   Preferences * eccePref = Preferences::getGlobalPref(true);
   eccePref->getString(PrefLabels::UNITFAMILY, p_unitFamily);

   p_ignoreCodeEventsFlag = true;
   wxCommandEvent event;
   OnChoicePlyrtCmpstSelected( event );
   OnTextctrlPlyrtDeg1Enter( event );
   OnTextctrlPlyrtDeg2Enter( event );
   OnTextctrlPlyrtDeg3Enter( event );
   OnChoicePlyrtCmpmdSelected( event );
   OnChoicePlyrtCmpaxSelected( event );
   p_ignoreCodeEventsFlag = false;

   Fit();
}


void Polyrate::initializeMessaging()
{
   // Inititialize messaging.
   p_msgDispatcher = new WxJMSMessageDispatch(WHOAMI, true);

   p_msgDispatcher->subscribe("ecce_invoke",
         this, (wxJmsCBFunc)&Polyrate::invokeMCB);
   p_msgDispatcher->subscribe("ecce_quit",
         this, (wxJmsCBFunc)&Polyrate::quitMCB);
   p_msgDispatcher->subscribe("ecce_identify",
         this, (wxJmsCBFunc)&Polyrate::identifyMCB);

   p_msgDispatcher->subscribe("ecce_url_renamed",
         this, (wxJmsCBFunc)&Polyrate::renameMCB);
   p_msgDispatcher->subscribe("ecce_url_removed",
         this, (wxJmsCBFunc)&Polyrate::removeMCB);

   p_msgDispatcher->subscribe("ecce_url_state",
         this, (wxJmsCBFunc)&Polyrate::stateMCB, false);
   p_msgDispatcher->subscribe("ecce_url_subject",
         this, (wxJmsCBFunc)&Polyrate::subjectMCB);
   p_msgDispatcher->subscribe("ecce_url_spin",
         this, (wxJmsCBFunc)&Polyrate::spinMCB);
   p_msgDispatcher->subscribe("ecce_url_property",
         this, (wxJmsCBFunc)&Polyrate::propertyMCB);

   p_msgDispatcher->subscribe("ecce_preferences_states",
         this, (wxJmsCBFunc)&Polyrate::statePrefsMCB);

   p_msgDispatcher->subscribe("ecce_auth_changed",
         this, (wxJmsCBFunc)&Polyrate::authMCB);

   p_msgDispatcher->startSubscriber();
   p_msgDispatcher->registerTopShell(this);
   p_msgDispatcher->notifyReady();

}


string Polyrate::getTitle() const
{
   return "POLYRATE Editor";
}


void Polyrate::invokeMCB(JMSMessage& msg)
{
   if (msg.containsProperty("calcurl")) {
      string url = msg.getProperty("calcurl");
      setContext(url);
      p_msgDispatcher->setPollContext(url);
      Raise();
   }
}


void Polyrate::quitMCB(JMSMessage& msg)
{
   if (msg.getSender().getName() == GATEWAY) {
      doQuit(true);
   }
}

/**
 * State color preferences have changed.
 * Update state icon in feedback area.
 */
void Polyrate::statePrefsMCB(JMSMessage& msg)
{
   p_feedback->readPreferences();
}


/**
 * Identify message call back function
 *
 * When gateway sent out identify message, periodic table app will response by sending a ID message back.
 */
void Polyrate::identifyMCB(JMSMessage&)
{
   ostrstream os;
   os << getTitle();

   // Add context info if we have context
   if (p_model != 0) {
      EcceURL url(p_model->getUrl());
      os << ": " << EDSIServerCentral().mapURLtoName(url);
   }
   os << ends;

   JMSPublisher publisher(WHOAMI);
   JMSMessage *msg = publisher.newMessage();
   msg->addProperty("info",os.str());
   publisher.publish("ecce_identify_reply", *msg);
   delete msg;
}


/**
 * Reset the gui to the task at the specified url.
 */
void Polyrate::setContext(const string& urlstr) throw (EcceException)
{
  wxBusyCursor busy;

  if (p_model) {

    // Set model's url
    p_model->setUrl(urlstr);

    try {

      // Get polyrateTask from factory
      p_polyrateTask = 
        dynamic_cast<PolyrateTask*>(EDSIFactory::getResource(EcceURL(urlstr)));
      if (p_polyrateTask == 0) {
        throw InvalidException("Could not get Polyrate task", WHERE);
      }

      // Get Session
      p_session = p_polyrateTask->getSession();
      if (p_session == 0) {
        throw InvalidException("Could not get Session", WHERE);
      }

      // Set run state and gui label and deserialize the GUI's model
      p_feedback->setRunState(p_polyrateTask->getState());
      updateDisabledState(p_feedback->getRunState(), true);
      EcceURL url = p_polyrateTask->getURL();
      EDSIServerCentral servers;
      p_feedback->setContextLabel(servers.mapURLtoName(url));
      p_feedback->setContextURL(url);
      p_polyrateTask->getTaskModel(*p_model);
    }
    catch (...) {
      this->Polyrate::~Polyrate();
      Destroy();
    }

  }

  // Refresh GUI with model
  initializeGUI();
  setSaveState(false);

  // For a created calc with the required component chemical systems defined,
  // immediately change to ready state and generate input file
  checkChemsysChange();
}


/**
 * Implementation of TaskApp::taskAppRaise.
 * Window is raised to front.
 */
void Polyrate::taskAppRaise()
{
   Raise();
}


/**
 * Implementation of TaskApp::taskAppShow.
 * Window is shown.
 */
void Polyrate::taskAppShow()
{
   Show(TRUE);
}


/**
 * Implementation of TaskApp::taskAppDestroy.
 * Window is destroyed.
 */
void Polyrate::taskAppDestroy()
{
   doQuit(true);
}



/**
 * quit but first handle any unsaved changes.
 * @param force without prompting even if error occurs during save
 */
void Polyrate::doQuit(const bool& force)
{
   if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
      long buttonFlags = wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION;
      if (!force) {
        buttonFlags |= wxCANCEL;
      }
      ewxMessageDialog prompt(this,"The current calculation has unsaved "
        "changes!  Do you want to save changes before quitting?",
        "Save POLYRATE Editor Changes?", buttonFlags);
      int status = prompt.ShowModal();
      if (status == wxID_YES) {
         try {
            saveModelAndInputFile();
         } catch (EcceException& ex) {
            p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
         }
      } else if (status == wxID_CANCEL) {
         // abort the quit - not sure if this will cause problems...
         return;
      }
   }

   Destroy();
}

/**
 * Save the model, generate and save input file.
 */
void Polyrate::saveModelAndInputFile() throw(IOException)
{
   PolyrateModel& model = getPolyrateModel();
   p_polyrateTask->setTaskModel(&model);
   generateInputFile();

   // Save TS fragment, if any, to Polyrate vdoc. This is structure that
   // will appear in calcviewer.
   DirDyVTSTTask *vtst_task = p_polyrateTask->getDirdyTask();
   Fragment *frag = vtst_task->getFragment(DirDyVTSTTask::TRANSITION);
   if (frag != 0) {
     p_polyrateTask->DavCalculation::fragment(frag);
     delete frag;
   }
   setSaveState(false);
}


/**
 * Implementation of TaskApp::setUnitFamily.
 * All unitted fields are updated to reflect the new unit family.
 */
void Polyrate::setUnitFamily(const string& family)
{
   ewxWindowUtils::setUnitFamily(this, family);
   Fit();
}



/**
 * Creates a new PrepareModel instance.
 * If there is an existing instance, it is destroyed.
 */
void Polyrate::initializeModel()
{
  if (p_model != 0) {
     delete p_model;
  }
  p_model = new PolyrateModel();

}


void Polyrate::processSave()
{
  wxBusyCursor busy;
  try {
    if (p_feedback->getEditStatus() == WxFeedback::MODIFIED)
      saveModelAndInputFile();
  } catch (EcceException& ex) {
    p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
  }
}


void Polyrate::doSaveButtonClick(wxMouseEvent &event)
{
  processSave();
  event.Skip();
}

/**
 * Send message(s) to request startup of launcher.
 */
void Polyrate::requestLauncher(const EcceURL &url, bool forceNew)
{
   processSave();

   JMSPublisher publisher(WHOAMI);
   JMSMessage *msg = publisher.newMessage();
   msg->addProperty("action","start");
   publisher.publish("ecce_activity",*msg);
   delete msg;

   // Send the start app request to gateway:
   Target gateway(GATEWAY, "");
   msg = publisher.newMessage(gateway);
   publisher.invoke(*msg, LAUNCHER, 0, url.toString());
   delete msg;
}

/**
 * A resource was renamed.
 */
void Polyrate::renameMCB(JMSMessage& msg)
{
  string oldURL = msg.getProperty("oldurl");
  string newURL = msg.getProperty("newurl");

  if (p_polyrateTask && p_polyrateTask->getURL().isChildOrMe(oldURL)) {
    EDSIFactory::renamePoolResource(oldURL, newURL);
    string calcurl = p_polyrateTask->getURL().toString();
    p_feedback->setContextLabel(EDSIServerCentral().mapURLtoName(calcurl));
    setContext(calcurl);
  }
}

/**
 * A resource was removed.
 */
void Polyrate::removeMCB(JMSMessage& msg)
{
  string url = msg.getProperty("url");
  if (p_polyrateTask && p_polyrateTask->getURL().isChildOrMe(url)) {
    EDSIFactory::removePoolResource(url);

    doQuit(true);
  }
}

/**
 * State has changed.
 */
void Polyrate::stateMCB(JMSMessage& msg)
{
   string fullurl = msg.getProperty("url");
   string url = fullurl; 

   string durl = p_polyrateTask->getURL().toString();

   if (url != durl) {
     string::size_type slash;
     if ((slash = url.rfind('/')) != string::npos) {
       if (url.length() == slash+1) {
         url.resize(slash);
         if ((slash = url.rfind('/')) != string::npos)
           url.resize(slash);
       } else {
         url.resize(slash);
       }
     }

     if ((slash = durl.rfind('/')) != string::npos)
       durl.resize(slash);

     if (url == durl) {
       EDSIFactory::changePoolResource(fullurl);

       string statestr = msg.getProperty("state");
       ResourceDescriptor::RUNSTATE state =
         ResourceUtils::stringToState(statestr);
       if (state == ResourceDescriptor::STATE_COMPLETED  ||
           state == ResourceDescriptor::STATE_UNSUCCESSFUL ||
           state == ResourceDescriptor::STATE_KILLED ||
           state == ResourceDescriptor::STATE_FAILED) {
         checkChemsysChange();
       }
     }
   } else {
     EDSIFactory::changePoolResource(fullurl);

     string statestr = msg.getProperty("state");
     ResourceDescriptor::RUNSTATE state =
                                  ResourceUtils::stringToState(statestr);

     // make sure this is actually a new run state
     if (state != p_feedback->getRunState()) {
       updateDisabledState(state);
       p_feedback->setRunState(state);
       if (state >= ResourceDescriptor::STATE_SUBMITTED) {
          p_feedback->setEditStatus(WxFeedback::READONLY);
       } else if (!isUnsavedState()) {
          p_feedback->setEditStatus(WxFeedback::EDIT);
       }
     }
   }
}


void Polyrate::updateDisabledState(ResourceDescriptor::RUNSTATE state, bool forceMsg)
{
  if (state >= ResourceDescriptor::STATE_SUBMITTED) {
    setFrameCustomDisabledStyle(false);

    if (forceMsg || state == ResourceDescriptor::STATE_SUBMITTED) {
      p_feedback->setMessage("This calculation is read only and setup cannot "
                             "be changed without terminating during the run or "
                             "resetting after completion.", WxFeedback::INFO);
    }
  } else {
    setFrameCustomDisabledStyle(true);
  }

  // Final Edit and Launch buttons should never be disabled
  ewxButton *btn = ((ewxButton*)FindWindow(ID_BUTTON_PLYRT_FINAL_EDIT));
  btn->setCustomDisabledStyle(true);
  btn = ((ewxButton*)FindWindow(ID_BUTTON_PLYRT_LAUNCH));
  btn->setCustomDisabledStyle(true);
}


/**
 * A molecule has changed.
 */
void Polyrate::subjectMCB(JMSMessage& msg)
{
   string fullurl = msg.getProperty("url");
   string url = fullurl;

   string durl = p_polyrateTask->getURL().toString();

   string::size_type slash;
   if ((slash = url.rfind('/')) != string::npos)
     url.resize(slash);
   if ((slash = durl.rfind('/')) != string::npos)
     durl.resize(slash);

   if (url == durl) {
     EDSIFactory::changePoolResource(fullurl);

     checkChemsysChange();
   }
}


/**
 * A molecule's spin has changed.
 */
void Polyrate::spinMCB(JMSMessage& msg)
{
   string fullurl = msg.getProperty("url");
   string url = fullurl;

   string durl = p_polyrateTask->getURL().toString();

   string::size_type slash;
   if ((slash = url.rfind('/')) != string::npos)
     url.resize(slash);
   if ((slash = durl.rfind('/')) != string::npos)
     durl.resize(slash);

   if (url == durl) {
     EDSIFactory::changePoolResource(fullurl);

     checkChemsysChange();
   }
}


void Polyrate::propertyMCB(JMSMessage& msg)
{
   string url = msg.getProperty("url");

   string durl = p_polyrateTask->getURL().toString();

   if (url == durl) {
     EDSIFactory::changePoolResource(url);

     // The user can change the annotation even for >ready state calculations
     // so make sure to check that to avoid overwriting the input file
     if (p_polyrateTask->getState() <= ResourceDescriptor::STATE_READY) {
       generateInputFile();
     }
   }
}


void Polyrate::authMCB(JMSMessage& msg)
{
  AuthCache::getCache().msgIn(msg, p_msgDispatcher->getMyID());
}


/**
 * Process user close button.
 * Create a window close event where save processing is handled.
 */
void Polyrate::OnMenuPlyrtCloseClick( wxCommandEvent& event )
{
    // This generates window close so no need to repeat save code here.
    Close(true);

    event.Skip();
}


/**
 * Make sure changes are saved if any.
 * @todo prompt them about unsaved changes instead of just doing it
 */
void Polyrate::OnCloseWindow( wxCloseEvent& event )
{
   doQuit(false);
}


/**
 * Re-generate the input file from the model and upload it to the data server.
 * @throw IOException if the input file generation failed for some reason or
 *       we couldn't upload the data.
 */
void Polyrate::generateInputFile() throw(IOException)
{
  SDirectory *dir =
    TempStorage::getTempJobDirectory(p_polyrateTask->getURL().getFilePathTail().c_str());
  string path = dir->path();

  //  Find all geometry files
  string ts,r1,r2,p1,p2;
  if (checkComponent(DirDyVTSTTask::TRANSITION)) {
    ts = path;
    ts.append("/ts.chemsys");
    ofstream outfile(ts.c_str());
    DirDyVTSTTask *dirdyTask = p_polyrateTask->getDirdyTask();
    if (dirdyTask != 0) {
      Fragment *fragment = dirdyTask->getFragment(DirDyVTSTTask::TRANSITION);
      dirdyTask = 0;
      if (fragment != 0) {
        fragment->dumpMVM(outfile);
        delete fragment;
      }
    }
    outfile.flush();
    outfile.close();
  }
  if (checkComponent(DirDyVTSTTask::REACT1)) {
    r1 = path;
    r1.append("/r1.chemsys");
    ofstream outfile(r1.c_str());
    DirDyVTSTTask *dirdyTask = p_polyrateTask->getDirdyTask();
    if (dirdyTask != 0) {
      Fragment *fragment = dirdyTask->getFragment(DirDyVTSTTask::REACT1);
      dirdyTask = 0;
      if (fragment != 0) {
        fragment->dumpMVM(outfile);
        delete fragment;
      }
    }
    outfile.flush();
    outfile.close();
  }
  if (checkComponent(DirDyVTSTTask::REACT2)) {
    r2 = path;
    r2.append("/r2.chemsys");
    ofstream outfile(r2.c_str());
    DirDyVTSTTask *dirdyTask = p_polyrateTask->getDirdyTask();
    if (dirdyTask != 0) {
      Fragment *fragment = dirdyTask->getFragment(DirDyVTSTTask::REACT2);
      dirdyTask = 0;
      if (fragment != 0) {
        fragment->dumpMVM(outfile);
        delete fragment;
      }
    }
    outfile.flush();
    outfile.close();
  }
  if (checkComponent(DirDyVTSTTask::PROD1)) {
    p1 = path;
    p1.append("/p1.chemsys");
    ofstream outfile(p1.c_str());
    DirDyVTSTTask *dirdyTask = p_polyrateTask->getDirdyTask();
    if (dirdyTask != 0) {
      Fragment *fragment = dirdyTask->getFragment(DirDyVTSTTask::PROD1);
      dirdyTask = 0;
      if (fragment != 0) {
        fragment->dumpMVM(outfile);
        delete fragment;
      }
    }
    outfile.flush();
    outfile.close();
  }
  if (checkComponent(DirDyVTSTTask::PROD2)) {
    p2 = path;
    p2.append("/p2.chemsys");
    ofstream outfile(p2.c_str());
    DirDyVTSTTask *dirdyTask = p_polyrateTask->getDirdyTask();
    if (dirdyTask != 0) {
      Fragment *fragment = dirdyTask->getFragment(DirDyVTSTTask::PROD2);
      dirdyTask = 0;
      if (fragment != 0) {
        fragment->dumpMVM(outfile);
        delete fragment;
      }
    }
    outfile.flush();
    outfile.close();
  }

  // Get NWDirDy VTST parameters
  string spath = path;
  spath.append("/param.path");
  ofstream sOutfile(spath.c_str());
  DirDyVTSTTask *dirdyTask = p_polyrateTask->getDirdyTask();
  NWDirdyModel dirdyModel;
  dirdyTask->getTaskModel(dirdyModel);
  sOutfile << dirdyTask->serializeModel(&dirdyModel);
  sOutfile.flush();
  sOutfile.close();

  spath = path;
  spath.append("/param.poly");
  ofstream rOutfile(spath.c_str());
  rOutfile << p_polyrateTask->serializeModel(p_model);
  rOutfile.flush();
  rOutfile.close();

  // Retrieve the user annotation (or fall back to the task name)
  string annotation = p_polyrateTask->getName();
  Resource *resource = dynamic_cast<Resource*>(p_polyrateTask);
  if (resource != (Resource*)0) {
    string annoprop = resource->getProp(VDoc::getEcceNamespace() +
                                        ":annotation");

    // remove any whitespace from the annoation including newlines
    // this is necessary because the annotation editor in the Organizer
    // is rather casual about whitespace and we want to make sure we
    // get something valid for this field for the input file
    STLUtil::stripLeadingAndTrailingWhiteSpace(annoprop);

    // If you clear an annotation in the Organizer, it actually leaves
    // a newline character
    if (annoprop != "") {
      if (annoprop.size() > 80)
        annoprop.resize(80);
      annotation = annoprop;
    }
  }

  spath = path;
  spath.append("/param.mult");
  ofstream mOutfile(spath.c_str());
  mOutfile << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>" << endl;
  mOutfile << "<FragmentMultiplicities>" << endl;
  DavCalculation *calc = getComponentTask(DirDyVTSTTask::TRANSITION);
  if (calc)
    mOutfile << "<TSMultiplicity>" << (int)calc->spinMultiplicity()-1 << "</TSMultiplicity>" << endl;

  calc = getComponentTask(DirDyVTSTTask::REACT1);
  if (calc)
    mOutfile << "<R1Multiplicity>" << (int)calc->spinMultiplicity()-1 << "</R1Multiplicity>" << endl;

  calc = getComponentTask(DirDyVTSTTask::REACT2);
  if (calc)
    mOutfile << "<R2Multiplicity>" << (int)calc->spinMultiplicity()-1 << "</R2Multiplicity>" << endl;

  calc = getComponentTask(DirDyVTSTTask::PROD1);
  if (calc)
    mOutfile << "<P1Multiplicity>" << (int)calc->spinMultiplicity()-1 << "</P1Multiplicity>" << endl;

  calc = getComponentTask(DirDyVTSTTask::PROD2);
  if (calc)
    mOutfile << "<P2Multiplicity>" << (int)calc->spinMultiplicity()-1 << "</P2Multiplicity>" << endl;

  // Just tack on the user annotation so no special logic is needed to retrieve
  // it in the input file generation script
  mOutfile << "<Annotation>" << annotation << "</Annotation>" << endl;

  mOutfile << "</FragmentMultiplicities>" << endl;
  mOutfile.flush();
  mOutfile.close();

  // Get the input generator script
  string inputGenFile;
  const JCode *codecap = p_polyrateTask->application();
  if (!codecap) {
    throw IOException("Could not retrieve POLYRATE's EDML file.", WHERE);
  }
  codecap->get_string("InputGenerator",inputGenFile);
  if (inputGenFile.empty()) {
    throw IOException("Could not find RX input generator file name", WHERE);
  }

  SFile *tmpInput = TempStorage::getTempFile();
  string cmd = Ecce::ecceDataControllersPath();
  cmd += "/" + inputGenFile;

  string args = " -prm";
  if (checkComponent(DirDyVTSTTask::TRANSITION)) {
    args.append("t");
  }
  if (checkComponent(DirDyVTSTTask::REACT1)) {
    args.append("c");
  }
  if (checkComponent(DirDyVTSTTask::REACT2)) {
    args.append("d");
  }
  if (checkComponent(DirDyVTSTTask::PROD1)) {
    args.append("e");
  }
  if (checkComponent(DirDyVTSTTask::PROD2)) {
    args.append("f");
  }
  cmd += args;
  cmd += " > ";
  cmd += tmpInput->path();

  chdir(path.c_str());
  CommandWrapper syscmd(cmd);
  syscmd.execute();

  // copy input file to web server
  ifstream ifs(tmpInput->path().c_str());
  if (!p_polyrateTask->putInputFile(codecap->getCodeFile(JCode::PRIMARY_INPUT).name(), &ifs)) {
    p_feedback->setMessage("Input file copy to web server failed.",
       WxFeedback::ERROR);
    return;
  }
  ifs.close();

  tmpInput->remove();
  delete tmpInput;

  dir->remove();
  delete dir;
}

/** 
 * Check to see if a component exists that is part of the NWDirdy Task that
 * is an input provider for the Polyrate task.
 * 
 * @param comp type of component to add
 *
 * @return returns true if the component exists.  Otherwise false is returned.
 *
 */

bool Polyrate::checkComponent(DirDyVTSTTask::Component comp)
{
  bool ret = false;

  DirDyVTSTTask *vtst_task = p_polyrateTask->getDirdyTask();
  DavCalculation *calc = 0;

  if (vtst_task != 0) {
    calc = vtst_task->getComponentTask(comp);
    vtst_task = 0;
  }

  // If calculation exists, return true
  if (calc != 0) {
    ret = true;
    calc = 0;
  }

  return ret;
}

Fragment* Polyrate::getComponentFragment(DirDyVTSTTask::Component comp)
{
  Fragment *frag = (Fragment*)0;
  DirDyVTSTTask *vtst_task = p_polyrateTask->getDirdyTask();
  DavCalculation *calc = vtst_task->getComponentTask(comp);
  if (calc) {
    frag = vtst_task->getFragment(comp);
  }
  return frag;
}

DavCalculation* Polyrate::getComponentTask(DirDyVTSTTask::Component comp)
{
  DirDyVTSTTask *vtst_task = p_polyrateTask->getDirdyTask();
  DavCalculation *calc = vtst_task->getComponentTask(comp);
  return calc;
}

void Polyrate::initProgressDialog()
{
   if (p_progressDialog != 0 ) {
      delete p_progressDialog;
      p_progressDialog = 0;
   }

   p_progressDialog = new ewxProgressDialog("ECCE POLYRATE Run", "",100,
         this, wxPD_AUTO_HIDE|wxPD_CAN_ABORT|wxPD_ELAPSED_TIME);
}


void Polyrate::OnButtonPlyrtLaunchClick(wxCommandEvent &event)
{
  requestLauncher(p_polyrateTask->getURL());
  event.Skip();
}

/**
 * Changes gui to show the user whether there are unsaved changes.
 */
void Polyrate::setSaveState(bool unsavedChanges)
{
   if (!p_ignoreCodeEventsFlag) {
      wxMenuItem *item = 0;
      wxMenuBar *menuBar = GetMenuBar();
      if (menuBar) {
         item = menuBar->FindItem(ID_MENU_PLYRT_SAVE);
      }

      if (item) item->Enable(unsavedChanges);
      if (unsavedChanges) {
         if (p_feedback->getEditStatus() != WxFeedback::MODIFIED) {
           setRunState(ResourceDescriptor::STATE_READY);
         }
         p_feedback->setEditStatus(WxFeedback::MODIFIED);
      }  else {
         p_feedback->setEditStatus(WxFeedback::EDIT);
      }
   }
}

void Polyrate::checkChemsysChange()
{
  // if the state is already "ready", a new input file and save model
  // should be done.
  // if the state is created, this method should check whether a
  // transition state, reactant 1, and product 1 have been defined.
  // if so, the state should be changed to ready and an input file should
  // be generated plus a model save.  If not, do nothing.
  if (p_polyrateTask->getState() == ResourceDescriptor::STATE_READY) {
    saveModelAndInputFile();
  } else if (p_polyrateTask->getState() == ResourceDescriptor::STATE_CREATED &&
             checkComponent(DirDyVTSTTask::TRANSITION) &&
             checkComponent(DirDyVTSTTask::REACT1) &&
             checkComponent(DirDyVTSTTask::PROD1)) {
    saveModelAndInputFile();
    readyStateChange();
  }
}


void Polyrate::readyStateChange()
{
  p_polyrateTask->setState(ResourceDescriptor::STATE_READY);
  p_feedback->setRunState(ResourceDescriptor::STATE_READY);

  // Notify others...
  JMSPublisher publisher(WHOAMI);
  JMSMessage *msg = publisher.newMessage();
  msg->addProperty("url", p_polyrateTask->getURL().toString());
  msg->addProperty("state", ResourceUtils::stateToString(
                                             ResourceDescriptor::STATE_READY));
  publisher.publish("ecce_url_state",*msg);
  delete msg;
}


/**
 * Set calculation state on data server, update feedback area,
 * and notify others that the state has changed.
 */
void Polyrate::setRunState(ResourceDescriptor::RUNSTATE state) 
{
  p_polyrateTask->setState(state);
  p_feedback->setRunState(state);

  // Notify others...
  JMSPublisher publisher(WHOAMI);
  JMSMessage *msg = publisher.newMessage();
  msg->addProperty("url", p_polyrateTask->getURL().toString());
  msg->addProperty("state", ResourceUtils::stateToString(state));
  publisher.publish("ecce_url_state",*msg);
  delete msg;
}


/**
 * Determines if there are unsaved changes.
 * This just looks at the gui indicators so don't do something like
 * call isUnsavedState to determine whether to call setSaveState.
 */
bool Polyrate::isUnsavedState()
{
   return (p_feedback->getEditStatus() == WxFeedback::MODIFIED);
}


/**
 * Process menu save button click.
 * The data is saved to the server and the gui updated.
 */
void Polyrate::OnMenuPlyrtSaveClick( wxCommandEvent& event )
{
  processSave();
  event.Skip();
}



void Polyrate::OnMenuFeedbackClick( wxCommandEvent& event )
{
  event.Skip();
}



void Polyrate::OnMenuPlyrtHelpClick( wxCommandEvent& event )
{
  event.Skip();
}



void Polyrate::OnButtonPlyrtFinalEditClick( wxCommandEvent& event )
{
  // Save if needed
  if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
     saveModelAndInputFile();
  }

  // Generate an input file if one doesn't exist
  if (p_polyrateTask->getDataFileNames(JCode::PRIMARY_INPUT).empty()) {
    generateInputFile();
  }

  istream* is = p_polyrateTask->getDataFile(JCode::PRIMARY_INPUT);
  if (is) {
     bool roFlag = p_feedback->getRunState() >
                   ResourceDescriptor::STATE_READY;
     if (roFlag) {
       p_feedback->setMessage("Input file is read only.  "
                              "Changes cannot be saved.", WxFeedback::INFO);
     } else {
       p_feedback->setMessage("In order for Final Edit changes to be"
                              " applied you must launch the task "
                              "without making any further changes.",
                              WxFeedback::INFO);
     }

     string text;
     StringConverter::streamToText(*is, text);
     delete is;

     WxEditSessionMgr sessionMgr;
     sessionMgr.edit(text, "testing", this, roFlag,"POLYRATE Input");

  } else {
     p_feedback->setMessage("Input file not found.", WxFeedback::WARNING);
  }

  event.Skip();
}



/**
 * EditListener interface.
 * An edit session has terminated.
 */
void Polyrate::processEditCompletion(const EditEvent& ee)
{
   wxBusyCursor busy;
   ifstream ifs(ee.filename.c_str());

   string infile;
   TypedFile tinfile;

   p_polyrateTask->getDataFile(JCode::PRIMARY_INPUT, tinfile);
   infile = tinfile.name();

   if (!p_polyrateTask->putInputFile(infile, &ifs)) {
      p_feedback->setMessage("Input file could not be copied back to DAV",
            WxFeedback::ERROR);
   } else {
      p_feedback->setMessage("The input file has been saved. Any changes "
            "you make to the gui will override your manual edits.");
   }

   ifs.close();
}

/**
 * Event handlers for GUI buttons, etc.
 */

void Polyrate::OnCheckboxPlyrtFroptClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_PLYRT_FROPT));
  getPolyrateModel().setFrwrdRevOpt(check->GetValue());
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnTextctrlPlyrtSclrvEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SCLRV));
  getPolyrateModel().setReverseRxScaleExp(text->getValueAsInt());
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnTextctrlPlyrtSgmfEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SGMF));
  getPolyrateModel().setForwardSymmetryFactor(text->getValueAsInt());
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnTextctrlPlyrtSgmrEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SGMR));
  getPolyrateModel().setReverseSymmetryFactor(text->getValueAsInt());
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnTextctrlPlyrtFsclEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_FSCL));
  getPolyrateModel().setFrequencyScale(text->getValueAsDouble());
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnCheckboxPlyrtAnlysClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_PLYRT_ANLYS));
  getPolyrateModel().setAnalysisFlag(check->GetValue());
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnCheckboxPlyrtSddlClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_PLYRT_SDDL));
  getPolyrateModel().setSaddleFlag(check->GetValue());
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnButtonPlyrtAddClick( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_TMPRTR));
  ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_TMPLST));

  double newTemp = text->getValueAsDouble();
  char buf[32];
  PolyrateModel &model = getPolyrateModel();
  bool addedFlag = true;

  // special case if the user is quick on the draw and is able to add in a
  // negative temp before the validation resets it, just ignore it
  if (newTemp < 0.0)
    return;

  list->Clear();
  int ntemps = model.getTemperatureListSize();

  if (ntemps > 0) {
    int idx;
    double temp;
    bool lessFlag = true;

    // the following two loops contain the logic of determining whether
    // the newly added temp is a duplicate with an existing value.  Since
    // I needed to iterate through all the temps anyway to re-build the
    // gui listbox, might as well optimize it to check for a duplicate at the
    // same time.
    for (idx=0; idx<ntemps && lessFlag; idx++) {
      temp = model.getTemperature(idx);

      if (temp < newTemp) {
        // no-op for temps less than the one being added
      } else if (temp - newTemp < 1.0e-6) {
        // duplicate temp (close enough at least) so get out of this loop
        // without adding the new temp to the model
        // don't need an absolute value call because we already know that
        // temp is >= newTemp by the previous check
        addedFlag = false;
        lessFlag = false;
      } else {
        // insert new non-duplicate temp here, update index because of new
        // insertion to model vector and also exit this loop
        idx++;
        lessFlag = false;

        model.addTemperature(newTemp);
        sprintf(buf,"%18.6f", newTemp);
        list->Append(buf);
      }
      sprintf(buf,"%18.6f", temp);
      list->Append(buf);
    }
    if (lessFlag && idx>=ntemps) {
      // new temp is at end
      model.addTemperature(newTemp);
      sprintf(buf,"%18.6f", newTemp);
      list->Append(buf);
    } else {
      // quickly step through remaining temps greater than the new one
      for (; idx<ntemps; idx++) {
        sprintf(buf,"%18.6f", model.getTemperature(idx));
        list->Append(buf);
      }
    }
  } else {
    // no existing temperatures--just add the new one
    model.addTemperature(newTemp);
    sprintf(buf,"%18.6f", newTemp);
    list->Append(buf);
  }

  if (addedFlag)
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnButtonPlyrtDeleteClick( wxCommandEvent& event )
{
  wxArrayInt selections;
  ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_TMPLST));
  list->GetSelections(selections);
  if (selections.size() > 0) {
    int selected = selections[0];
    list->Delete(selected);
    getPolyrateModel().deleteTemperature(selected);
    if (selected > 0) {
      list->SetSelection(selected-1, true);
    }
  }
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnChoicePlyrtCmpstSelected( wxCommandEvent& event )
{
  // Need to get current unit family so that conversion can
  // be implemented
  PolyrateModel &model = getPolyrateModel();
  int idx;
  ewxTextCtrl *text;
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPST));
  string component = convertString(menu->GetStringSelection().c_str());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG1));
  idx = model.getDegeneracy1(component);
  text->setValueAsInt(idx);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG1));
  text->setValueAsDouble(model.getElectronicEnergy1(component),p_unitFamily);
  if (idx > 0) {
    text->Enable(true);
  } else {
    text->Enable(false);
  }
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG2));
  idx = model.getDegeneracy2(component);
  text->setValueAsInt(idx);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG2));
  if (model.getElectronicEnergy2(component) == 0.0) {
    text->SetValue("");
  } else {
    text->setValueAsDouble(model.getElectronicEnergy2(component),p_unitFamily);
  }
  if (idx > 0) {
    text->Enable(true);
  } else {
    text->Enable(false);
  }
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG3));
  idx = model.getDegeneracy3(component);
  text->setValueAsInt(idx);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG3));
  if (model.getElectronicEnergy3(component) == 0.0) {
    text->SetValue("");
  } else {
    text->setValueAsDouble(model.getElectronicEnergy3(component),p_unitFamily);
  }
  if (idx > 0) {
    text->Enable(true);
  } else {
    text->Enable(false);
  }

  event.Skip();
}

void Polyrate::OnTextctrlPlyrtDeg1Enter( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG1));
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPST));
  string component = convertString(menu->GetStringSelection().c_str());
  model.setDegeneracy1(component,text->getValueAsInt());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG1));
  if (model.getDegeneracy1(component) > 0) {
    text->Enable(true);
  } else {
    text->Enable(false);
  }
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnTextctrlPlyrtNrg1Enter( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG1));
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPST));
  string component = convertString(menu->GetStringSelection().c_str());
  double rval = text->getBaseValueAsDouble();
  if (rval != 0.0) {
    text->setValueAsDouble(0.0);
    model.setElectronicEnergy1(component,0.0);
    p_feedback->setMessage("Ground state energy must be zero",WxFeedback::ERROR);
  }
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnTextctrlPlyrtDeg2Enter( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG2));
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPST));
  string component = convertString(menu->GetStringSelection().c_str());
  model.setDegeneracy2(component,text->getValueAsInt());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG2));
  if (model.getDegeneracy2(component) > 0) {
    text->Enable(true);
  } else {
    text->Enable(false);
  }
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnTextctrlPlyrtNrg2Enter( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG1));
  double rval1 = text->getBaseValueAsDouble();
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG3));
  double rval3 = text->getBaseValueAsDouble();
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG2));
  double rval2 = text->getBaseValueAsDouble();
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPST));
  string component = convertString(menu->GetStringSelection().c_str());
  if (rval2 != rval1 && rval2 != rval3) {
    model.setElectronicEnergy2(component,rval2);
  } else {
    if (model.getElectronicEnergy2(component) == 0.0) {
      text->SetValue("");
    } else {
      text->setValueAsDouble(model.getElectronicEnergy2(component),p_unitFamily);
    }
    p_feedback->setMessage("Value must be different from other energies",
                           WxFeedback::ERROR);
  }
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnTextctrlPlyrtDeg3Enter( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG3));
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPST));
  string component = convertString(menu->GetStringSelection().c_str());
  model.setDegeneracy3(component,text->getValueAsInt());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG3));
  if (model.getDegeneracy3(component) > 0) {
    text->Enable(true);
  } else {
    text->Enable(false);
  }
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnTextctrlPlyrtNrg3Enter( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG1));
  double rval1 = text->getBaseValueAsDouble();
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG2));
  double rval2 = text->getBaseValueAsDouble();
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG3));
  double rval3 = text->getBaseValueAsDouble();
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPST));
  string component = convertString(menu->GetStringSelection().c_str());
  if (rval3 != rval1 && rval3 != rval2) {
    model.setElectronicEnergy3(component,rval3);
  } else {
    if (model.getElectronicEnergy3(component) == 0.0) {
      text->SetValue("");
    } else {
      text->setValueAsDouble(model.getElectronicEnergy3(component),p_unitFamily);
    }
    p_feedback->setMessage("Value must be different from other energies",
                           WxFeedback::ERROR);
  }
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnButtonPlyrtClrstClick( wxCommandEvent& event )
{
  int i;
  PolyrateModel &model = getPolyrateModel();
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPST));
  ewxTextCtrl *text;
  string component;
  for (i=0; i<5; i++) {
    if (i == 0) {
      component = "react1";
    } else if (i == 1) {
      component = "react2";
    } else if (i == 2) {
      component = "transition";
    } else if (i == 3) {
      component = "prod1";
    } else if (i == 4) {
      component = "prod2";
    }
    if (model.getComponentStatus(component)){
      model.setDegeneracy1(component, model.DEG1);
      model.setElectronicEnergy1(component, model.ES1);
      model.setDegeneracy2(component, model.DEG2);
      model.setElectronicEnergy2(component, model.ES2);
      model.setDegeneracy3(component, model.DEG3);
      model.setElectronicEnergy3(component, model.ES3);
    }
  }
  menu->SetSelection(0);
  component = convertString(menu->GetStringSelection().c_str());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG1));
  text->setValueAsInt(model.getDegeneracy1(component));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG1));
  text->setValueAsDouble(model.getElectronicEnergy1(component));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG2));
  text->setValueAsInt(model.getDegeneracy2(component));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG2));
  text->setValueAsDouble(model.getElectronicEnergy2(component));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DEG3));
  text->setValueAsInt(model.getDegeneracy3(component));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_NRG3));
  text->setValueAsDouble(model.getElectronicEnergy3(component));
  OnTextctrlPlyrtDeg1Enter( event );
  OnTextctrlPlyrtDeg2Enter( event );
  OnTextctrlPlyrtDeg3Enter( event );
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnChoicePlyrtCmpmdSelected( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
  string component = convertString(menu->GetStringSelection().c_str());
  ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_TRSN));
  list->Clear();
  int numModes = model.getNumModes(component);
  int imode, idx;
  string cmd;
  // Set torsion mode list
  int icnt = 0;
  for (idx=0; idx<numModes; idx++) {
    imode = model.getMode(component,idx);
    if (model.getAnharmonicity(component,imode) == "tor") {
      cmd = model.getTorsion(component,imode);
      list->Append(cmd.c_str());
      icnt++;
    }
  }
  setModeMenu();
  if (icnt > 0) {
    list->SetSelection(0);
    OnListboxPlyrtTrsnSelected( event );
  }
  ewxPanel *panel = ((ewxPanel*)FindWindow(ID_PANEL_PLYRT_TRSNS));
  if (model.getNumAtoms(component)<4 || model.getLinearStatus(component)) {
    panel->Enable(false);
  } else {
    panel->Enable(true);
  }

  // Set stretch mode list
  list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_MRS));
  list->Clear();
  icnt = 0;
  char buf[120];
  for (idx=0; idx<numModes; idx++) {
    imode = model.getMode(component,idx);
    if (model.getAnharmonicity(component,imode) == "morse") {
      sprintf(buf,"  %4i",imode);
      cmd = buf;
      cmd.append(" morse");
      list->Append(cmd.c_str());
      icnt++;
    }
  }
  if (icnt > 0) {
    list->SetSelection(0);
    OnListboxPlyrtMrsSelected( event );
  }

  event.Skip();
}

void Polyrate::OnButtonPlyrtMrsaddClick( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_MRSMD));
  int mode = text->getValueAsInt();
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
  string component = convertString(menu->GetStringSelection().c_str());

  // Check to make sure that mode is not being used
  int numModes = model.getNumModes(component);
  int i;
  bool notUsed = true;
  for (i=0; i<numModes; i++) {
    if (mode == model.getMode(component,i)) {
      notUsed = false;
      break;
    }
  }


  if (notUsed) {
    model.addMode(component,mode);
    model.setAnharmonicity(component, mode, "morse");
    string cmd;
    char buf[128];
    sprintf(buf,"  %4i morse",mode);
    ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_MRS));
    if ((int)list->GetCount()<model.getNumStretchModes(component)) {
      list->Append(buf);
    }
  } else {
    p_feedback->setMessage("This mode is already assigned. You must delete "
                           " it before reassigning properties",WxFeedback::ERROR);
  }
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnButtonPlyrtMrsdelClick( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  int mode;
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
  string component = convertString(menu->GetStringSelection().c_str());
  ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_MRS));
  string cmd = list->GetStringSelection().c_str();
  int idx = list->GetSelection();
  list->Delete(idx);
  model.translateMorse(cmd,mode);
  model.deleteMode(component,mode);
  int nitems = list->GetCount();
  list->SetSelection(nitems-1);
  cmd = list->GetStringSelection().c_str();
  model.translateMorse(cmd,mode);
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_MRSMD));
  text->setValueAsInt(mode);
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnListboxPlyrtMrsSelected( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  int mode;
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
  string component = convertString(menu->GetStringSelection().c_str());
  ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_MRS));
  string cmd = list->GetStringSelection().c_str();
  model.translateMorse(cmd,mode);
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_MRSMD));
  text->setValueAsInt(mode);
  event.Skip();
}

void Polyrate::OnButtonPlyrtTrsnadClick( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxTextCtrl *text;
  ewxChoice *menu;
  ewxListBox *list;
  char buf[32];
  int imode;
  string component;
  menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
  component = convertString(menu->GetStringSelection().c_str());
  string cmd = "  ";
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_TRSNMD));
  int ival = text->getValueAsInt();

  // Check to make sure that mode is not being used
  int numModes = model.getNumModes(component);
  int i;
  bool notUsed = true;
  for (i=0; i<numModes; i++) {
    if (ival == model.getMode(component,i)) {
      notUsed = false;
      break;
    }
  }

  imode = ival;
  if (notUsed) {
    string scheme, level;
    int dmin;
    model.addMode(component,imode);
    model.setAnharmonicity(component,imode,"tor");
    sprintf(buf,"%4i",imode);
    cmd.append(buf);
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DMIN));
    ival = text->getValueAsInt();
    dmin = ival;
    sprintf(buf,"%4i",ival);
    cmd.append(buf);
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SYMNUM));
    ival = text->getValueAsInt();
    sprintf(buf,"%4i",ival);
    cmd.append(buf);
    menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_SCHM));
    string selection = menu->GetStringSelection().c_str();
    if (selection == "Frequency-Barrier") {
      cmd.append("  OW");
      scheme = "OW";
    } else if (selection == "Frequency-Rectilinear") {
      cmd.append("  RO");
      scheme = "RO";
    } else if (selection == "Frequency-Curvilinear") {
      cmd.append("  CO");
      scheme = "CO";
    } else if (selection == "Barrier-Rectilinear") {
      cmd.append("  RW");
      scheme = "RW";
    } else if (selection == "Barrier-Curvilinear") {
      cmd.append("  CW");
      scheme = "CW";
    }
    menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_APLVL));
    selection = menu->GetStringSelection().c_str();
    if (selection == "Full") {
      cmd.append("  FULL");
      level = "FULL";
    } else if (selection == "Single Conformer") {
      cmd.append("    SC");
      level = "SC";
    } else if (selection == "Single Frequency") {
      cmd.append("    SF");
      level = "SF";
    }
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SBGRP));
    ival = text->getValueAsInt();
    sprintf(buf,"%4i",ival);
    cmd.append(buf);
    list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_TRSN));
    if ((int)list->GetCount()<model.getNumTorsionModes(component)) {
      list->Append(cmd.c_str());
    }
    model.setTorsion(component,imode,cmd);
    setDataTypeMenu(dmin,level,scheme);
    setModeMenu();
    setSaveState(true);
  } else {
    p_feedback->setMessage("This mode is already assigned. You must delete "
                           " it before reassigning properties",WxFeedback::ERROR);
  }
  event.Skip();
}

void Polyrate::OnButtonPlyrtTrsndlClick( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
  string component;
  component = convertString(menu->GetStringSelection().c_str());
  ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_TRSN));
  string cmd = list->GetStringSelection().c_str();
  int idx = list->GetSelection();
  int mode, dmin, sym, ngrp;
  string scheme, level;
  model.translateTorsion(cmd, mode, dmin, sym, scheme, level, ngrp);
  model.deleteMode(component,mode);
  list->Delete(idx);
  if (idx > 0) {
    list->SetSelection(idx-1);
  } else if (list->GetCount() > 0) {
    list->SetSelection(0);
  }
  if (list->GetCount() > 0) {
    string cmd = list->GetStringSelection().c_str();
    int mode, imode, dmin, sym, ngrp;
    string scheme, level;
    model.translateTorsion(cmd, mode, dmin, sym, scheme, level, ngrp);
    menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_MODE));
    imode = model.getTorsionModeIndex(component,mode);
    menu->SetSelection(imode);
    setModeMenu();
    OnChoicePlyrtModeSelected( event );
  } else {
    menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_MODE));
    menu->Clear();
  }
  setSaveState(true);
  event.Skip();
}

void Polyrate::OnListboxPlyrtTrsnSelected( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_TRSN));
  if (list->GetCount()>0) {
    string cmd = list->GetStringSelection().c_str();
    int mode, imode, dmin, sym, ngrp;
    string scheme, level;
    model.translateTorsion(cmd, mode, dmin, sym, scheme, level, ngrp);
    ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
    string component;
    component = convertString(menu->GetStringSelection().c_str());
    menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_MODE));
    imode = model.getTorsionModeIndex(component,mode);
    setModeMenu();
    if (imode >= 0) {
      menu->SetSelection(imode);
      OnChoicePlyrtModeSelected( event );
    }
    ewxTextCtrl* text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_TRSNMD));
    text->setValueAsInt(mode);
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DMIN));
    text->setValueAsInt(dmin);
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SYMNUM));
    text->setValueAsInt(sym);
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_SBGRP));
    text->setValueAsInt(ngrp);
    menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_SCHM));
    if (scheme == "OW") {
      menu->SetSelection(0);
    } else if (scheme == "RO") {
      menu->SetSelection(1);
    } else if (scheme == "CO") {
      menu->SetSelection(2);
    } else if (scheme == "RW") {
      menu->SetSelection(3);
    } else if (scheme == "CW") {
      menu->SetSelection(4);
    }
    menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_APLVL));
    if (level == "FULL") {
      menu->SetSelection(0);
    } else if (level == "SC") {
      menu->SetSelection(1);
    } else if (level == "SF") {
      menu->SetSelection(2);
    }
  }
  event.Skip();
}

void Polyrate::OnChoicePlyrtModeSelected( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
  string component;
  component = convertString(menu->GetStringSelection().c_str());
  menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_MODE));
  int mode = atoi(menu->GetStringSelection().c_str());
  string cmd = model.getTorsion(component, mode);
  int dmin, sym, ngrp;
  string scheme, level;
  model.translateTorsion(cmd, mode, dmin, sym, scheme, level, ngrp);
  if (mode > 0) {
    setDataTypeMenu(dmin,level,scheme);
    OnChoicePlyrtDtypSelected( event );
    vector<string> opts = model.getTorsionOpts(component,mode);
    ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_OPTLST));
    list->Clear();
    int idx;
    for (idx = 0; idx < (int)opts.size(); idx++) {
      list->Append(opts[idx].c_str());
    }
    list->SetSelection(0);
    OnListboxPlyrtOptlstSelected( event );
  }
  checkTorsionOpts();
  event.Skip();
}

void Polyrate::OnChoicePlyrtDtypSelected( wxCommandEvent& event )
{
  ewxStaticText *units = ((ewxStaticText*)FindWindow(wxID_STATIC_PLYRT_DVALS));
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_DTYP));
  string selection = menu->GetStringSelection().c_str();
  string unit;
  if (selection == "Barrier Heights" || selection == "Excitation Energies" ||
      selection == "Barrier Frequencies") {
    unit = "cm**-1";
  } else if (selection == "Moments of Inertia") {
    unit = "m_e*a_0**2";
  } else {
    unit = "";
  }
  units->SetLabel(unit.c_str());
  event.Skip();
}

void Polyrate::OnTextctrlPlyrtDvalsEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DVALS));
  string input = text->GetValue().c_str();
  StringTokenizer tokenizer(input);
  vector<string> tokens = tokenizer.tokenize(" \t");
  int nvals = tokens.size();
  int i;
  bool isReal, isValid = true;
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_DTYP));
  string selection = menu->GetStringSelection().c_str();
  if (selection == "Barrier Heights" || selection == "Excitation Energies" ||
      selection == "Barrier Frequencies" || selection == "Moments of Inertia") {
    isReal = true;
  } else {
    isReal = false;
  }
  int ival;
  double dval;
  for (i=0; i<nvals; i++) {
    if (isReal) {
      if (!StringConverter::toDouble(tokens[i],dval)) {
        isValid = false;
      }
    } else {
      if (!StringConverter::toInt(tokens[i],ival)) {
        isValid = false;
      }
    }
  }
  if (isValid) {
    p_dataValues = input;
  } else {
    text->SetValue(p_dataValues.c_str());
    if (isReal) {
      p_feedback->setMessage("Input must be a space-delimited list of real numbers",
                             WxFeedback::ERROR);
    } else {
      p_feedback->setMessage("Input must be a space-delimited list of integers",
                             WxFeedback::ERROR);
    }
  }
  event.Skip();
}

void Polyrate::OnButtonPlyrtOptaddClick( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
  string component = convertString(menu->GetStringSelection().c_str());
  menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_MODE));
  int mode = atoi(menu->GetStringSelection().c_str());
  menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_DTYP));
  char buf[128];
  sprintf(buf,"    %d",mode);
  string cmd(buf);
  string selection = menu->GetStringSelection().c_str();
  bool isReal, isValid = true;
  if (selection == "Barrier Heights") {
    cmd.append(" W ");
    isReal = true;
  } else if (selection == "First Subgroup Atoms") {
    cmd.append(" ISB ");
    isReal = false;
  } else if (selection == "Indices of Rotation Axis Atoms") {
    cmd.append(" NBOND ");
    isReal = false;
  } else if (selection == "Symmetry Numbers") {
    cmd.append(" SIGMA ");
    isReal = false;
  } else if (selection == "Excitation Energies") {
    cmd.append(" U ");
    isReal = true;
  } else if (selection == "Barrier Frequencies") {
    cmd.append(" OMEGA ");
    isReal = true;
  } else if (selection == "Moments of Inertia") {
    cmd.append(" I ");
    isReal = true;
  }
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DVALS));
  string values = text->GetValue().c_str();
  // Check validity of options
  StringTokenizer tokenizer(values);
  vector<string> tokens = tokenizer.tokenize(" \t");
  int nvals = tokens.size();
  int i, ival;
  double dval;
  for (i=0; i<nvals; i++) {
    if (isReal) {
      if (!StringConverter::toDouble(tokens[i],dval)) {
        isValid = false;
      }
    } else {
      if (!StringConverter::toInt(tokens[i],ival)) {
        isValid = false;
      }
    }
  }
  if (isValid) {
    p_dataValues = values;
    cmd.append(values.c_str());
    ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_OPTLST));
    list->Append(cmd.c_str());
    model.addTorsionOpt(component,mode,cmd);
    setSaveState(true);
  } else {
    text->SetValue(p_dataValues.c_str());
    if (isReal) {
      p_feedback->setMessage("Input must be a space-delimited list of real numbers",
                             WxFeedback::ERROR);
    } else {
      p_feedback->setMessage("Input must be a space-delimited list of integers",
                             WxFeedback::ERROR);
    }
  }
  checkTorsionOpts();
  event.Skip();
}

void Polyrate::OnButtonPlyrtOptdelClick( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
  string component = convertString(menu->GetStringSelection().c_str());
  menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_MODE));
  int mode = atoi(menu->GetStringSelection().c_str());
  ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_OPTLST));
  int idx = list->GetSelection();
  if (idx != wxNOT_FOUND) {
    list->Delete(idx);
    model.deleteTorsionOpt(component,mode,idx);
    setSaveState(true);
  }
  event.Skip();
}

void Polyrate::OnListboxPlyrtOptlstSelected( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_OPTLST));
  string cmd = list->GetStringSelection().c_str();
  string type, values;
  int mode;
  model.translateTorsionOpt(cmd,mode,type,values);
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_DTYP));
  string substr;
  if (type == "OMEGA") {
    menu->SetStringSelection("Barrier Frequencies");
    substr = "Frequencies";
  } else if (type == "U") {
    menu->SetStringSelection("Excitation Energies");
    substr = "Energies";
  } else if (type == "I") {
    menu->SetStringSelection("Moments of Inertia");
    substr = "Inertia";
  } else if (type == "SIGMA") {
    menu->SetStringSelection("Symmetry Numbers");
    substr = "Symmetry";
  } else if (type == "W") {
    menu->SetStringSelection("Barrier Heights");
    substr = "Heights";
  } else if (type == "ISB") {
    menu->SetStringSelection("First Subgroup Atoms");
    substr = "Subgroup";
  } else if (type == "NBOND") {
    menu->SetStringSelection("Indices of Rotation Axis Atoms");
    substr = "Rotation";
  }
  int size = menu->GetCount();
  int idx = -1;
  string label;
  for (int i = 0; i<size; i++) {
    label = menu->GetString(i).c_str();
    if (STLUtil::containsString(label,substr)) {
      idx = i;
      break;
    }
  }
  if (idx >= 0) {
    menu->SetSelection(idx);
  } 
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DVALS));
  text->SetValue(values.c_str());
  event.Skip();
}

void Polyrate::OnChoicePlyrtCmpaxSelected( wxCommandEvent& event )
{
  PolyrateModel &model = getPolyrateModel();
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPAX));
  string component = convertString(menu->GetStringSelection().c_str());
  menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_AXIS));
  string axis;
  if (model.getLinearStatus(component)) {
    menu->Enable(true);
    axis.append(1,model.getComponentAxis(component));
    menu->SetStringSelection(axis.c_str());
  } else {
    axis.append(1,model.getComponentAxis(component));
    menu->SetStringSelection(axis.c_str());
    menu->Enable(false);
  }
  event.Skip();
}

void Polyrate::OnChoicePlyrtAxisSelected( wxCommandEvent& event)
{
  PolyrateModel &model = getPolyrateModel();
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPAX));
  string component = convertString(menu->GetStringSelection().c_str());
  menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_AXIS));
  model.setComponentAxis(component, *(menu->GetStringSelection().c_str()));
  setSaveState(true);
  event.Skip();
}


string Polyrate::convertString(string input)
{
  string ret = "";
  if (input == "Reactant 1") {
    ret = "react1";
  } else if (input == "Reactant 2") {
    ret = "react2";
  } else if (input == "Transition State") {
    ret = "transition";
  } else if (input == "Product 1") {
    ret = "prod1";
  } else if (input == "Product 2") {
    ret = "prod2";
  }
  return ret;
}

void Polyrate::initializeMenu(ewxChoice *menu)
{
  PolyrateModel &model = getPolyrateModel();
  string component;
  menu->Clear();
  if (checkComponent(DirDyVTSTTask::REACT1)) {
    menu->Append("Reactant 1");
    component = convertString("Reactant 1");
    model.setComponentStatus(component,true);
  }
  if (checkComponent(DirDyVTSTTask::REACT2)) {
    menu->Append("Reactant 2");
    component = convertString("Reactant 2");
    model.setComponentStatus(component,true);
  }
  if (checkComponent(DirDyVTSTTask::TRANSITION)) {
    menu->Append("Transition State");
    component = convertString("Transition State");
    model.setComponentStatus(component,true);
  }
  if (checkComponent(DirDyVTSTTask::PROD1)) {
    menu->Append("Product 1");
    component = convertString("Product 1");
    model.setComponentStatus(component,true);
  }
  if (checkComponent(DirDyVTSTTask::PROD2)) {
    menu->Append("Product 2");
    component = convertString("Product 2");
    model.setComponentStatus(component,true);
  }
}

void Polyrate::setDataTypeMenu(int dmin, string level, string scheme)
{
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_DTYP));
  menu->Clear();
  if (dmin ==1) {
    if (scheme == "OW") {
      menu->Append("Barrier Heights");
    } else if (scheme == "RO") {
      menu->Append("First Subgroup Atoms");
    } else if (scheme == "CO") {
      menu->Append("First Subgroup Atoms");
      menu->Append("Indices of Rotation Axis Atoms");
    } else if (scheme == "RW") {
      menu->Append("First Subgroup Atoms");
      menu->Append("Barrier Heights");
    } else if (scheme == "CW") {
      menu->Append("First Subgroup Atoms");
      menu->Append("Indices of Rotation Axis Atoms");
      menu->Append("Barrier Heights");
    }
  } else if (dmin > 1) {
    menu->Append("Symmetry Numbers");
    if (level == "FULL") {
      menu->Append("Excitation Energies");
      if (scheme == "OW") {
        menu->Append("Barrier Frequencies");
        menu->Append("Barrier Heights");
      } else {
        menu->Append("Moments of Inertia");
        menu->Append("First Subgroup Atoms");
        if (scheme == "RO") {
          menu->Append("Barrier Frequencies");
        } else if (scheme == "CO") {
          menu->Append("Indices of Rotation Axis Atoms");
          menu->Append("Barrier Frequencies");
        } else if (scheme == "RW") {
          menu->Append("Barrier Heights");
        } else if (scheme == "CW") {
          menu->Append("Indices of Rotation Axis Atoms");
          menu->Append("Barrier Heights");
        }
      }
    } else if (level == "SF") {
      menu->Append("Excitation Energies");
      if (scheme == "OW") {
        menu->Append("Barrier Heights");
      } else {
        menu->Append("Moments of Inertia");
        menu->Append("First Subgroup Atoms");
        if (scheme == "CO") {
          menu->Append("Indices of Rotation Axis Atoms");
        } else if (scheme == "RW") {
          menu->Append("Barrier Heights");
        } else if (scheme == "CW") {
          menu->Append("Indices of Rotation Axis Atoms");
          menu->Append("Barrier Heights");
        }
      }
    } else if (level == "SC") {
      if (scheme == "OW") {
        menu->Append("Barrier Heights");
      } else {
        menu->Append("First Subgroup Atoms");
        if (scheme == "CO") {
          menu->Append("Indices of Rotation Axis Atoms");
        } else if (scheme == "RW") {
          menu->Append("Barrier Heights");
        } else if (scheme == "CW") {
          menu->Append("Indices of Rotation Axis Atoms");
          menu->Append("Barrier Heights");
        }
      }
    }
  }
  menu->SetSelection(0);
}

void Polyrate::setModeMenu()
{
  PolyrateModel &model = getPolyrateModel();
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
  ewxTextCtrl *text;
  ewxButton *button;
  ewxListBox *list;
  string component;
  component = convertString(menu->GetStringSelection().c_str());
  int nmodes = model.getNumModes(component);
  int i, mode;
  char buf[32];
  menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_MODE));
  menu->Clear();
  int tmodes = 0;
  for (i=0; i<nmodes; i++) {
    mode = model.getMode(component, i);
    if (model.getAnharmonicity(component,mode) == "tor") {
      tmodes++;
      sprintf(buf,"%4i",mode);
      menu->Append(buf);
    }
  }
  if (tmodes > 0) {
    menu->SetSelection(tmodes-1);
    menu->Enable(true);
    menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_DTYP));
    menu->Enable(true);
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DVALS));
    text->Enable(true);
    button = ((ewxButton*)FindWindow(ID_BUTTON_PLYRT_OPTADD));
    button->Enable(true);
    button = ((ewxButton*)FindWindow(ID_BUTTON_PLYRT_OPTDEL));
    button->Enable(true);
    list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_OPTLST));
    list->Enable(true);
  } else {
    menu->Enable(false);
    menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_DTYP));
    menu->Enable(false);
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLYRT_DVALS));
    text->SetValue("");
    text->Enable(false);
    button = ((ewxButton*)FindWindow(ID_BUTTON_PLYRT_OPTADD));
    button->Enable(false);
    button = ((ewxButton*)FindWindow(ID_BUTTON_PLYRT_OPTDEL));
    button->Enable(false);
    list = ((ewxListBox*)FindWindow(ID_LISTBOX_PLYRT_OPTLST));
    list->Clear();
    list->Enable(false);
  }
}

void Polyrate::checkTorsionOpts()
{
  PolyrateModel &model = getPolyrateModel();
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_CMPMD));
  string component;
  component = convertString(menu->GetStringSelection().c_str());
  menu = ((ewxChoice*)FindWindow(ID_CHOICE_PLYRT_MODE));
  int mode = atoi(menu->GetStringSelection().c_str());
  ewxStaticText *label = ((ewxStaticText*)FindWindow(wxID_STATIC_PLYRT_CHKOPT));
  label->Show(!model.checkTorsionOpts(component, mode));
}

