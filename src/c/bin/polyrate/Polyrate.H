/**
 * @file
 *
 *
 */

#ifndef POLYRATE_HH
#define POLYRATE_HH


#include "wx/wx.h"

#include "util/EcceException.H"
#include "util/IOException.H"
#include "util/EditListener.H"

#include "tdat/TaskApp.H"
#include "tdat/PolyrateModel.H"

#include "dsm/ResourceDescriptor.H"
#include "dsm/DirDyVTSTTask.H"

#include "wxgui/ewxChoice.H"
#include "wxgui/CalcDropHandler.H"
#include "wxgui/WxDavAuth.H"
#include "wxgui/WxJMSListener.H"
#include "wxgui/WxFeedback.H"

#include "PolyrateGUI.H"

class EditEvent;
class ewxDialog;
class ewxProgressDialog;
class JMSMessage;
class PolyrateTask;
class WxJMSMessageDispatch;


/**
 * GUI Implementation of NWChem DIRDYVTST editor.
 */

class Polyrate: public PolyrateGUI, 
               public WxDavAuth, 
               public WxJMSListener, 
               public EditListener,
               public TaskApp,
               public CalcDropHandler,
               public FeedbackSaveHandler
{    

   public:

      /** States that dictate actions a user can take. */
      enum PolyrateState { NO_SYSTEM, VALID_SYSTEM, PROVIDED_SYSTEM, VALID_TOPOLOGY};

      Polyrate(wxWindow* parent, wxWindowID id = SYMBOL_POLYRATEGUI_IDNAME,
            const wxString& caption = SYMBOL_POLYRATEGUI_TITLE,
            const wxPoint& pos = SYMBOL_POLYRATEGUI_POSITION,
            const wxSize& size = SYMBOL_POLYRATEGUI_SIZE,
            long style = SYMBOL_POLYRATEGUI_STYLE );
      ~Polyrate();

      void setState(Polyrate::PolyrateState state);
      Polyrate::PolyrateState getState() const;
      void refreshGUI();

      virtual void updateDisabledState(ResourceDescriptor::RUNSTATE state,
                                       bool forceMsg=false);

      virtual bool Show(bool show);

      // EditListener virtuals
      virtual void processEditCompletion(const EditEvent& ee);

      virtual string getTitle() const;
      virtual void setContext(const string& url) throw(EcceException);
      virtual void taskAppRaise();
      virtual void taskAppDestroy();
      virtual void taskAppShow();

      virtual void setUnitFamily(const string& family);

      // CalcDropHandler virtuals
      virtual void processDrop(const string& url);
      virtual void processSave();

      void setSaveState(bool unsavedChanges);
      void checkChemsysChange();
      void readyStateChange();
      bool isUnsavedState();

      void requestLauncher(const EcceURL& url, bool forceNew=true);

      /**
       * Event handlers
       */
      virtual void OnCheckboxPlyrtFroptClick( wxCommandEvent& event );
      virtual void OnTextctrlPlyrtSclrvEnter( wxCommandEvent& event );
      virtual void OnTextctrlPlyrtSgmfEnter( wxCommandEvent& event );
      virtual void OnTextctrlPlyrtSgmrEnter( wxCommandEvent& event );
      virtual void OnTextctrlPlyrtFsclEnter( wxCommandEvent& event );
      virtual void OnCheckboxPlyrtSddlClick( wxCommandEvent& event );
      virtual void OnCheckboxPlyrtAnlysClick( wxCommandEvent& event );
      virtual void OnButtonPlyrtAddClick( wxCommandEvent& event );
      virtual void OnButtonPlyrtDeleteClick( wxCommandEvent& event );
      virtual void OnChoicePlyrtCmpstSelected( wxCommandEvent& event );
      virtual void OnTextctrlPlyrtDeg1Enter( wxCommandEvent& event );
      virtual void OnTextctrlPlyrtNrg1Enter( wxCommandEvent& event );
      virtual void OnTextctrlPlyrtDeg2Enter( wxCommandEvent& event );
      virtual void OnTextctrlPlyrtNrg2Enter( wxCommandEvent& event );
      virtual void OnTextctrlPlyrtDeg3Enter( wxCommandEvent& event );
      virtual void OnTextctrlPlyrtNrg3Enter( wxCommandEvent& event );
      virtual void OnButtonPlyrtClrstClick( wxCommandEvent& event );
      virtual void OnChoicePlyrtCmpmdSelected( wxCommandEvent& event );
      virtual void OnButtonPlyrtMrsaddClick( wxCommandEvent& event );
      virtual void OnButtonPlyrtMrsdelClick( wxCommandEvent& event );
      virtual void OnListboxPlyrtMrsSelected( wxCommandEvent& event );
      virtual void OnButtonPlyrtTrsnadClick( wxCommandEvent& event );
      virtual void OnButtonPlyrtTrsndlClick( wxCommandEvent& event );
      virtual void OnListboxPlyrtTrsnSelected( wxCommandEvent& event );
      virtual void OnChoicePlyrtModeSelected( wxCommandEvent& event );
      virtual void OnChoicePlyrtDtypSelected( wxCommandEvent& event );
      virtual void OnTextctrlPlyrtDvalsEnter( wxCommandEvent& event );
      virtual void OnButtonPlyrtOptaddClick( wxCommandEvent& event );
      virtual void OnButtonPlyrtOptdelClick( wxCommandEvent& event );
      virtual void OnListboxPlyrtOptlstSelected( wxCommandEvent& event );
      virtual void OnChoicePlyrtCmpaxSelected( wxCommandEvent& event );
      virtual void OnChoicePlyrtAxisSelected( wxCommandEvent& event );

   protected:
      void initProgressDialog();
      void generateInputFile() throw(IOException);
      bool checkComponent(DirDyVTSTTask::Component comp);
      Fragment* getComponentFragment(DirDyVTSTTask::Component comp);
      DavCalculation* getComponentTask(DirDyVTSTTask::Component comp);
      void saveModelAndInputFile() throw(IOException);
      void doQuit(const bool& force);

      void setRunState(ResourceDescriptor::RUNSTATE state);
      void doSaveButtonClick(wxMouseEvent& event);

      /* utility functions */
      void initializeMenu(ewxChoice *menu);
      string convertString(string input);
      void setDataTypeMenu(int dmin, string level, string scheme);
      void setModeMenu();
      void checkTorsionOpts();

      virtual void OnMenuPlyrtCloseClick( wxCommandEvent &event );
      virtual void OnCloseWindow( wxCloseEvent &event);
      virtual void OnButtonPlyrtLaunchClick( wxCommandEvent& event );
      virtual void OnMenuPlyrtSaveClick( wxCommandEvent& event );
      virtual void OnMenuFeedbackClick( wxCommandEvent& event );
      virtual void OnMenuPlyrtHelpClick( wxCommandEvent& event );
      virtual void OnButtonPlyrtFinalEditClick( wxCommandEvent& event );

      // Messaging
      void initializeMessaging();
      void invokeMCB(JMSMessage& msg);
      void identifyMCB(JMSMessage& msg);
      void quitMCB(JMSMessage& msg);
      void createMCB(JMSMessage& msg);
      void renameMCB(JMSMessage& msg);
      void removeMCB(JMSMessage& msg);
      void changePropMCB(JMSMessage& msg);
      void stateMCB(JMSMessage& msg);
      void subjectMCB(JMSMessage& msg);
      void spinMCB(JMSMessage& msg);
      void propertyMCB(JMSMessage& msg);
      void statePrefsMCB(JMSMessage& msg);
      void authMCB(JMSMessage& msg);
      WxJMSMessageDispatch *p_msgDispatcher;

      void initializeModel();
      void initializeGUI();
      PolyrateModel& getPolyrateModel() const;

      ewxDialog         *p_inputDialog;
      ewxProgressDialog *p_progressDialog;

      /** Used to ignore events sent via code internals instead of user actions */
      bool  p_ignoreCodeEventsFlag;

      /** Ecce feedback/status area */
      WxFeedback *p_feedback;

      /** Parameter to determine if any information has been sent from Builder */
      bool p_touch;
      PolyrateState p_state;

      PolyrateTask *p_polyrateTask;  /**< The Polyrate task for the reaction study */
      Session *p_session; /**< The Reaction study */

      /** Grab the current unit family at initialization and store here. This
       *  will guarantee consisency if user changes units in gateway while this
       *  editor is up */
      string p_unitFamily;

      /** Save current string in data values window and use it to refresh window
       * if invalid string is entered */
      string p_dataValues;

      vector<int> p_trsnModes;

};


#endif
