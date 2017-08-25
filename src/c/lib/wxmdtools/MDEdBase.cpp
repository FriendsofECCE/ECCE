// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
  using std::cout;
  using std::endl;
  using std::ends;
#include <fstream>
  using std::ofstream;
  using std::ifstream;
#include <strstream>
  using std::istrstream;
  using std::ostrstream;

#include "util/BrowserHelp.H"
#include "util/Ecce.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/ResourceUtils.H"
#include "util/StringConverter.H"
#include "util/TempStorage.H"
#include "util/TypedFile.H"
#include "util/EditEvent.H"

#include "tdat/TaskApp.H"
#include "tdat/DynamicsModel.H"
#include "tdat/AuthCache.H"

#include "dsm/NWChemMDModel.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/MdTask.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceTool.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WxEditSessionMgr.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/WxJMSMessageDispatch.H"

#include "wxgui/MDEdBase.H"

/**
 * Constructor.
 * Subclasses should invoke constructor() in their constructors
 * and override constructor() if necessary.
 */
MDEdBase::MDEdBase( )
        : TaskApp("MD Base", 1000), WxDavAuth(0)
{
}


/**
 * Constructor.
 * Subclasses should invoke constructor() in their constructors
 * and override constructor() if necessary.
 */
MDEdBase::MDEdBase( wxWindow* parent, wxWindowID id, const wxString& caption,
                    const wxPoint& pos, const wxSize& size, long style )
                  : MDEdBaseGUI(parent, id, caption, pos, size, style),
                    TaskApp("MD Base", 1000)
{
}


/**
 * Attach to messaging,
 * Finish GUI creation.
 */
void MDEdBase::constructor()
{
   p_feedback = (WxFeedback*)FindWindow(ID_FEEDBACK);
   p_feedback->setDropHandler(this);
   p_feedback->setSaveHandler(this);

   EDSIFactory::addAuthEventListener(this);

   initializeModel();

   createPanels();

   Fit();

   p_ignoreCodeEventsFlag = false;

   // restore window location from preferences
   Preferences prefs(getAppName());
   restoreWindowSettings(getAppName(), prefs, false);

   // Get Registry and set destop icon
   ResourceDescriptor rs = ResourceDescriptor::getResourceDescriptor();
   SetIcon(wxIcon(ewxBitmap::pixmapFile(rs.getTool(getAppName())->getIcon()),
                                        wxBITMAP_TYPE_XPM));
}



/**
 * Create the subpanels (Interaction, Constraint, Control, Files)
 * Safe to call more than once.
 */
void MDEdBase::createPanels()
{
   int numPages = p_notebook->GetPageCount();
   if (numPages == 0) {
   }
}


/**
 * Disable the gui depending on the state.
 * @param state new state that will be taking effect.
 * @todo optimize so call is not made if there is no effective change
 */
void MDEdBase::updateDisabledState(ResourceDescriptor::RUNSTATE state, bool forceMsg)
{
   static ResourceDescriptor::RUNSTATE lastState =
                                       ResourceDescriptor::STATE_READY;

   if (state >= ResourceDescriptor::STATE_SUBMITTED) {
      setFrameCustomDisabledStyle(false);
      if (forceMsg ||
          (state>lastState && state==ResourceDescriptor::STATE_SUBMITTED)) {
         p_feedback->setMessage("This calculation is read only and setup cannot be changed without terminating during the run or resetting after completion.", WxFeedback::INFO);
         lastState = state;
      }
   } else {
      setFrameCustomDisabledStyle(true);
   }

   // A few buttons should never be disabled
   ewxButton *btn = ((ewxButton*)FindWindow(ID_BUTTON_BUILDER));
   btn->setCustomDisabledStyle(true);
   btn = ((ewxButton*)FindWindow(ID_BUTTON_FINALEDIT));
   btn->setCustomDisabledStyle(true);
}



void MDEdBase::processDrop(const string& url)
{
   setContext(url);
}


/**
 * Functions created to handle the message area
 */
