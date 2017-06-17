/**
 *  @file
 *  @author Ken Swanson
 *
 *  Utility for viewing the status of machines that are registered to run
 *  calculations.  Specific information includes general machine status,
 *  queue or processes, and disk usage.
 *
 */


#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <vector>
    using std::vector;


#include "util/STLUtil.H"
#include "util/Ecce.H"
#include "util/Preferences.H"
#include "util/KeyValueReader.H"
#include "util/SFile.H"
#include "util/EcceSortedVector.H"
#include "util/EcceMap.H"
#include "util/StringTokenizer.H"
#include "util/JMSMessage.H"
#include "util/BrowserHelp.H"

#include "tdat/Queue.H"
#include "tdat/QueueMgr.H"

#include "dsm/MachinePreferences.H"
#include "dsm/CodeFactory.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceTool.H"

#include "comm/RCommand.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxTextCtrl.H"

#include "MachineBrowser.H"


BEGIN_EVENT_TABLE(MachineBrowser, MachineBrowserGUI)
    EVT_COMMAND(ID_MACHINES_TABLEVIEW, ewxEVT_TABLE_SELECTION_CHANGED,
                    MachineBrowser::machinesTableViewSelectionChangedCB)
END_EVENT_TABLE()


MachineBrowser::MachineBrowser(      wxWindow* parent,
                                     wxWindowID id,
                               const wxString& caption,
                               const wxPoint& pos,
                               const wxSize& size,
                                     long style)
        : MachineBrowserGUI(parent, id, caption, pos, size, style),
          JMSPublisher(MACHBROWSER)
{
    p_inCtrlUpdate = false;
    TableColumn *clmn = NULL;

    p_machModel = new DefaultTableModel(8);

    p_machModel->setColumnLabel(0, "");
    clmn = p_machModel->getColumn(0);
    clmn->setMinMaxWidth(30, 30);
    clmn->setTextAlignment(wxLIST_FORMAT_CENTRE);
    clmn->setUserResizeAllowed(false);

    p_machModel->setColumnLabel(1, "Machine");
    clmn = p_machModel->getColumn(1);
    clmn->setMinMaxWidth(80, 150);

    p_machModel->setColumnLabel(2, "Defined");
    clmn = p_machModel->getColumn(2);
    clmn->setMinMaxWidth(60, 80);

    p_machModel->setColumnLabel(3, "Procs");
    clmn = p_machModel->getColumn(3);
    clmn->setMinMaxWidth(50, 70);

    p_machModel->setColumnLabel(4, "Queue");
    clmn = p_machModel->getColumn(4);
    clmn->setMinMaxWidth(70, 70);

    p_machModel->setColumnLabel(5, "Shells");
    clmn = p_machModel->getColumn(5);
    clmn->setMinMaxWidth(80, 200);

    p_machModel->setColumnLabel(6, "Available Codes");
    clmn = p_machModel->getColumn(6);
    clmn->setMinMaxWidth(120, 250);

    p_machModel->setColumnLabel(7, "Description");
    clmn = p_machModel->getColumn(7);
    clmn->setMinMaxWidth(250, 400);
    clmn->setSizeAdjustWeight(1);

    this->updateListModel();
    p_machModel->setSortColumn(0);
    p_machModel->setSortDescending(true);

    p_slctElmt = NULL;

    p_statusQueries.reserve(3);
    p_statusQueries.push_back("");
    p_statusQueries.push_back("");
    p_statusQueries.push_back("");

    p_modeDscpns.reserve(3);
    p_modeDscpns.push_back("Machine Status");
    p_modeDscpns.push_back("Disk Usage");
    p_modeDscpns.push_back("Processes");

    this->createControls();

    Preferences *prefs = new Preferences("MachineBrowser");
    this->loadSettings(prefs);
    delete prefs;

    p_statFrame = new WxMachineStatus(this);
    p_statFrame->SetSize(700, 350);
    p_configDialog = new WxConfigureAccess(this);

    p_inCtrlUpdate = true;
    
    p_machView->update();
    p_slctElmt = (MachineTableElement *)(p_machView->getSelectedElement());
    this->refreshControls();

    // Get Registry
    ResourceDescriptor rs = ResourceDescriptor::getResourceDescriptor();

    // Set desktop icon
    SetIcon(wxIcon(ewxBitmap::pixmapFile(rs.getTool("MachineBrowser")->
                                         getIcon()), wxBITMAP_TYPE_XPM));

    p_inCtrlUpdate= false;
}


MachineBrowser::~MachineBrowser()
{
    Preferences *prefs = new Preferences("MachineBrowser");
    this->saveSettings(prefs);
    prefs->saveFile();

    delete prefs;

    p_machView->Destroy();
    delete p_machModel;
}


