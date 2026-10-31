/**
 * @file
 * @author Ken Swanson
 *
 */


#include <fstream>
    using std::ifstream;
    using std::ofstream;
#include <ios>
    using std::ios;
#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <thread>
    using std::find;
    using std::min;
    using std::max;

#ifdef _WIN32
#include <windows.h>
#endif

#include "wx/event.h"
#include "wx/wx.h"

#include "util/BrowserHelp.H"
#include "wxgui/WxHelpViewer.H"
#include "wxgui/WxJMSSubscriber.H"
#include "util/Ecce.H"
#include "util/EcceSortedVector.H"
#include "util/ErrMsg.H"
#include "util/ResourceUtils.H"
#include "util/SFile.H"
#include "util/STLUtil.H"
#include "util/StringConverter.H"
#include "util/CpuCores.H"
#include "util/StringTokenizer.H"
#include "util/TempStorage.H"
#include "util/TypedFile.H"

#include "tdat/Fragment.H"
#include "tdat/GUIValues.H"
#include "tdat/QmSolventWarning.H"
#include "tdat/QueueMgr.H"
#include "tdat/TTheory.H"
#include "tdat/AuthCache.H"

#include "MemoryUnits.H"

#include "dsm/NWChemMDModel.H"
#include "dsm/CodeFactory.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ICalculation.H"
#include "dsm/JCode.H"
#include "dsm/MdTask.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceTool.H"
#include "dsm/VDoc.H"

#include "comm/EcceShell.H"
#include "comm/LaunchBatch.H"
#include "comm/JobWorker.H"
#include "comm/RCommand.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxMenuItem.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/ewxTextCtrl.H"

#include "wxgui/ewxThemeColours.H"
#include "WxLauncher.H"

#define MAX_PROCS_LIMIT     32
#define DFLT_WALL_TIME      120
#define MAX_PRIORITY        19


IMPLEMENT_CLASS( WxLauncher, WxLauncherGUI )

BEGIN_EVENT_TABLE( WxLauncher, WxLauncherGUI )
    EVT_SET_FOCUS( WxLauncher::focusGainedCB )
    EVT_KILL_FOCUS( WxLauncher::focusLostCB )

    EVT_COMMAND(ID_PARAMEDIT_WXLAUNCHER_WKSTNPROCS, ewxEVT_PARAMETER_VALUE_CHANGED, WxLauncher::wkstnProcsParamValueChangedCB)
    EVT_COMMAND(ID_PARAMEDIT_WXLAUNCHER_BATCHPROCS, ewxEVT_PARAMETER_VALUE_CHANGED, WxLauncher::batchProcsParamValueChangedCB)
    EVT_COMMAND(ID_PARAMEDIT_WXLAUNCHER_BATCHNODES, ewxEVT_PARAMETER_VALUE_CHANGED, WxLauncher::batchNodesParamValueChangedCB)
    EVT_COMMAND(ID_PARAMEDIT_WXLAUNCHER_MAXMEMORY, ewxEVT_PARAMETER_VALUE_CHANGED, WxLauncher::maxMemoryParamValueChangedCB)
    EVT_COMMAND(ID_PARAMEDIT_WXLAUNCHER_MINSCRATCH, ewxEVT_PARAMETER_VALUE_CHANGED, WxLauncher::minScratchParamValueChangedCB)
    EVT_COMMAND(ID_TIMESPANEDIT_WXLAUNCHER_WALLTIME, ewxEVT_TIMESPAN_VALUE_CHANGED, WxLauncher::wallTimeLimitChangedCB)
END_EVENT_TABLE()

int p_indentLevel = 0;
static int s_priorityLevels[] = { 0, 10, 19 };
static string s_machSeparator = " : : : : : : : : : : : : : : ";



// 80-column "ruler" for maximum line length
/*
         1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*/


WxLauncher::WxLauncher(      wxWindow* parent,
                             wxWindowID id,
                       const wxString& caption,
                       const wxPoint& pos,
                       const wxSize& size,
                             long style)
            : WxLauncherGUI(parent, id, caption, pos, size, style),
              WxDavAuth(this),
              JMSPublisher(LAUNCHER)
{

    p_toolPrefs = NULL;

    p_taskJob = NULL;
    p_slctPrefs = NULL;
    p_prefsEdited = false;
    p_inCtrlUpdate = false;
    p_editAllwd = true;
    p_jobStaged = false;
    p_launchData = NULL;

    this->createControls();

    // Get Registry
    ResourceDescriptor rs = ResourceDescriptor::getResourceDescriptor();

    // Set desktop icon
    ewxWindowUtils::setToolIcon(this, LAUNCHER);
}


WxLauncher::~WxLauncher()
{
    if (p_toolPrefs != NULL)
    {
        this->saveSettings();
        delete p_toolPrefs;
    }

}


void WxLauncher::loadSettings(string fname)
{
    p_toolPrefs = new Preferences(fname);

    int x = WXLAUNCHER_DFLT_LEFT;
    int y = WXLAUNCHER_DFLT_TOP;

    const string topic = "LauncherFrame";
    string member;

    member = topic + ".X";

    if (p_toolPrefs->isDefined(member))
        p_toolPrefs->getInt(member, x);

    member = topic + ".Y";

    if (p_toolPrefs->isDefined(member))
        p_toolPrefs->getInt(member, y);

    this->Move(x, y);
}


void WxLauncher::saveSettings()
{
    const string topic = "LauncherFrame";

    wxRect r = this->GetRect();

    p_toolPrefs->setInt(topic + ".X", r.GetLeft());
    p_toolPrefs->setInt(topic + ".Y", r.GetTop());

    p_toolPrefs->saveFile();
}


void WxLauncher::focusGainedCB(wxFocusEvent& event)
{
    event.Skip();
}


void WxLauncher::focusLostCB(wxFocusEvent& event)
{
    event.Skip();
}


/**
 *  Initialize pointers to graphical controls.
 */
void WxLauncher::createControls()
{
    p_machinesChoice        =     (ewxChoice *)(FindWindow(ID_CHOICE_WXLAUNCHER_MACHINES));
    p_machineNameStaticText = (ewxStaticText *)(FindWindow(ID_STATIC_WXLAUNCHER_MACHINENAME));

    p_batchSpcfcPanel       = (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_BATCHSPCFC));
    p_wkstnSpcfcPanel       = (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_WRKSTSPCFC));

    p_wkstnProcsParamEdit   = (WxParameterEdit *)(FindWindow(ID_PARAMEDIT_WXLAUNCHER_WKSTNPROCS));
    p_coresWarnStaticText   = (ewxStaticText *)(FindWindow(ID_STATIC_WXLAUNCHER_CORESWARN));
    p_localCores            = false;
    p_priorityPanel         =        (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_PRIORITY));
    p_priorityChoice        =       (ewxChoice *)(FindWindow(ID_CHOICE_WXLAUNCHER_PRIORITY));
    p_priorityChoice->Append("Normal");
    p_priorityChoice->Append("Reduced");
    p_priorityChoice->Append("Low");

    p_allocAcctPanel        =      (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_ALLOCACCT));
    p_allocAcctTextCtrl     =   (ewxTextCtrl *)(FindWindow(ID_TEXTCTRL_WXLAUNCHER_ALLOCACCT));
    p_queuePanel            =      (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_QUEUE));
    p_queueLabelLeft        = (ewxStaticText *)(FindWindow(ID_STATIC_WXLAUNCHER_QUEUELABELLEFT));
    p_queueLabelRight       = (ewxStaticText *)(FindWindow(ID_STATIC_WXLAUNCHER_QUEUELABELRIGHT));
    p_queueChoice           =     (ewxChoice *)(FindWindow(ID_CHOICE_WXLAUNCHER_QUEUE));

    p_batchProcsParamEdit   = (WxParameterEdit *)(FindWindow(ID_PARAMEDIT_WXLAUNCHER_BATCHPROCS));
    p_batchProcsMaxPanel         =        (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_BATCHPROCSMAX));
    p_batchProcsMaxStaticText    =   (ewxStaticText *)(FindWindow(ID_STATIC_WXLAUNCHER_BATCHPROCSMAX));
    p_batchNodesPanel       =        (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_BATCHNODES));
    p_batchNodesParamEdit   = (WxParameterEdit *)(FindWindow(ID_PARAMEDIT_WXLAUNCHER_BATCHNODES));

    p_maxMemoryPanel        =        (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_MAXMEMORY));
    p_maxMemoryParamEdit    = (WxParameterEdit *)(FindWindow(ID_PARAMEDIT_WXLAUNCHER_MAXMEMORY));
    p_minScratchPanel       =        (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_MINSCRATCH));
    p_minScratchParamEdit   = (WxParameterEdit *)(FindWindow(ID_PARAMEDIT_WXLAUNCHER_MINSCRATCH));
    p_wallTimePanel         =        (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_WALLTIME));
    p_wallTimeEdit          =  (WxTimeSpanEdit *)(FindWindow(ID_TIMESPANEDIT_WXLAUNCHER_WALLTIME));
    p_maxWallTimePanel      =        (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_MAXWALLTIME));
    p_maxWallTimeStaticText =   (ewxStaticText *)(FindWindow(ID_STATIC_WXLAUNCHER_MAXWALLTIME));

    p_remShellChoice        =       (ewxChoice *)(FindWindow(ID_CHOICE_WXLAUNCHER_REMSHELL));
    p_shellOpenButton       =       (ewxButton *)(FindWindow(ID_BUTTON_WXLAUNCHER_SHELLOPEN));

    p_usernameLabel         =   (ewxStaticText *)(FindWindow(ID_STATIC_WXLAUNCHER_USERNAMELABEL));
    p_usernameTextCtrl      =     (ewxTextCtrl *)(FindWindow(ID_TEXTCTRL_WXLAUNCHER_USERNAME));
    p_password1Panel        =        (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_PASSWORD1));
    p_password1TextCtrl     =     (ewxTextCtrl *)(FindWindow(ID_TEXTCTRL_WXLAUNCHER_PASSWORD1));
    p_password1Label        =   (ewxStaticText *)(FindWindow(ID_STATIC_WXLAUNCHER_PASSWORD1LABEL));

    p_calcDrctyPanel        =        (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_CALCDIR));
    p_calcDrctyTextCtrl     =     (ewxTextCtrl *)(FindWindow(ID_TEXTCTRL_WXLAUNCHER_CALCDIR));
    p_scratchDrctyPanel     =        (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_SCRATCHDIR));
    p_scratchDrctyTextCtrl  =     (ewxTextCtrl *)(FindWindow(ID_TEXTCTRL_WXLAUNCHER_SCRATCHDIR));

    p_launchButton          =       (ewxButton *)(FindWindow(ID_BUTTON_WXLAUNCHER_LAUNCH));
    p_launchButton->Enable(false);

    p_machSettingsButton    =       (ewxButton *)(FindWindow(ID_BUTTON_WXLAUNCHER_MACHSETTINGS));
    this->buildAdvanced();
    this->buildProfileRow();

    p_messagesFeedback = (WxFeedback *)(FindWindow(ID_FEEDBACK_WXLAUNCHER_MESSAGES));
    p_messagesFeedback->SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY | wxWS_EX_PROCESS_UI_UPDATES);
    p_messagesFeedback->setContextLabel("");
    p_messagesFeedback->setEditStatus(WxFeedback::NA);
    p_messagesFeedback->GetSizer()->SetMinSize(-1, 1);
    //  A fixed five lines: the fields, not the log, take the window's
    //  height, and a taller window gives the log no more room.
    p_messagesFeedback->SetMinSize(wxSize(-1, 5 * p_messagesFeedback->GetCharHeight() + 8));

    ewxMenuBar *mbar = (ewxMenuBar *)(this->GetMenuBar());
    p_machRegisterMenuItem = (ewxMenuItem *)(mbar->FindItem(ID_MENUITEM_WXLAUNCHER_MACHREGISTER));
    p_stageLaunchMenuItem  = (ewxMenuItem *)(mbar->FindItem(ID_MENUITEM_WXLAUNCHER_STAGELAUNCH));
    p_finishLaunchMenuItem = (ewxMenuItem *)(mbar->FindItem(ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH));
    p_quitMenuItem         = (ewxMenuItem *)(mbar->FindItem(wxID_EXIT));
    p_helpLauncherMenuItem = (ewxMenuItem *)(mbar->FindItem(wxID_HELP));
    p_helpSupportMenuItem  = (ewxMenuItem *)(mbar->FindItem(ID_MENUITEM_WXLAUNCHER_HELPSUPPORT));
}


/**
 *  Load the machines combobox with current list of machines from the MachinePreferences
 *  class.
 *
 *  This method depends on up-to-date information on the context, so any method that affects
 *  the context needs to be called before this one.  If a machine had been selected prior to
 *  this method being called, no attempt to reselect it will be made within this method.
 */
void WxLauncher::populateMachinesList()
{
    bool prevState = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    p_machinesChoice->Clear();

    p_numCfgd = 0;
    p_numIncl = 0;

    string code;
    if (p_taskJob != NULL)
        code = p_taskJob->application()->getCodeName();

    vector<MachinePreferences *> items = MachinePreferences::itemsForCode(code);

    for (size_t i = 0; i < items.size(); i++)
    {
        MachinePreferences *prefs = items[i];
        string refname = prefs->getRegisteredMachine()->refname();

        if (connectAllowed(prefs))
        {
            p_machinesChoice->Insert(refname, p_numCfgd, prefs);
            p_numCfgd++;
        }
        else
        {
            p_machinesChoice->Append(refname, prefs);
        }

        p_numIncl++;
    }

    if ((p_numCfgd > 0) && (p_numCfgd < p_numIncl))
        p_machinesChoice->Insert(s_machSeparator, p_numCfgd);

    p_inCtrlUpdate = prevState;
}


bool WxLauncher::connectAllowed(MachinePreferences *prefs)
{
    bool result = true;

    if (prefs->isOptionSupported("UN"))
        result = prefs->getUsername().size() > 0;

    return result;
}


/**
 *  Invoke the feedback area to display the indicated caption.
 */
void WxLauncher::setFeedbackCaption(string cptn)
{
    p_messagesFeedback->setContextLabel(cptn);
}


/**
 *  Place, hide/show controls depending on the properties/attributes of the selected
 *  machine.
 *
 *  This method assumes that the slctPrefs object is non-null.
 */
/**
 *  Size the frame to the controls now shown.  Fit() alone cannot shrink it:
 *  the minimum size set when every panel was still shown would hold it tall.
 */
void WxLauncher::refit()
{
    GetSizer()->SetSizeHints(this);
}


void WxLauncher::refreshControls()
{
    bool prevState = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    RefMachine *machRgstn = p_slctPrefs->getRegisteredMachine();
    // The host name and where the job runs are in the choice's tooltip;
    // the label beside it is only for "None registered".
    p_machineNameStaticText->SetLabel("");
    p_machineNameStaticText->Hide();

    bool isBatch = p_slctPrefs->isOptionSupported("Q");

    //  A calculation launched on this machine opens on the profile it was
    //  launched with, otherwise the profile last used here. Chosen once per
    //  machine: reloading after Register Machines saves keeps the user's pick.
    if (p_profileMachine != p_slctPrefs->getItemKey())
    {
        bool launchedHere = p_ctxLaunched &&
                            p_ctxLaunch.machine == p_slctPrefs->getItemKey();
        p_profile = p_slctPrefs->chooseProfile(
                        launchedHere ? p_ctxLaunch.profile : "", launchedHere);
        p_profileMachine = p_slctPrefs->getItemKey();
    }
    this->refreshProfileControls();

    if (isBatch)   // If the selected machine supports job queues...
    {
        this->refreshAllocationAccount();
        const Queue *slctQueue = this->refreshQueues(machRgstn);
        this->refreshProcessors(machRgstn, slctQueue);
        this->refreshMemoryLimit(slctQueue);
        this->refreshScratchSpace(slctQueue);
        this->refreshWallTime(slctQueue);
    }
    else
    {
        this->refreshProcessors(machRgstn);
        this->updatePriority(p_slctPrefs->getPriority());
    }

    //  Show and/or hide controls depending on whether or not machine is queued
    p_wkstnSpcfcPanel->Show(!isBatch);
    p_batchSpcfcPanel->Show(isBatch);

    string shellName = this->refreshShells(machRgstn);
    this->refreshAuthentication(machRgstn, shellName);
    this->refreshLocality();
    this->refreshRemoteSection();

    this->refreshCalcDirectory();
    this->refreshScratchDirectory();

    bool cnctAllwd = this->checkConnectAllowed();
    p_shellOpenButton->Enable(cnctAllwd);

    if (p_taskJob != NULL)
        this->refreshLaunchControls(cnctAllwd);

    this->Layout();
    this->refit();
    
    p_inCtrlUpdate = prevState;
}