void MDEdBase::processSave()
{
   // Maybe the lock icon is showing in which case just ignore the user action
   if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
      ResourceDescriptor::RUNSTATE state = p_feedback->getRunState();
      if (state <= ResourceDescriptor::STATE_READY) {
         try {
            MdTask *task
               = dynamic_cast<MdTask*>(EDSIFactory::getResource(p_model->getUrl()));
            saveModelAndInputFile(task);
         } catch (EcceException& ex) {
            p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
         }
      } else {
         p_feedback->setMessage("This calculation is running or complete.  Changes can no longer be saved.", WxFeedback::ERROR);
      }
   }
}

/**
 * Functions created to handle the message area
 */
void MDEdBase::doSaveButtonClick(wxCommandEvent &event)
{
   processSave();
}


/**
 * Save the model, generate and save input file.
 */
void MDEdBase::saveModelAndInputFile(MdTask *task) throw(IOException)
{
   if (task) {
      NWChemMDModel& model = getNWChemMDModel();

      // can throw an exception
      task->setTaskModel(&model);

      // If we got this far, the save was succesful
      setSaveState(false);

      if (p_feedback->getRunState() == ResourceDescriptor::STATE_READY) {
        // can throw an exception
        generateInputFile(task);
      }
   }
}

/**
 *  Enable or disable the Final Edit and Launch buttons
 *  @param flag: if true, enable buttons otherwise disable them
 */
void MDEdBase::enableLaunch(bool flag)
{
  ewxButton* finalEdit = ((ewxButton*)FindWindow(ID_BUTTON_FINALEDIT));
  ewxButton* launch = ((ewxButton*)FindWindow(ID_BUTTON_LAUNCH));
  if (flag) {
    finalEdit->Enable(true);
    launch->Enable(true);
  } else {
    finalEdit->Enable(false);
    launch->Enable(false);
  }
}

/**
 * Re-generate the input file from the model and upload it to the data server.
 * @throw IOException if the input file generation failed for some reason or
 *       we couldn't upload the data.
 */
void MDEdBase::generateInputFile(MdTask *task) throw(IOException)
{
   NWChemMDModel& model = getNWChemMDModel();

   const JCode *codecap = task->application();
   if (codecap == 0) {
     throw IOException("Could not find code cap file", WHERE);
   }

   // Write the model to a file
   SFile *tmpModel = TempStorage::getTempFile();
   ofstream outfile(tmpModel->path().c_str());
   outfile << task->serializeModel(&model);
   outfile.flush();
   outfile.close();

   // Run the conversion script
   SFile *tmpInput = TempStorage::getTempFile();
   string cmd = Ecce::ecceDataControllersPath();
   cmd += "/";
   string inputGenFile;
   codecap->get_string("TaskInputGenerator", inputGenFile);
   if (inputGenFile.empty()) {
     throw IOException("Could not find input generator file name", WHERE);
   }
   cmd += inputGenFile + " < ";
   cmd += tmpModel->path();
   cmd += " > ";
   cmd += tmpInput->path();
   int istatus = system(cmd.c_str());
   istatus = istatus >> 8;
   if (istatus != 0) {
      throw IOException("Input file generation failed.", WHERE);
   }


   // copy input file to web server
   TypedFile inputFileType = codecap->getCodeFile(JCode::PRIMARY_INPUT);
   string inputFile = inputFileType.name();
   if (inputFile.empty()) {
     throw IOException("Could not find primary input file name", WHERE);
   }
   ifstream ifs(tmpInput->path().c_str());
   if (!task->putInputFile(inputFile, &ifs)) {
      p_feedback->setMessage("Failed to save input file", WxFeedback::ERROR);
   } else {
     // Lisong 07/24/06: It wasn't sending message, now Task will.
     // Would this cause problem?
      task->setState(ResourceDescriptor::STATE_READY);
      setRunState(ResourceDescriptor::STATE_READY);
   }
   ifs.close();

   tmpInput->remove();
   delete tmpInput;

   tmpModel->remove();
   delete tmpModel;
}


void MDEdBase::OnExitClick( wxCommandEvent& event )
{
    // This generates window close so no need to repeat save code here.
    Close(true);

    event.Skip();
}



