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
    using std::find;
    using std::min;
    using std::max;

#include "wx/event.h"
#include "wx/wx.h"

#include "util/BrowserHelp.H"
#include "util/Ecce.H"
#include "util/EcceSortedVector.H"
#include "util/ErrMsg.H"
#include "util/ResourceUtils.H"
#include "util/SFile.H"
#include "util/STLUtil.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"
#include "util/TempStorage.H"
#include "util/TypedFile.H"

#include "tdat/QueueMgr.H"

#include "dsm/NWChemMDModel.H"
#include "dsm/CodeFactory.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/JCode.H"
#include "dsm/MdTask.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceTool.H"

#include "comm/EcceShell.H"
#include "comm/RCommand.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxMenuItem.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/ewxTextCtrl.H"

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
    SetIcon(wxIcon(ewxBitmap::pixmapFile(rs.getTool(LAUNCHER)-> getIcon()),
                                         wxBITMAP_TYPE_XPM));
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
    p_password2Panel        =        (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_PASSWORD2));
    p_password2TextCtrl     =     (ewxTextCtrl *)(FindWindow(ID_TEXTCTRL_WXLAUNCHER_PASSWORD2));
    p_password2Label        =   (ewxStaticText *)(FindWindow(ID_STATIC_WXLAUNCHER_PASSWORD2LABEL));

    p_calcDrctyPanel        =        (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_CALCDIR));
    p_calcDrctyTextCtrl     =     (ewxTextCtrl *)(FindWindow(ID_TEXTCTRL_WXLAUNCHER_CALCDIR));
    p_scratchDrctyPanel     =        (ewxPanel *)(FindWindow(ID_PANEL_WXLAUNCHER_SCRATCHDIR));
    p_scratchDrctyTextCtrl  =     (ewxTextCtrl *)(FindWindow(ID_TEXTCTRL_WXLAUNCHER_SCRATCHDIR));

    p_launchButton          =       (ewxButton *)(FindWindow(ID_BUTTON_WXLAUNCHER_LAUNCH));
    p_launchButton->Enable(false);

    p_messagesFeedback = (WxFeedback *)(FindWindow(ID_FEEDBACK_WXLAUNCHER_MESSAGES));
    p_messagesFeedback->SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY | wxWS_EX_PROCESS_UI_UPDATES);
    p_messagesFeedback->setContextLabel("");
    p_messagesFeedback->setDropHandler(this);
    p_messagesFeedback->setEditStatus(WxFeedback::NA);
    p_messagesFeedback->GetSizer()->SetMinSize(-1, 1);

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

    int totMachs = MachinePreferences::countItems();
    p_numCfgd = 0;
    p_numIncl = 0;

    //  Retrieve the user preferences for each registered machine
    for (int i = 0; i < totMachs; i++)
    {
        MachinePreferences *prefs = MachinePreferences::getItemAt(i);
        RefMachine *rgstn = prefs->getRegisteredMachine();

        if (rgstn != NULL)
        {
            string refname = rgstn->refname();
            bool include = true;

            if (p_taskJob != NULL)
                include = validateMachineWithCode(refname, p_taskJob->application()->getCodeName());

            if (include)
            {
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
        }
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


bool WxLauncher::isGlobusSSH(RefMachine *machRgstn, string shellName)
{
    return (isGlobusSSH(isRemoteGlobus(machRgstn, shellName), shellName));
}


bool WxLauncher::isGlobusSSH(bool remoteGlobus, string shellName)
{
    return (remoteGlobus && (shellName.find("ssh") != string::npos));
}


bool WxLauncher::isRemoteGlobus(RefMachine *machRgstn,
                                             string shellName)
{
    string machName = machRgstn->fullname();
    bool isGlobus = (shellName.find("Globus") != string::npos);
    bool isRemote = RCommand::isRemote(machName, shellName);
    bool result = (isGlobus && isRemote);

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
void WxLauncher::refreshControls()
{
    bool prevState = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    RefMachine *machRgstn = p_slctPrefs->getRegisteredMachine();
    p_machineNameStaticText->SetLabel(machRgstn->fullname());

    bool isBatch = p_slctPrefs->isOptionSupported("Q");

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

    this->refreshCalcDirectory();
    this->refreshScratchDirectory();

    bool cnctAllwd = this->checkConnectAllowed();
    p_shellOpenButton->Enable(cnctAllwd);

    if (p_taskJob != NULL)
        this->refreshLaunchControls(cnctAllwd);

    this->Layout();
    this->Fit();
    
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
    p_password2TextCtrl->Clear();

    p_shellOpenButton->Enable(false);

    if (p_taskJob != NULL)
        this->refreshLaunchControls(false);

    this->Layout();
    this->Fit();

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
 *  Contents of the auxiliary password text area have changed.  This method
 *  first establishes that the changes do not arise programmatically--that
 *  they are the result of user interaction.  If so, the prefsChgd flag is
 *  set.
 */
void WxLauncher::password2TextCtrlUpdateCB(wxCommandEvent& event)
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
                      rgstn->frontendMachine(), rgstn->frontendBypass());

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

        bool cnctAllwd = this->checkConnectAllowed();

        this->verifyMachineListLocation(cnctAllwd);
        p_shellOpenButton->Enable(cnctAllwd);

        this->refreshLaunchControls(cnctAllwd);

        this->Layout();
        this->Fit();
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
            string user = (string)p_usernameTextCtrl->GetValue();
            result = (STLUtil::trim(user)).size() > 0;
        }
    }

    return result;
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
 *  Return true/false depending on whether the indicated shell is Globus-SSH.
 */
bool WxLauncher::checkGlobusSSH(bool remoteGlobus, string shellName)
{
    return (remoteGlobus && (shellName.find("ssh") != string::npos));
}


/**
 *  Return true/false depending on whether the currently selected shell is Globus-SSH.
 */
bool WxLauncher::checkGlobusSSH()
{
    string slctShell = (string)p_remShellChoice->GetStringSelection();
    RefMachine *machRgstn = p_slctPrefs->getRegisteredMachine();

    return (checkGlobusSSH(machRgstn, slctShell));
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
    bool result = (cnctAllwd && (p_taskJob != NULL)
                             && (p_taskJob->getState() == ResourceDescriptor::STATE_READY));

    if (result)
    {
        //  If this point is reached, then we know that p_taskJob is not null
        string code = p_taskJob->application()->getCodeName();
        result = (result && machRgstn->hasCode(code));
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
        if (p_queueChoice->GetCount() > 0)
        {
            string queueName = (string)(p_queueChoice->GetStringSelection());
            p_slctPrefs->setQueueName(queueName);
        }

        p_slctPrefs->setProcessors(p_batchProcsParamEdit->getValue());
        p_slctPrefs->setNodes(p_batchNodesParamEdit->getValue());

        if (p_slctPrefs->isOptionSupported("TL"))
            p_slctPrefs->setMaxWallTime(p_wallTimeEdit->getValue());

        if (p_slctPrefs->isOptionSupported("AA"))
            p_slctPrefs->setAllocationAccount(STLUtil::trim((string)(p_allocAcctTextCtrl->GetValue())));

        if (p_slctPrefs->isOptionSupported("MM"))
            p_slctPrefs->setMemoryLimit(p_maxMemoryParamEdit->getValue());

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

    if (p_slctPrefs->isOptionSupported("RD"))
        p_slctPrefs->setRemoteDirectory(STLUtil::trim((string)(p_calcDrctyTextCtrl->GetValue())));

    p_slctPrefs->setScratchDirectory(STLUtil::trim((string)(p_scratchDrctyTextCtrl->GetValue())));
    p_prefsEdited = false;
}


/**
 *  Notify other interested Ecce tools, via messaging, that the preferences have
 *  changed for the indicated machine.
 */
void WxLauncher::prefsChangeNotify(string refname)
{
    //  Publish a prefs_updated message
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
    this->launchCalc(true);
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
 *  Return true or false depending on whether a second password is required
 *  to invoke the indicated remote shell on the machine.
 */
bool WxLauncher::checkRemoteGlobus(RefMachine *machRgstn,
                                        string shellName)
{
    string machName = machRgstn->fullname();
    bool isGlobus = (shellName.find("Globus") != string::npos);
    bool isRemote = RCommand::isRemote(machName, shellName);

    return (isGlobus && isRemote);
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
        p_allocAcctTextCtrl->SetValue(p_slctPrefs->getAllocationAccount());

    p_allocAcctPanel->Show(supported);      //  Need to adjust alignment of the queue label according to visibility
                                            //  of the allocation account panel.  Not as straightforward as it sounds.

    p_queueLabelLeft->Show(!supported);     //  The SetWindowStyle() does nothing.  The alignment of a static text
    p_queueLabelRight->Show(supported);     //  control (i.e., Label) cannot be changed once the control is created.
                                            //  This is confirmed from information gleaned off the on-line discussion
                                            //  forums.  Two ways to get the desired behavior:  (1) Destroy and recreate
                                            //  the control with the proper alignment; (2) Have two controls with
                                            //  opposite alignments, and ensure only one is visible at a time.
}


/**
 *  Load the names of the available queues into the appropriate drop-down list.
 *  Show only if more than one to choose from.  Queue referenced by the machRgstn
 *  object is selected and returned.
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

        string slctnName = p_slctPrefs->getQueueName();

        if (slctnName == "")
        {
            p_queueChoice->SetSelection(0);
        }
        else
        {
            p_queueChoice->SetStringSelection(slctnName);
        }

        slctQueue = this->getQueueSelection();
    }

    p_queuePanel->Show(p_queueChoice->GetCount() > 1);
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
        unsigned long value = p_slctPrefs->getMemoryLimit();
        unsigned long vmax = value;
        unsigned long dflt = 0;
        string units = "MBytes";

        if (slctQueue != NULL)
        {
            vmax = slctQueue->memLimit();
            dflt = slctQueue->defMemory();
            units = slctQueue->memUnits();           //  string representation; could be empty
        }

        if (value == 0)
            value = ((dflt == 0) ? (vmax / 10) : dflt);

        if (value > vmax)
            value = vmax / 2;

        p_maxMemoryParamEdit->setRange(0, vmax);
        p_maxMemoryParamEdit->setValue(value);
        p_maxMemoryParamEdit->setUnitsLabel(units);
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

        if (value > vmax)
            value = vmax / 2;

        p_minScratchParamEdit->setRange(0, vmax);
        p_minScratchParamEdit->setValue(value);
        p_minScratchParamEdit->setUnitsLabel(units);
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

    p_wkstnProcsParamEdit->setRange(minProcs, maxProcs);
    p_wkstnProcsParamEdit->setValue(numProcs);
    p_wkstnProcsParamEdit->Update();
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
        p_calcDrctyTextCtrl->SetValue(p_slctPrefs->getRemoteDirectory());

    p_calcDrctyPanel->Show(supported);
}


void WxLauncher::refreshScratchDirectory()
{
    bool supported = p_slctPrefs->isOptionSupported("SD");

    if (supported)
        p_scratchDrctyTextCtrl->SetValue(p_slctPrefs->getScratchDirectory());

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


void WxLauncher::refreshAuthentication(RefMachine *machRgstn, string shellName)
{
    string uname = "";
    bool remoteGlobus = false;
    bool supported = p_slctPrefs->isOptionSupported("UN");

    if (supported)
    {
        uname = p_slctPrefs->getUsername();

        if (!shellName.empty())
            remoteGlobus = this->checkRemoteGlobus(machRgstn, shellName);
    }

    p_usernameTextCtrl->SetValue(uname);
    p_usernameTextCtrl->Enable(!remoteGlobus);
    p_usernameLabel->Show(supported);
    p_usernameTextCtrl->Show(supported);

    this->refreshPasswords(remoteGlobus, shellName,
                           machRgstn->fullname(), uname);
}


void WxLauncher::refreshPasswords(const bool& remoteGlobus,
                                  const string& shellName,
                                  const string& machName, const string& user)
{
    bool supported = p_slctPrefs->isOptionSupported("PW");
    bool pswd2Rqrd = false;

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

        //  UI now indicates required fields with asterisk.
        //  Make sure they are included in the labels for the password fields.
        if (remoteGlobus)
        {
            pswd2Rqrd = checkGlobusSSH(remoteGlobus, shellName);
            p_password1Label->SetLabel(pswd2Rqrd ? "ssh Pass:": "Pass Phrase:");
        }
        else
            p_password1Label->SetLabel("Password:");

        if (pswd2Rqrd)
        {
            string pswd2 = "";

            string url = "ssh://" + machName;
            ba = AuthCache::getCache().getAuthentication(url, user, "", 1);
            if (ba != NULL) {
              pswd2 = ba->m_pass;
              delete ba;
            }

            p_password2TextCtrl->SetValue(pswd2);
        }
    }

    p_password1Panel->Show(supported);
    p_password2Panel->Show(supported && pswd2Rqrd);
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
            }
        }
    }


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
    }

    this->Layout();
    this->Fit();

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
            p_maxMemoryParamEdit->setValue(ldat.maxmemory);
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
            p_password2TextCtrl->SetValue("");
        }

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
    this->Fit();

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
 *  Implementation of a DropListener interface.
 */
void WxLauncher::processDrop(const string &url)
{
    this->setContext(url);
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

    this->refreshLaunchControls();
}


void WxLauncher::updateStatePreferences()
{
    p_messagesFeedback->readPreferences();
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
    bool doLaunch =  fullLaunchFlag && !rgstn->userSubmit();

    p_launchData = new Launch(p_taskJob, kvargs, doLaunch);
    bool valid = true;

    while (valid && !p_launchData->done())
    {
        string dscpn = p_launchData->description();
        p_messagesFeedback->setMessage(dscpn, WxFeedback::INFO, false, true);

        if (!(valid = p_launchData->nextOperation()))
            p_messagesFeedback->setMessage(p_launchData->message(), WxFeedback::ERROR, false, true);

        else if (p_launchData->message() != "")
            p_messagesFeedback->setMessage(p_launchData->message(), WxFeedback::WARNING, false, true);

        else if (p_launchData->info() != "")
            p_messagesFeedback->setMessage(p_launchData->info(), WxFeedback::INFO, false, true);

        this->UpdateWindowUI();
    }

    if (!p_launchData->done())
    {
        p_messagesFeedback->setMessage("Launch aborted...", WxFeedback::WARNING);
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
                p_messagesFeedback->setMessage(mesg, WxFeedback::INFO);
            }
            else
            {
                mesg = "Job submission failed.";
                p_messagesFeedback->setMessage(mesg, WxFeedback::ERROR);
            }

            delete p_launchData;
            p_launchData = NULL;
        }
        else
        {
            p_jobStaged = p_launchData->done();

            if (p_jobStaged)
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
            kvargs["##account_no##"] = p_slctPrefs->getAllocationAccount();

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

        if (this->checkGlobusSSH())
        {
          string ptext2
                  = STLUtil::trim((string)(p_password2TextCtrl->GetValue()));
          kvargs["##password2##"] = ptext2;
        }
    }

    if (ldat.remoteShell.find("Globus") != string::npos)
        kvargs["##globus##"] = "true";

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

    string pathBase = p_slctPrefs->getRemoteDirectory();
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
    // an xterm shell in the run directory to be created
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
        p_messagesFeedback->setMessage(p_launchData->message(), WxFeedback::ERROR);
        p_messagesFeedback->setMessage("Job submission failed.",
                                       WxFeedback::ERROR);
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
    BrowserHelp help;
    help.showPage(help.URL("Launcher"));
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
        if (rfrnc->hasCode(code))
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
    ldat.rundir = p_slctPrefs->getRemoteDirectory();
    ldat.scratchdir = p_slctPrefs->getScratchDirectory();

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