void WxLauncher::clearControls()
{
    p_machineNameStaticText->SetLabel("");

    bool prevState = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    p_allocAcctTextCtrl->Clear();
    p_queueChoice->Clear();
    p_batchProcsParamEdit->setValue(1);
    p_batchNodesParamEdit->setValue(1);
    p_maxMemoryParamEdit->setValue(0);
    p_minScratchParamEdit->setValue(0);
    p_wallTimeEdit->setValue(0);

    p_wkstnProcsParamEdit->setValue(0);
    p_priorityChoice->SetSelection(-1);

    p_calcDrctyTextCtrl->Clear();
    p_scratchDrctyTextCtrl->Clear();

    p_remShellChoice->Clear();
    p_usernameTextCtrl->Clear();
    p_password1TextCtrl->Clear();

    p_shellOpenButton->Enable(false);

    if (p_taskJob != NULL)
        this->refreshLaunchControls(false);

    this->Layout();
    this->refit();

    p_inCtrlUpdate = prevState;
}


/**
 *  Called when the window closes.  A close event that can be vetoed arises via user
 *  interaction with the dialogue, typically via the window decorations.  If this is
 *  the case, and the application is running in standalone mode, then ask the user to
 *  confirm the close.  Otherwise, if this is a forced close, as from some other tool,
 *  or if not running in standalone mode, then just close without confirming.
 */
void WxLauncher::mainWindowCloseCB(wxCloseEvent& event)
{
    this->ensureEditsApplied();
    this->Destroy();
}


/**
 *  Contents of the allocation account text area have changed.  This method
 *  first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set, and the controls related to job launching are refreshed.
 */
void WxLauncher::allocationAccountTextCtrlUpdateCB(wxCommandEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        p_prefsEdited = true;

        this->refreshLaunchControls();
    }
}


/**
 *  Contents of the username text area have changed.  This method
 *  first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set, and if the application has a context, then the controls related
 *  to remote connections and job launching are refreshed.
 */
void WxLauncher::usernameTextCtrlUpdateCB(wxCommandEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        p_prefsEdited = true;

        bool cnctAllwd = this->checkConnectAllowed();

        p_shellOpenButton->Enable(cnctAllwd);
        this->verifyMachineListLocation(cnctAllwd);
        this->refreshLaunchControls(cnctAllwd);

        //  For a machine naming this host, the username decides local vs
        //  ssh (#144) -- keep the statement of which in step with it.
        this->refreshLocality();
        this->refreshRemoteSection();
        this->Layout();
        this->refit();
    }
}


/**
 *  Contents of the primary password text area have changed.  This method
 *  first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set.
 */
void WxLauncher::password1TextCtrlUpdateCB(wxCommandEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        p_prefsEdited = true;
    }
}


/**
 *  Opens a remote shell window to the selected compute server.  Any changes to
 *  the preferences for the selected machine area saved before the shell is
 *  opened.
 */
void  WxLauncher::shellOpenButtonClickCB(wxCommandEvent& event)
{
    string mesg;

    p_messagesFeedback->clearMessage();

    RefMachine *rgstn = p_slctPrefs->getRegisteredMachine();

    string remShell = (string)p_remShellChoice->GetStringSelection();
    string uname = (string)(p_usernameTextCtrl->GetValue());
    string pswd1 = STLUtil::trim((string)(p_password1TextCtrl->GetValue()));

    bool frontendFlag = rgstn->singleConnect() ||
                       (rgstn->frontendMachine()!="" &&
                        (rgstn->frontendBypass()=="" ||
                         !RCommand::isSameDomain(rgstn->frontendBypass())));

    if (!frontendFlag) {
        RCommand rcmd(rgstn->fullname(), remShell, rgstn->shell(), uname, pswd1,
                      rgstn->frontendMachine(), rgstn->frontendBypass(),
                      rgstn->shellPath(), rgstn->libPath(), rgstn->sourceFile(),
                      rgstn->twoFactorAuth() == "totp");

        if (!rcmd.isOpen())
        {
            mesg = "Opening remote shell failed:  " + rcmd.commError();
            p_messagesFeedback->setMessage(mesg, WxFeedback::ERROR, false);
            return;
        }
    }

    this->ensureEditsApplied();

    EcceShell *eshell = new EcceShell();
    string result = eshell->topshell(rgstn->refname(), remShell, uname, pswd1);

    if (result.size() == 0)
    {
        mesg = "Opening remote shell...\n"
               "Use the command \"df -k\" to show disk usage.";
        p_messagesFeedback->setMessage(mesg, WxFeedback::INFO, false);
    }
    else
    {
        mesg = "Opening remote shell failed:  " + result;
        p_messagesFeedback->setMessage(mesg, WxFeedback::ERROR, false);
    }
}


/**
 *  Contents of the calculation, or "run", directory text area have changed.
 *  This method first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set, and the controls related to job launching are refreshed.
 */
void WxLauncher::calculationDirectoryTextCtrlUpdateCB(wxCommandEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        p_prefsEdited = true;

        this->refreshLaunchControls();
    }
}


/**
 *  Contents of the scratch directory text area have changed.
 *  This method first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set, and the controls related to job launching are refreshed.
 */
void WxLauncher::scratchDirectoryTextCtrlUpdateCB(wxCommandEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        p_prefsEdited = true;

        this->refreshLaunchControls();
    }
}


/**
 *  Selection of the priority combo box has changed.
 *  This method first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set, and the controls related to job launching are refreshed.
 */
void WxLauncher::priorityChoiceSelectedCB(wxCommandEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        p_prefsEdited = true;

        this->refreshLaunchControls();
    }
}


/**
 *  Value of the workstation processors parameter edit has changed.
 *  This method first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set, and the controls related to job launching are refreshed.
 */
void WxLauncher::wkstnProcsParamValueChangedCB(wxCommandEvent& event)
{

//    cout << "WxLauncher::wkstnProcsParamValueChangedCB()" << endl;

    if (!p_inCtrlUpdate)
    {
        p_prefsEdited = true;

        this->refreshCoresWarning();
        this->refreshLaunchControls();
    }
}


/**
 *  Value of the batch (queued) processors parameter edit has changed.
 *  This method first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set, and the controls related to job launching are refreshed.
 */
void WxLauncher::batchProcsParamValueChangedCB(wxCommandEvent& event)
{
 //   cout << "WxLauncher::batchProcsParamValueChangedCBv()" << endl;
    if (!p_inCtrlUpdate)
    {
        p_prefsEdited = true;

        int maxNodes = p_slctPrefs->getRegisteredMachine()->nodes();

        if (maxNodes > 1)
        {
            //  Need to validate the nodes.
            p_inCtrlUpdate = true;

            int numProcs = p_batchProcsParamEdit->getValue();
            int minNodes = numProcs / p_maxProcsPerNode;

            if ((numProcs % p_maxProcsPerNode) > 0)
                minNodes++;

            p_batchNodesParamEdit->setValue(minNodes);
            p_inCtrlUpdate = false;
        }

        this->refreshLaunchControls();
    }
}


/**
 *  Value of the batch (queued) nodes parameter edit has changed.
 *  This method first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set, and the controls related to job launching are refreshed.
 */
void WxLauncher::batchNodesParamValueChangedCB(wxCommandEvent& event)
{
//    cout << "WxLauncher::batchNodesParamValueChangedCB()" << endl;
    if (!p_inCtrlUpdate)
    {
        p_prefsEdited = true;

        int numNodes = p_batchNodesParamEdit->getValue();
        int minProcs = numNodes;
        int maxProcs = numNodes * p_maxProcsPerNode;

        int numProcs = p_batchProcsParamEdit->getValue();

        if (numProcs < minProcs)
            p_batchProcsParamEdit->setValue(minProcs);

        if (numProcs > maxProcs)
            p_batchProcsParamEdit->setValue(maxProcs);

        this->refreshLaunchControls();
    }
}


/**
 *  Value of the maximum memory parameter edit has changed.
 *  This method first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set, and the controls related to job launching are refreshed.
 */
void WxLauncher::maxMemoryParamValueChangedCB(wxCommandEvent& event)
{
//    cout << "WxLauncher::maxMemoryParamValueChangedCB()" << endl;
    if (!p_inCtrlUpdate)
    {
        p_prefsEdited = true;

        this->refreshLaunchControls();
    }
}


/**
 *  Value of the minimum scratch parameter edit has changed.
 *  This method first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set, and the controls related to job launching are refreshed.
 */
void WxLauncher::minScratchParamValueChangedCB(wxCommandEvent& event)
{
//    cout << "WxLauncher::minScratchParamValueChangedCB()" << endl;
    if (!p_inCtrlUpdate)
    {
        p_prefsEdited = true;

        this->refreshLaunchControls();
    }
}


/**
 *  Value of the wall time time-span edit has changed.
 *  This method first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set, and the controls related to job launching are refreshed.
 */
void WxLauncher::wallTimeLimitChangedCB(wxCommandEvent& event)
{
//    cout << "WxLauncher::wallTimeLimitChangedCB()" << endl;
    if (!p_inCtrlUpdate)
    {
        p_prefsEdited = true;

        this->refreshLaunchControls();
    }
}


/**
 *  Queue selection has changed.
 *  This method first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set, and the controls related to job launching are refreshed.
 *  Potentially, as a queue selection can also
 *  affect the processors, walltime, memory limit, and scratch space, these attributes
 *  are verified against selected queue.
 */
void WxLauncher::queueChoiceSelectedCB(wxCommandEvent& event)
{
//    cout << "WxLauncher::queueChoiceSelectedCB()" << endl;
    if (!p_inCtrlUpdate)
    {
        const Queue *slctQueue
                 = (const Queue *)(p_queueChoice->GetClientData(p_queueChoice->GetSelection()));

        //  Here should be not so much updating, but just ensuring the existing
        //  values are within range for the newly selected queue.  Don't want
        //  to erase what has already been entered.

        p_prefsEdited = true;

        RefMachine *machRgstn = p_slctPrefs->getRegisteredMachine();

        this->refreshProcessors(machRgstn, slctQueue);
        this->refreshWallTime(slctQueue);
        this->refreshMemoryLimit(slctQueue);
        this->refreshScratchSpace(slctQueue);
        this->refreshLaunchControls();
    }
}


/**
 *  Updates the launch button and appropriate menu items.  This method
 *  first establishes that a connection is allowed.
 */
void WxLauncher::refreshLaunchControls()
{
    this->refreshLaunchControls(this->checkConnectAllowed());
}


/**
 *  Updates the launch button and appropriate menu items.  This
 *  version accomodates those situations in which the ability
 *  to establish a connection has already been determined.
 */
void WxLauncher::refreshLaunchControls(bool cnctAllwd)
{
    bool launchAllwd = this->checkLaunchAllowed(cnctAllwd);

    p_launchButton->Enable(launchAllwd);
    p_stageLaunchMenuItem->Enable(launchAllwd);
    p_finishLaunchMenuItem->Enable(launchAllwd && p_jobStaged);
}


/**
 *  Remote Shell selection has changed.
 *  This method first establishes that the change did not arise programmatically--that
 *  it is the result of user interaction.  If so, the prefsChgd flag is
 *  set, and if the application has a context, then the controls related
 *  to job launching are refreshed.  Potentially, as a shell selection can also
 *  affect the authentication values (username, password(s)), these attributes
 *  are verified against selected shell.
 */
void WxLauncher::remshellChoiceSelectedCB(wxCommandEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        //  Again, just enable/disable according to current options
        p_prefsEdited = true;

        string slctShell = (string)p_remShellChoice->GetStringSelection();
        RefMachine *machRgstn = p_slctPrefs->getRegisteredMachine();
        this->refreshAuthentication(machRgstn, slctShell);
        this->refreshLocality();

        bool cnctAllwd = this->checkConnectAllowed();

        this->verifyMachineListLocation(cnctAllwd);
        p_shellOpenButton->Enable(cnctAllwd);

        this->refreshLaunchControls(cnctAllwd);

        this->Layout();
        this->refit();
    }
}


/**
 *  Return true/false depending on whether all requirements for establishing a
 *  connection to the selected compute server have been met.
 */
bool WxLauncher::checkConnectAllowed()
{
    bool result = false;

    if (p_slctPrefs != NULL)
    {
        if (p_slctPrefs->isOptionSupported("UN"))
        {
            //  An empty Username on this computer is the user's own login:
            //  the job runs here (updatePreferences fills in the name).
            string user = (string)p_usernameTextCtrl->GetValue();
            result = (STLUtil::trim(user)).size() > 0 || namesThisHost();
        }
    }

    return result;
}


/**
 *  True when the selected machine is this computer and, with no Username,
 *  the job runs here rather than over the remote shell (#144).
 */
bool WxLauncher::namesThisHost() const
{
    if (p_slctPrefs == NULL)
        return false;
    RefMachine *machRgstn = p_slctPrefs->getRegisteredMachine();
    if (machRgstn == NULL)
        return false;
    return !RCommand::isRemote(machRgstn->fullname(),
                    (string)p_remShellChoice->GetStringSelection(), "");
}


/**
 *  True when the job for the selected machine runs on this computer: the
 *  machine names this host and the Username is empty or the user's own.
 *  Asked of RCommand::isRemote() as the launch does.
 */
bool WxLauncher::runsHere() const
{
    if (p_slctPrefs == NULL)
        return false;
    RefMachine *machRgstn = p_slctPrefs->getRegisteredMachine();
    if (machRgstn == NULL)
        return false;
    string user;
    if (p_slctPrefs->isOptionSupported("UN"))
        user = STLUtil::trim((string)p_usernameTextCtrl->GetValue());
    return !RCommand::isRemote(machRgstn->fullname(),
                    (string)p_remShellChoice->GetStringSelection(), user);
}


/**
 *  A job that runs here needs no login, password or remote shell, so the
 *  Remote Machine Access section is left out and only what still has an
 *  effect (another Username, Scratch Directory, Machine settings) sits
 *  under Advanced.  A Username for another user on a machine naming this
 *  host is the deliberate remote route: the section is shown then.
 *  The machine, not the typed name, decides where the Username field
 *  lives, so it never moves while the user types in it.
 */
//  A hidden star would collapse its grid column and shift the field to the
//  left of the rows that show one; painting it in the background keeps the
//  fields on one x.
static void starShown(wxWindow *st, bool shown)
{
    st->SetForegroundColour(shown ? ewxThemeColours::statusText(ewxThemeColours::BAD)
                                  : st->GetParent()->GetBackgroundColour());
    st->Refresh();
}


void WxLauncher::refreshRemoteSection()
{
    const bool thisHost = namesThisHost();
    //  A queued machine keeps the full form, as in 9.2.2, even on this
    //  host: the simplified local form is for jobs started directly here.
    const bool batch = p_slctPrefs->isOptionSupported("Q");
    const bool fold = thisHost && !batch;
    const bool hide = runsHere() && !batch;
    if (wxWindow *req = FindWindow(ID_STATIC_WXLAUNCHER_USERNAMEREQUIRED))
        starShown(req, !thisHost);

    this->placeAdvancedWidgets(fold);

    if (wxWindow *userPanel = FindWindow(ID_PANEL_WXLAUNCHER_USERNAME))
        userPanel->Show(p_slctPrefs->isOptionSupported("UN"));
    if (wxWindow *block = FindWindow(ID_PANEL_WXLAUNCHER_6))
        block->Show(!hide);
    //  A job that runs here has no password or shell to give; a queued one
    //  still shows its Username (another name is the ssh-to-self route).
    {
        const bool here = runsHere();
        if (wxWindow *shellPanel = FindWindow(ID_PANEL_WXLAUNCHER_REMSHELL))
            shellPanel->Show(!here);
        p_shellOpenButton->Show(!here);
        p_password1Panel->Show(!here && p_slctPrefs->isOptionSupported("PW"));
    }
    if (wxWindow *heading = FindWindow(ID_STATIC_WXLAUNCHER_REMOTEHEADING))
    {
        wxSizer *row = heading->GetContainingSizer();
        wxWindow *parent = heading->GetParent();
        if (row && parent && parent->GetSizer())
            parent->GetSizer()->Show(row, !hide, true);
    }

    // Another user's login saved for this host: show where it is.
    if (fold && !hide && !p_advOpen)
        this->setAdvanced(true, false);

    for (wxWindowList::compatibility_iterator n =
             p_calcDrctyPanel->GetChildren().GetFirst(); n; n = n->GetNext())
        if (wxStaticText *st = wxDynamicCast(n->GetData(), wxStaticText))
            if (st->GetLabel() == "*")
                starShown(st, !hide);

    // Nothing is marked required for a local job, so neither is the key.
    if (wxWindow *key = FindWindow(ID_STATIC_WXLAUNCHER_REQUIREDFIELDSKEY))
        if (wxSizer *row = key->GetContainingSizer())
            for (size_t i = 0; i < row->GetItemCount(); i++)
                if (wxStaticText *st = wxDynamicCast(row->GetItem(i)->GetWindow(), wxStaticText))
                    if (st == key)   // emptied, not hidden: it pushes Launch right
                        st->SetLabel(hide ? "" : "Required value");
                    else if (st->GetLabel() == "*")
                        starShown(st, !hide);
}