void MachineBrowser::createControls()
{
    int w, h;

    p_viewDetachButton
        = (ewxButton *)(FindWindowById(ID_BUTTON_MACHINEBROWSER_VIEWDETACH));
    p_machineQueryButton
        = (ewxButton *)(FindWindowById(ID_BUTTON_MACHINEBROWSER_QUERYMACHINE));
    p_diskQueryButton
        = (ewxButton *)(FindWindowById(ID_BUTTON_MACHINEBROWSER_QUERYDISK));
    p_processQueryButton
        = (ewxButton *)(FindWindowById(ID_BUTTON_MACHINEBROWSER_QUERYPROCESS));

    p_machineConfigButton
        = (ewxButton *)(FindWindowById(ID_BUTTON_MACHINEBROWSER_CONFIGURE));

    ewxPanel *panMachines
        = (ewxPanel *)(FindWindowById(ID_PANEL_MACHINEBROWSER_MACHINES));
    p_machView = new WxTableView(panMachines, ID_MACHINES_TABLEVIEW);
    panMachines->GetSizer()->Add(p_machView, 1, wxGROW|wxALL, 5);

    p_machRgstnLabel
        = (ewxStaticText*)(FindWindowById(ID_STATICTEXT_MACHINEBROWSER_MACHINENAME));
    p_machRgstnLabel->SetLabel("");

    p_modeDscpnLabel
        = (ewxStaticText *)(FindWindowById(ID_STATIC_MACHINEBROWSER_MODE));
    p_modeDscpnLabel->SetLabel("");

    p_statusTextCtrl
        = (ewxTextCtrl*)(FindWindowById(ID_TEXTCTRL_MACHINEBROWSER_STATUS));
    p_statusTextCtrl->SetEditable(false);

    w = p_statusTextCtrl->GetCharWidth() * 80;
    h = p_statusTextCtrl->GetCharHeight() * 24;
    p_statusTextCtrl->SetClientSize(w, h);

    p_machView->addRowColor(getWindowColor()); // ECCE official gray90 (window)
    p_machView->addRowColor(getHightLightRowColor()); // ECCE official gray75 (shadow)
    p_machView->setBackground(getWindowColor());
    p_machView->setModel(p_machModel);

    this->Fit();
    this->SetAutoLayout(true);
}


void MachineBrowser::saveSettings(Preferences *prefs)
{
    const string topic = "BrowserFrame";

    wxRect r = this->GetRect();

    prefs->setInt(topic + ".Left", r.GetLeft());
    prefs->setInt(topic + ".Top", r.GetTop());
    prefs->setInt(topic + ".Width", r.GetWidth());
    prefs->setInt(topic + ".Height", r.GetHeight());

    p_machModel->saveSettings(prefs, "MachinesViewModel");
}


void MachineBrowser::loadSettings(Preferences *prefs)
{
    int x = WXMACHINEBROWSER_DFLT_LEFT;
    int y = WXMACHINEBROWSER_DFLT_TOP;
    int w = WXMACHINEBROWSER_DFLT_WIDTH;
    int h = WXMACHINEBROWSER_DFLT_HEIGHT;

    const string topic = "BrowserFrame";
    string member;

    p_machModel->loadSettings(prefs, "MachinesViewModel");

    member = topic + ".Left";

    if (prefs->isDefined(member))
        prefs->getInt(member, x);

    member = topic + ".Top";

    if (prefs->isDefined(member))
        prefs->getInt(member, y);

    member = topic + ".Width";

    if (prefs->isDefined(member))
        prefs->getInt(member, w);

    member = topic + ".Height";

    if (prefs->isDefined(member))
        prefs->getInt(member, h);

    this->SetSize(x, y, w, h);
}


void MachineBrowser::refreshList()
{
    this->updateListModel();
    p_machView->update();
    p_slctElmt = (MachineTableElement *)(p_machView->getSelectedElement());
}


void MachineBrowser::configureButtonClickCB( wxCommandEvent& event )
{
    bool cncld = false;
    MachinePreferences *prefs = p_slctElmt->getPreferences();

    p_configDialog->showConfigure(prefs, &cncld);

    if (!cncld)
    {
        p_slctElmt->setQueryAllowed(checkConnectAllowed(prefs));
        p_machView->update();

        if (p_slctElmt->isQueryAllowed())
            p_modeDscpnLabel->SetLabel("");
        else
            p_modeDscpnLabel->SetLabel("(Not Setup for Access)");

        this->updateQueryButtons();

        MachinePreferences::saveChanges();

       //  Publish a prefs_updated message
        JMSMessage* mesg = newMessage();
        mesg->addProperty("machName", prefs->getItemKey());
        publish("ecce_machconf_changed", *mesg);

    }
}


void MachineBrowser::machineQueryButtonClickCB(wxCommandEvent& event)
{
    wxBusyCursor cursor;

    p_modeDscpnLabel->SetLabel(p_modeDscpns[0]);
    p_statusTextCtrl->SetValue(queryStatus(0));
    p_viewDetachButton->Enable(true);
}


