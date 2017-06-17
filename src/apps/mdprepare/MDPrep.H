#ifndef _MDPREP_H_
#define _MDPREP_H_

/*!
 * Includes
 */

#include "wx/wx.h"

#include "util/EcceException.H"
#include "util/IOException.H"
#include "util/EditListener.H"

#include "tdat/FragmentSummary.H"
#include "tdat/TaskApp.H"

#include "dsm/ResourceDescriptor.H"

#include "wxgui/CalcDropHandler.H"
#include "wxgui/WxDavAuth.H"
#include "wxgui/WxJMSListener.H"

#include "MDPrepGUI.H"

class EditEvent;
class ewxDialog;
class ewxListBox;
class ewxProgressDialog;
class ewxTextCtrl;
class JMSMessage;
class PrepareModel;
class MdTask;
class NWInputPanel;
class WxFeedback;
class WxJMSMessageDispatch;


/**
 * GUI Implementation of NWChem MD prepare module.
 */

class MDPrep: public MDPrepGUI, 
              public WxJMSListener, 
              public TaskApp,
              public WxDavAuth, 
              public EditListener,
              public CalcDropHandler,
              public FeedbackSaveHandler
{    

   public:

      /** States that dictate actions a user can take. */
      enum PrepState {NO_SYSTEM, VALID_SYSTEM, PROVIDED_SYSTEM, VALID_TOPOLOGY};

      MDPrep(wxWindow* parent, wxWindowID id = SYMBOL_MDPREPGUI_IDNAME,
            const wxString& caption = SYMBOL_MDPREPGUI_TITLE,
            const wxPoint& pos = SYMBOL_MDPREPGUI_POSITION,
            const wxSize& size = SYMBOL_MDPREPGUI_SIZE,
            long style = SYMBOL_MDPREPGUI_STYLE );
      ~MDPrep();

      void addListCmd(string cmd);

      void setState(MDPrep::PrepState state);
      void clearAll();
      MDPrep::PrepState getState() const;
      void refreshGUI();

      void updateDisabledState(ResourceDescriptor::RUNSTATE state, 
            bool forceMsg=false);

      void showTranslatePanel();
      void showOrientPanel();
      void showSolvatePanel();
      void enableModifyPanel();
      virtual bool Show(bool show);

      // EditListener virtuals
      virtual void processEditCompletion(const EditEvent& ee);

      // TaskApp virtuals
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
      bool isUnsavedState();
      void setTextDefault(ewxTextCtrl *text, double val);

   protected:
      void initProgressDialog();
      void reloadFragSummary(MdTask *task);
      void generateInputFile(MdTask *task) throw(IOException);
      void saveModelAndInputFile(MdTask *task) throw(IOException);
      void doQuit(bool force, bool notifyEditor);

      void setRunState(ResourceDescriptor::RUNSTATE state);
      //void doSaveButtonClick(wxCommandEvent& event);
      void doSaveButtonClick(wxMouseEvent& event);

      /// wxEVT_CLOSE_WINDOW event handler for ID_MDPREP_FRAME
      virtual void OnCloseWindow( wxCloseEvent& event );

      virtual void OnButtonMdprepGetchargeClick( wxCommandEvent& event );

      virtual void OnButtonMdprepConfigureClick( wxCommandEvent& event );

      virtual void OnButtonMdprepBuilderClick( wxCommandEvent& event );

      virtual void OnButtonMdprepChemsysClick( wxCommandEvent& event );

      virtual void OnRadiobuttonMdprepHidSelected( wxCommandEvent& event );

      virtual void OnRadiobuttonMdprepHieSelected( wxCommandEvent& event );

      virtual void OnRadiobuttonMdprepHipSelected( wxCommandEvent& event );

      virtual void OnChoiceMdprepFfchoiceSelected( wxCommandEvent& event );

      virtual void OnComboboxMdprepCinameSelected( wxCommandEvent& event );

      virtual void OnComboboxMdprepCinameEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepCinumEnter( wxCommandEvent& event );

      virtual void OnButtonMdprepCiaddClick( wxCommandEvent& event );

      virtual void OnButtonMdprepCidelClick( wxCommandEvent& event );

      virtual void OnTextctrlMdprepCiscaleEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepCigridszEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepCimindisEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepFrac1Enter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepFrac2Enter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepFrac3Enter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepFrac4Enter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepFrac5Enter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepFrac6Enter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepFrac7Enter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepFrac8Enter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepFrac9Enter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepFrac10Enter( wxCommandEvent& event );

      virtual void OnButtonMdprepLnkSelectClick( wxCommandEvent& event );

      virtual void OnButtonMdprepLnkAddClick( wxCommandEvent& event );

      virtual void OnButtonMdprepLnkDeleteClick( wxCommandEvent& event );

      virtual void OnButtonMdprepCenterClick( wxCommandEvent& event );

      virtual void OnButtonMdprepCenterxClick( wxCommandEvent& event );

      virtual void OnButtonMdprepCenteryClick( wxCommandEvent& event );

      virtual void OnButtonMdprepCenterzClick( wxCommandEvent& event );

      virtual void OnButtonMdprepTrnsSolClick( wxCommandEvent& event );

      virtual void OnRadiobuttonMdprepTrnsAtomSelected( wxCommandEvent& event );

      virtual void OnRadiobuttonMdprepTrnsMolSelected( wxCommandEvent& event );

      virtual void OnRadiobuttonMdprepTrnsSegSelected( wxCommandEvent& event );

      virtual void OnTextctrlMdprepTrnsXEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepTrnsYEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepTrnsZEnter( wxCommandEvent& event );

      virtual void OnButtonMdprepOrientClick( wxCommandEvent& event );

      virtual void OnRadiobuttonMdprepOrntAtomSelected( wxCommandEvent& event );

      virtual void OnRadiobuttonMdprepOrntMolSelected( wxCommandEvent& event );

      virtual void OnRadiobuttonMdprepOrntSegSelected( wxCommandEvent& event );

      virtual void OnTextctrlMdprepOrntAngleEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepOrntXEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepOrntYEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepOrntZEnter( wxCommandEvent& event );

      virtual void OnButtonMdprepRotateClick( wxCommandEvent& event );

      virtual void OnButtonMdprepAlignClick( wxCommandEvent& event );

      virtual void OnButtonMdprepOrntslClick( wxCommandEvent& event );

      virtual void OnChoiceMdprepBcoptSelected( wxCommandEvent& event );

      virtual void OnButtonMdprepSetbcClick( wxCommandEvent& event );

      virtual void OnTextctrlMdprepSlvMindistEnter( wxCommandEvent& event );

      virtual void OnChoiceMdprepSolventsSelected( wxCommandEvent& event );

      virtual void OnTextctrlMdprepSolNamEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepSolModEnter( wxCommandEvent& event );

      virtual void OnChoiceMdprepSlvGeomSelected( wxCommandEvent& event );

      virtual void OnButtonMdprepSolvateClick( wxCommandEvent& event );

      virtual void OnTextctrlMdprepMinsolvEnter( wxCommandEvent& event );

      virtual void OnButtonMdprepSetMinslvClick( wxCommandEvent& event );

      virtual void OnTextctrlMdprepExpndEnter( wxCommandEvent& event );

      virtual void OnButtonMdprepSetExpndClick( wxCommandEvent& event );

      virtual void OnTextctrlMdprepSlvtolEnter( wxCommandEvent& event );

      virtual void OnButtonMdprepSetSlvtolClick( wxCommandEvent& event );

      virtual void OnChoiceMdprepRmvOptSelected( wxCommandEvent& event );

      virtual void OnCheckboxMdprepXrangeClick( wxCommandEvent& event );

      virtual void OnCheckboxMdprepYrangeClick( wxCommandEvent& event );

      virtual void OnCheckboxMdprepZrangeClick( wxCommandEvent& event );

      virtual void OnButtonMdprepSlvRmvClick( wxCommandEvent& event );

      virtual void OnButtonMdprepSetdimClick( wxCommandEvent& event );

      virtual void OnChoiceMdprepRepoptsSelected( wxCommandEvent& event );

      virtual void OnTextctrlMdprepXrepEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepYrepEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepZrepEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepMinrsepEnter( wxCommandEvent& event );

      virtual void OnCheckboxMdprepRanxClick( wxCommandEvent& event );

      virtual void OnCheckboxMdprepRanyClick( wxCommandEvent& event );

      virtual void OnCheckboxMdprepRanzClick( wxCommandEvent& event );

      virtual void OnCheckboxMdprepInvcpyClick( wxCommandEvent& event );

      virtual void OnCheckboxMdprepUsezboxClick( wxCommandEvent& event );

      virtual void OnButtonMdprepReplicClick( wxCommandEvent& event );

      virtual void OnTextctrlMdprepSkipxEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepSkipyEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepSkipzEnter( wxCommandEvent& event );

      virtual void OnButtonMdprepSkipClick( wxCommandEvent& event );

      virtual void OnCheckboxMdprepSetmemClick( wxCommandEvent& event );

      virtual void OnTextctrlMdprepMemheapEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepMemstkEnter( wxCommandEvent& event );

      virtual void OnTextctrlMdprepMemglblEnter( wxCommandEvent& event );

      virtual void OnChoiceMdprepPrtlevSelected( wxCommandEvent& event );

      virtual void OnExitClick( wxCommandEvent& event );

      virtual void OnButtonMdprepLocalClick( wxCommandEvent& event );

      virtual void OnButtonMdprepRemoteClick( wxCommandEvent& event );

      virtual void OnButtonMdprepResetClick( wxCommandEvent& event );

      virtual void OnSaveClick( wxCommandEvent& event );

      virtual void OnMenuFeedbackClick( wxCommandEvent& event );

      virtual void OnMenuMdprepHelpClick( wxCommandEvent& event );

      virtual void OnButtonMdprepFinalEditClick( wxCommandEvent& event );

      virtual void OnButtonMdprepDeleteClick( wxCommandEvent& event );

      virtual void OnButtonMdprepUpClick( wxCommandEvent& event );

      virtual void OnButtonMdprepDownClick( wxCommandEvent& event );

      virtual void OnChoicePmfOptionSelected( wxCommandEvent& event );
      void doPmfOptionSelected( wxString choice );

      virtual void OnTextctrlPmfSelectAtoms1Updated( wxCommandEvent& event );

      virtual void OnButtonPmfSelectAtoms1Click( wxCommandEvent& event );

      virtual void OnButtonPmfShowAtoms1Click( wxCommandEvent& event );

      virtual void OnTextctrlPmfSelectAtoms2Updated( wxCommandEvent& event );

      virtual void OnButtonPmfSelectAtoms2Click( wxCommandEvent& event );

      virtual void OnButtonPmfShowAtoms2Click( wxCommandEvent& event );

      virtual void OnTextctrlPmfSelectAtoms3Updated( wxCommandEvent& event );

      virtual void OnButtonPmfSelectAtoms3Click( wxCommandEvent& event );

      virtual void OnButtonPmfShowAtoms3Click( wxCommandEvent& event );

      virtual void OnTextctrlPmfSelectAtoms4Updated( wxCommandEvent& event );

      virtual void OnButtonPmfSelectAtoms4Click( wxCommandEvent& event );

      virtual void OnButtonPmfShowAtoms4Click( wxCommandEvent& event );
      void doPmfSelectAtomsUpdated( int idText, int idIndex );
      void doPmfSelectAtoms( int id );
      void doPmfShowAtoms( int id );

      void OnButtonPmfDefaultsDistanceClick( wxCommandEvent& event );
       
      void OnButtonPmfDefaultsAngleClick( wxCommandEvent& event );

      virtual void OnButtonPmfAddDirectiveClick( wxCommandEvent& event );

      virtual void OnListboxPmfDirectivesSelected( wxCommandEvent& event );
      void doPmfRestoreDirective( wxString directive );

      virtual void OnButtonPmfUpClick( wxCommandEvent& event );

      virtual void OnButtonPmfDownClick( wxCommandEvent& event );

      virtual void OnButtonPmfDeleteClick( wxCommandEvent& event );


      // Messaging
      void initializeMessaging();
      void urlRenamedMCB(JMSMessage& msg);
      void urlRemovedMCB(JMSMessage& msg);
      void urlStateMCB(JMSMessage& msg);
      void urlSubjectMCB(JMSMessage& msg);
      void molSelectionMCB(JMSMessage& msg);
      void getChargeMCB(JMSMessage& msg);
      void getNWChemSelectionMCB(JMSMessage& msg);
      void getNWChemDistanceMCB(JMSMessage& msg);
      void getNWChemAngleMCB(JMSMessage& msg);
      void statePrefsMCB(JMSMessage& msg);
      void invokeMCB(JMSMessage& msg);
      void quitMCB(JMSMessage& msg);
      void identifyMCB(JMSMessage& msg);
      void prefsMCB(JMSMessage& msg);
      void authMCB(JMSMessage& msg);
      void processGlobalPreferenceChange();
      WxJMSMessageDispatch *p_msgDispatcher;

      void requestBuilder(const string& actionTopic="", bool forceNew=true,
                          map<string,string> * properties = NULL);
      void refreshMoleculeFields();
      void refreshFragmentDefaults(double *dims);


      void initializeModel();
      void initializeGUI();
      PrepareModel& getPrepareModel() const;

      ewxDialog         *p_inputDialog;
      NWInputPanel      *p_inputPanel;
      ewxListBox        *p_listBox;
      ewxProgressDialog *p_progressDialog;
      PrepState          p_state;
      FragmentSummary    p_fragSummary;

      /** Used to ignore events sent via code internals instead of user actions */
      bool  p_ignoreCodeEventsFlag;

      /** Ecce feedback/status area */
      WxFeedback *p_feedback;

      /** Parameter to determine if any information has been sent from Builder */
      bool p_touch;

      /** keeps track of which PMF selection text field is requesting from builder */
      int pmfSelectionId;
};

#endif
