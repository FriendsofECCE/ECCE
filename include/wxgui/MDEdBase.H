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

#include "dsm/MDCompositeModel.H"

#include "wxgui/WxDavAuth.H"
#include "wxgui/CalcDropHandler.H"

#include "wxgui/MDEdBaseGUI.H"
#include "wxgui/MDPanelHelper.H"
#include "wxgui/FeedbackSaveHandler.H"

class JMSMessage;
class WxJMSMessageDispatch;
class WxFragmentImport;

class MDCompositeModel;
class GromacsInputsPanel;
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
      virtual void setContext(const string& url);
      virtual void generateInputFile(MdTask *task);
      virtual void saveModelAndInputFile(MdTask *task);
      virtual void resetPanel();
      void enableLaunch(bool flag);

      /** The GROMACS inputs (topology, structure) changed: recheck Launch. */
      void gromacsInputsChanged();

      /**
       * ECCE_TEST_MDED=<dir>: commands appended to <dir>/<tool>.cmd are run
       * as a person would (attach a file, save, switch tab...) and answered
       * in <dir>/<tool>.out.  Inert unless set; see runTestCommand().
       */
      void startTestHook();

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

      /** The task model, whichever MD code the task belongs to. */
      virtual MDCompositeModel& getMDModel() const;

      /**
       * Make the editor's model, and the panels bound to it, those of the
       * given code (an editor window is reused for tasks of either code).
       */
      void selectCode(bool gromacs);

      /** Point every panel at the sub-models of the current model. */
      string runTestCommand(const string& line);

      virtual void bindModels();

      /** Hide the controls that mean nothing for GROMACS, or show them. */
      virtual void applyCodeProfile(bool gromacs);

      /** "GROMACS" or "NWChem": the first word of the editor's title. */
      string codeTitle() const;

      /**
       * Notebook pages come and go with the code of the task (the Control
       * page is NWChem's, the Inputs page GROMACS's), so the pages are
       * kept in one list in their canonical order and shown or hidden by
       * identity rather than by index.
       */
      void registerPages();
      void setPageShown(wxWindow *page, bool show);

      struct PageEntry {
        wxWindow *win;
        wxString  title;
        bool      shown;
      };
      vector<PageEntry> p_pages;
      GromacsInputsPanel *p_gmxInputs;
      bool p_launchWanted;

      /** The panels the current model carries, set by initializeModel(). */
      vector<MDCompositeModel::GUIPanel> p_panelSet;

      bool p_isGromacs;

      /** Used to ignore events generated during initialization */
      bool p_ignoreCodeEventsFlag;

      FragmentSummary p_fragSummary;

      /** Ecce feedback/status area */
      WxFeedback *p_feedback;


};

#endif