void MDEdBase::OnSaveClick( wxCommandEvent& event )
{
  doSaveButtonClick(event);
}


void MDEdBase::OnMenuSupportClick( wxCommandEvent& event )
{
  BrowserHelp help;
  help.showFeedbackPage();
}

void MDEdBase::OnMenuHelpClick( wxCommandEvent& event )
{
  // title is "NWChem MD <Task>".  Remove spaces to make a better help key
  string helpkey = getTitle();
  size_t ind;
  while ((ind = helpkey.find(' ')) != string::npos)
    helpkey.erase(ind, 1);

  BrowserHelp help;
  help.showPage(help.URL(helpkey));
}

/**
 * Disables GUI.
 */
void MDEdBase::clearContext()
{
   p_notebook->Disable();
}

void MDEdBase::setContext(const string& urlstr) throw (EcceException)
{
  ResourceDescriptor::RUNSTATE state = ResourceDescriptor::STATE_CREATED;
  if (p_model) {

    // Check if the context is the same as the existing one and bail
    // if it is because there is no need to reset the context
    if (urlstr == p_model->getUrl())
      return;

    // Set model's url
    p_model->setUrl(urlstr);

    // Get task model from db (deserialize)
    MdTask *mdTask =  dynamic_cast<MdTask*>(EDSIFactory::getResource(EcceURL(urlstr)));
    if (mdTask != 0) {
      try {
        FragmentSummary fragSum;
        mdTask->getFragmentSummary(fragSum);
        getNWChemMDModel().getInteractionModel()->setDefaultGrid(fragSum);

        state = mdTask->getState();
        p_feedback->setRunState(state);
        updateDisabledState(p_feedback->getRunState(), true);
        EcceURL url = mdTask->getURL();
        EDSIServerCentral servers;
        p_feedback->setContextLabel(servers.mapURLtoName(url));
        p_feedback->setContextURL(url);

        mdTask->getTaskModel(*p_model);

        // See if no input file exists yet - if not, then generate
        vector<EcceURL> files = mdTask->getDataFiles(JCode::PRIMARY_INPUT);
        if (files.size() == 0) {
           // only dynamics tasks have required input before an input file
           // can be generated
           if (mdTask->getContentType() != ResourceDescriptor::CT_MDDYNAMICS) {
             generateInputFile(mdTask);
           } else {
             NWChemMDModel& mdModel = getNWChemMDModel();
             DynamicsModel *dynModel = mdModel.getDynamicsModel();
             if (dynModel) {
                // only generate an input file if the number of data
                // steps is set
                if (dynModel->getDataSteps() > 0)
                  generateInputFile(mdTask);
             }
           }
        }

        p_msgDispatcher->setPollContext(urlstr);
      }
      catch (...) {
        mdTask = 0;
        throw EcceException("Deserialize model failed", WHERE);
      }
      mdTask = 0;
    }

    //Refresh GUI with model
    refreshGUI();


  }

  // set the help menu item label based on the task title
  wxMenuItem *item = 0;
  wxMenuBar *menuBar = GetMenuBar();
  if (menuBar) {
    string text = "on " + getTitle() + "...";
    item = menuBar->FindItem(ID_MENU_HELP);
    item->SetText(text.c_str());
  }
  getTitle();

  // Do these last after filling gui since it can trigger callbacks
  setSaveState(false);

  if (state >= ResourceDescriptor::STATE_SUBMITTED) {
     p_feedback->setEditStatus(WxFeedback::READONLY);
  } else {
     p_feedback->setEditStatus(WxFeedback::EDIT);
  }

}


/**
 *  Does nothing.
 *  Subclasses should override.
 */
void MDEdBase::refreshGUI()
{
}



/**
 * Changes gui to show the user whether there are unsaved changes.
 */