string MachineBrowser::getSelectionName()
{
    string machName = "";

    if (p_slctElmt != NULL)
        machName = p_slctElmt->getPreferences()->getRegisteredMachine()->fullname();

    return machName;
}


bool MachineBrowser::checkConnectAllowed(MachinePreferences *prefs)
{
    string shellName = prefs->getRemoteShell();
    bool result = (shellName.size() > 0);

    if (result && prefs->isOptionSupported("UN"))
    {
        result = (result && (prefs->getUsername()).size() > 0);
    }

    return result;
}

/**
 *  Enables/Disables query buttons depending on whether or not the
 *  preferences object is non-null and a connection attempt may be
 *  made to the reference machine.
 *
 */
void MachineBrowser::updateQueryButtons()
{
    bool prevState = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    bool queryAllwd = (p_slctElmt != NULL)
                            && p_slctElmt->isQueryAllowed();

    p_machineQueryButton->SetLabel(p_modeDscpns[0]);
    p_machineQueryButton->Enable(queryAllwd);

    p_diskQueryButton->SetLabel(p_modeDscpns[1]);
    p_diskQueryButton->Enable(queryAllwd);

    p_processQueryButton->SetLabel(p_modeDscpns[2]);
    p_processQueryButton->Enable(queryAllwd);

    p_inCtrlUpdate = prevState;
}


string MachineBrowser::queryStatus(int mode)
{
    string result;
    string dscpn;
    string cmnd;

    MachinePreferences *prefs = p_slctElmt->getPreferences();
    RefMachine *rgstn = prefs->getRegisteredMachine();

    string fullName = rgstn->fullname();
    string remShell = prefs->getRemoteShell();
    string shell = rgstn->shell();
    string userName = prefs->getUsername();
    string shellPath = rgstn->shellPath();
    string libPath = rgstn->libPath();
    string sourceFile = rgstn->sourceFile();
    string frontendMachine = rgstn->frontendMachine();
    string frontendBypass = rgstn->frontendBypass();

    string password = p_configDialog->getPassword();

    RCommand cnctn(fullName, remShell, shell, userName, password,
                   frontendMachine, frontendBypass,
                   shellPath, libPath, sourceFile);

    if (cnctn.isOpen())
    {
        cmnd = p_statusQueries[mode];
        string mesg = "Can't execute query:  " + cmnd;
        string output;

        if (cnctn.execout(cmnd, output, mesg))
        {
            result = output;

        }
        else
        {
            result = cnctn.commError();

            if (result.find("Command output buffer length exceeded") != string::npos)
                result += "\n\n" + output;
        }
    }
    else
    {
        result = cnctn.commError();
    }

    return result;
}


void MachineBrowser::diskQueryButtonClickCB(wxCommandEvent& event)
{
    wxBusyCursor cursor;

    p_modeDscpnLabel->SetLabel(p_modeDscpns[1]);
    p_statusTextCtrl->SetValue(queryStatus(1));
    p_viewDetachButton->Enable(true);
}


void MachineBrowser::processQueryButtonClickCB(wxCommandEvent& event)
{
    wxBusyCursor cursor;

    p_modeDscpnLabel->SetLabel(p_modeDscpns[2]);
    p_statusTextCtrl->SetValue(queryStatus(2));
    p_viewDetachButton->Enable(true);
}


void MachineBrowser::machinesTableViewSelectionChangedCB(wxCommandEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        p_slctElmt = (MachineTableElement *)(p_machView->getSelectedElement());
        p_statusTextCtrl->Clear();

        this->refreshControls();
    }
}


void MachineBrowser::refreshControls()
{
    bool prevState = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    p_modeDscpnLabel->SetLabel("");

    if (p_slctElmt != NULL)
    {
        if (p_slctElmt->isQueryAllowed())
            p_modeDscpnLabel->SetLabel("");
        else
            p_modeDscpnLabel->SetLabel("(Not Setup for Access)");

        MachinePreferences *prefs = p_slctElmt->getPreferences();

        if (prefs != NULL)
        {
            RefMachine *rgstn = prefs->getRegisteredMachine();

            p_machRgstnLabel->SetLabel( rgstn->fullname());

            string machName = prefs->getItemKey();
            const QueueManager *queueMngr = QueueManager::lookup(machName);
            bool isWkstn = (queueMngr == NULL);

            string mngrName = isWkstn ? "Shell" : queueMngr->queueMgrName();
            const RefQueueManager *refQMngr = RefQueueManager::lookup(mngrName);

            p_statusQueries[0] = refQMngr->queryMachineCommand();
            p_statusQueries[1] = refQMngr->queryDiskUsageCommand();

            string cmnd = refQMngr->queryQueueCommand();

            if (!isWkstn)
            {
                int chpos = cmnd.find("##queue##");

                if (chpos != string::npos)
                {
                    cmnd.replace(chpos, 9, prefs->getQueueName());
                }
            }

            p_statusQueries[2] = cmnd;

            p_modeDscpns[0] = "Machine Status";
            p_modeDscpns[1] = "Disk Usage";
            p_modeDscpns[2] = (isWkstn ? "Processes" : "Queue");
        }

        p_viewDetachButton->Enable(false);
    }

    updateQueryButtons();
    p_machineConfigButton->Enable(p_slctElmt != NULL);

    p_inCtrlUpdate = prevState;
}


