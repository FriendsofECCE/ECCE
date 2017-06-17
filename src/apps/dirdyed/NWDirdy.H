/**
 * @file
 *
 *
 */

#ifndef NWDIRDY_HH
#define NWDIRDY_HH

#include <fstream>
  using std::ofstream;

#include <string>
  using std::string;

#include "wx/wx.h"
#include "wx/socket.h"

#include "util/EcceException.H"
#include "util/IOException.H"
#include "util/EditListener.H"

#include "dsm/ResourceDescriptor.H"
#include "dsm/DirDyVTSTTask.H"

#include "tdat/TaskApp.H"
#include "tdat/FragmentSummary.H"
#include "tdat/NWDirdyModel.H"
#include "tdat/SpinMult.H"

#include "wxgui/WxDavAuth.H"
#include "wxgui/WxJMSListener.H"
#include "wxgui/CalcDropHandler.H"
#include "wxgui/WxFeedback.H"

#include "NWDirdyGUI.H"
#include "NWDirdySetup.H"

#define wxID_THEORY_CHANGE 10998
#define wxID_SPTHEORY_CHANGE 10999

class ewxDialog;
class ewxProgressDialog;
class ewxChoice;
class ICalculation;
class JMSMessage;
class SFile;
class WxFeedback;
class WxJMSMessageDispatch;

class EditEvent;

/**
 * GUI Implementation of NWChem DIRDYVTST editor.
 */