/**
 *  The profile row goes in under the machine choice. Built here, not in the
 *  generated layout, so the form's sizers stay as they are.
 */
void WxLauncher::buildProfileRow()
{
    wxWindow *machRow = p_machinesChoice->GetParent()->GetParent();
    wxWindow *top = machRow->GetParent();
    wxSizer *sz = top->GetSizer();
    size_t at = 0;
    for (; at < sz->GetItemCount(); at++)
        if (sz->GetItem(at)->GetWindow() == machRow)
            break;

    p_profilePanel = new wxPanel(top);
    wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
    p_profilePanel->SetSizer(row);
    p_profileLabel = new wxStaticText(p_profilePanel, wxID_ANY, "Profile:",
                                      wxDefaultPosition, wxSize(120, -1));
    p_profileChoice = new wxChoice(p_profilePanel, wxID_ANY);
    p_profileChoice->SetToolTip("An allocation profile of this machine: account, run directory, scratch directory and queue. Profiles are made in Register Machines.");
    row->Add(p_profileLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);
    row->Add(p_profileChoice, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);
    sz->Insert(at + 1, p_profilePanel, 0, wxGROW|wxALL, 3);
    p_profilePanel->Hide();

    p_profileChoice->Bind(wxEVT_CHOICE, &WxLauncher::profileChoiceCB, this);
}


/**
 *  What the profile row shows for the selected machine and p_profile.
 */
void WxLauncher::refreshProfileControls()
{
    vector<string> names = p_slctPrefs->profileNames();

    p_profileChoice->Clear();
    for (size_t i = 0; i < names.size(); i++)
        p_profileChoice->Append(names[i]);
    p_profileChoice->SetStringSelection(p_profile);

    bool several = names.size() > 1;
    p_profileLabel->Show(several);
    p_profileChoice->Show(several);
    p_profilePanel->Show(several);
    p_profilePanel->Layout();
}


/**
 *  The per-allocation values as the controls show them, under name; a value
 *  the machine has no field for keeps what the profile held.
 */
AllocationProfiles::Profile WxLauncher::profileFromControls(const string& name)
{
    AllocationProfiles::Profile p = p_slctPrefs->getProfile(p_profile);
    p.name = name;

    if (p_slctPrefs->isOptionSupported("Q") && p_queueChoice->GetCount() > 0)
        p.queue = (string)(p_queueChoice->GetStringSelection());

    if (p_slctPrefs->isOptionSupported("AA"))
        p.account = STLUtil::trim((string)(p_allocAcctTextCtrl->GetValue()));

    if (p_slctPrefs->isOptionSupported("RD"))
        p.rundir = STLUtil::trim((string)(p_calcDrctyTextCtrl->GetValue()));

    p.scratchdir = STLUtil::trim((string)(p_scratchDrctyTextCtrl->GetValue()));
    return p;
}


void WxLauncher::selectProfile(const string& name)
{
    p_messagesFeedback->clearMessage();
    this->updatePreferences();     // the edits stay with the profile being left
    this->ensureEditsApplied();
    p_profile = p_slctPrefs->chooseProfile(name, true);
    p_slctPrefs->setLastProfile(p_profile);
    MachinePreferences::saveChanges();
    this->refreshControls();
}


void WxLauncher::profileChoiceCB(wxCommandEvent& event)
{
    int k = p_profileChoice->GetSelection();
    if (k >= 0 && !p_inCtrlUpdate)
        this->selectProfile((string)p_profileChoice->GetString(k));
}


bool WxLauncher::hookProfile(const string& name)
{
    vector<string> names = p_slctPrefs->profileNames();
    for (size_t i = 0; i < names.size(); i++)
        if (AllocationProfiles::lower(names[i]) == AllocationProfiles::lower(name))
        {
            this->selectProfile(names[i]);
            return true;
        }
    return false;
}


/**
 *  The fold's button and panel go in after the directories.  A button, not
 *  a wxCollapsiblePane, as in Register Machines.
 */
void WxLauncher::buildAdvanced()
{
    wxWindow *dirs = p_calcDrctyPanel->GetParent();
    wxWindow *top = dirs->GetParent();
    wxSizer *sz = top->GetSizer();
    size_t at = 0;
    for (; at < sz->GetItemCount(); at++)
        if (sz->GetItem(at)->GetWindow() == dirs)
            break;

    p_advButton = new wxButton(top, wxID_ANY,
        wxString::FromUTF8("\xe2\x96\xb8 Advanced"),
        wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT|wxBORDER_NONE);
    p_advButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        this->setAdvanced(!p_advOpen, true);
    });
    p_advPanel = new wxPanel(top);
    p_advPanel->SetSizer(new wxBoxSizer(wxVERTICAL));
    sz->Insert(at + 1, p_advButton, 0, wxALIGN_LEFT|wxLEFT, 6);
    sz->Insert(at + 2, p_advPanel, 0, wxGROW|wxLEFT|wxRIGHT, 3);
    p_advButton->Hide();
    p_advPanel->Hide();

    wxWindow *wins[3] = { FindWindow(ID_PANEL_WXLAUNCHER_USERNAME),
                          p_scratchDrctyPanel, p_machSettingsButton };
    for (int i = 0; i < 3; i++)
    {
        wxSizer *s = wins[i]->GetContainingSizer();
        wxSizerItem *it = s->GetItem(wins[i]);
        size_t idx = 0;
        for (; idx < s->GetItemCount(); idx++)
            if (s->GetItem(idx) == it)
                break;
        p_slots[i] = { wins[i], wins[i]->GetParent(), s, idx,
                       it->GetProportion(), it->GetFlag(), it->GetBorder() };
    }

    //  Moved into the fold, these controls are no longer below the panel
    //  whose event table handles them, so they are bound directly.
    p_usernameTextCtrl->Bind(wxEVT_TEXT, &WxLauncher::usernameTextCtrlUpdateCB, this);
    if (wxWindow *sc = FindWindow(ID_TEXTCTRL_WXLAUNCHER_SCRATCHDIR))
        sc->Bind(wxEVT_TEXT, &WxLauncher::scratchDirectoryTextCtrlUpdateCB, this);
    p_machSettingsButton->Bind(wxEVT_BUTTON, &WxLauncher::machRegisterMenuitemClickCB, this);

    bool open = false;
    ewxConfig::getConfig("wxlauncher.ini")->Read("/Launcher/AdvancedOpen", &open, false);
    p_advOpen = open;
}


void WxLauncher::setAdvanced(bool open, bool save)
{
    p_advOpen = open;
    p_advButton->SetLabel(wxString::FromUTF8(open ? "\xe2\x96\xbe Advanced"
                                                  : "\xe2\x96\xb8 Advanced"));
    p_advPanel->Show(p_inFold && open);
    if (save)
    {
        ewxConfig *cfg = ewxConfig::getConfig("wxlauncher.ini");
        cfg->Write("/Launcher/AdvancedOpen", open);
        cfg->Flush();
    }
    this->Layout();
    this->refit();
}


/**
 *  Move Username, Scratch Directory and Machine settings into the fold or
 *  back to their places; a remote machine shows them where they always were.
 */
void WxLauncher::placeAdvancedWidgets(bool inFold)
{
    if (inFold != p_inFold)
    {
        for (int i = 0; i < 3; i++)
        {
            FoldSlot &s = p_slots[i];
            if (inFold)
            {
                s.sizer->Detach(s.win);
                s.win->Reparent(p_advPanel);
                //  The fields span the column; the button keeps its size.
                p_advPanel->GetSizer()->Add(s.win, 0,
                    (s.win == p_machSettingsButton ? wxALIGN_LEFT : wxGROW)|wxALL, 3);
            }
            else
            {
                p_advPanel->GetSizer()->Detach(s.win);
                s.win->Reparent(s.parent);
                s.sizer->Insert(s.idx, s.win, s.prop, s.flag, s.border);
            }
        }
        p_inFold = inFold;
    }
    p_advButton->Show(inFold);
    p_advPanel->Show(inFold && p_advOpen);
    p_advButton->SetLabel(wxString::FromUTF8(p_advOpen ? "\xe2\x96\xbe Advanced"
                                                       : "\xe2\x96\xb8 Advanced"));
}


void WxLauncher::verifyMachineListLocation(bool cnctAllwd)
{
    string refname = p_slctPrefs->getItemKey();
    int k = p_machinesChoice->FindString(refname);
    int linePostn = ((p_numCfgd == 0) ? -1 : p_numCfgd);

    if (cnctAllwd && (k > linePostn))
    {
        if (linePostn < 0)
        {
            //  Insert a line at position 0, increment position of selected item
            p_machinesChoice->Insert(s_machSeparator, 0);
            linePostn = 0;
            k++;
        }

        this->relocateMachineListItem(k, linePostn);
        linePostn++;

        if (linePostn == p_numIncl)
            p_machinesChoice->Delete(linePostn);

    }
    else if ((k < linePostn) && !cnctAllwd)
    {
        if (linePostn == p_numIncl)
        {
            p_machinesChoice->Append(s_machSeparator);
        }

        this->relocateMachineListItem(k, linePostn + 1);
        linePostn--;

        if (linePostn == 0)
            p_machinesChoice->Delete(linePostn);
    }

    p_machinesChoice->SetStringSelection(refname);
}


/**
 *  Returns true/false depending on whether all requirements for launching a
 *  job on the selected compute server have been met.
 */
bool WxLauncher::checkLaunchAllowed()
{
    bool result = false;

    if (p_slctPrefs != NULL)
    {
        RefMachine *machRgstn = p_slctPrefs->getRegisteredMachine();

        bool cnctAllwd = checkConnectAllowed();
        result = checkLaunchAllowed(machRgstn, cnctAllwd);
    }

    return result;
}


/**
 *  Returns true/false depending on whether all requirements for launching a
 *  job on the selected compute server have been met.
 */
bool WxLauncher::checkLaunchAllowed(bool cnctAllwd)
{
    bool result = false;

    if (p_slctPrefs != NULL)
    {
        RefMachine *machRgstn = p_slctPrefs->getRegisteredMachine();
        result = checkLaunchAllowed(machRgstn, cnctAllwd);
    }

    return result;
}


/**
 *  Returns true/false depending on whether all requirements for launching a
 *  job on the selected compute server have been met.
 */
bool WxLauncher::checkLaunchAllowed(RefMachine *machRgstn, bool cnctAllwd)
{
    //  Issue #94: for a noRemoteAccess machine there is no connection to
    //  allow, so requiring one here disables the Launch button outright and
    //  clicking it does nothing at all -- no message, no error. checkConnect
    //  Allowed() returns false unless the machine supports the "UN" option
    //  AND a username has been typed, and a machine ECCE never contacts has
    //  no business asking for either. The connect requirement is therefore
    //  waived for these; everything below it (job state, the machine having
    //  the code, queue/account/directory fields) still applies, because the
    //  input deck and submit script are generated exactly as they would be
    //  for a real machine.
    bool localOnly = (machRgstn != (RefMachine*)0 && machRgstn->noRemoteAccess());

    bool result = ((cnctAllwd || localOnly) && (p_taskJob != NULL)
                             && (p_taskJob->getState() == ResourceDescriptor::STATE_READY));

    if (result)
    {
        //  If this point is reached, then we know that p_taskJob is not null
        string code = p_taskJob->application()->getCodeName();
        result = (result && machRgstn->offersCode(code));
    }

    if (result && p_slctPrefs->isOptionSupported("Q"))
    {
        result = result && (p_queueChoice->GetSelection() >= 0);
    }

    if (result && p_slctPrefs->isOptionSupported("AA"))
    {
        result = result && ((STLUtil::trim((string)(p_allocAcctTextCtrl->GetValue()))).size() > 0);
    }

    if (result && p_slctPrefs->isOptionSupported("RD"))
    {
        string fpath = STLUtil::trim((string)(p_calcDrctyTextCtrl->GetValue()));
        result = result && (fpath.size() > 0);
    }

    return result;
}


/**
 *  Update properties of the current preferences with values from the graphical
 *  controls.
 */
void WxLauncher::updatePreferences()
{
    string remShell = (string)(p_remShellChoice->GetStringSelection());
    p_slctPrefs->setRemoteShell(remShell);

    if (p_slctPrefs->isOptionSupported("Q"))
    {
        p_slctPrefs->setProcessors(p_batchProcsParamEdit->getValue());
        p_slctPrefs->setNodes(p_batchNodesParamEdit->getValue());

        if (p_slctPrefs->isOptionSupported("TL"))
            p_slctPrefs->setMaxWallTime(p_wallTimeEdit->getValue());

        if (p_slctPrefs->isOptionSupported("MM"))
            //  Field is GB; setMemoryLimit()/MachinePreferences store MB.
            p_slctPrefs->setMemoryLimit(MemoryUnits::gbToMB(p_maxMemoryParamEdit->getValue()));

        if (p_slctPrefs->isOptionSupported("SS"))
            p_slctPrefs->setScratchSpace(p_minScratchParamEdit->getValue());
    }
    else
    {
        p_slctPrefs->setProcessors(p_wkstnProcsParamEdit->getValue());

        int k = p_priorityChoice->GetSelection();
        p_slctPrefs->setPriority(s_priorityLevels[k]);
    }

    if (p_slctPrefs->isOptionSupported("UN"))
    {
        string ptext = (string)p_usernameTextCtrl->GetValue();

        if ((STLUtil::trim(ptext)).empty())
            p_slctPrefs->setUsername(Ecce::realUser());
        else
            p_slctPrefs->setUsername(ptext);
    }

    //  Account, run and scratch directory and queue belong to the profile in
    //  use, so a second allocation never overwrites the first's.
    p_slctPrefs->setProfile(this->profileFromControls(p_profile));
    p_slctPrefs->setLastProfile(p_profile);
    p_prefsEdited = false;
}


/**
 *  Notify other interested Ecce tools, via messaging, that the preferences have
 *  changed for the indicated machine.
 */
void WxLauncher::prefsChangeNotify(string refname)
{
    //  Publish a prefs_updated message; the test hook runs without a broker.
    if (getenv("ECCE_LAUNCHER_SCRIPT") != NULL)
        return;
    JMSMessage* mesg = newMessage();

    mesg->addProperty("machName", refname);
    publish("ecce_machconf_changed", *mesg);
}


/**
 *  Machine selection has changed.  This method first establishes that
 *  the change did not arise programmatically--that it is the result of
 *  user interaction.  It also establishes whether it was the separator line
 *  between the configured and unconfigured machines that was selected.
 *  If it was the separator line, then the selection is effectively cancelled.
 *  If it is one of the machines, then save any changes to the current preferences,
 *  select the new preferences, and update the graphical interface.
 */
void WxLauncher::machinesChoiceSelectedCB(wxCommandEvent& event)
{
    string slctKey = ((p_slctPrefs != NULL) ? p_slctPrefs->getItemKey() : "");
    int k = p_machinesChoice->GetSelection();

    if (p_machinesChoice->FindString(slctKey) != k)
    {
        int linePostn = ((p_numCfgd == 0) ? -1 : p_numCfgd);

        if (k == linePostn)
            p_machinesChoice->SetStringSelection(slctKey);

        else
        {
            p_messagesFeedback->clearMessage();

            this->ensureEditsApplied();
            p_slctPrefs = (MachinePreferences *)p_machinesChoice->GetClientData(k);

            this->refreshControls();
        }
    }
}