void MachineBrowser::mainWindowCloseCB(wxCloseEvent& event)
{
    p_statFrame->Destroy();
    this->Destroy();
}


void MachineBrowser::viewDetachButtonClickCB(wxCommandEvent& event)
{
    string machName, dscpn, status;

    machName = p_machRgstnLabel->GetLabel();
    dscpn = p_modeDscpnLabel->GetLabel();
    status = p_statusTextCtrl->GetValue();

    if (p_statFrame->IsShown())
    {
        p_statFrame->showStatus(dscpn, status);
        p_statFrame->Raise();
    }
    else
    {
        p_statFrame->SetTitle(machName);
        p_statFrame->showStatus(dscpn, status);
        p_statFrame->Show(true);
    }
}



void MachineBrowser::machineRegisterMenuItemClickCB( wxCommandEvent& event )
{
    this->showRegister(p_slctElmt);
}


void MachineBrowser::exitMenuitemClickCB(wxCommandEvent& event)
{
    this->Close(true);
}


void MachineBrowser::helpAppMenuitemClickCB(wxCommandEvent& event)
{
    BrowserHelp help;
    help.showPage(help.URL("MachineBrowser"));
}


void MachineBrowser::helpSupportMenuitemClickCB(wxCommandEvent& event)
{
    BrowserHelp help;
    help.showFeedbackPage();
}


void MachineBrowser::updateListModel()
{
    //  2005.0728
    //  One thing this is not doing yet is checking that existing
    //  table elements are removed from the model if a corresponding
    //  machine reference no longer exists.

    MachinePreferences *prefs = NULL;
    MachineTableElement *elmt = NULL;

    p_machModel->removeAllElements();
    int numPrefs = MachinePreferences::countItems();
    int i;

    for (i = 0; i < numPrefs; i++)
    {
        prefs = MachinePreferences::getItemAt(i);

        if (prefs->getRegisteredMachine() != NULL)
        {
            elmt = findTableElement(prefs->getItemKey());

            if (elmt == NULL)
            {
                elmt = new MachineTableElement(prefs);
                elmt->setQueryAllowed(checkConnectAllowed(prefs));
                p_machModel->addElement(elmt);
            }
        }
    }

    RefMachine::markUpdateUserPrefs();
    vector<string> *siteMachNames =
                        RefMachine::referenceNames(RefMachine::siteMachines);
    for (i = 0; i < siteMachNames->size(); i++)
    {
        elmt = this->findTableElement(siteMachNames->at(i));

        if (elmt != NULL)
            elmt->setSiteAccessible(true);
    }

    RefMachine::markUpdateUserPrefs();
    vector<string> *userMachNames =
                        RefMachine::referenceNames(RefMachine::userMachines);
    for (i = 0; i < userMachNames->size(); i++)
    {
        elmt = this->findTableElement(userMachNames->at(i));

        if (elmt != NULL)
            elmt->setSiteAccessible(false);
    }

    RefMachine::markUpdateUserPrefs();
}


void MachineBrowser::showRegister(MachineTableElement *elmt)
{
    // Send the start app request to gateway:
    Target trgt(GATEWAY, "");

    JMSMessage* mesg = newMessage();
    mesg->setTarget(trgt);
    mesg->addProperty("action", "start");
    publish("ecce_activity", *mesg);
    delete mesg;

    mesg = new JMSMessage();
    mesg->setTarget(trgt);
    mesg->addProperty("appname", MACHREGISTER);

    if (elmt != NULL)
        mesg->addProperty("initmachine", elmt->getPreferences()->getItemKey());

    mesg->addIntProperty("forcenew", 0);
    publish("ecce_get_app", *mesg);

    delete mesg;
}


MachineTableElement * MachineBrowser::findTableElement(string itemKey)
{
    int i = 0;
    bool found = false;
    MachineTableElement *elmt = NULL;
    int n = p_machModel->size();

    while ((i < n) && !found)
    {
        elmt = (MachineTableElement *)(p_machModel->elementAt(i));
        found = (elmt->getPreferences()->getItemKey().compare(itemKey) == 0);
        i++;
    }

    if (!found)
        elmt = NULL;

    return elmt;
}