void MDEdBase::setSaveState(bool unsavedChanges)
{
   if (!p_ignoreCodeEventsFlag) {
      wxMenuItem *item = 0;
      wxMenuBar *menuBar = GetMenuBar();
      if (menuBar) {
         item = menuBar->FindItem(wxID_SAVE);
      }

      // Make sure we aren't locked.  But do it always if this is a call
      // to set it to false so we don't have to worry about order dependencies
      // between the state and edit state
      if (!unsavedChanges || (unsavedChanges &&
            p_feedback->getEditStatus() != WxFeedback::READONLY)) {
         if (item) item->Enable(unsavedChanges);
         if (unsavedChanges) {
            p_feedback->setEditStatus(WxFeedback::MODIFIED);
         }  else {
            p_feedback->setEditStatus(WxFeedback::EDIT);
         }
      }
   }
}


/**
 * Set calculation state on data server, update feedback area,
 * and notify others that the state has changed.
 */
void MDEdBase::setRunState(ResourceDescriptor::RUNSTATE state)
{
   string url = p_model->getUrl();
   MdTask *mdTask =
      dynamic_cast<MdTask*>(EDSIFactory::getResource(EcceURL(url)));
   if (mdTask) {
      mdTask->setState(state);
      // Call before actually changing state so it can decide whether
      // or not to post a message
      updateDisabledState(state);

      p_feedback->setRunState(state);

      // enable/disable final edit and launch buttons based on state
      enableLaunch(state >= ResourceDescriptor::STATE_READY);
   }
}


/**
 * Determines if there are unsaved changes.
 * This just looks at the gui indicators so don't do something like
 * call isUnsavedState to determine whether to call setSaveState.
 */
bool MDEdBase::isUnsavedState()
{
  return (p_feedback->getEditStatus() == WxFeedback::MODIFIED);
}

WxFeedback& MDEdBase::getFeedback()
{
   return *p_feedback;
}

void MDEdBase::notifyChange()
{
  // no-op for apps that don't have special processing
}


bool MDEdBase::Show(bool show)
{
   return MDEdBaseGUI::Show(show);
}


void MDEdBase::_invokeMCB(JMSMessage& msg)
{
   if (msg.containsProperty("calcurl")) {
      string url = msg.getProperty("calcurl");
      setContext(url);
      p_msgDispatcher->setPollContext(url);
      Raise();
   }
}

void MDEdBase::_quitMCB(JMSMessage& msg)
{
   if (msg.getSender().getName() == GATEWAY) {
      // save window location
      Preferences prefs(getAppName());
      saveWindowSettings(getAppName(), prefs, false);
      prefs.saveFile();

      Destroy();
   }
}

/**
 * Respond to request for identification.
 * Return tool name and context information.
 *
 */
void MDEdBase::_identifyMCB(JMSMessage&)
{
   ostrstream os;
   os << getTitle();

   // Add context info if we have context
   if (p_model != 0) {
      EcceURL url(p_model->getUrl());
      os << ": " << EDSIServerCentral().mapURLtoName(url);
   }
   os << ends;

   JMSPublisher publisher(getAppName());
   JMSMessage *msg = publisher.newMessage();
   msg->addProperty("info",os.str());
   publisher.publish("ecce_identify_reply", *msg);
   delete msg;
}


/**
 * Handle the preference change message which indicates a unit family change.
 */
void MDEdBase::_prefsMCB(JMSMessage& msg)
{
   ewxWindowUtils::processGlobalPreferenceChange(this);
}




/**
 * State has changed.
 */
void MDEdBase::_stateMCB(JMSMessage& msg)
{
   string url = msg.getProperty("url");

   if (url == p_model->getUrl()) {
     // Update EDSIFactory Pool
     EDSIFactory::changePoolResource(url);

     string statestr = msg.getProperty("state");
     ResourceDescriptor::RUNSTATE state = ResourceUtils::stringToState(statestr);
     p_feedback->setRunState(state);
     updateDisabledState(state);
     if (state >= ResourceDescriptor::STATE_SUBMITTED) {
        p_feedback->setEditStatus(WxFeedback::READONLY);
     } else if (!isUnsavedState()) {
        p_feedback->setEditStatus(WxFeedback::EDIT);
     }
   }
}

/**
 *  Selection has changed
 */
void MDEdBase::_molSelectionMCB(JMSMessage& msg)
{
  string url = msg.getProperty("url");
  istrstream is(msg.getProperty("frag_sum").c_str());
  if (is) {
    is >> p_fragSummary;
  }
}