/**
 *  Return true/false depending on whether editing is allowed on the interface.
 */
bool WxLauncher::getEditAllowed()
{
    return p_editAllwd;
}


/**
 *  Allow/disallow editing.  This method typically called when launching a job
 *  to prevent the user from changing the launch parameters, and when resetting an inactive job to
 *  restart.
 */
void WxLauncher::setEditAllowed(bool allwd, bool shellOpenAvbl)
{
    if (p_editAllwd != allwd)
    {
        p_editAllwd = allwd;
        this->setFrameCustomDisabledStyle(p_editAllwd);
        p_messagesFeedback->setEditStatus(p_editAllwd ? WxFeedback::EDIT : WxFeedback::READONLY);

        if (shellOpenAvbl)   //  Ensure that the shell open button is still available
            p_shellOpenButton->setCustomDisabledStyle(true);

        //  Changing a machine's settings does not edit this launch.
        p_machSettingsButton->setCustomDisabledStyle(true);
    }
}


/**
 *  Display the indicated message in the feedback area.
 */
void WxLauncher::showFeedbackMessage(string mesg, WxFeedback::Severity severity)
{
    p_messagesFeedback->setMessage(mesg, severity);
}


/**
 *  Launch the job on the selected machine with the parameters set in the graphical
 *  interface.  In many cases, the task job already has some of its parameters set,
 *  and these could be different than those contained in the preferences object;
 *  however, any differences will be reflected in the UI.  This method will first save
 *  the job, launch it, then ensure that the selected machine is at the top of the
 *  list in the combobox, and within the MachinePreferences collection itself.
 */
void WxLauncher::launchButtonClickCB(wxCommandEvent& event)
{
    if (p_jobStaged)
        this->cancelStagedLaunch();

    this->refreshLaunchControls();

    p_messagesFeedback->clearMessage();

    this->ensureEditsApplied(true);
    if (p_taskJob != NULL &&
        !confirmMpiLauncher(p_taskJob->application()->getCodeName()))
    {
        p_messagesFeedback->setMessage("Launch cancelled.", WxFeedback::INFO);
        return;
    }
    if (!confirmQmSize())
    {
        p_messagesFeedback->setMessage("Launch cancelled.", WxFeedback::INFO);
        return;
    }
    this->launchCalc(true);
}


// A range of times in round minutes, or in hours from 90 minutes up.
static string roughRange(double lo, double hi)
{
    char buf[96];
    if (hi < 60.0)
        return "under a minute";
    if (hi < 90.0 * 60.0)
        snprintf(buf, sizeof(buf), "%.0f to %.0f minutes", lo / 60.0, hi / 60.0);
    else
        snprintf(buf, sizeof(buf), "%.1f to %.1f hours", lo / 3600.0, hi / 3600.0);
    return buf;
}


/**
 * The text of the ECCE-QM size warning, "" when the run is small.  ECCE-QM
 * keeps the two-electron integrals in memory (nbf^4 bytes: 170 MB at 114
 * functions, 4 GB at 250) and a frequency run does 6N gradients, so its time
 * grows as N^2 * nbf: about 0.06 N^2 nbf s for DFT on 8 idle cores, a third
 * of that for HF (docs/qm/README.md).  A 4-core laptop takes twice as long.
 * Raman activities add a polarisability (six more SCFs) at every displaced
 * geometry, about seven times the frequency run.
 */
static string qmSizeWarning(const string& runType, bool dft, size_t atoms, int nbf, bool raman)
{
    string msg;
    double work = (double)atoms * (double)atoms * (double)nbf;
    bool frequencies = runType == "Vibration" || runType == "GeoVib";
    if (frequencies && raman)
        work *= 7.0;
    if (frequencies && work > (dft ? 5000.0 : 15000.0))
    {
        double desktop = work * (dft ? 0.06 : 0.02);
        msg = "A " + string(runType == "Vibration" ? "frequency" : "geometry and frequency") +
              " run" + (raman ? " with Raman activities" : "") + " on " +
              StringConverter::toString((int)atoms) + " atoms with " +
              StringConverter::toString(nbf) + " basis functions takes roughly " +
              roughRange(desktop, 2.0 * desktop) +
              " on a typical laptop (the shorter time is for an 8-core desktop).";
    }
    if (atoms > 25 || nbf > 150)
    {
        double bytes = (double)nbf * nbf * nbf * nbf;
        char buf[64];
        if (bytes >= 1.0e9)
            snprintf(buf, sizeof(buf), "%.1f GB", bytes / 1.0e9);
        else
            snprintf(buf, sizeof(buf), "%.0f MB", bytes / 1.0e6);
        if (!msg.empty())
            msg += "\n\n";
        msg += "ECCE-QM keeps the two-electron integrals in memory: " +
               StringConverter::toString((int)atoms) + " atoms and " +
               StringConverter::toString(nbf) + " basis functions need about " + buf +
               " of RAM (170 MB at 114 functions, 4 GB at 250).";
    }
    return msg;
}


/**
 * Launch of an ECCE-QM calculation that is large for a student machine: say
 * how long it is likely to take and how much memory it needs, and offer
 * Cancel.  Only a warning; launching as set is the default.  False when the
 * user cancelled.
 */
bool WxLauncher::confirmQmSize()
{
    if (p_taskJob == NULL || p_taskJob->application()->getCodeName() != "ECCE-QM")
        return true;
    ICalculation *calc = dynamic_cast<ICalculation*>(p_taskJob);
    if (calc == NULL)
        return true;

    string msg;
    Fragment *frag = calc->fragment();
    TGBSConfig *config = calc->gbsConfig();
    TTheory *theory = calc->theory();
    bool raman = false;
    string solventMsg;
    GUIValues *details = calc->guiparams();
    if (details != NULL)
    {
        GUIValue *solv = details->get("ES.Theory.ECCEQM.Solvent");
        GUIValue *tick = details->get("ES.Runtype.Vibration.UseRaman");
        raman = tick != NULL && tick->getValueAsString() == "1";
        solventMsg = qmSolventWarning(solv != NULL ? solv->getValueAsString() : string(),
                                      calc->runtype().name(), raman);
        delete details;
    }
    if (frag != NULL && config != NULL && theory != NULL)
    {
        TagCountMap *tags = frag->tagCountsSTL();
        if (tags != NULL)
        {
            msg = qmSizeWarning(calc->runtype().name(), theory->category() == "DFT",
                                frag->numAtoms(), config->num_functions(*tags), raman);
            delete tags;
        }
    }
    delete frag;
    delete config;
    delete theory;
    const bool sizeWarn = !msg.empty();
    if (!solventMsg.empty())
        msg = solventMsg + (msg.empty() ? "" : "\n\n" + msg);
    if (msg.empty())
        return true;

    ewxMessageDialog dlg(this, msg, sizeWarn ? "Large ECCE-QM calculation" : "ECCE-QM solvent", wxICON_EXCLAMATION);
    dlg.AddButton(wxID_CANCEL, "Cancel");
    dlg.AddButton(wxID_YES, "Launch anyway")->SetDefault();
    return dlg.ShowModal() == wxID_YES;
}


// The MPI launcher the job script starts for this code on more than one
// core, when it cannot be found here; "" when it is found or not needed.
// A user command or setup snippet in CONFIG may bring its own MPI (a
// module load, say), so a code configured that way is not checked.
static string missingMpiLauncher(const string& code, RefMachine *mach)
{
    string key;
    StringConverter::toLower(code, key);
    if (key == "nwchemmd")
        key = "nwchem";          // gensub runs NWChemMD with the nwchem entry
    if (key != "nwchem" && key != "orca" && key != "quantumespresso" &&
        key != "cp2k" && key != "gromacs")
        return "";

    map<string,string> cfg = RefMachine::config(mach->refname());
    auto value = [&cfg](const string& k) {
        map<string,string>::const_iterator it = cfg.find(k);
        return it == cfg.end() ? string() : STLUtil::trim(it->second);
    };
    if (!value(key + "command").empty() || !value(key + "_setup").empty() ||
        !value("setup").empty())
        return "";

    string exe = value(key);
    string::size_type slash = exe.find_last_of("/\\");
    string base = slash == string::npos ? exe : exe.substr(slash + 1);
    if (key == "gromacs" && base.compare(0, 7, "gmx_mpi") != 0)
        return "";               // thread-MPI gmx needs no mpirun
    if (key == "cp2k" && base.size() >= 4 &&
        (base.compare(base.size() - 4, 4, "ssmp") == 0 ||
         base.compare(base.size() - 4, 4, "sopt") == 0))
        return "";               // the threads-only builds run without mpirun

#ifdef __WXMSW__
    string launcher = key == "orca" ? "mpiexec" : "mpirun";
    wxString file = launcher + ".exe";
#else
    string launcher = "mpirun";
    wxString file = launcher;
#endif
    wxPathList dirs;
    dirs.AddEnvList("PATH");
    if (!value("xappspath").empty())
        dirs.Add(value("xappspath"));
    if (!value("qmgrpath").empty())
        dirs.Add(value("qmgrpath"));
    if (slash != string::npos) {
        dirs.Add(exe.substr(0, slash));
        if (key == "nwchem")     // an ECCE-deployed NWChem's own MPI
            dirs.Add(exe.substr(0, slash) + "/../../system/bin");
    }
    return dirs.FindAbsoluteValidPath(file).empty() ? launcher : "";
}


/**
 * Launch with more than one core on this computer, for a code whose job
 * script starts mpirun/mpiexec that is not installed: offer one core or
 * Cancel before anything is submitted.  Remote and queued machines are not
 * checked; their PATH is not this one.  False when the user cancelled.
 */
bool WxLauncher::confirmMpiLauncher(const string& code)
{
    if (p_slctPrefs == NULL || p_slctPrefs->isOptionSupported("Q"))
        return true;
    RefMachine *mach = p_slctPrefs->getRegisteredMachine();
    int procs = p_slctPrefs->getProcessors();
    if (mach == NULL || procs <= 1)
        return true;
    string user = p_slctPrefs->isOptionSupported("UN")
                      ? p_slctPrefs->getUsername() : "";
    if (RCommand::isRemote(mach->fullname(), p_slctPrefs->getRemoteShell(), user))
        return true;
    string missing = missingMpiLauncher(code, mach);
    if (missing.empty())
        return true;

    string mpi = missing == "mpiexec" ? "Microsoft MPI" : "Open MPI";
    string msg = code + " needs " + missing + " to run on " +
                 StringConverter::toString(procs) + " cores, and " + missing +
                 " was not found on this computer.\n\nInstall " + mpi +
                 " to run in parallel, or run this job on 1 core.";
    //  Only a warning: the search can miss an MPI the user's own setup
    //  provides, so launching as set stays the default.
    ewxMessageDialog dlg(this, msg, "MPI not found", wxICON_EXCLAMATION);
    dlg.AddButton(wxID_CANCEL, "Cancel");
    dlg.AddButton(wxID_OK, "Run on 1 core");
    dlg.AddButton(wxID_YES, "Launch anyway")->SetDefault();
    const int answer = dlg.ShowModal();
    if (answer == wxID_YES)
        return true;
    if (answer != wxID_OK)
        return false;

    p_inCtrlUpdate = true;
    p_wkstnProcsParamEdit->setValue(1);
    p_inCtrlUpdate = false;
    p_prefsEdited = true;
    this->ensureEditsApplied(false);
    return true;
}


/**
 * A problem the code's launch check found that the job will meet.  Only a
 * warning: the user knows best, so launching anyway is the default.  False
 * when the user cancelled.
 */
bool WxLauncher::confirmLaunchWarning(const string& text)
{
    // A check that reworded the tool's message sends the original after a
    // marker line; it goes in a closed pane.
    string shown = text, original;
    const string mark = "\nECCE-ORIGINAL-MESSAGE\n";
    size_t at = text.find(mark);
    if (at != string::npos)
    {
        shown = text.substr(0, at);
        original = text.substr(at + mark.size());
    }
    // the text is UTF-8 (a real minus sign); a byte string that is not falls
    // back to the 8-bit reading
    wxString msg = wxString::FromUTF8(shown.c_str());
    if (msg.empty() && !shown.empty())
        msg = wxString::From8BitData(shown.c_str());
    ewxMessageDialog dlg(this, msg, "Launch check", wxICON_EXCLAMATION);
    if (!original.empty())
        dlg.AddDetails("grompp's message", wxString::FromUTF8(original.c_str()));
    dlg.AddButton(wxID_CANCEL, "Cancel");
    dlg.AddButton(wxID_YES, "Launch anyway")->SetDefault();
    return dlg.ShowModal() == wxID_YES;
}


void WxLauncher::ensureEditsApplied(bool itemToFront)
{
    int slctnIdx = p_machinesChoice->GetSelection();

    if ((p_prefsEdited) || (itemToFront && (slctnIdx > 0)))
    {
        if (p_prefsEdited)
            this->updatePreferences();

        string refname = p_slctPrefs->getItemKey();

        if (itemToFront && (slctnIdx > 0))
        {
            MachinePreferences::moveItemToFront(refname);
            this->relocateMachineListItem(refname, 0);
            p_machinesChoice->SetStringSelection(refname);
        }

        MachinePreferences::saveChanges();

        if (p_prefsEdited)
        {
            string msg = "Machine setup updated for ";
            msg += refname + ".";
            p_messagesFeedback->setMessage(msg, WxFeedback::INFO);
        }

        this->prefsChangeNotify(refname);
    }
}


void WxLauncher::relocateMachineListItem(string refname, int trgtPostn)
{
    this->relocateMachineListItem(p_machinesChoice->FindString(refname), trgtPostn);
}


void WxLauncher::relocateMachineListItem(int actlPostn, int trgtPostn)
{
    if ((trgtPostn >= 0) && (trgtPostn <= p_machinesChoice->GetCount()))
    {
        if (actlPostn != trgtPostn)
        {
            bool prevState = p_inCtrlUpdate;
            p_inCtrlUpdate = true;

            MachinePreferences *prefs = (MachinePreferences *)p_machinesChoice->GetClientData(actlPostn);
            p_machinesChoice->Delete(actlPostn);

            if (actlPostn < trgtPostn)
                trgtPostn--;

            if (actlPostn <= p_numCfgd)
            {
                p_numCfgd--;
            }

            p_machinesChoice->Insert(prefs->getItemKey(), trgtPostn, (void *)prefs);

            if (trgtPostn <= p_numCfgd)
            {
                p_numCfgd++;
            }

            p_inCtrlUpdate = prevState;
        }
    }
}

/**
 *  Programmatically select the appropriate item in the priority combo,
 *  depending on the value of pval.
 */
void WxLauncher::updatePriority(int pval)
{
    int k;

    if (pval < ((s_priorityLevels[0] + s_priorityLevels[1]) / 2))
        k = 0;
    else if (pval < ((s_priorityLevels[1] + s_priorityLevels[2]) / 2))
        k = 1;
    else
        k = 2;

    p_priorityChoice->SetSelection(k);
}


/**
 *  Programmatically update the graphical controls pertaining to the allocation account.
 *  This method verifies that the selected machine supports allocation accounts, and
 *  show or hides the controls appropriately.  If supported, then the value from
 *  the selected preferences is displayed.
 */
void WxLauncher::refreshAllocationAccount()
{
    bool supported = p_slctPrefs->isOptionSupported("AA");

    if (supported)
        p_allocAcctTextCtrl->SetValue(p_slctPrefs->getProfile(p_profile).account);

    p_allocAcctPanel->Show(supported);

    //  The Queue choice no longer shares this row -- it sits beside
    //  Processors -- so the row is empty without an allocation account
    //  and is hidden outright rather than left as a zero-height cell.
    //  The queue label is always the right-aligned one now, matching
    //  Nodes: and Memory Limit: in the same column position.
    wxWindow *batch0 = FindWindow(ID_PANEL_WXLAUNCHER_BATCH0);
    if (batch0 != NULL)
        batch0->Show(supported);

    p_queueLabelLeft->Show(false);
    p_queueLabelRight->Show(true);
}


/**
 *  Load the names of the available queues into the appropriate drop-down list.
 *  Always shown for a queued machine (this method is only reached when the
 *  machine supports the "Q" option) -- even a single queue must still be
 *  visible and selectable, since isLaunchAllowed() requires a queue selection
 *  and hiding the control leaves Launch silently disabled with nothing to
 *  explain why (#131).  Queue referenced by the machRgstn object is selected
 *  and returned.
 */
