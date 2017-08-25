/**
 * @file
 * @author Ken Swanson
 *
 */

#ifndef WXLAUNCHER_HH
#define WXLAUNCHER_HH

#include <string>
    using std::string;

#include "util/EcceMap.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/Preferences.H"

#include "tdat/Queue.H"
#include "tdat/RefMachine.H"

#include "dsm/MachinePreferences.H"
#include "dsm/TaskJob.H"
#include "dsm/ResourceDescriptor.H"

#include "comm/Launch.H"

#include "wxgui/CalcDropHandler.H"
#include "wxgui/WxDavAuth.H"
#include "wxgui/WxFeedback.H"

#include "WxLauncherGUI.H"

class ewxButton;
class ewxChoice;
class ewxMenuItem;
class ewxPanel;
class ewxStaticText;
class ewxTextCtrl;

#define WXLAUNCHER_DFLT_LEFT         -1
#define WXLAUNCHER_DFLT_TOP          -1
#define WXLAUNCHER_DFLT_WIDTH        -1
#define WXLAUNCHER_DFLT_HEIGHT       -1


class WxLauncher : public WxLauncherGUI,
                   public WxDavAuth,
                   public JMSPublisher,
                   public CalcDropHandler
{
    DECLARE_CLASS(WxLauncher)
    DECLARE_EVENT_TABLE()

    public:
        // Various states for switching context
        //      newContext - Start new context, no save required
        //      saveNew    - Save current job, start new context
        //      cancel     - Cancel invoke or drop
        enum          ContextSwitch { newContext, saveNew, cancel };

        //  Constructor, Destructor
        WxLauncher(      wxWindow* parent,
                         wxWindowID id = SYMBOL_WXLAUNCHERGUI_IDNAME,
                   const wxString& caption = SYMBOL_WXLAUNCHERGUI_TITLE,
                   const wxPoint& pos = SYMBOL_WXLAUNCHERGUI_POSITION,
                   const wxSize& size = SYMBOL_WXLAUNCHERGUI_SIZE,
                         long style = SYMBOL_WXLAUNCHERGUI_STYLE);
        ~WxLauncher();

        //  Implementation of CalcDropHandler interface
        virtual void processDrop(const string &url);

        void setStandAlone(bool w);
        void loadSettings(string fname);


        //  Event handler callbacks
        void mainWindowCloseCB(wxCloseEvent& event);

        void machinesChoiceSelectedCB(wxCommandEvent& event);

        void queueChoiceSelectedCB(wxCommandEvent& event);
        void allocationAccountTextCtrlUpdateCB( wxCommandEvent& event );

        void wkstnProcsParamValueChangedCB(wxCommandEvent& event);
        void priorityChoiceSelectedCB(wxCommandEvent& event);

        void batchProcsParamValueChangedCB(wxCommandEvent& event);
        void batchNodesParamValueChangedCB(wxCommandEvent& event);
        void maxMemoryParamValueChangedCB(wxCommandEvent& event);
        void minScratchParamValueChangedCB(wxCommandEvent& event);
        void wallTimeLimitChangedCB(wxCommandEvent& event);

        void remshellChoiceSelectedCB(wxCommandEvent& event);
        void usernameTextCtrlUpdateCB( wxCommandEvent& event);
        void password1TextCtrlUpdateCB( wxCommandEvent& event);
        void password2TextCtrlUpdateCB( wxCommandEvent& event);
        void shellOpenButtonClickCB(wxCommandEvent& event);

        void calculationDirectoryTextCtrlUpdateCB( wxCommandEvent& event );
        void scratchDirectoryTextCtrlUpdateCB( wxCommandEvent& event );
        void launchButtonClickCB(wxCommandEvent& event);

        static bool checkRemoteGlobus(RefMachine *machRgstn, string remShell);

        void focusGainedCB(wxFocusEvent& event);
        void focusLostCB(wxFocusEvent& event);

        void machRegisterMenuitemClickCB(wxCommandEvent& event);
        void stageLaunchMenuItemClickCB(wxCommandEvent& event);
        void finishLaunchMenuItemClickCB(wxCommandEvent& event);
        void quitMenuItemClickCB(wxCommandEvent& event);

        void helpLauncherMenuitemClickCB(wxCommandEvent& event);
        void helpSupportMenuitemClickCB(wxCommandEvent& event);

        void setContext(string cxtxt);
        void clearContext();
        EcceURL getContext();
        bool hasContext();
        void updateContext(const ResourceDescriptor::RUNSTATE& state);

        void validateCode();

        void showLaunchData(Launchdata ldat);
        void launchChangeNotify();
        void calcStateNotify(const ResourceDescriptor::RUNSTATE& state);
        void prefsChangeNotify(string refname);

        void setFeedbackCaption(string cptn);
        void showFeedbackMessage(string mesg, WxFeedback::Severity severity);

        void updateStatePreferences();

        void reloadMachinePreferences();

        static bool connectAllowed(MachinePreferences *prefs);
        static bool isGlobusSSH(RefMachine *machRgstn, string shellName);
        static bool isGlobusSSH(bool remoteGlobus, string shellName);
        static bool isRemoteGlobus(RefMachine *machRgstn, string shellName);

    private:
        void saveSettings();

        void populateMachinesList();
        string refreshShells(RefMachine *machRgstn);

        const Queue * refreshQueues(RefMachine *machRgstn);
        const Queue * getQueueSelection();

        void saveJob();

        void updatePreferences();
        void createControls();
        void ensureEditsApplied(bool moveToFront = false);

        bool checkConnectAllowed();
        bool checkConnectAllowed(bool pswd2Rqrd);

        bool checkLaunchAllowed();
        bool checkLaunchAllowed(bool cnctAllwd);
        bool checkLaunchAllowed(RefMachine *machRgstn, bool cnctAllwd);

        bool checkGlobusSSH(bool remoteGlobus, string shellName);
        bool checkGlobusSSH();

        unsigned long parseWallTime(string s);
        void stageLaunch();
        void cancelStagedLaunch();
        void finishStagedLaunch();

        bool validateMachineWithCode(const string& machineName, const string& code);
/*
        static void DebugEnterMethod(string name, string args);
        static void DebugEnterMethod(string name);
        static void DebugLeaveMethod();
        static void DebugWriteMessage(string mesg);
*/
        void refreshControls();
        void clearControls();
        void updateControls(Launchdata ldat);

        void refreshAllocationAccount();
        void refreshProcessors(RefMachine *machRgstn, const Queue *queue);
        void refreshNodes(int numProcs, int maxNodes);

        void refreshProcessors(RefMachine *machRgstn);

        void refreshPasswords(const bool& remoteGlobus, const string& shellName,
                              const string& machName, const string& user);

        int computeNodesValue(int numProcs, int numNodes, int slctProcs);
        int calcProcsPerNode(int procs, int nodes);

        void refreshWallTime(const Queue *queue);
        void refreshMemoryLimit(const Queue *slctQueue);
        void refreshScratchSpace(const Queue *slctQueue);

        void refreshCalcDirectory();
        void refreshScratchDirectory();

        bool getEditAllowed();
        void setEditAllowed(bool allwd, bool shellOpenAvbl = true);

        void updatePriority(int pval);
        void refreshAuthentication(RefMachine *machRgstn, string shellName);

        void refreshLaunchControls();
        void refreshLaunchControls(bool cnctAllwd);

        void verifyMachineListLocation(bool cnctAllwd);
        void relocateMachineListItem(int actlPostn, int trgtPostn);
        void relocateMachineListItem(string refname, int trgtPostn);

        void launchCalc(const bool &fullLaunchFlag);
        void buildArgs(EcceMap& kvargs);
        void startShell();


        Preferences         *p_toolPrefs;

        ewxChoice           *p_machinesChoice;
        ewxStaticText       *p_machineNameStaticText;

        ewxPanel            *p_batchSpcfcPanel;
        ewxPanel            *p_wkstnSpcfcPanel;

        WxParameterEdit     *p_wkstnProcsParamEdit;
        ewxPanel            *p_priorityPanel;
        ewxChoice           *p_priorityChoice;

        ewxPanel            *p_allocAcctPanel;
        ewxTextCtrl         *p_allocAcctTextCtrl;
        ewxPanel            *p_queuePanel;
        ewxStaticText       *p_queueLabelLeft;
        ewxStaticText       *p_queueLabelRight;
        ewxChoice           *p_queueChoice;

        WxParameterEdit     *p_batchProcsParamEdit;
        ewxPanel            *p_batchNodesPanel;
        WxParameterEdit     *p_batchNodesParamEdit;
        ewxStaticText       *p_batchProcsMaxStaticText;
        ewxPanel            *p_batchProcsMaxPanel;

        ewxPanel            *p_maxMemoryPanel;
        WxParameterEdit     *p_maxMemoryParamEdit;
        ewxPanel            *p_minScratchPanel;
        WxParameterEdit     *p_minScratchParamEdit;
        ewxPanel            *p_wallTimePanel;
        WxTimeSpanEdit      *p_wallTimeEdit;
        ewxPanel            *p_maxWallTimePanel;
        ewxStaticText       *p_maxWallTimeStaticText;

        ewxStaticText       *p_remShellLabel;
        ewxChoice           *p_remShellChoice;
        ewxButton           *p_shellOpenButton;

        ewxStaticText       *p_usernameLabel;
        ewxTextCtrl         *p_usernameTextCtrl;

        ewxStaticText       *p_password1Label;
        ewxTextCtrl         *p_password1TextCtrl;
        ewxStaticText       *p_password2Label;
        ewxTextCtrl         *p_password2TextCtrl;
        ewxPanel            *p_password1Panel;
        ewxPanel            *p_password2Panel;

        ewxPanel            *p_calcDrctyPanel;
        ewxTextCtrl         *p_calcDrctyTextCtrl;
        ewxPanel            *p_scratchDrctyPanel;
        ewxTextCtrl         *p_scratchDrctyTextCtrl;

        ewxButton           *p_launchButton;

        ewxMenuItem         *p_jobSaveMenuItem;
        ewxMenuItem         *p_machRegisterMenuItem;
        ewxMenuItem         *p_configAccessMenuItem;
        ewxMenuItem         *p_savePrefsMenuItem;
        ewxMenuItem         *p_stageLaunchMenuItem;
        ewxMenuItem         *p_finishLaunchMenuItem;
        ewxMenuItem         *p_quitMenuItem;
        ewxMenuItem         *p_helpLauncherMenuItem;
        ewxMenuItem         *p_helpSupportMenuItem;

        WxFeedback          *p_messagesFeedback;

        int                  p_maxProcsPerNode;

        MachinePreferences  *p_slctPrefs;
        TaskJob             *p_taskJob;

        bool                 p_prefsEdited;

        int p_numCfgd;
        int p_numIncl;


        bool                 p_inCtrlUpdate;
        bool                 p_editAllwd;
        bool                 p_jobStaged;
        Launch              *p_launchData;
 };


#endif  //  WXLAUNCHER_HH