class NWDirdy: public NWDirdyGUI, 
               public WxDavAuth, 
               public WxJMSListener, 
               public EditListener,
               public TaskApp,
               public CalcDropHandler,
               public FeedbackSaveHandler
{    

   public:

      /** States that dictate actions a user can take. */
      enum NWDirdyState { NO_SYSTEM, VALID_SYSTEM, PROVIDED_SYSTEM, VALID_TOPOLOGY};

      NWDirdy(wxWindow* parent, wxWindowID id = SYMBOL_NWDIRDYGUI_IDNAME,
            const wxString& caption = SYMBOL_NWDIRDYGUI_TITLE,
            const wxPoint& pos = SYMBOL_NWDIRDYGUI_POSITION,
            const wxSize& size = SYMBOL_NWDIRDYGUI_SIZE,
            long style = SYMBOL_NWDIRDYGUI_STYLE );
      ~NWDirdy();

      void setState(NWDirdy::NWDirdyState state);
      NWDirdy::NWDirdyState getState() const;
      void refreshGUI();

      bool getIgnoreFlag();
      void setIgnoreFlag(bool ignore);

      virtual void updateDisabledState(ResourceDescriptor::RUNSTATE state,
                                       bool forceMsg=false);

      virtual bool Show(bool show);

      // EditListener virtuals
      virtual void processEditCompletion(const EditEvent& ee);

      virtual string getTitle() const;
      virtual void setContext(const string& url) throw(EcceException);
      virtual void freeContext();
      virtual void taskAppRaise();
      virtual void taskAppDestroy();
      virtual void taskAppShow();

      virtual void setUnitFamily(const string& family);

      // CalcDropHandler virtuals
      virtual void processDrop(const string& url);
      virtual void processSave();

      bool doSave(const bool& reportFlag=true);
      void setSaveState(bool unsavedChanges);
      bool isUnsavedState();

      void requestBuilder(const EcceURL& url,
	                  const string& actionTopic="", bool forceNew=true);
      void requestCalcEditor(const EcceURL& url, const string& codename);
      void requestLauncher(const EcceURL& url, bool forceNew=true);
      void requestBasisTool(const string& actionTopic="", bool forceNew=true);
      void startGeneralTheoryApp(const bool& initFlag=false);
      void startSingleTheoryApp(const bool& initFlag=false);
      void closeGeneralTheoryApp(const bool& sendTerm);
      void closeSingleTheoryApp(const bool& sendTerm);
      void notifySubject(const string& url);
      void notifySpin(const string& url);

      virtual void OnGeneralTheoryChange( wxSocketEvent& event );
      virtual void OnSingleTheoryChange( wxSocketEvent& event );

      SpinMult::SpinMultEnum tsSpinMultiplicity(void);
      int tsCharge(void);
      int tsNuclearCharge(void);
      WxFeedback *getFeedback(void);
      DirDyVTSTTask *getDirDyVTSTTask();


   protected:
      void initProgressDialog();
      bool generateInputFile() throw(IOException);
      bool checkUniqueAtomIds(Fragment *frag1, Fragment *frag2, Fragment *ts,
                              const bool& reactFlag);
      bool checkUniqueTSAtomIds(Fragment *ts);
      Resource *addComponent(DirDyVTSTTask::Component comp);
      bool checkComponent(DirDyVTSTTask::Component comp);
      bool removeComponent(DirDyVTSTTask::Component comp);
      Resource *getComponent(DirDyVTSTTask::Component comp);
      bool saveModelAndInputFile(const bool& reportFlag) throw(IOException);
      void doQuit(const bool& force);
      void separateFragments(Fragment *frag1, Fragment *frag2, Fragment *ts);
      void alignFragment(Fragment *frag);
      bool restrictSpin(const int& numElectrons, ewxChoice* choice,
                        const bool& tsFlag=false);

      void checkStatus();
      void setRunState(ResourceDescriptor::RUNSTATE state);
      void doSaveButtonClick(wxMouseEvent& event);

      virtual  void OnBitmapbuttonDirdyBuildR1Click( wxCommandEvent& event );

      virtual  void OnButtonDirdyCalcR1Click( wxCommandEvent& event );

      virtual  void OnComboboxDirdyChargeR1Selected( wxCommandEvent& event );

      virtual  void OnComboboxDirdyChargeR1Enter( wxCommandEvent& event );

      virtual  void OnChoiceDirdySpinR1Selected( wxCommandEvent& event );

      virtual  void OnBitmapbuttonDirdyBuildR2Click( wxCommandEvent& event );

      virtual  void OnButtonDirdyCalcR2Click( wxCommandEvent& event );

      virtual  void OnComboboxDirdyChargeR2Selected( wxCommandEvent& event );

      virtual  void OnComboboxDirdyChargeR2Enter( wxCommandEvent& event );

      virtual  void OnChoiceDirdySpinR2Selected( wxCommandEvent& event );

      virtual  void OnBitmapbuttonDirdyBuildTsClick( wxCommandEvent& event );

      virtual  void OnButtonDirdyCalcTsClick( wxCommandEvent& event );

      virtual  void OnComboboxDirdyChargeTsSelected( wxCommandEvent& event );

      virtual  void OnComboboxDirdyChargeTsEnter( wxCommandEvent& event );

      virtual  void OnChoiceDirdySpinTsSelected( wxCommandEvent& event );

      virtual  void OnButtonDirdyCreateReactClick( wxCommandEvent& event );

      virtual  void OnButtonDirdyCreateProdClick( wxCommandEvent& event );

      virtual  void OnBitmapbuttonDirdyBuildP1Click( wxCommandEvent& event );

      virtual  void OnButtonDirdyCalcP1Click( wxCommandEvent& event );

      virtual  void OnComboboxDirdyChargeP1Selected( wxCommandEvent& event );

      virtual  void OnComboboxDirdyChargeP1Enter( wxCommandEvent& event );

      virtual  void OnChoiceDirdySpinP1Selected( wxCommandEvent& event );

      virtual  void OnBitmapbuttonDirdyBuildP2Click( wxCommandEvent& event );

      virtual  void OnButtonDirdyCalcP2Click( wxCommandEvent& event );

      virtual  void OnComboboxDirdyChargeP2Selected( wxCommandEvent& event );

      virtual  void OnComboboxDirdyChargeP2Enter( wxCommandEvent& event );

      virtual  void OnChoiceDirdySpinP2Selected( wxCommandEvent& event );

      /// wxEVT_CLOSE_WINDOW event handler for ID_NWDIRDY_FRAME
      virtual void OnCloseWindow( wxCloseEvent& event );

      virtual void OnExitClick( wxCommandEvent& event );

      virtual void OnButtonNwdirdySetupClick( wxCommandEvent& event );

      virtual void OnButtonNwdirdyLaunchClick( wxCommandEvent& event );

      virtual void OnSaveClick( wxCommandEvent& event );

      virtual void OnMenuFeedbackClick( wxCommandEvent& event );

      virtual void OnMenuNwdirdyHelpClick( wxCommandEvent& event );

      virtual void OnButtonNwdirdyFinalEditClick( wxCommandEvent& event );

      // Messaging
      void initializeMessaging();
      void invokeMCB(JMSMessage& msg);
      void identifyMCB(JMSMessage& msg);
      void quitMCB(JMSMessage& msg);
      void renameMCB(JMSMessage& msg);
      void removeMCB(JMSMessage& msg);
      void basisMCB(JMSMessage& msg);
      void stateMCB(JMSMessage& msg);
      void subjectMCB(JMSMessage& msg);
      void codeMCB(JMSMessage& msg);
      void propertyMCB(JMSMessage& msg);
      void statePrefsMCB(JMSMessage& msg);
      void selectionMCB(JMSMessage& msg);
      void thumbnailMCB(JMSMessage& msg);
      void authMCB(JMSMessage& msg);
      WxJMSMessageDispatch *p_msgDispatcher;

      void initializeModel();
      void initializeGUI();
      void createDynamicGUI();
      void enableChildren(wxBoxSizer* sizer, const bool& enabled=true);
      void reparentCodes(const bool& theoryFlag, const string& codeName,
                         wxBoxSizer* codeSizer, wxBoxSizer* calcedSizer);
      NWDirdyModel& getNWDirdyModel() const;

      string buildTheoryArgs(const string& theoryType);
      void orbitalParams(Fragment* frag, const string& theoryType,
                         int& numElectrons, int& spin,
                         int& numFrozenOrbs, int& numOccupiedOrbs,
                         int& numVirtualOrbs, int& numNormalModes);
      unsigned long coreElectrons(const unsigned long atomicNumber) const;
      void processGeneralTheoryInput(const char*);
      void processSingleTheoryInput(const char*);
      bool detailsReady(void);
      SpinMult::SpinMultEnum getSpinMult(const int& choiceId) const;
      void setDefaultDirdyBasisSet(DavCalculation* calc, Fragment* frag);
      void setDefaultBasisSet(DavCalculation* calc, Fragment* frag);
      void checkFragmentSave();
      void setBarrierSummaryField();


      ewxDialog         *p_inputDialog;
      ewxProgressDialog *p_progressDialog;

      /** Used to ignore events sent via code internals instead of user actions */
      bool  p_ignoreCodeEventsFlag;

      /** Ecce feedback/status area */
      WxFeedback *p_feedback;

      /** Parameter to determine if any information has been sent from Builder */
      bool p_touch;
      NWDirdyState p_state;
      NWDirdySetup *p_setupGUI;

      DirDyVTSTTask *p_dirdyTask;  /** The DirDyVTST task for the reaction study */
      Session *p_session; /** The reaction study */

      GUIValues        *p_generalGUIValues;
      int               p_generalTheoryPid;
      wxDatagramSocket *p_generalTheoryInSocket;
      unsigned short    p_generalTheoryInPort;
      SFile            *p_generalTheoryOutFile;
      ofstream         *p_generalTheoryOutStream;
      bool              p_generalTheoryInFlag;
      bool              p_generalTheoryHoldFlag;
      bool              p_generalTheoryInitFlag;
      GUIValues        *p_singleGUIValues;
      int               p_singleTheoryPid;
      wxDatagramSocket *p_singleTheoryInSocket;
      unsigned short    p_singleTheoryInPort;
      SFile            *p_singleTheoryOutFile;
      ofstream         *p_singleTheoryOutStream;
      bool              p_singleTheoryInFlag;
      bool              p_singleTheoryHoldFlag;
      bool              p_singleTheoryInitFlag;
      bool              p_debugFlag;

      // DECLARE_EVENT_TABLE()

};

#endif