const Queue * WxLauncher::refreshQueues(RefMachine *machRgstn)
{
    const Queue *slctQueue = NULL;

    bool prevState = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    p_queueChoice->Clear();

    vector<string *> *machQueues = machRgstn->queues();
    const QueueManager *queueMngr = QueueManager::lookup(machRgstn->refname());

    if ((machQueues != NULL) && (machQueues->size() > 0))
    {
        string queueName;

        for (int i = 0; i < machQueues->size(); i++)
        {
            queueName = *(*machQueues)[i];

            p_queueChoice->Append(queueName);
            p_queueChoice->SetClientData(i, (void *)(queueMngr->queue(queueName)));
        }

        string slctnName = p_slctPrefs->getProfile(p_profile).queue;
        if (slctnName.empty())
            slctnName = p_slctPrefs->getQueueName();

        if (slctnName == "")
        {
            p_queueChoice->SetSelection(0);
        }
        else if (!p_queueChoice->SetStringSelection(slctnName))
        {
            //  The remembered queue is no longer defined for this machine
            //  (removed or renamed in Machine Registration).  Without this
            //  the choice opens BLANK, getQueueSelection() returns NULL and
            //  isLaunchAllowed() refuses with no visible reason.
            p_queueChoice->SetSelection(0);
        }

        slctQueue = this->getQueueSelection();
    }
    else
    {
        this->showFeedbackMessage(
            "No queues are defined for " + machRgstn->fullname() +
            " -- add one in Machine Registration's Queues tab.",
            WxFeedback::ERROR);
    }

    p_queuePanel->Show(true);
    p_inCtrlUpdate = prevState;

    return slctQueue;
}


/**
 *  Updates the control(s) that indicate the number of processors to
 *  use for the job on the selected machine.
 *
 *  Use of this method is appropriate only for queued machines, and
 *  must be called only after the queue for the machine is selected.
 *  The queue should not evaluate to NULL when this method is called;
 *  if it does, then some other error is occurring.
 */
void WxLauncher::refreshProcessors(RefMachine *machRgstn, const Queue *slctQueue)
{
    //  Might need to check that I'm properly handling the situation in which the
    //  selected queue changes for a given machine registration.  Don't necessarily
    //  want to discard edits, but do want to verify that that current values (in
    //  the param edit) are valid.  Same with mem limit, scratch space, etc.

    int minProcs = 1;
    int maxProcs = machRgstn->proccount();

    int numProcs = p_slctPrefs->getProcessors();
    int dfltProcs = 1;

    if (slctQueue != NULL)
    {
        minProcs = max(minProcs, (int)slctQueue->minProcessors());
        maxProcs = min(maxProcs, (int)slctQueue->maxProcessors());
        dfltProcs = slctQueue->defProcessors();
    }

    if (numProcs <= 0)
    {
        if (dfltProcs <= 0)
            dfltProcs = max(minProcs, (int)(floor((double)(maxProcs / 2))));

        numProcs = dfltProcs;
    }

    numProcs = max(numProcs, minProcs);
    numProcs = min(numProcs, maxProcs);

    p_batchProcsParamEdit->setRange(minProcs, maxProcs);
    p_batchProcsParamEdit->setValue(numProcs);
    p_batchProcsParamEdit->Update();

    int totProcs = machRgstn->proccount();
    int totNodes = machRgstn->nodes();
    p_maxProcsPerNode = calcProcsPerNode(totProcs, totNodes);
    int maxNodes = maxProcs / p_maxProcsPerNode;

    if ((maxProcs % p_maxProcsPerNode) > 0)
        maxNodes++;

    p_batchProcsMaxStaticText->SetLabel(StringConverter::toString(p_maxProcsPerNode));

    this->refreshNodes(numProcs, maxNodes);

    bool hasMultiNodes = (totNodes > 1);
    p_batchNodesPanel->Show(hasMultiNodes);
    p_batchProcsMaxPanel->Show(hasMultiNodes);
}


/**
 *  Update the controls pertaining to the memory limit based on the selected
 *  queue.  This method first establishes that the selected machine supports
 *  a memory limit, and shows/hides the controls accordingly.  If supported
 *  the current value displayed, subject to the limits imposed by the queue.
 */
void WxLauncher::refreshMemoryLimit(const Queue *slctQueue)
{
    bool supported = p_slctPrefs->isOptionSupported("MM");

    if (supported)
    {
        //  All of these are MB (Queue::memLimit()/defMemory(),
        //  MachinePreferences' stored memoryLimit) -- the field is shown
        //  in GB (Andy, 2026-09-07), converted here at the widget only.
        unsigned long value = p_slctPrefs->getMemoryLimit();
        unsigned long vmax = value;
        unsigned long dflt = 0;

        if (slctQueue != NULL)
        {
            vmax = slctQueue->memLimit();
            dflt = slctQueue->defMemory();
        }

        if (value == 0)
            value = ((dflt == 0) ? (vmax / 10) : dflt);

        //  0 (a machine without queues) and Queue's INT_MAX (a queue
        //  without memLimit) mean no limit: no upper bound shown, and the
        //  saved value stands.
        const bool limited = vmax > 0 && vmax < (unsigned long)INT_MAX;
        if (limited && value > vmax)
            value = vmax / 2;

        p_maxMemoryParamEdit->setRange(0, limited ? MemoryUnits::mbToGB(vmax)
                                                  : 65536);
        p_maxMemoryParamEdit->setRangeVisible(limited);
        p_maxMemoryParamEdit->setValue(MemoryUnits::mbToGB(value));
        p_maxMemoryParamEdit->setUnitsLabel("GB");
        // The generated layout hides the units label; show it once it has text.
        p_maxMemoryParamEdit->setUnitsVisible(true);
        p_maxMemoryParamEdit->Layout();
        p_maxMemoryParamEdit->Fit();
    }

    p_maxMemoryPanel->Show(supported);
}

/**
 *  Update the controls pertaining to the scratch space based on the selected
 *  queue.  This method first establishes that the selected machine supports
 *  scratch space, and shows/hides the controls accordingly.  If supported
 *  the current value displayed, subject to the limits imposed by the queue.
 */
void WxLauncher::refreshScratchSpace(const Queue *slctQueue)
{
    bool supported = p_slctPrefs->isOptionSupported("SS");

    if (supported)
    {
        unsigned long value = p_slctPrefs->getScratchSpace();
        unsigned long vmax = value;
        unsigned long dflt = 0;
        string units = "MBytes";

        if (slctQueue != NULL)
        {
            vmax = slctQueue->scratchLimit();
            dflt = slctQueue->defScratch();
        }

        if (value == 0)
            value = ((dflt > 0) ? dflt : (vmax / 10));

        //  No limit as for the memory limit.
        const bool limited = vmax > 0 && vmax < (unsigned long)INT_MAX;
        if (limited && value > vmax)
            value = vmax / 2;

        p_minScratchParamEdit->setRange(0, limited ? (int)vmax : 100000000);
        p_minScratchParamEdit->setRangeVisible(limited);
        p_minScratchParamEdit->setValue(value);
        p_minScratchParamEdit->setUnitsLabel(units);
        p_minScratchParamEdit->setUnitsVisible(true);
        p_minScratchParamEdit->Layout();
        p_minScratchParamEdit->Fit();
    }

    p_minScratchPanel->Show(supported);
}


/**
 *  Update the controls pertaining to the wall time limit based on the selected
 *  queue.  This method first establishes that the selected machine supports
 *  a time limit, and shows/hides the controls accordingly.  If supported
 *  the current value displayed, subject to the limits imposed by the queue.
 */
void WxLauncher::refreshWallTime(const Queue *slctQueue)
{
    bool supported = (p_slctPrefs->isOptionSupported("TL"));

    if (supported)
    {
        long dfltValue = DFLT_WALL_TIME;
        long maxValue = INT_MAX;
        bool maxExists = false;
        long value = p_slctPrefs->getMaxWallTime();

        if (slctQueue != NULL)
        {
            int queueMax, queueDflt;

            queueMax = slctQueue->runLimit();  // May be MAXINT if not specified
            queueDflt = slctQueue->defRun();

            if (queueMax > 0)
            {
                maxValue = queueMax;
                maxExists = true;
            }

            if (queueDflt > 0)
                dfltValue = queueDflt;
        }

        if (value <= 0)
            value = dfltValue;
        else if (value > maxValue)
            value = maxValue;

        p_maxWallTimePanel->Show(maxExists);

        if (maxExists)
            p_maxWallTimeStaticText->SetLabel(WxTimeSpanEdit::formatTime(maxValue));

        p_wallTimeEdit->setMaxValue(maxValue);
        p_wallTimeEdit->setRangeEnforced(maxExists);
        p_wallTimeEdit->setValue(value);
    }

    p_wallTimePanel->Show(supported);
}


/**
 *  Update the controls pertaining to the number of physical nodes of the
 *  selected machine, based on the number of processors, the maximum
 *  number of nodes, and the maximum processors per node.  The controls
 *  are hidden if there is only one node.
 */
void WxLauncher::refreshNodes(int numProcs, int maxNodes)
{
    int minNodes = numProcs / p_maxProcsPerNode;

    if ((numProcs % p_maxProcsPerNode) > 0)
        minNodes++;

    int numNodes = p_slctPrefs->getNodes();

    numNodes = max(numNodes, minNodes);
    numNodes = min(numNodes, maxNodes);

    p_batchNodesParamEdit->setRange(1, maxNodes);
    p_batchNodesParamEdit->setValue(numNodes);
}


/**
 *  Update the controls pertaining to the number of processors of the
 *  indicated machine.
 */
void WxLauncher::refreshProcessors(RefMachine *machRgstn)
{
    int numProcs = p_slctPrefs->getProcessors();
    int minProcs = 1;
    int maxProcs = machRgstn->proccount();

    // This computer: up to its hardware threads, defaulting to the physical
    // cores, since hyperthreads rarely help quantum chemistry (#436).
    p_localCores = (machRgstn->refname() == "localhost");
    if (p_localCores)
    {
        maxProcs = CpuCores::logical();
        if (numProcs <= 0)
            numProcs = CpuCores::physical();
    }
    else if (numProcs <= 0)
        numProcs = 1;
    // Stored, so a launch with the field untouched uses this default.
    p_slctPrefs->setProcessors(numProcs);

    p_wkstnProcsParamEdit->setRange(minProcs, maxProcs);
    p_wkstnProcsParamEdit->setValue(numProcs);
    p_wkstnProcsParamEdit->Update();

    if (p_localCores)
    {
        string label = "of " + StringConverter::toString((int)CpuCores::physical()) + " cores";
        if (CpuCores::logical() != CpuCores::physical())
            label += " (" + StringConverter::toString((int)CpuCores::logical()) + " threads)";
        p_wkstnProcsParamEdit->setUnitsLabel(label);
    }
    p_wkstnProcsParamEdit->setUnitsVisible(p_localCores);
    this->refreshCoresWarning();
}


/**
 *  A plain warning, never a block, when the local run asks for more
 *  processors than there are physical cores.
 */
void WxLauncher::refreshCoresWarning()
{
    int phys = (int)CpuCores::physical();
    bool warn = p_localCores && p_wkstnProcsParamEdit->getValue() > phys;
    p_coresWarnStaticText->SetLabel(warn
        ? "More than the " + StringConverter::toString(phys) +
          " physical cores: hyperthreads rarely speed up quantum chemistry."
        : "");
    p_coresWarnStaticText->Show(warn);
    p_coresWarnStaticText->GetContainingSizer()->Layout();
}


int WxLauncher::calcProcsPerNode(int maxProcs, int maxNodes)
{
    int result = maxProcs / maxNodes;

    if ((maxProcs % maxNodes) > 0)
        result++;

    return result;
}


int WxLauncher::computeNodesValue(int totalProcs, int totalNodes, int slctProcs)
{
    int slctNodes;
    int procsPerNode = totalProcs / totalNodes;

    if (totalProcs % totalNodes != 0)
        procsPerNode++;

    slctNodes = slctProcs / procsPerNode;

    if (slctProcs % procsPerNode != 0)
        slctNodes++;

    return ((slctNodes > 0) ? slctNodes : 1);
}


void WxLauncher::refreshCalcDirectory()
{
    bool supported = p_slctPrefs->isOptionSupported("RD");

    if (supported)
        p_calcDrctyTextCtrl->SetValue(p_slctPrefs->getProfile(p_profile).rundir);
    // This computer, no run directory saved yet: one in the user's home.
    if (supported && runsHere() &&
        STLUtil::trim((string)p_calcDrctyTextCtrl->GetValue()).empty())
    {
        string dflt = string(Ecce::realUserHome()) + "/ecce-runs";
        p_calcDrctyTextCtrl->SetValue(dflt);
        // Launch reads the preferences, not the box: without this the job
        // got an empty run directory ("Unable to 'cd' to run directory").
        AllocationProfiles::Profile pr = p_slctPrefs->getProfile(p_profile);
        pr.rundir = dflt;
        p_slctPrefs->setProfile(pr);
    }

    p_calcDrctyPanel->Show(supported);
}


void WxLauncher::refreshScratchDirectory()
{
    bool supported = p_slctPrefs->isOptionSupported("SD");

    if (supported)
        p_scratchDrctyTextCtrl->SetValue(p_slctPrefs->getProfile(p_profile).scratchdir);

    p_scratchDrctyPanel->Show(supported);
}


/**
 *  Fill the RemoteShells drop-down list, in accordance with the indicated machine.
 *  Try to select the shell in p_slctPrefs, return name of selected shell
 *  (can be empty)
 */
string WxLauncher::refreshShells(RefMachine *machRgstn)
{
    int slctIdx = 0;
    string slctShell = "";

    p_remShellChoice->Clear();
    vector<string>* machShells = machRgstn->remshells();

    if ((machShells != NULL) && (machShells->size() > 0))
    {
        for (int i = 0; i < machShells->size(); i++)
            p_remShellChoice->Append((*machShells)[i]);

        slctShell = p_slctPrefs->getRemoteShell();

        if (slctShell.size() > 0)
            slctIdx = p_remShellChoice->FindString(slctShell);

        p_remShellChoice->SetSelection(slctIdx);
    }

    return slctShell;
}


/**
 *  Show, beside the machine's full name, whether a machine that names
 *  THIS host (localhost, 127.0.0.1, ::1, or its own hostname) will run the
 *  job locally or go over the remote shell -- which depends on the login
 *  name (#144). Same shell and username the launch will use, and asked of
 *  RCommand::isRemote() itself, so it cannot disagree with the launch.
 *  Nothing is added for an ordinary remote machine.
 */
void WxLauncher::refreshLocality()
{
    if (p_slctPrefs == NULL)
        return;
    RefMachine *machRgstn = p_slctPrefs->getRegisteredMachine();
    if (machRgstn == NULL)
        return;

    string user = "";
    if (p_slctPrefs->isOptionSupported("UN"))
        user = (string)p_usernameTextCtrl->GetValue();
    string shell = (string)p_remShellChoice->GetStringSelection();

#ifdef _WIN32
    // On Windows a job for this computer runs here directly: no login, no
    // remote shell, and a run folder in the user's home unless one is set.
    bool local = !RCommand::isRemote(machRgstn->fullname(), shell, user);
    if (wxWindow *shellPanel = FindWindow(ID_PANEL_WXLAUNCHER_REMSHELL))
        shellPanel->Show(!local);
    p_shellOpenButton->Show(!local);
    if (local)
        p_password1Panel->Show(false);
#endif

    string note = RCommand::localityNote(machRgstn->fullname(), shell, user);
    string tip = machRgstn->fullname();
    if (!user.empty() && note.empty())
        tip += ", as " + user;
    if (!note.empty())
        tip += " (" + note + ")\n\nThis machine names the computer ECCE is "
               "running on. Jobs for it run locally when the Username is "
               "empty or your own, and over the remote shell to it as that "
               "user otherwise -- e.g. through a port forward to a cluster.";
    p_machinesChoice->SetToolTip(tip);
}


void WxLauncher::refreshAuthentication(RefMachine *machRgstn, string shellName)
{
    string uname = "";
    bool supported = p_slctPrefs->isOptionSupported("UN");

    if (supported)
    {
        uname = p_slctPrefs->getUsername();
    }

    p_usernameTextCtrl->SetValue(uname);
    p_usernameLabel->Show(supported);
    p_usernameTextCtrl->Show(supported);

    this->refreshPasswords(shellName, machRgstn->fullname(), uname);
}


