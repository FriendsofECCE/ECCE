/**
 * @file
 *
 *
 */
#ifndef _MDEDBASE_H_
#define _MDEDBASE_H_


#include "util/IOException.H"
#include "util/EditListener.H"

#include "tdat/TaskApp.H"
#include "tdat/FragmentSummary.H"

#include "wxgui/WxDavAuth.H"
#include "wxgui/CalcDropHandler.H"

#include "wxgui/MDEdBaseGUI.H"
#include "wxgui/MDPanelHelper.H"
#include "wxgui/FeedbackSaveHandler.H"

class JMSMessage;
class WxJMSMessageDispatch;
class WxFragmentImport;

class NWChemMDModel;
class MdTask;
class InteractionPanel;
class ConstraintPanel;
class ControlPanel;
class FilesPanel;


/**
 * A base class for MD editor guis.
 * The editors we know of for NWChem are very similar and much can
 * be abstracted into a superclass.  All functions are virtual.
 * getAppName() is pure virtual. The subclass must implement this and return
 * the registry name for the tool in order to successfully connect with
 * messaging.
 */
class MDEdBase: public MDEdBaseGUI,
                public TaskApp,
                public WxDavAuth,
                public EditListener,
                public MDPanelHelper,
                public CalcDropHandler,
                public FeedbackSaveHandler
{    

   public:
      /// Constructors
      MDEdBase( );
      MDEdBase( wxWindow* parent, wxWindowID id = SYMBOL_MDEDBASEGUI_IDNAME, const wxString& caption = SYMBOL_MDEDBASEGUI_TITLE, const wxPoint& pos = SYMBOL_MDEDBASEGUI_POSITION, const wxSize& size = SYMBOL_MDEDBASEGUI_SIZE, long style = SYMBOL_MDEDBASEGUI_STYLE );


      // TaskApp virtuals
      virtual string getTitle() const;
      virtual void taskAppRaise();
      virtual void taskAppDestroy();
      virtual void taskAppShow();
      virtual void setUnitFamily(const string& family);

      // MDPanelHelper virtuals
      virtual void setSaveState(bool usavedChanges);
      virtual void setRunState(ResourceDescriptor::RUNSTATE state);
      virtual bool isUnsavedState();
      virtual WxFeedback& getFeedback();
      virtual void notifyChange();

      // CalcDropHandler virtuals
      virtual void processDrop(const string& url);

      // FeedbackSaveHandler virtuals
      virtual void processSave();

      virtual void updateDisabledState(ResourceDescriptor::RUNSTATE state, 
            bool forceMsg=false);

      virtual bool Show(bool show);

      virtual void clearContext();

      virtual void doQuit(bool force, bool notifyEditor);
      virtual void setContext(const string& url) throw (EcceException);
      virtual void generateInputFile(MdTask *task) throw(IOException);
      virtual void saveModelAndInputFile(MdTask *task) throw(IOException);
      virtual void resetPanel();
      void enableLaunch(bool flag);

   protected:
      // Callbacks
      virtual void doSaveButtonClick(wxCommandEvent &event);
      virtual void OnCloseWindow( wxCloseEvent& event );
      virtual void OnSaveClick( wxCommandEvent& event );
      virtual void OnExitClick( wxCommandEvent& event );
      virtual void OnMenuSupportClick( wxCommandEvent& event );
      virtual void OnMenuHelpClick( wxCommandEvent& event );
      virtual void OnButtonBuilderClick( wxCommandEvent& event );
      virtual void OnButtonFinaleditClick( wxCommandEvent& event );
      virtual void OnButtonLaunchClick( wxCommandEvent& event );
      virtual void OnButtonResetwClick( wxCommandEvent& event );
      virtual void OnButtonResetaClick( wxCommandEvent& event );

      // EditListener virtuals
      virtual void processEditCompletion(const EditEvent& ee);

      // Messaging
      // Could not do this work in base class and have it work on
      // sgi so provide methods we can call from subclasses at least
      virtual void _invokeMCB(JMSMessage& msg);
      virtual void _quitMCB(JMSMessage& msg);
      virtual void _identifyMCB(JMSMessage& msg);
      virtual void _stateMCB(JMSMessage& msg);
      virtual void _statePrefsMCB(JMSMessage& msg);
      virtual void _prefsMCB(JMSMessage& msg);
      virtual void _renameMCB(JMSMessage& msg);
      virtual void _removeMCB(JMSMessage& msg);
      virtual void _molSelectionMCB(JMSMessage& msg);
      virtual void _authMCB(JMSMessage& msg, const string& callerID);

      /** Message dispatcher */
      WxJMSMessageDispatch *p_msgDispatcher;

      /** Returns the app name that is the name in the resource registry. */
      virtual string getAppName() const = 0;

      virtual void initializeModel();
      virtual void createPanels();
      virtual void constructor();
      virtual void refreshGUI();

      virtual NWChemMDModel& getNWChemMDModel() const;

      /** Used to ignore events generated during initialization */
      bool p_ignoreCodeEventsFlag;

      FragmentSummary p_fragSummary;

      /** Ecce feedback/status area */
      WxFeedback *p_feedback;


};

#endif