/**
 * An object was deleted.
 * If it was our object, clear and disable the gui.
 */
void MDEdBase::_removeMCB(JMSMessage& msg)
{
   string urlstr = msg.getProperty("url");

   if (p_model) {
      EcceURL url = p_model->getUrl();
      if (url.isChildOrMe(urlstr)) {
        p_feedback->setMessage("The calculation or a parent project "
                               "has been deleted.", WxFeedback::WARNING);
        clearContext();

        // Update EDSIFactory Pool
        EDSIFactory::removePoolResource(urlstr);
     }
   }
}



/**
 * An object was renamed.
 * If it was our object, reset variables.
 */
void MDEdBase::_renameMCB(JMSMessage& msg)
{
   string oldURL = msg.getProperty("oldurl");
   string newURL = msg.getProperty("newurl");

   if (p_model) {
      string calcurl = p_model->getUrl();
      if (EcceURL(calcurl).isChildOrMe(oldURL)) {
         // Update EDSIFactory Pool
         EDSIFactory::renamePoolResource(oldURL, newURL);
         calcurl.replace(0, oldURL.length(), newURL);
         p_model->setUrl(calcurl);

         // Update the path in the feedback area
         p_feedback->setContextLabel(EDSIServerCentral().mapURLtoName(calcurl));
         p_feedback->setContextURL(calcurl);
         p_msgDispatcher->setPollContext(p_model->getUrl());
      }
   }
}


/**
 * State color preferences have changed.
 * Update state icon in feedback area.
 */
void MDEdBase::_statePrefsMCB(JMSMessage& msg)
{
   p_feedback->readPreferences();
}


/**
 * Authorization cache needs to be updated.
 */
void MDEdBase::_authMCB(JMSMessage& msg, const string& callerID)
{
   AuthCache::getCache().msgIn(msg, callerID);
}



void MDEdBase::initializeModel()
{
   vector<NWChemMDModel::GUIPanel> panels;
   p_model = new NWChemMDModel(panels);
}



NWChemMDModel& MDEdBase::getNWChemMDModel() const
{
   return (NWChemMDModel&)*p_model;
}

void MDEdBase::taskAppShow()
{
   Show(true);
}
void MDEdBase::taskAppDestroy()
{
   // save window location
   Preferences prefs(getAppName());
   saveWindowSettings(getAppName(), prefs, false);
   prefs.saveFile();

   Destroy();
}
void MDEdBase::taskAppRaise()
{
   Raise();
}

void MDEdBase::setUnitFamily(const string& family)
{
   ewxWindowUtils::setUnitFamily(this,family);
   Fit();
}

string MDEdBase::getTitle() const
{
   ResourceDescriptor& rd = ResourceDescriptor::getResourceDescriptor();
   ResourceTool *tool = rd.getTool(getAppName());
   return tool->getLabel();
}


void MDEdBase::OnCloseWindow( wxCloseEvent& event )
{
   doQuit(false, true);
}


/**
 * quit but first handle any unsaved changes.
 * @param force without prompting even if error occurs during save
 * @todo implement force param
 */
void MDEdBase::doQuit(bool force, bool notifyEditor)
{
   if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
      ewxMessageDialog prompt(this,"You have unsaved changes!  Do you "
        "want to save your changes?", "ECCE Save Changes",
        wxYES_NO | wxCANCEL | wxICON_QUESTION);
      int status = prompt.ShowModal();
      if (status == wxID_YES) {
         try {
            MdTask *task = 
               dynamic_cast<MdTask*>(EDSIFactory::getResource(p_model->getUrl()));
            saveModelAndInputFile(task);
         } catch (EcceException& ex) {
            p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
         }
      } else if (status == wxID_CANCEL) {
         // abort the quit - not sure if this will cause problems...
         return;
      }
   }

   if (notifyEditor) {
      notifyClose();
   }

   // save window location
   Preferences prefs(getAppName());
   saveWindowSettings(getAppName(), prefs, false);
   prefs.saveFile();

   Destroy();
}