void WxLauncher::refreshPasswords(const string& shellName,
                                  const string& machName, const string& user)
{
    bool supported = p_slctPrefs->isOptionSupported("PW");

    if (supported)
    {
        string pswd1 = "";

        string url = shellName + "://" + machName;
        BasicAuth *ba = AuthCache::getCache().getAuthentication(url, user,"",1);
        if (ba != NULL) {
          pswd1 = ba->m_pass;
          delete ba;
        }

        p_password1TextCtrl->SetValue(pswd1);

        p_password1Label->SetLabel("Password:");
    }

    p_password1Panel->Show(supported);
}


/**
 *  Returns true or false depending on whether this app has context.
 */
bool WxLauncher::hasContext()
{
    return (p_taskJob != NULL);
}


/**
 *  Returns the context, in the form of an EcceURL, of the launcher frame.
 *  You must be sure that the frame has a context when this is called.
 */
EcceURL WxLauncher::getContext()
{
    return p_taskJob->getURL();
}


/**
 *  Returns the run state of the launcher frame's current context.  You
 *  must be sure that the frame has a context when this is called.
 */
ResourceDescriptor::RUNSTATE WxLauncher::getContextState()
{
    return p_taskJob->getState();
}


void WxLauncher::setContext(string cntxt)
{
    // Bail if it is the same context as the current one and just
    // raise the window in the stacking order (done by caller)
    if (hasContext() && getContext().toString()==cntxt)
    {
        return;
    }

    EcceURL url;
    string slctnKey = "";

    if (p_slctPrefs != NULL)
    {
        if (p_prefsEdited)
            this->updatePreferences();

        slctnKey = p_slctPrefs->getItemKey();
        p_slctPrefs = NULL;
    }

    //  Initialize to trivial values
    p_taskJob = NULL;

    Launchdata ldat;
    p_ctxLaunched = false;
    p_profileMachine = "";

    if (cntxt.size() > 0)
    {
        url = EcceURL(cntxt);

        // First lets make sure is all is ok with the object we were handed.
        Resource *rsc = EDSIFactory::getResource(url);

        if (rsc != 0)
        {
            p_taskJob = dynamic_cast<TaskJob *>(rsc);

            if (p_taskJob != NULL)
            {
                ldat = p_taskJob->launchdata();
                slctnKey = ldat.machine;
                p_ctxLaunched = !ldat.rundir.empty();   // launched before
            }
        }
    }


    p_ctxLaunch = ldat;
    p_inCtrlUpdate = true;
    p_machinesChoice->Clear();
    this->populateMachinesList();       //  Loads all machines if p_taskJob is NULL

    //  At this point, if p_taskJob is NULL, then slctnKey is empty.
    //  If p_taskJob is not NULL, then slctnKey probably not empty, but may contain the
    //      name of a machine that is not in the list.

    if (p_numIncl > 0)      //  (i.e., if there are machines in the drop-down list)
    {
        //
        int slctnIdx = 0;
        bool found = false;

        if (slctnKey.size() > 0)
        {
            //  Will reach this point only if p_taskJob is not NULL, and ldat has a machine name in it.
            slctnIdx = p_machinesChoice->FindString(slctnKey);
            found = (slctnIdx >= 0);
        }

        if (found)
        {
            p_machinesChoice->SetSelection(slctnIdx);
            p_slctPrefs = (MachinePreferences *)(p_machinesChoice->GetClientData(slctnIdx));
            this->refreshControls();
            this->updateControls(ldat);
        }
        else
        {
            p_machinesChoice->SetSelection(0);
            p_slctPrefs = (MachinePreferences *)(p_machinesChoice->GetClientData(0));
            this->refreshControls();
        }

        p_machinesChoice->Show(true);
    }
    else
    {
        this->clearControls();
        string mesg = "None registered";

        if (p_taskJob != NULL)
            mesg += " for " + p_taskJob->application()->getCodeName() + " jobs";

        p_machinesChoice->Show(false);
        p_machineNameStaticText->SetLabel("(" + mesg + ".)");
        p_machineNameStaticText->Show();

        //  A warning only: the user may register a machine and reopen.
        if (p_taskJob != NULL)
            p_messagesFeedback->setMessage(
                "No machine is registered for " +
                p_taskJob->application()->getCodeName() +
                ". Use Tools > Register Machines in the Organizer, or see "
                "Help > Running Codes on This Computer.", WxFeedback::WARNING);
    }

    this->Layout();
    this->refit();

    ResourceDescriptor::RUNSTATE rs = ResourceDescriptor::STATE_CREATED;

    if (p_taskJob != NULL)
    {
        rs = p_taskJob->getState();

        p_messagesFeedback->setContextLabel(
                            EDSIServerCentral().mapURLtoName(url));
        p_messagesFeedback->setContextURL(url);
        p_messagesFeedback->setRunState(rs);

        if (rs < ResourceDescriptor::STATE_READY)
        {
            string mesg = "Calculation must be in the Ready state "
                          "in order to launch job.";
            p_messagesFeedback->setMessage(mesg, WxFeedback::INFO);
        }
    }
    else
    {
        p_messagesFeedback->setContextLabel("");
        p_messagesFeedback->setRunState(rs);
    }

//    this->setEditAllowed((p_slctPrefs != NULL) && (rs < ResourceDescriptor::STATE_SUBMITTED));
    this->setEditAllowed((p_slctPrefs != NULL) && (rs == ResourceDescriptor::STATE_READY));
    this->refreshLaunchControls();



    p_inCtrlUpdate = false;
}


/**
 *  Verifies that the machine specified in the LaunchData structure of the TaskJob has
 *  the selected code installed in it.  Updates the graphical interface accordingly.
 */
void WxLauncher::validateCode()
{
    Launchdata ldat = p_taskJob->launchdata();
    string refname = ldat.machine;

    bool machineHasCode = validateMachineWithCode(refname, p_taskJob->application()->getCodeName());

    this->refreshLaunchControls();

    if (!machineHasCode)
        p_messagesFeedback->setMessage(
            "The code required to run this calculation is "
            "not available on " + refname, WxFeedback::ERROR);
}


/**
 *  Graphical controls are updated to reflect the values in the specified LaunchData
 *  structure.
 *
 *  This method assumes that the value of p_slctPrefs is non-NULL.
 */
void WxLauncher::updateControls(Launchdata ldat)
{
    bool prevState = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    if (p_slctPrefs->isOptionSupported("Q"))
    {
        if (!ldat.queue.empty())
        {
            p_queueChoice->SetStringSelection(ldat.queue);
            p_prefsEdited = true;
        }

        if (ldat.totalprocs  > 0)
        {
            p_batchProcsParamEdit->setValue(ldat.totalprocs);
            p_prefsEdited = true;
        }

        if (ldat.nodes > 0)
        {
            p_batchNodesParamEdit->setValue(ldat.nodes);
            p_prefsEdited = true;
        }

        if (p_slctPrefs->isOptionSupported("MM") && (ldat.maxmemory > 0))
        {
            //  ldat.maxmemory is MB (TaskJob.H); field is GB.
            p_maxMemoryParamEdit->setValue(MemoryUnits::mbToGB(ldat.maxmemory));
            p_prefsEdited = true;
        }

        if (p_slctPrefs->isOptionSupported("SS") && (ldat.minscratch > 0))
        {
            p_minScratchParamEdit->setValue(ldat.minscratch);
            p_prefsEdited = true;
        }

        if (p_slctPrefs->isOptionSupported("TL") && !ldat.maxwall.empty())
        {
            int days, hrs, mins;
            sscanf(ldat.maxwall.c_str(), "%d %d:%d:00", &days, &hrs, &mins);
            long value = WxTimeSpanEdit::calcTimeMinutes(days, hrs, mins);
            p_wallTimeEdit->setValue(value);
            p_prefsEdited = true;
        }
    }
    else
    {
        if (ldat.totalprocs > 0)
        {
            p_wkstnProcsParamEdit->setValue(ldat.totalprocs);
            p_prefsEdited = true;
        }

        if (!ldat.priority.empty())
        {
            this->updatePriority(atoi(ldat.priority.c_str()));
            p_prefsEdited = true;
        }
    }

    if (!ldat.remoteShell.empty())
    {
        p_remShellChoice->SetStringSelection(ldat.remoteShell);
        p_prefsEdited = true;
    }

    if (p_slctPrefs->isOptionSupported("UN") && !ldat.user.empty())
    {
        if (ldat.user != p_slctPrefs->getUsername())
        {
            p_usernameTextCtrl->SetValue(ldat.user);
            p_password1TextCtrl->SetValue("");
        }

        p_prefsEdited = true;
    }

    if (p_slctPrefs->isOptionSupported("AA") && !ldat.account.empty())
    {
        p_allocAcctTextCtrl->SetValue(ldat.account);
        p_prefsEdited = true;
    }

    if (p_slctPrefs->isOptionSupported("RD") && !ldat.rundir.empty())
    {
        p_calcDrctyTextCtrl->SetValue(ldat.rundir);
        p_prefsEdited = true;
    }

    if (p_slctPrefs->isOptionSupported("SD") && !ldat.scratchdir.empty())
    {
        p_scratchDrctyTextCtrl->SetValue(ldat.scratchdir);
        p_prefsEdited = true;
    }

    this->Layout();
    this->refit();

    p_inCtrlUpdate = prevState;
}


/**
 *  Returns the currently selected Queue object.
 */
const Queue * WxLauncher::getQueueSelection()
{
    const Queue * slctQueue = NULL;

    int slctnIdx = p_queueChoice->GetSelection();

    if (slctnIdx >= 0)
        slctQueue = (const Queue *)(p_queueChoice->GetClientData(slctnIdx));

    return slctQueue;
}


/**
 *  Enables/disables UI controls, typically as a result to a change in the
 *  run state of the current TaskJob.
 */
void WxLauncher::updateContext(const ResourceDescriptor::RUNSTATE& state)
{
//    this->setEditAllowed(state < ResourceDescriptor::STATE_SUBMITTED);
    this->setEditAllowed(state == ResourceDescriptor::STATE_READY);
    p_messagesFeedback->setRunState(state);

    // Andy (2026-09-27): "the reason should be discoverable. The
    // Launcher should tell me" -- his case was a job that failed within
    // seconds, with the Launcher window for it still open. eccejobstore
    // (JobStore.C's recordStatusReason()) writes the most specific
    // reason it has into this same namespaced metadata key whenever a
    // calc goes unsuccessful/failed, so show it here the same way any
    // other launch feedback is shown, rather than making the user go
    // find Organizer's View Run Log.
    //  updateContext() runs on every state notification, and a job that
    //  ends badly sends more than one -- with, after a job-store restart,
    //  more than one wording -- so the reason was printed twice.  Show one
    //  reason per failure: remembered by calc, and forgotten as soon as
    //  the calc is anything but failed (e.g. relaunched and running).
    static string lastShown;
    if (state == ResourceDescriptor::STATE_UNSUCCESSFUL ||
        state == ResourceDescriptor::STATE_FAILED ||
        state == ResourceDescriptor::STATE_SYSTEM_FAILURE ||
        state == ResourceDescriptor::STATE_WAITING)
    {
        string reason = p_taskJob->getProp(VDoc::getEcceNamespace() +
                                            ":runStatusReason");
        const string key = p_taskJob->getURL().toString();
        if (!reason.empty() && key != lastShown) {
            lastShown = key;
            p_messagesFeedback->setMessage(reason, WxFeedback::ERROR, false, true);
        }
    } else {
        lastShown.clear();
    }

    this->refreshLaunchControls();
}


void WxLauncher::updateStatePreferences()
{
    p_messagesFeedback->readPreferences();
}


void WxLauncher::machRegChanged()
{
    RefMachine::markUpdateUserPrefs();
    this->reloadMachinePreferences();
}


void WxLauncher::reloadMachinePreferences()
{
    if (p_prefsEdited)
    {
        //  Save the changes to the current configuration; don't validate at
        //  this time.  Validate only when launching or when opening remote
        //  shell.
        this->updatePreferences();
    }

    //  p_queueChoice's client data are raw Queue* owned by QueueManager's
    //  cache, keyed by machine refname and loaded once from the Queues
    //  file.  Machine Registration writes to that file directly, so a
    //  running launcher never saw a newly added/changed queue and could
    //  hold dangling client data once the cache below is torn down (#131).
    //  Clear the choice first, then drop the cache so the next
    //  refreshQueues() call re-reads it from disk.
    p_queueChoice->Clear();
    QueueManager::finalize();

    string slctnKey = p_slctPrefs->getItemKey();
    p_slctPrefs = NULL;

    //  Must clear out the machines list
    p_machinesChoice->Clear();

    MachinePreferences::refresh(true);
    this->populateMachinesList();

    int slctnIdx = p_machinesChoice->FindString(slctnKey);

    if (p_numIncl > 0)
    {
        if (slctnIdx < 0)
            slctnIdx = 0;

        p_slctPrefs = (MachinePreferences *)(p_machinesChoice->GetClientData(slctnIdx));
    }

    p_machinesChoice->SetSelection(slctnIdx);

    //  SetSelection() above does not fire wxEVT_CHOICE, so nothing would
    //  otherwise repopulate p_queueChoice (cleared above) -- refreshControls()
    //  is what machinesChoiceSelectedCB() itself calls, and it re-reads the
    //  queue list from the now-reinitialized QueueManager cache.
    if (p_slctPrefs != NULL)
        this->refreshControls();
}



/**
 *
 *  Set the calculation reference for which machine to launch the
 *  calculation on, validate launch parameters on the compute machine
 *  and launch the job.
 *
 *  Rather than have RefQueueMgr rely on Calculations via the Launch
 *  class, parameters for generating the submit script are created here
 *  and passed into Launch.
 */
void WxLauncher::launchCalc(const bool& fullLaunchFlag)
{
    string mesg;

    wxBusyCursor cursor;
    
    this->saveJob();

    EcceMap kvargs;
    buildArgs(kvargs);

    // userSubmit configuration setting forces a staged launch
    RefMachine *rgstn = p_slctPrefs->getRegisteredMachine();
    // noRemoteAccess (issue #94) goes further: ECCE must not reach the
    // machine at all, so there is nothing to submit and nothing to stage
    // remotely. Launch stops after generating the deck locally.
    bool localOnly = (rgstn != (RefMachine*)0 && rgstn->noRemoteAccess());
    bool doLaunch =  fullLaunchFlag && !rgstn->userSubmit() && !localOnly;

#ifdef _WIN32
    // Keep the Windows GUI out of the long-lived SSH/monitor process. A
    // saved record selects RESUME so reopening a calculation never submits
    // it a second time.
    if (doLaunch) {
        JobWorker::Record record;
        bool resume = false;
        if (JobWorker::loadRecord(p_taskJob->getURL().toString(), record)) {
            if ((record.phase == JobWorker::SUBMITTING ||
                 record.phase == JobWorker::NEEDS_RECONCILIATION) &&
                record.schedulerJobId.empty()) {
                p_messagesFeedback->setMessage(
                    "The prior submission may have reached the scheduler. "
                    "Reconcile the scheduler job before retrying.",
                    WxFeedback::ERROR, false, true);
                return;
            }
            resume = !record.schedulerJobId.empty() &&
                record.phase != JobWorker::COMPLETE &&
                record.phase != JobWorker::FAILED;
        }
        string workerError;
        if (runJobWorker(resume, workerError)) {
            calcStateNotify(p_taskJob->getState());
            p_messagesFeedback->setMessage(
                resume ? "Monitoring reconnected to the saved job."
                       : "Successfully submitted job.",
                WxFeedback::INFO, false, true);
        } else if (!workerError.empty()) {
            p_messagesFeedback->setMessage(workerError, WxFeedback::ERROR,
                                            false, true);
        }
        return;
    }
#endif

    p_launchData = new Launch(p_taskJob, kvargs, doLaunch);
    bool valid = true;
    p_launchError = "";

    //  Broker messages (state, invoke, quit, machine changes) touch the
    //  calculation the steps are using: they wait until the launch ends.
    struct DeferMessages {
        WxJMSSubscriber *s;
        DeferMessages() : s(dynamic_cast<WxJMSSubscriber*>(wxTheApp))
            { if (s) s->deferMessages(true); }
        ~DeferMessages() { if (s) s->deferMessages(false); }
    } defer;
    //  No input while the steps run; the window still repaints.
    wxWindowDisabler noInput;

    while (valid && !p_launchData->done())
    {
        string dscpn = p_launchData->description();
        p_messagesFeedback->setMessage(dscpn, WxFeedback::INFO, false, true);

        if (!(valid = runLaunchStep()))
        {
            p_launchError = p_launchData->message();
            LaunchBatch::recordFailure(p_taskJob, p_launchError);
            p_messagesFeedback->setMessage(p_launchData->message(), WxFeedback::ERROR, false, true);
        }

        else if (p_launchData->message() != "")
            p_messagesFeedback->setMessage(p_launchData->message(), WxFeedback::WARNING, false, true);

        else if (p_launchData->info() != "")
            p_messagesFeedback->setMessage(p_launchData->info(), WxFeedback::INFO, false, true);

        if (valid && p_launchData->launchWarning() != "")
        {
            string warning = p_launchData->launchWarning();
            p_launchData->clearLaunchWarning();
            if (!confirmLaunchWarning(warning))
            {
                p_messagesFeedback->setMessage("Launch cancelled.", WxFeedback::INFO);
                p_launchData->removeStaging();
                delete p_launchData;
                p_launchData = NULL;
                return;
            }
        }

        this->UpdateWindowUI();
    }

    if (!p_launchData->done())
    {
        p_messagesFeedback->setMessage("Launch aborted...", WxFeedback::WARNING);
        p_launchData->removeStaging();
    }
    else
    {
        string mesg = "";

        if (doLaunch)
        {
            ResourceDescriptor::RUNSTATE currentState = p_taskJob->getState();
            calcStateNotify(currentState);

            if (currentState <= ResourceDescriptor::STATE_COMPLETED)
            {
                mesg = "Successfully submitted job.";
                LaunchBatch::clearFailure(p_taskJob);
                p_messagesFeedback->setMessage(mesg, WxFeedback::INFO);
            }
            else
            {
                mesg = "Job submission failed.";
                LaunchBatch::recordFailure(p_taskJob, mesg);
                p_messagesFeedback->setMessage(mesg, WxFeedback::ERROR);
            }

            delete p_launchData;
            p_launchData = NULL;
        }
        else
        {
            p_jobStaged = p_launchData->done();

            if (p_jobStaged && localOnly)
            {
                // Deliberately no startShell(): there is no shell to open,
                // which is the whole reason this machine is flagged. The
                // directory IS the deliverable, so name it rather than
                // reporting a vague success.
                mesg = "Input files generated in " +
                       p_launchData->stagingDirectory() +
                       "\nECCE will not submit this job. Copy that directory "
                       "to the machine, submit it yourself, then import the "
                       "output file when it finishes.";
                p_messagesFeedback->setMessage(mesg, WxFeedback::INFO);
            }
            else if (p_jobStaged)
            {
                mesg = "Successfully staged calculation for launch.";
                p_messagesFeedback->setMessage(mesg, WxFeedback::INFO);

                this->startShell();
            }
            else
            {
                mesg = "Staging calculation failed.";
                p_messagesFeedback->setMessage(mesg, WxFeedback::ERROR);
            }

            this->refreshLaunchControls();
        }
    }
}

#ifdef _WIN32
namespace {
static bool writeNamedPipe(HANDLE pipe, const string& text)
{
    size_t sent = 0;
    while (sent < text.size()) {
        DWORD n = 0;
        if (!WriteFile(pipe, text.data() + sent,
                       (DWORD)(text.size() - sent), &n, NULL) || n == 0)
            return false;
        sent += n;
    }
    return true;
}

static bool readNamedPipeLine(HANDLE pipe, string& line)
{
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(20);
    while (std::chrono::steady_clock::now() < deadline) {
        DWORD available = 0;
        if (!PeekNamedPipe(pipe, NULL, 0, NULL, &available, NULL)) return false;
        if (available) {
            char c;
            DWORD n = 0;
            if (!ReadFile(pipe, &c, 1, &n, NULL) || n != 1) return false;
            if (c == '\n') {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                return true;
            }
            line += c;
        } else {
            wxTheApp->Yield(true);
            wxMilliSleep(20);
        }
    }
    return false;
}

static string decodeHex(const string& encoded)
{
    string value;
    if (encoded.size() % 2) return value;
    for (size_t i = 0; i < encoded.size(); i += 2) {
        char byte[3] = {encoded[i], encoded[i + 1], '\0'};
        value += (char)strtoul(byte, NULL, 16);
    }
    return value;
}

static bool readAvailable(HANDLE pipe, string& pending, string& line,
                          bool& disconnected)
{
    DWORD available = 0;
    if (!PeekNamedPipe(pipe, NULL, 0, NULL, &available, NULL)) {
        disconnected = true;
        return false;
    }
    if (!available) return false;
    char buf[4096];
    DWORD n = 0;
    if (!ReadFile(pipe, buf, (DWORD)std::min<size_t>(sizeof(buf), available), &n, NULL)) {
        disconnected = true;
        return false;
    }
    pending.append(buf, n);
    size_t end = pending.find('\n');
    if (end == string::npos) return false;
    line = pending.substr(0, end);
    pending.erase(0, end + 1);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return true;
}
}

bool WxLauncher::runJobWorker(bool resume, string& error)
{
    struct MessageDeferral {
        WxJMSSubscriber *subscriber;
        MessageDeferral() : subscriber(dynamic_cast<WxJMSSubscriber*>(wxTheApp))
            { if (subscriber) subscriber->deferMessages(true); }
        ~MessageDeferral() { if (subscriber) subscriber->deferMessages(false); }
    } defer;
    wxWindowDisabler noInput;

    const string pipeName = "\\\\.\\pipe\\ECCE.JobWorker." +
        std::to_string((unsigned long)GetCurrentProcessId()) + "." +
        std::to_string((unsigned long long)GetTickCount64());
    HANDLE pipe = CreateNamedPipeA(pipeName.c_str(), PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT | PIPE_REJECT_REMOTE_CLIENTS,
        1, 8192, 8192, 0, NULL);
    if (pipe == INVALID_HANDLE_VALUE) {
        error = "Cannot create the local job worker pipe";
        return false;
    }

    string helper = Ecce::ecceBinCommand("eccejobworker");
    if (helper.size() >= 2 && helper[0] == '"' && helper[helper.size()-1] == '"')
        helper = helper.substr(1, helper.size() - 2);
    wxArrayString args;
    args.Add(wxString::FromUTF8(helper.c_str()));
    args.Add("-ipc"); args.Add(wxString::FromUTF8(pipeName.c_str()));

    const string authPipe = AuthCache::pipeName();
    args.Add("-authPipe"); args.Add(wxString::FromUTF8(authPipe.c_str()));
    long pid = wxExecute(args, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE);
    if (pid <= 0) {
        CloseHandle(pipe);
        error = "Unable to start eccejobworker";
        return false;
    }
    AuthCache::getCache().pipeOut(authPipe);

    bool connected = false;
    const auto connectDeadline = std::chrono::steady_clock::now() +
                                 std::chrono::seconds(20);
    while (!connected && std::chrono::steady_clock::now() < connectDeadline) {
        if (ConnectNamedPipe(pipe, NULL)) connected = true;
        else if (GetLastError() == ERROR_PIPE_CONNECTED) connected = true;
        else if (GetLastError() != ERROR_PIPE_LISTENING) break;
        if (!connected) { wxTheApp->Yield(true); wxMilliSleep(20); }
    }
    if (!connected) {
        CloseHandle(pipe);
        error = "eccejobworker did not connect to the Launcher pipe";
        return false;
    }
    DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
    if (!SetNamedPipeHandleState(pipe, &mode, NULL, NULL)) {
        DisconnectNamedPipe(pipe); CloseHandle(pipe);
        error = "Cannot set the Launcher pipe to blocking mode";
        return false;
    }

    string ready;
    if (!readNamedPipeLine(pipe, ready) || ready != "READY") {
        DisconnectNamedPipe(pipe); CloseHandle(pipe);
        error = "eccejobworker did not complete the pipe handshake";
        return false;
    }
    if (!writeNamedPipe(pipe, string(resume ? "RESUME\n" : "START\n") +
                              p_taskJob->getURL().toString() + "\n")) {
        DisconnectNamedPipe(pipe); CloseHandle(pipe);
        error = "Cannot send the calculation request to eccejobworker";
        return false;
    }

    bool monitoring = false, failed = false, disconnected = false;
    string pending;
    const auto eventDeadline = std::chrono::steady_clock::now() +
                               std::chrono::minutes(10);
    while (!monitoring && !failed && !disconnected &&
           std::chrono::steady_clock::now() < eventDeadline) {
        string frame;
        if (readAvailable(pipe, pending, frame, disconnected)) {
            vector<string> fields;
            size_t start = 0;
            for (size_t i = 0; i <= frame.size(); ++i) {
                if (i == frame.size() || frame[i] == '|') {
                    fields.push_back(frame.substr(start, i - start));
                    start = i + 1;
                }
            }
            if (fields.size() >= 5 && fields[0] == "EVT") {
                const string type = fields[1], phase = fields[2];
                const string message = decodeHex(fields[4]);
                WxFeedback::Severity severity = WxFeedback::INFO;
                if (type == "failure") severity = WxFeedback::ERROR;
                else if (type == "warning") severity = WxFeedback::WARNING;
                if (!message.empty())
                    p_messagesFeedback->setMessage(message, severity, false, true);
                if (type == "warning") {
                    const bool proceed = confirmLaunchWarning(message);
                    if (!writeNamedPipe(pipe, proceed ? "CONTINUE\n" : "CANCEL\n"))
                        disconnected = true;
                    if (!proceed) { failed = true; error = "Launch cancelled."; }
                } else if (type == "failure") {
                    failed = true;
                    error = message;
                } else if (phase == "monitoring") {
                    monitoring = true;
                }
            }
        }
        if (!monitoring && !failed && !disconnected) {
            wxTheApp->Yield(true);
            wxMilliSleep(20);
        }
    }
    if (!monitoring && !failed && !disconnected)
        error = "Timed out waiting for the job worker to connect its monitor. "
                "The saved worker record can be resumed from Run Management.";
    DisconnectNamedPipe(pipe);
    CloseHandle(pipe); // the detached worker continues with durable state
    return monitoring;
}
#endif


/**
 * One launch step on a worker thread while this thread keeps handling
 * events: steps wait on ssh, the data server and the queue's submit command,
 * and a window that stops answering is offered for killing by the desktop.
 * The caller has disabled input and deferred broker messages, so nothing
 * else touches the calculation meanwhile.
 */
bool WxLauncher::runLaunchStep()
{
    std::atomic<bool> finished(false);
    bool ok = false;
    Launch *launch = p_launchData;
    std::thread worker([launch, &ok, &finished]() {
        try { ok = launch->nextOperation(); }
        catch (...) { ok = false; }
        finished = true;
    });
    p_stepPumps = 0;
    while (!finished)
    {
        wxTheApp->Yield(true);
        p_stepPumps++;
        wxMilliSleep(20);
    }
    worker.join();
    return ok;
}


bool WxLauncher::getAuthorization(AuthEvent& event)
{
    if (wxThread::IsMain())
        return WxDavAuth::getAuthorization(event);
    //  Asked from runLaunchStep's worker: runLaunchStep's loop runs this.
    std::promise<bool> answer;
    wxTheApp->CallAfter([this, &event, &answer]() {
        answer.set_value(WxDavAuth::getAuthorization(event));
    });
    return answer.get_future().get();
}


void WxLauncher::buildArgs(EcceMap& kvargs)
{
    //  At this point, the launchdata structure is (or should be) synchronized with whatever appears
    //  in the UI.  However, it may not necessarily be true that the parameters have been saved to the
    //  corresponding MachinePreferences object.
    Launchdata ldat = p_taskJob->launchdata();

    this->showLaunchData(ldat);

    TypedFile infile;
    p_taskJob->getDataFile(JCode::PRIMARY_INPUT, infile);
    kvargs["##input##"] = infile.name();

    TypedFile parsefile;
    p_taskJob->getDataFile(JCode::PARSE_OUTPUT, parsefile);
    kvargs["##parse##"] = parsefile.name();

    TypedFile outfile;
    p_taskJob->getDataFile(JCode::PRIMARY_OUTPUT, outfile);
    kvargs["##output##"] = outfile.name();

    TypedFile propfile;
    p_taskJob->getDataFile(JCode::PROPERTY_OUTPUT, propfile);
    kvargs["##property##"] = propfile.name();

    TypedFile auxfile;
    p_taskJob->getDataFile(JCode::AUXILIARY_OUTPUT, auxfile);
    kvargs["##auxiliary##"] = auxfile.name();

    // Add MD args, if necessary
    MdTask *mdTask = dynamic_cast<MdTask*>(p_taskJob);

    if (mdTask != 0)
    {
        try
        {
            // Fragment
            kvargs["##output_frag##"] = mdTask->getOutputFragmentName();

            // Restart
            kvargs["##restart##"] = mdTask->getRestartName();

            // MD Output
            kvargs["##md_output##"] = mdTask->getMdOutputName();

            // Topology
            kvargs["##topology##"] = mdTask->getTopologyName();
        }
        catch (...)
        {
            string mesg = "Failed to set MD job output file parameters";
            p_messagesFeedback->setMessage(mesg, WxFeedback::ERROR);
        }
    }

    // The title of the calc can affect the files so we need to make sure its
    // accurate.  This is a bit of a hack but it does avoid actual problems
    // while leaving potential inconsistencies in place.
    string tmp = p_taskJob->getName();
    char buf[32];

    kvargs["##title##"] = tmp;

    if (p_slctPrefs->isOptionSupported("Q"))
    {
        kvargs["##queue##"] = ldat.queue;

        if (p_slctPrefs->isOptionSupported("AA"))
            kvargs["##account_no##"] = p_slctPrefs->getProfile(p_profile).account;

        if (p_slctPrefs->isOptionSupported("MM"))
            kvargs["##maxmemory##"] =
                    StringConverter::toString((size_t)ldat.maxmemory);

        if (p_slctPrefs->isOptionSupported("SS"))
            kvargs["##minscratch##"] =
                    StringConverter::toString((size_t)ldat.minscratch);

        if (p_slctPrefs->isOptionSupported("TL"))
        {
            int days, hours, mins;
            sscanf(ldat.maxwall.c_str(), "%d %d:%d:00", &days, &hours, &mins);
            hours += days*24;
            sprintf(buf, "%d:%d:00", hours, mins);
            kvargs["##wall_clock_time##"] = buf;
            int seconds = days*86400 + hours*3600 + mins*60;
            sprintf(buf, "%d", seconds);
            kvargs["##wall_clock_seconds##"] = buf;
            sprintf(buf, "%d:%d", hours, mins);
            kvargs["##wall_clock_hrmin##"] = buf;
        }
    }

    if (p_slctPrefs->isOptionSupported("PW"))
    {
        string ptext1
            = STLUtil::trim((string)(p_password1TextCtrl->GetValue()));

        kvargs["##password1##"] = ptext1;
    }

    sprintf(buf, "%lu", ldat.totalprocs);
    kvargs["##numProcs##"] = buf;

    sprintf(buf, "%lu", ldat.nodes);
    kvargs["##numNodes##"] = buf;

    if (p_slctPrefs->isOptionSupported("SD"))
        kvargs["##scratch##"] = ldat.scratchdir;

    Jobdata jobdata = p_taskJob->jobdata();
    kvargs["##runDir##"] = jobdata.jobpath;

    string nicestr = "";
    if (ldat.priority!="" && ldat.priority!="0")
    {
        nicestr = "nice";
        if (ldat.priority != "10")
            nicestr += " -" + ldat.priority;
    }
    kvargs["##priority##"] = nicestr;

    //  DebugLeaveMethod();
}


void WxLauncher::startShell()
{
    RefMachine *rgstn = p_slctPrefs->getRegisteredMachine();

    string fullName = rgstn->fullname();
    string shell = p_slctPrefs->getRemoteShell();
    string user = p_slctPrefs->getUsername();
    string password = STLUtil::trim((string)(p_password1TextCtrl->GetValue()));

    //  The calculation's own run directory, not the machine's current one: a
    //  later profile or edit must not point the shell elsewhere.
    string pathBase = p_taskJob->launchdata().rundir;
    if (pathBase.empty())
        pathBase = p_slctPrefs->getProfile(p_profile).rundir;
    Jobdata job = p_taskJob->jobdata();
    string pathTail = job.jobpath;

    if (!pathTail.empty())
    {
        EcceShell eshell;
        string mesg = eshell.dirshell(p_taskJob->getName(), fullName, shell,
                                user, password.c_str(), pathBase, pathTail);

        if (mesg != "")
            p_messagesFeedback->setMessage(mesg, WxFeedback::WARNING);
    }
    else
        p_messagesFeedback->setMessage(
                            "No launch data for calculation available.");
}