void MDEdBase::OnButtonBuilderClick( wxCommandEvent& event )
{
   try {

      NWChemMDModel& model = getNWChemMDModel();
      MdTask *task = 
         dynamic_cast<MdTask*>(EDSIFactory::getResource(model.getUrl()));

      // Save if needed
      if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
         saveModelAndInputFile(task);
      }

      string myname = (const char *)GetName();
      JMSPublisher publisher(myname);
      JMSMessage *msg = publisher.newMessage();
      msg->addProperty("action","start");
      publisher.publish("ecce_activity",*msg);
      delete msg;

      // Send the start app request to gateway:
      Target gateway(GATEWAY, "");
      msg = publisher.newMessage(gateway);

      publisher.invoke(*msg, BUILDER, 0, model.getUrl());
   } catch (EcceException& ex) {
      p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
   }
}


void MDEdBase::OnButtonFinaleditClick( wxCommandEvent& event )
{
   try {

     NWChemMDModel& model = getNWChemMDModel();
     MdTask *task = dynamic_cast<MdTask*>(EDSIFactory::getResource(model.getUrl()));

     // Save if needed
     if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
       saveModelAndInputFile(task);
     }

     istream* is = task->getDataFile(JCode::PRIMARY_INPUT);
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

        string title = getTitle() + " Input";

        WxEditSessionMgr sessionMgr;
        sessionMgr.edit(text, "testing", this, roFlag, title.c_str());
     } else {
        p_feedback->setMessage("Input file not found.", WxFeedback::WARNING);
     }

   } catch (EcceException& ex) {
      p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
   }
}



/**
 * EditListener interface.
 * An edit session has terminated.
 */
void MDEdBase::processEditCompletion(const EditEvent& ee)
{
   wxBusyCursor busy;
   ifstream ifs(ee.filename.c_str());

   string infile;
   TypedFile tinfile;
   MdTask *task = 
       dynamic_cast<MdTask*>(EDSIFactory::getResource(getNWChemMDModel().getUrl()));
   task->getDataFile(JCode::PRIMARY_INPUT, tinfile);
   infile = tinfile.name();

   if (!task->putInputFile(infile, &ifs)) {
      p_feedback->setMessage("Input file could not be copied back to DAV",
            WxFeedback::ERROR);
   } else {
     p_feedback->setMessage("The input file has been saved. Any changes "
            "you make to the gui will override your manual edits.",
            WxFeedback::INFO);
   }
   ifs.close();
}



void MDEdBase::OnButtonLaunchClick( wxCommandEvent& event )
{
   try {

      NWChemMDModel& model = getNWChemMDModel();
      MdTask *task = 
         dynamic_cast<MdTask*>(EDSIFactory::getResource(model.getUrl()));

      // Save if needed
      if (p_feedback->getEditStatus() == WxFeedback::MODIFIED) {
         saveModelAndInputFile(task);
      }

      string myname = (const char *)GetName();
      JMSPublisher publisher(myname);
      JMSMessage *msg = publisher.newMessage();
      msg->addProperty("action","start");
      publisher.publish("ecce_activity",*msg);
      delete msg;

      // Send the start app request to gateway:
      Target gateway(GATEWAY, "");
      msg = publisher.newMessage(gateway);

      publisher.invoke(*msg, LAUNCHER, 0, model.getUrl());
   } catch (EcceException& ex) {
      p_feedback->setMessage(ex.what(), WxFeedback::ERROR);
   }
}

/**
 *  Reset all parameters in the editor to their default values.
 */
void MDEdBase::OnButtonResetaClick( wxCommandEvent& event )
{
  ewxMessageDialog prompt(this,"Are you sure you want to reset all values?",
      "ECCE Reset All",wxYES_NO);
  int status = prompt.ShowModal();
  if (status == wxID_YES) {
    getNWChemMDModel().reset();
    refreshGUI();
  }
}

/**
 *  Reset all parameters in the current window to their default values.
 */
void MDEdBase::OnButtonResetwClick( wxCommandEvent& event )
{
  resetPanel();
}

/**
 *  Dummy function that must be overwritten in the individual applications.
 */
void MDEdBase::resetPanel()
{
}