void WxLauncher::machRegisterMenuitemClickCB(wxCommandEvent& event)
{
    //  Under the test hook there is no gateway: record what would be asked.
    if (getenv("ECCE_LAUNCHER_SCRIPT") != NULL)
    {
        p_lastRegisterRequest = "appname=" + string(MACHREGISTER) +
            " initmachine=" +
            ((p_slctPrefs != NULL) ? p_slctPrefs->getItemKey() : string());
        fprintf(stderr, "[LAUNCHER] request: %s\n", p_lastRegisterRequest.c_str());
        return;
    }

    Target trgt(GATEWAY, "");

    JMSMessage* mesg = newMessage();
    mesg->setTarget(trgt);
    mesg->addProperty("action", "start");
    publish("ecce_activity", *mesg);
    delete mesg;

    mesg = newMessage();
    mesg->setTarget(trgt);
    mesg->addProperty("appname", MACHREGISTER);

    if (p_slctPrefs != NULL)
        mesg->addProperty("initmachine", p_slctPrefs->getItemKey());

    mesg->addIntProperty("forcenew", 0);
    publish("ecce_get_app", *mesg);

    delete mesg;
}


void WxLauncher::stageLaunchMenuItemClickCB(wxCommandEvent& event)
{
    this->stageLaunch();
}


void WxLauncher::finishLaunchMenuItemClickCB(wxCommandEvent& event)
{
    //  DebugEnterMethod("finishLaunchMenuItemClickCB(wxCommandEvent&)");
    //  Really just adapted from the tu-version.  Not much more.
    this->finishStagedLaunch();
    this->refreshLaunchControls();

    //  DebugLeaveMethod();
}


void WxLauncher::stageLaunch()
{
    if (p_jobStaged)
        this->cancelStagedLaunch();

    this->refreshLaunchControls();

    p_messagesFeedback->clearMessage();

    this->saveJob();
    this->ensureEditsApplied(true);

    // false parameter to launchCalc causes the job not to be submitted and
    // a shell in the run directory to be opened
    this->launchCalc(false);
}


void WxLauncher::cancelStagedLaunch()
{
    if (p_launchData != NULL)
    {
        delete p_launchData;
        p_launchData = NULL;
    }

    p_jobStaged = false;
}



void WxLauncher::finishStagedLaunch()
{
    if (p_launchData->validateLocalDir() &&
        p_launchData->validateRemoteLogin() &&
        p_launchData->generateJobMonitoringFiles() &&
        p_launchData->doLaunch())
    {
        ResourceDescriptor::RUNSTATE currentState = p_taskJob->getState();
        calcStateNotify(currentState);

        if (currentState <= ResourceDescriptor::STATE_COMPLETED)
        {
            p_messagesFeedback->setMessage(p_launchData->info(), WxFeedback::INFO);
            p_messagesFeedback->setMessage("Successfully submitted job.",
                                           WxFeedback::INFO);
        }
        else
        {
            p_messagesFeedback->setMessage("Job submission failed.",
                                           WxFeedback::ERROR);
        }
    }
    else
    {
        LaunchBatch::recordFailure(p_taskJob, p_launchData->message());
        p_messagesFeedback->setMessage(p_launchData->message(), WxFeedback::ERROR);
        p_messagesFeedback->setMessage("Job submission failed.",
                                       WxFeedback::ERROR);
        p_launchData->removeStaging();
    }

    p_jobStaged = false;
    delete p_launchData;

    p_launchData = NULL;
}


void WxLauncher::quitMenuItemClickCB(wxCommandEvent& event)
{
    this->Close();
}


void WxLauncher::helpLauncherMenuitemClickCB(wxCommandEvent& event)
{
    WxHelpViewer::showKey("Launcher");
}


void WxLauncher::helpSupportMenuitemClickCB(wxCommandEvent& event)
{
    BrowserHelp help;
    help.showFeedbackPage();
}


/**
 *  Based upon a machine name, validate whether the code required for
 *  the current job is available on that machine.  If not, return
 *  false.
 */
bool WxLauncher::validateMachineWithCode(const string& machineName,
                                         const string& code)
{
    bool result = false;
    RefMachine* rfrnc = RefMachine::refLookup(machineName);

    if (rfrnc != (RefMachine*)0)
    {
        if (rfrnc->offersCode(code))
            result = true;
    }
    else
    {
         p_messagesFeedback->setMessage(machineName +
                             " is not listed as a registered machine in ECCE.",
                             WxFeedback::WARNING);
    }

    return result;
}


void WxLauncher::saveJob()
{
    //  These should come from the Preferences object itself.  By the time
    //  this gets called, it should already be updated.  Need to double check
    //  that somewhere in the process of the machine preferences getting saved
    //  that the reference to p_slctPrefs does not become invalid.

    string calcName;
    Launchdata ldat = p_taskJob->launchdata();

    ldat.machine = p_slctPrefs->getRegisteredMachine()->refname();

    ldat.queue = "";
    ldat.nodes = 1;
    ldat.maxwall = "0 0:0";
    ldat.maxmemory = 0;
    ldat.minscratch = 0;
    ldat.user = "";
    ldat.totalprocs = 1;
    ldat.remoteShell = "";
    ldat.rundir = "";
    ldat.scratchdir = "";

    if (p_slctPrefs->isOptionSupported("Q"))
    {
        ldat.queue = p_slctPrefs->getProfile(p_profile).queue;
        if (ldat.queue.empty())
            ldat.queue = p_slctPrefs->getQueueName();
        ldat.nodes = p_slctPrefs->getNodes();
        ldat.priority = "";

        if (p_slctPrefs->isOptionSupported("TL"))
            ldat.maxwall = WxTimeSpanEdit::toString(p_slctPrefs->getMaxWallTime());

        if (p_slctPrefs->isOptionSupported("MM"))
            ldat.maxmemory = p_slctPrefs->getMemoryLimit();

        if (p_slctPrefs->isOptionSupported("SS"))
            ldat.minscratch = p_slctPrefs->getScratchSpace();
    }
    else
    {
        ldat.queue = "";
        ldat.nodes = 1;
        ldat.priority = StringConverter::toString(p_slctPrefs->getPriority());

        if (p_slctPrefs->isOptionSupported("TL"))
            ldat.maxwall = "0 0:0";

        if (p_slctPrefs->isOptionSupported("MM"))
            ldat.maxmemory = 0;

        if (p_slctPrefs->isOptionSupported("SS"))
            ldat.minscratch = 0;
    }

    ldat.totalprocs = p_slctPrefs->getProcessors();
    ldat.remoteShell = p_slctPrefs->getRemoteShell();
    AllocationProfiles::Profile prof = p_slctPrefs->getProfile(p_profile);
    ldat.rundir = prof.rundir;
    ldat.scratchdir = prof.scratchdir;
    ldat.profile = AllocationProfiles::isDefault(p_profile) ? "" : prof.name;
    ldat.account = p_slctPrefs->isOptionSupported("AA") ? prof.account : "";

    if (p_slctPrefs->isOptionSupported("UN"))
    {
        ldat.user = p_slctPrefs->getUsername();
    }

    this->showLaunchData(ldat);

    struct Jobdata jobdata;
    string fullFile = TempStorage::getJobRunDirectoryPath(p_taskJob->getURL());
    jobdata.jobpath = ldat.rundir + fullFile;

    // save to calculation
    if (p_taskJob->launchdata(ldat) && p_taskJob->jobdata(jobdata))
    {
        // Send message that the calc was modified so calcmgr updates the
        // machine name for launching.
        launchChangeNotify();
    }
    else
    {
        string msg = "Unable to save launch settings for ";
        msg += p_taskJob->getName();
        msg += ".";
        p_messagesFeedback->setMessage(msg, WxFeedback::ERROR);
    }
}


void WxLauncher::showLaunchData(Launchdata ldat)
{
}


void WxLauncher::clearContext()
{
    // Can't launch or save a configuration until a machine has been
    // selected.

    p_taskJob = NULL;

    if (p_jobStaged)
        this->cancelStagedLaunch();

    this->populateMachinesList();
    this->refreshLaunchControls();
    p_messagesFeedback->clearMessage();
    p_messagesFeedback->setContextLabel("");
}


void WxLauncher::launchChangeNotify()
{
    JMSMessage *mesg = newMessage();
    mesg->addProperty("url", p_taskJob->getURL().toString());
    publish("ecce_url_launch", *mesg);

    delete mesg;
}


void WxLauncher::calcStateNotify(const ResourceDescriptor::RUNSTATE& state)
{
    string statestr = ResourceUtils::stateToString(state);

    JMSMessage *mesg = new JMSMessage();
    mesg->addProperty("url", p_taskJob->getURL().toString());
    mesg->addProperty("state", statestr);
    publish("ecce_url_state",*mesg);

    delete mesg;
}



//  ---- test hook -----------------------------------------------------------

bool WxLauncher::selectMachine(const string& refname)
{
    if (p_machinesChoice->FindString(refname) < 0)
        return false;
    p_machinesChoice->SetStringSelection(refname);
    wxCommandEvent ev(wxEVT_CHOICE, p_machinesChoice->GetId());
    ev.SetEventObject(p_machinesChoice);
    this->machinesChoiceSelectedCB(ev);
    return true;
}


/**
 *  A user's pick: the popup changes the shown entry, then the event goes
 *  the choice's own way, through its disabler, as from the GUI.
 */
bool WxLauncher::pickMachine(const string& refname)
{
    int n = p_machinesChoice->FindString(refname);
    if (n < 0)
        return false;
    p_machinesChoice->wxChoice::SetSelection(n);
    wxCommandEvent ev(wxEVT_CHOICE, p_machinesChoice->GetId());
    ev.SetEventObject(p_machinesChoice);
    ev.SetInt(n);
    ev.SetString(refname);
    p_machinesChoice->GetEventHandler()->ProcessEvent(ev);
    return true;
}


bool WxLauncher::clickMachineSettings()
{
    wxWindow* b = FindWindow(ID_BUTTON_WXLAUNCHER_MACHSETTINGS);
    if (b != NULL && p_inFold && !p_advOpen)
        this->setAdvanced(true, false);
    if (b == NULL || !b->IsShown() || !b->IsEnabled())
        return false;
    wxCommandEvent ev(wxEVT_BUTTON, b->GetId());
    ev.SetEventObject(b);
    this->GetEventHandler()->ProcessEvent(ev);
    return true;
}


bool WxLauncher::clickLaunch()
{
    wxWindow* b = FindWindow(ID_BUTTON_WXLAUNCHER_LAUNCH);
    if (b == NULL || !b->IsEnabled())
        return false;
    wxCommandEvent ev(wxEVT_BUTTON, b->GetId());
    ev.SetEventObject(b);
    this->GetEventHandler()->ProcessEvent(ev);
    return true;
}


bool WxLauncher::selectQueue(const string& name)
{
    if (p_queueChoice->FindString(name) == wxNOT_FOUND)
        return false;
    p_queueChoice->SetStringSelection(name);
    wxCommandEvent ev(wxEVT_CHOICE, p_queueChoice->GetId());
    ev.SetEventObject(p_queueChoice);
    this->queueChoiceSelectedCB(ev);
    return true;
}


void WxLauncher::setRunDirectory(const string& path)
{
    p_calcDrctyTextCtrl->SetValue(path);
}


void WxLauncher::setAccount(const string& a)
{
    p_allocAcctTextCtrl->SetValue(a);
}


void WxLauncher::setProcessors(int n)
{
    if (p_slctPrefs != NULL && p_slctPrefs->isOptionSupported("Q"))
        p_batchProcsParamEdit->setValue(n);
    else
        p_wkstnProcsParamEdit->setValue(n);
    this->refreshCoresWarning();
    p_prefsEdited = true;
    this->ensureEditsApplied(false);
}


string WxLauncher::hookGet(const string& what)
{
    if (what == "request")
        return p_lastRegisterRequest;
    if (what == "machine")
        return (string)p_machinesChoice->GetStringSelection();
    if (what == "machines" || what == "queues")
    {
        ewxChoice* c = (what == "machines") ? p_machinesChoice : p_queueChoice;
        string all;
        for (unsigned i = 0; i < c->GetCount(); i++)
            all += (i ? "," : "") + (string)c->GetString(i);
        return all;
    }
    if (what == "queue")
        return (string)p_queueChoice->GetStringSelection();
    if (what == "procsmax")
        return StringConverter::toString(p_batchProcsParamEdit->getMaximum());
    if (what == "procs")
        return StringConverter::toString(p_batchProcsParamEdit->getValue());
    if (what == "wsprocs")
        return StringConverter::toString(p_wkstnProcsParamEdit->getValue());
    if (what == "wsprocsmax")
        return StringConverter::toString(p_wkstnProcsParamEdit->getMaximum());
    if (what == "wsprocslabel")
        return p_wkstnProcsParamEdit->getUnitsVisible()
                   ? p_wkstnProcsParamEdit->getUnitsLabel() : "";
    if (what == "coreswarn")
        return p_coresWarnStaticText->IsShown()
                   ? (string)p_coresWarnStaticText->GetLabel() : "";
    if (what == "memory")
        return StringConverter::toString(p_maxMemoryParamEdit->getValue());
    if (what == "memrange")
        return p_maxMemoryParamEdit->getRangeVisible()
                   ? p_maxMemoryParamEdit->getRangeLabel() : "";
    if (what == "memunits")
        return p_maxMemoryParamEdit->getUnitsVisible()
                   ? p_maxMemoryParamEdit->getUnitsLabel() : "";
    if (what == "scratchunits")
        return p_minScratchParamEdit->getUnitsVisible()
                   ? p_minScratchParamEdit->getUnitsLabel() : "";
    if (what == "block")
        return FindWindow(ID_PANEL_WXLAUNCHER_6)->IsShown() ? "1" : "0";
    if (what == "password")
        return (p_password1Panel->IsShown() &&
                FindWindow(ID_PANEL_WXLAUNCHER_6)->IsShown()) ? "1" : "0";
    if (what == "username")
        return p_usernameTextCtrl->IsShownOnScreen() ? "1" : "0";
    if (what == "scratchdir")
        return p_scratchDrctyPanel->IsShownOnScreen() ? "1" : "0";
    if (what == "advanced")
        return !p_advButton->IsShown() ? "none" : (p_advOpen ? "open" : "closed");
    if (what == "rundir")
        return STLUtil::trim((string)p_calcDrctyTextCtrl->GetValue());
    if (what == "account")
        return STLUtil::trim((string)p_allocAcctTextCtrl->GetValue());
    if (what == "scratch")
        return STLUtil::trim((string)p_scratchDrctyTextCtrl->GetValue());
    if (what == "profile")
        return p_profile;
    if (what == "profiles")
    {
        vector<string> n = p_slctPrefs->profileNames();
        string all;
        for (size_t i = 0; i < n.size(); i++)
            all += (i ? "," : "") + n[i];
        return all;
    }
    if (what == "profilechoice")
        return p_profileChoice->IsShown() ? "1" : "0";
    if (what == "connect")
        return this->checkConnectAllowed() ? "1" : "0";
    if (what == "pumps")
        return StringConverter::toString(p_stepPumps);
    if (what == "launcherror")
        return p_launchError;
    if (what == "launchable")
        return p_launchButton->IsEnabled() ? "1" : "0";
    if (what == "button")
    {
        wxWindow* b = FindWindow(ID_BUTTON_WXLAUNCHER_MACHSETTINGS);
        return (b != NULL && b->IsShown()) ? "1" : "0";
    }
    return "<unknown>";
}


void WxLauncher::setUsername(const string& u)
{
    //  As typed: the text control sets its value without an event.
    p_usernameTextCtrl->ChangeValue(u);
    wxCommandEvent ev(wxEVT_TEXT, p_usernameTextCtrl->GetId());
    ev.SetEventObject(p_usernameTextCtrl);
    this->usernameTextCtrlUpdateCB(ev);
}
