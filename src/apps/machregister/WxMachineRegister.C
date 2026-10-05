/**
 *  @file
 *  @author Ken Swanson
 *
 *  Utility to allow registration functions for calculation servers.
 *  Supports adding/editing/deleting of registered Machine instances.
 *  Usually this is invoked from the Gateway, but can be invoked from
 *  the command line via "ecce -admin".  In this mode, the use of the
 *  utility is intended for administrators, and a correspondingly more
 *  complete interface is displayed.
 *
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <regex>
  using std::ifstream;

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/BrowserHelp.H"
#include "util/Ecce.H"
#include "util/EcceMap.H"
#include "util/EcceSortedVector.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/KeyValueReader.H"
#include "util/ProcessMachine.H"
#include "util/SFile.H"
#include "util/STLUtil.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"

#include "tdat/Queue.H"
#include "tdat/QueueMgr.H"

#include "comm/RCommand.H"

#include "dsm/CodeFactory.H"
#include "dsm/MachinePreferences.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceTool.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxScrolledWindow.H"
#include "wxgui/ewxStaticBoxSizer.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxWindowUtils.H"

#include "wx/display.h"

#include "WxMachineRegister.H"
#include "MemoryUnits.H"

#define MAXLINE 512

WxMachineRegister::WxMachineRegister(wxWindow* parent,
                             const bool admin,
                             wxWindowID id,
                             const wxString& caption,
                             const wxPoint& pos,
                             const wxSize& size,
                             long style)
    : WxMachineRegisterGUI(parent, id, caption, pos, size, style)
{
    p_config = new EcceMap();
    p_adminFlag = admin;
    p_prefillFromSite = false;
    p_queuesDirty = false;

    p_slctRgstn = NULL;

    p_codeNames = CodeFactory::getFullySupportedCodeNames();

    p_shellNames.push_back("ssh");
    p_shellNames.push_back("ssh/ftp");
    p_shellNames.push_back("sshpass");

    this->initialize();
    this->loadMachinesList();
    this->loadQueueManagerList();

    Preferences prefs("MachineRegister");
    restoreSettings(prefs);

    // Get Registry
    ResourceDescriptor rs = ResourceDescriptor::getResourceDescriptor();

    // Set desktop icon
    ewxWindowUtils::setToolIcon(this, "MachineRegister");
}


WxMachineRegister::~WxMachineRegister()
{
    delete p_config;
}


void WxMachineRegister::saveSettings(Preferences& prefs)
{
    ewxWindowUtils::saveWindowSettings(this, MACHREGISTER, prefs, false);
}


void WxMachineRegister::restoreSettings(Preferences& prefs)
{
    if (prefs.isValid())
    {
        ewxWindowUtils::restoreWindowSettings(this, MACHREGISTER, prefs, false);
    }
}


//****  2005.0519
//  Initialize the form contents.
//  Replaces functionality of initialize(const char*) in configsvrs_cdlg.C
//  One important difference is that the messaging service calls now appear
//  in the WxMachineRegisterApp code.
void WxMachineRegister::initialize()
{
    int i;
    string s;

        //  Found a bug in DialogBlocks.  If the member variable name is set on
        //  a FlexGridSizer, and one or more of the columns is indicated to
        //  grow,the DialogBlocks generates the code for adding the growable
        //  column using the temporary identifier for the sizer, even though
        //  the temporary identifier is no longer declared within the
        //  CreateControls() method.  Manually correcting the identifier does
        //  not help; when the project is saved, it regenerates using the
        //  temporary identifier.  So, to get around this, I don't specify the
        //  growable column within the superclass; instead do it when I get into
        //  this method.


        ewxPanel *panel;
        wxFlexGridSizer *gridsizer;
        ewxStaticBoxSizer *boxsizer;

        //  Create and populate the "Applications" static box.
        panel = (ewxPanel *)(wxWindow::FindWindowById(ID_PANEL_WXMACHINEREGISTER_APPLICATIONS, this));
        boxsizer = new ewxStaticBoxSizer(wxHORIZONTAL, panel, _("Applications"));

        gridsizer = new wxFlexGridSizer(0, 2, 4, 4);
        gridsizer->AddGrowableCol(1);
        boxsizer->Add(gridsizer, 1, wxALIGN_CENTER_VERTICAL|wxALL, 8);

        for (i = 0; i < p_codeNames.size(); i++)
        {
            s = p_codeNames[i] + ":";

            ewxStaticText* lbl = new ewxStaticText(panel, wxID_STATIC, (wxString)(s), wxDefaultPosition, wxDefaultSize, 0);
            gridsizer->Add(lbl, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5);

            ewxTextCtrl* txt = new ewxTextCtrl(panel, (ID_TEXT_APPLICATIONS_ROOT + i), _T(""), wxDefaultPosition, wxDefaultSize, 0);
            gridsizer->Add(txt, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5);

            p_codePaths.push_back(txt);
        }

        panel->SetSizer(boxsizer);
        boxsizer->SetSizeHints(panel);

        //  Create and populate the "Misc. Paths" static box
        panel = (ewxPanel*)(wxWindow::FindWindowById(ID_PANEL_WXMACHINEREGISTER_MISCPATHS, this));
        boxsizer = new ewxStaticBoxSizer(wxHORIZONTAL, panel, _("Misc. Paths"));

        gridsizer = new wxFlexGridSizer(0, 2, 4, 4);
        gridsizer->AddGrowableCol(1);
        boxsizer->Add(gridsizer, 1, wxALIGN_CENTER_VERTICAL|wxALL, 8);

        ewxStaticText* lbl = new ewxStaticText(panel, wxID_STATIC, _("Perl 5:"), wxDefaultPosition, wxDefaultSize, 0);
        gridsizer->Add(lbl, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5);

        ewxTextCtrl* txt = new ewxTextCtrl(panel, wxID_ANY, _T(""), wxDefaultPosition, wxDefaultSize, 0);
        gridsizer->Add(txt, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5);

        p_miscPathsText.push_back(txt);

        if (p_adminFlag)
        {
            ewxStaticText* lbl  = new ewxStaticText(panel, wxID_STATIC, _("Queue Mgr:"), wxDefaultPosition, wxDefaultSize, 0);
            gridsizer->Add(lbl, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);

            ewxTextCtrl* txt = new ewxTextCtrl(panel, wxID_ANY, _T(""), wxDefaultPosition, wxDefaultSize, 0);
            gridsizer->Add(txt, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5);

            p_miscPathsText.push_back(txt);
        }

        panel->SetSizer(boxsizer);
        boxsizer->SetSizeHints(panel);

        //  The queues panel is shown to everyone.  It used to be gated on
        //  the admin flag, from the era when a site administrator configured
        //  queues once for a department; ECCE is per-user now, and that left
        //  no in-application way to describe a queue at all (#131).
        panel = (ewxPanel*)(this->FindWindowById(ID_PANEL_QUEUES));
        panel->Show(true);

        //  Find all other components on the form
        p_machinesList = (ewxListBox*)(this->FindWindowById(ID_LISTBOX_MACHINES));
        p_formScroll = (ewxScrolledWindow*)(this->FindWindowById(ID_SCROLLEDWINDOW_WXMACHINEREGISTER_FORM));

        p_machineFullNameText = (ewxTextCtrl*)(this->FindWindowById(ID_TEXT_MACHINE_FULLNAME));
        p_localityText = (ewxStaticText*)(this->FindWindowById(ID_STATIC_MACHINE_LOCALITY));
        p_machineRefNameText = (ewxTextCtrl*)(this->FindWindowById(ID_TEXT_MACHINE_REFNAME));
        p_machineVendorText = (ewxTextCtrl*)(this->FindWindowById(ID_TEXT_MACHINE_VENDOR));
        p_machineModelText = (ewxTextCtrl*)(this->FindWindowById(ID_TEXT_MACHINE_MODEL));
        p_machineProcessorText = (ewxTextCtrl*)(this->FindWindowById(ID_TEXT_MACHINE_PROC));
        p_queueNameText = (ewxTextCtrl*)(this->FindWindowById(ID_TEXT_QUEUE_NAME));

        p_machineNumProcsSpin = (ewxSpinCtrl*)(this->FindWindowById(ID_SPIN_MACHINE_NUMPROCS));
        p_machineNumNodesSpin = (ewxSpinCtrl*)(this->FindWindowById(ID_SPIN_MACHINE_NUMNODES));
        p_queueMinProcsSpin = (ewxSpinCtrl*)(this->FindWindowById(ID_SPIN_QUEUE_MINPROCS));
        p_queueMaxProcsSpin = (ewxSpinCtrl*)(this->FindWindowById(ID_SPIN_QUEUE_MAXPROCS));
        p_queueMaxWallSpin = (ewxSpinCtrl*)(this->FindWindowById(ID_SPIN_QUEUE_MAXWALL));
        p_queueMaxMemorySpin = (ewxSpinCtrl*)(this->FindWindowById(ID_SPIN_QUEUE_MAXMEMORY));
        p_queueMinScratchSpin = (ewxSpinCtrl*)(this->FindWindowById(ID_SPIN_QUEUE_MINSCRATCH));

        p_queueManagerChoicebox = (ewxChoice*)(this->FindWindowById(ID_CHOICEBOX_QUEUE_MANAGER));
        p_queuesChoicebox = (ewxChoice*)(this->FindWindowById(ID_CHOICEBOX_QUEUE));

        ewxCheckBox *checkbox;

        checkbox = (ewxCheckBox*)(this->FindWindowById(ID_CHECKBOX_REMSHELL_SSH));
        p_remshellsCheckboxes.push_back(checkbox);

        checkbox = (ewxCheckBox*)(this->FindWindowById(ID_CHECKBOX_REMSHELL_SSH_FTP));
        p_remshellsCheckboxes.push_back(checkbox);

        checkbox = (ewxCheckBox*)(this->FindWindowById(ID_CHECKBOX_REMSHELL_SSH_PASS));
        p_remshellsCheckboxes.push_back(checkbox);

        p_queueAllctnAcctsCheckbox = (ewxCheckBox*)(wxWindow::FindWindowById(ID_CHECKBOX_QUEUE_ALLOCATION));

        p_queueAcceptButton = (ewxButton*)(this->FindWindowById( ID_BUTTON_QUEUE_CHANGE));
        p_queueRemoveButton = (ewxButton*)(this->FindWindowById( ID_BUTTON_QUEUE_REMOVE));
        p_queueClearButton = (ewxButton*)(this->FindWindowById( ID_BUTTON_QUEUE_CLEAR));
        p_machineChangeButton = (ewxButton*)(this->FindWindowById( ID_BUTTON_MACHINE_CHANGE));
        p_machineDeleteButton = (ewxButton*)(this->FindWindowById( ID_BUTTON_MACHINE_DELETE));
        p_formClearButton = (ewxButton*)(this->FindWindowById( ID_BUTTON_FORM_CLEAR));
        p_formCloseButton = (ewxButton*)(this->FindWindowById( ID_BUTTON_FORM_CLOSE));
        p_helpButton = (ewxButton*)(this->FindWindowById( ID_BUTTON_HELP));

        //  The whole form (everything above) was built inside p_formScroll
        //  (#187) -- give it its scrollbar and its virtual (content) size
        //  now that all of it exists.
        if (p_formScroll != NULL)
        {
            p_formScroll->SetScrollRate(0, 10);
            p_formScroll->FitInside();
        }

        this->InvalidateBestSize();
        this->GetSizer()->SetSizeHints(this);
        this->GetSizer()->Fit(this);

        //  Open showing the whole form, capped to the display (#187).
        this->growToFitSizer(true);

        //  ewxFrame::Create()'s Centre() ran on the small, pre-content
        //  window (Applications/Misc Paths/Queues are empty placeholders
        //  until the loop above fills them in) -- so the position it
        //  chose can leave a since-grown, now display-capped frame
        //  hanging off the bottom of the screen even though its SIZE is
        //  correct. Only done here, once, at construction: a later grow
        //  (the locality note) must not relocate a window the user may
        //  already have moved.
        this->keepOnScreen();
}

void WxMachineRegister::mainWindowCloseCB(wxCloseEvent& event)
{
    //  Confirm exit only if admin flag is set.  Among other things,
    //  it indicates that the app was started standalone.

    //  Queue edits live only in p_qnames until Add/Change runs
    //  processmachine; closing used to drop them without a word (#131).
    if (event.CanVeto() && p_queuesDirty)
    {
        int answer = this->confirmUnsavedQueues();
        if (answer == wxID_CANCEL || (answer == wxID_YES && !saveRegistration()))
        {
            event.Veto();
            return;
        }
    }

    if (event.CanVeto() && p_adminFlag && !this->confirmExit())
    {
        // Veto allowed, and exit not confirmed by user.
        event.Veto();     //  Veto the close event.
    }
    else
    {
        Preferences prefs("MachineRegister");
        saveSettings(prefs);

        //  Either veto disallowed by forced close, or exit confirmed by
        //  user.  Destroying window.
        this->Destroy();
    }
}


bool WxMachineRegister::hasMinimalInput()
{
    return (((p_machineFullNameText->GetValue()).Length() > 0)
                && ((p_machineRefNameText->GetValue()).Length() > 0));
}


void WxMachineRegister::machinesListBoxSelectedCB(wxCommandEvent& event)
{
    if (event.IsSelection())
    {
        string refname = (string)(event.GetString());

        if (refname != "" && p_queuesDirty && p_slctRgstn != NULL &&
            refname != p_slctRgstn->refname())
        {
            //  selectMachine() reloads p_qnames from disk, which would
            //  silently discard the previous machine's unsaved queues.
            string prev = p_slctRgstn->refname();
            int answer = this->confirmUnsavedQueues();
            if (answer == wxID_CANCEL ||
                (answer == wxID_YES && !saveRegistration()))
            {
                p_machinesList->SetSelection(
                    p_machinesList->FindString((wxString)prev, true));
                return;
            }
            p_queuesDirty = false;
        }

        if (refname != "")
        {
            this->selectMachine(refname);
            p_machineDeleteButton->Enable(true);
            p_formClearButton->Enable(true);
        }
    }
}

void WxMachineRegister::machineFullNameUpdatedCB(wxCommandEvent& event)
{
    string refName = (string)(p_machineFullNameText->GetValue());
    int chpos = refName.find('.');

    //  Don't cut an IP address at its first '.' -- 127.0.0.1 must stay
    //  127.0.0.1, not become "127" (matches RunMgmt::registerLocalMachine).
    bool isAddress = refName.find_first_not_of("0123456789.") == string::npos;

    if (chpos != string::npos && !isAddress)
        refName = refName.substr(0, chpos);

    //  Only when the user types a machine: loading a saved entry must keep
    //  its own name, or Delete looks for a name that was never saved.
    //  Follow the machine only while Name is empty or still the value
    //  filled in here, so a name the user typed is never overwritten.
    if (!p_inCtrlUpdate) {
        string current = (string)(p_machineRefNameText->GetValue());
        if (current.empty() || current == p_autoRefName) {
            p_autoRefName = refName;
            p_machineRefNameText->SetValue(refName);
        }
        p_machineChangeButton->Enable(true);
    }

    this->refreshLocality();
}


/**
 *  A machine registered as localhost, 127.0.0.1, ::1 or this host's own
 *  name runs jobs LOCALLY when the login name used for it is empty or
 *  your own, and over ssh to that name when it is anyone else's -- the
 *  escape hatch a port-forwarded cluster login depends on (#144). Nothing
 *  said which, so say it here, where the name is typed. The login name
 *  itself is chosen in the Launcher, not here: show the rule, and the
 *  outcome too when a login name has already been saved for the machine.
 *  Both come from RCommand::isRemote(), the function the launch uses.
 */
void WxMachineRegister::refreshLocality()
{
    if (p_localityText == NULL || p_machineFullNameText == NULL)
        return;

    string machine = (string)p_machineFullNameText->GetValue();
    string text = "";

    if (!RCommand::localityNote(machine, "ssh", "").empty())
    {
        text = "This computer: jobs run locally when the login name is empty or ";
        text += Ecce::realUser();
        text += ";\nany other login name goes via ssh to " + machine +
                " as that user.";

        string refName = (string)p_machineRefNameText->GetValue();
        MachinePreferences *prefs = refName.empty() ? NULL :
                                    MachinePreferences::lookup(refName);
        if (prefs != NULL && prefs->isOptionSupported("UN") &&
            !prefs->getUsername().empty())
        {
            text += "\nWith the saved login name \"" + prefs->getUsername() +
                    "\": " + RCommand::localityNote(machine,
                                                    prefs->getRemoteShell(),
                                                    prefs->getUsername()) + ".";
        }
    }

    if ((string)p_localityText->GetLabel() != text)
    {
        p_localityText->SetLabel(text);
        p_localityText->Show(!text.empty());

        //  The note changes the form's natural size; re-measure it so the
        //  scrolled form and the frame's minimum follow (#187).
        this->growToFitSizer();
    }
}


/**
 *  #187: keep the frame's minimum in step with the form and, with
 *  fitWholeForm, grow the frame to show all of it, capped to the display.
 *  The scrolled form is the one flexible item and its minimum is only a
 *  small floor, so the frame's minimum always includes the whole button
 *  row: any resize takes height from the form, never from the buttons.
 */
void WxMachineRegister::growToFitSizer(bool fitWholeForm)
{
    if (this->GetSizer() == NULL || p_formScroll == NULL
        || p_formScroll->GetSizer() == NULL)
        return;

    //  Smallest useful slice of the form; below this it scrolls in a
    //  window too small to read.
    const int MIN_FORM_HEIGHT = 100;

    wxSize cap = this->maxClientSizeForDisplay();
    wxSize have = this->GetClientSize();

    //  A scrolled window's best size is not its content's, so measure the
    //  form's sizer. Re-measured every call: the locality note (#144)
    //  changes it.
    wxSize natural = p_formScroll->GetSizer()->GetMinSize();

    //  The frame size that shows the whole form without scrolling.
    p_formScroll->SetMinSize(natural);
    wxSize full = this->GetSizer()->GetMinSize();

    //  The real minimum: the width stays natural (no horizontal scroll),
    //  the height may shrink to the floor. Applied as the frame's own
    //  minimum too, so no resize can cut into the button row.
    p_formScroll->SetMinSize(wxSize(natural.x,
                                    wxMin(natural.y, MIN_FORM_HEIGHT)));
    p_formScroll->FitInside();
    wxSize need = this->GetSizer()->GetMinSize();
    this->SetMinClientSize(need);

    //  Only at construction: later (the locality note) the form just
    //  scrolls, so a frame the user shrank stays that size.
    wxSize target = fitWholeForm ? full : need;
    wxSize want(wxMax(target.x, have.x), wxMax(target.y, have.y));
    want.x = wxMax(need.x, wxMin(want.x, cap.x));
    want.y = wxMax(need.y, wxMin(want.y, cap.y));

    //  Applied only now that the target is known, so it cannot clamp the
    //  SetClientSize() below; it widens past the display only if `need`
    //  itself does.
    wxSize decoration = this->GetSize() - this->GetClientSize();
    this->SetMaxSize(wxSize(wxMax(want.x, cap.x) + decoration.x,
                            wxMax(want.y, cap.y) + decoration.y));

    if (want != have)
        this->SetClientSize(want);

    this->Layout();

    if (getenv("ECCE_DEBUG_MACHREGISTER_SIZE") != NULL)
    {
        wxSize formSize = p_formScroll->GetSize();
        wxSize formVirt = p_formScroll->GetVirtualSize();
        fprintf(stderr,
                "[MACHREG_SIZE] cap=%dx%d need=%dx%d full=%dx%d frameClient=%dx%d "
                "frameOuter=%dx%d formSize=%dx%d formVirtual=%dx%d\n",
                cap.x, cap.y, need.x, need.y, full.x, full.y,
                this->GetClientSize().x, this->GetClientSize().y,
                this->GetSize().x, this->GetSize().y,
                formSize.x, formSize.y, formVirt.x, formVirt.y);
    }
}


/**
 *  Nudge the frame back fully onto its display (#187) -- falls back to
 *  display 0 when the frame isn't associated with one yet (e.g. before
 *  the first Show()). Only moves it, never resizes it.
 */
void WxMachineRegister::keepOnScreen()
{
    int dpyIdx = wxDisplay::GetFromWindow(this);
    wxDisplay display((unsigned)(dpyIdx == wxNOT_FOUND ? 0 : dpyIdx));
    wxRect avail = display.GetClientArea();

    wxPoint pos = this->GetPosition();
    wxSize size = this->GetSize();

    int x = pos.x, y = pos.y;
    if (x + size.x > avail.x + avail.width)
        x = avail.x + avail.width - size.x;
    if (y + size.y > avail.y + avail.height)
        y = avail.y + avail.height - size.y;
    if (x < avail.x)
        x = avail.x;
    if (y < avail.y)
        y = avail.y;

    if (x != pos.x || y != pos.y)
        this->SetPosition(wxPoint(x, y));
}


/**
 *  The most this frame's client area can be without the window (frame,
 *  borders and all) exceeding the usable area of the display it's on
 *  (#187) -- falls back to display 0 when the frame isn't associated
 *  with one yet (e.g. before the first Show()).
 */
wxSize WxMachineRegister::maxClientSizeForDisplay()
{
    int dpyIdx = wxDisplay::GetFromWindow(this);
    wxDisplay display((unsigned)(dpyIdx == wxNOT_FOUND ? 0 : dpyIdx));
    wxRect avail = display.GetClientArea();

    //  A pure query -- growToFitSizer() is the one that applies a max
    //  size, once it knows the final target, so this can't clamp a
    //  SetClientSize() out from under it (see the comment there).
    wxSize decoration = this->GetSize() - this->GetClientSize();
    wxSize cap(avail.width - decoration.x, avail.height - decoration.y);
    if (cap.x <= 0)
        cap.x = avail.width;
    if (cap.y <= 0)
        cap.y = avail.height;

    return cap;
}


void WxMachineRegister::machineRefNameUpdatedCB(wxCommandEvent& event)
{
    p_machineChangeButton->Enable(true);
}


void WxMachineRegister::machineVendorTextEnterCB(wxCommandEvent& event)
{

    bool emslFlag = false;
    string currPath;
    string vendor = (string)(p_machineVendorText->GetValue());

    STLUtil::toUpper(vendor);

    if ((vendor.find("SGI") == 0) || (vendor.find("LINUX") == 0))
    {
        wxString fullname = p_machineFullNameText->GetValue();

        if (fullname.find('.') != string::npos &&
            (fullname.find(".emsl.pnl.gov") == fullname.find('.') ||
             fullname.find(".pnl.gov") == fullname.find('.')))
        {
            emslFlag = true;
            string base = Ecce::ecceHome();
            base += "/siteconfig/";
            string path;

            // ignore any model specific CONFIG files and assume
            // vendor CONFIG files always exist for the same model

            bool anyCode = false;

            for (int idx = 0; idx < p_codeNames.size(); idx++)
            {
                if (RefMachine::exePath(p_codeNames[idx].c_str(), fullname.ToStdString(), vendor.c_str()) != "")
                {
                    anyCode = true;
                    p_codePaths[idx]->SetValue(_("(EMSL default path)"));
                    p_miscPathsText[0]->SetValue(_("(EMSL default perl path)"));
                }
                else
                {
                    currPath = (string)(p_codePaths[idx]->GetValue());

                    if (currPath == "(EMSL default path)")
                        p_codePaths[idx]->Clear();
                }
            }

            if (!anyCode)
            {
                currPath = p_miscPathsText[0]->GetValue();

                if (currPath == "(EMSL default perl path)")
                {
                    p_miscPathsText[0]->Clear();
                }
            }
        }
    }

    if (!emslFlag)
    {
        for (int idx = 0; idx < p_codePaths.size(); idx++)
        {
            currPath = (string)(p_codePaths[idx]->GetValue());

            if (currPath == "(EMSL default path)")
            {
                p_codePaths[idx]->Clear();
            }
        }

        currPath = (string)(p_miscPathsText[0]->GetValue());

        if (currPath == "(EMSL default perl path)")
        {
            p_miscPathsText[0]->Clear();
        }
    }
}


void WxMachineRegister::queueChoiceboxSelectedCB(wxCommandEvent& event)
{

//    cout << "WxMachineRegister::queueChoiceboxSelectedCB(wxCommandEvent&)" << endl;

    wxString slctQueue = p_queueNameText->GetValue();

    if (event.GetString() != slctQueue)
    {
        this->showQueue((string)(event.GetString()));
    }
}


void WxMachineRegister::queueChangeButtonClickedCB(wxCommandEvent& event)
{
    string name =(string)(p_queueNameText->GetValue());
    STLUtil::stripLeadingAndTrailingWhiteSpace(name);

    if (name.empty())
    {
        displayMessage("You must supply a queue name.");
    }
    else if (!std::regex_match(name, std::regex("^[A-Za-z0-9_.-]+$")))
    {
        //  <machine>.Q separates queue names with spaces and keys with
        //  '|', so a name cannot hold either (#131); processmachine
        //  refuses the same set.
        displayMessage("Queue name '" + name + "' is not valid.\n"
            "Queue names may contain only letters, digits, '_', '.' and '-'.");
    }
    else
    {
        int it;
        for (it=0; it<p_qnames.size() && p_qnames[it]!=name; it++);

        if (it < p_qnames.size())
        {
            // this is a modify
            p_minProcs[it] = p_queueMinProcsSpin->GetValue();
            p_maxProcs[it] = p_queueMaxProcsSpin->GetValue();
            p_maxWall[it] = p_queueMaxWallSpin->GetValue();
            p_maxMem[it] = MemoryUnits::gbToMB(p_queueMaxMemorySpin->GetValue());
            p_minScratch[it] = p_queueMinScratchSpin->GetValue();
        }
        else
        {
            // this is an add
            p_qnames.push_back(name);

            p_minProcs.push_back(p_queueMinProcsSpin->GetValue());
            p_maxProcs.push_back(p_queueMaxProcsSpin->GetValue());
            p_maxWall.push_back(p_queueMaxWallSpin->GetValue());
            p_maxMem.push_back(MemoryUnits::gbToMB(p_queueMaxMemorySpin->GetValue()));
            p_minScratch.push_back(p_queueMinScratchSpin->GetValue());

            p_queuesChoicebox->Append(_(name.c_str()));
        }
        p_queuesDirty = true;
    }
}


void WxMachineRegister::queueRemoveButtonClickedCB(wxCommandEvent& event)
{
    this->removeQueue();
    p_queuesDirty = true;
}


void WxMachineRegister::queueClearButtonClickedCB(wxCommandEvent& event)
{
    this->clearQueues();
    p_queuesDirty = true;
}


void WxMachineRegister::machineChangeButtonClickedCB(wxCommandEvent& event)
{
    saveRegistration();
}


bool WxMachineRegister::saveRegistration()
{
    bool saved = false;

    if (verifyInput())
    {
        string settings = "type=accept";
        settings += collectSettings();

        int status = ProcessMachine::run(settings);

        if (status != 0)
            displayMessage("Unable to save changes to machine registration!");
        else
        {
            saved = true;
            p_queuesDirty = false;
            string refName = (string)(p_machineRefNameText->GetValue());
            redo(refName);
            selectMachine(refName);

            this->notifyUpdate();
        }
    }

    return saved;
}


//  Yes = save now, No = discard, Cancel = stay.
int WxMachineRegister::confirmUnsavedQueues()
{
    string name = (string)(p_machineRefNameText->GetValue());
    ewxMessageDialog dlg(this,
        "The queues for '" + name + "' have been changed but not saved.\n"
        "\"Add/Change Queue\" only edits the list; \"Add/Change\" writes it.\n\n"
        "Save them now?",
        "Unsaved Queue Changes",
        wxYES_NO|wxCANCEL|wxICON_QUESTION, wxDefaultPosition);
    return dlg.ShowModal();
}


void WxMachineRegister::machineDeleteButtonClickedCB(wxCommandEvent& event)
{
    string refName = p_slctRgstn->refname();

    if (confirmRemove(refName))
    {
        int result = this->removeMachine(refName);

        if (result != 0)
            displayMessage("Unable to delete registered machine!");
        else
        {
            redo(refName);
            this->clearForm();
            p_machinesList->SetSelection(-1);
            p_slctRgstn = NULL;
//            p_machineChangeButton->Enable(false);
//            p_machineDeleteButton->Enable(false);
            this->notifyUpdate();

        }
    }
}


void WxMachineRegister::formClearButtonClickedCB(wxCommandEvent& event)
{

    p_machinesList->SetSelection(-1);
    p_slctRgstn = NULL;

    this->clearForm();
    p_queuesDirty = false;   // an explicit Clear Form is a deliberate discard


//    p_machineChangeButton->Enable(false);
//    p_machineDeleteButton->Enable(false);

}


void WxMachineRegister::formCloseButtonClickedCB(wxCommandEvent& event)
{
    this->Close();
}


void WxMachineRegister::helpButtonClickedCB(wxCommandEvent& event)
{
   BrowserHelp help;
   help.showPage(help.URL("ConfigSvrs"));
}


bool WxMachineRegister::selectMachine(string refName)
{
    bool found;

    //  Case-sensitive: wx matches list strings ignoring case, but machine
    //  names are case-sensitive, so after deleting "tellurium" it selected
    //  "Tellurium" while refLookup("tellurium") returned NULL.
    int idx = p_machinesList->FindString((wxString)(refName), true);
    found = idx != wxNOT_FOUND;
    if (found)
        p_machinesList->SetSelection(idx);
    p_prefillFromSite = false;

    if (found)
    {
        p_slctRgstn = RefMachine::refLookup(refName.c_str());
        found = p_slctRgstn != NULL;
        if (found)
            this->refreshControls();
    }
    else if (!p_adminFlag)
    {
        //  A site machine (e.g. the Machine Browser's Register action on
        //  "dummy"/"localhost") never appears in the user's own list, so
        //  the lookup above always misses.  Fall back to the site
        //  definition, unselected, so the form can be edited and saved as
        //  the user's own shadowing copy (#104).
        vector<string> *siteNames = RefMachine::referenceNames(RefMachine::siteMachines);
        bool isSiteMachine = (find(siteNames->begin(), siteNames->end(), refName)
                               != siteNames->end());
        delete siteNames;

        if (isSiteMachine)
        {
            p_machinesList->SetSelection(-1);
            p_slctRgstn = RefMachine::refLookup(refName.c_str());
            p_prefillFromSite = true;
            this->refreshControls();
            //  Nothing of the user's to delete yet.
            p_machineDeleteButton->Enable(false);

            displayMessage("'" + refName + "' is a site machine, shared by "
                "everyone using this ECCE installation, so it can't be "
                "changed here. The form now shows its settings: edit them "
                "and press Add/Change to save your own copy under the same "
                "name. Your copy is used instead of the site one; deleting "
                "it brings the site version back.");

            found = true;
        }
    }

    return found;
}


void WxMachineRegister::loadMachinesList()
{
    int i, nNames;
    RefMachine::machineContextEnum context;

    p_machinesList->Clear();


    context = p_adminFlag ? RefMachine::siteMachines : RefMachine::userMachines;
    vector<string> *names = RefMachine::referenceNames(context);
    EcceSortedVector<string, less<string> > sortedNames(*names);

    nNames= sortedNames.size();

    for (i = 0; i < nNames; i++)
    {
        p_machinesList->Append((wxString)(sortedNames[i]));
    }

    if (nNames > 0)
    {
        p_machinesList->SetSelection(0);
        p_prefillFromSite = false;
        string refName = (string)(p_machinesList->GetString(0));
        p_slctRgstn = RefMachine::refLookup(refName.c_str());
        this->refreshControls();
    }
}


//****  2005.0518
//  Load up list of supported queue managers from file.
//  Replaces functionality of initQueueMgrList() in configsvrs_cdlg.C
void WxMachineRegister::loadQueueManagerList()
{
    bool found = false;

    string path = Ecce::ecceHome();
    path.append("/siteconfig/QueueManagers");

    KeyValueReader reader(path.c_str());
    string key, value;

    p_queueManagerChoicebox->Clear();
    p_queueManagerChoicebox->Append("None");

    while (!found && (reader.getpair(key, value)))
    {
        if (key == "QueueManagers")
        {
            found = true;

            char *tokestr = strdup(value.c_str());
            char *tok = strtok(tokestr, " \t");

            while (tok)
            {
                p_queueManagerChoicebox->Append(tok);
                tok = strtok((char*)0, " \t");
            }

            free(tokestr);
        }
    }


    p_queueManagerChoicebox->SetSelection(0);
}


void WxMachineRegister::refreshControls()
{
    p_inCtrlUpdate = true;

    p_machineRefNameText->SetValue(p_slctRgstn->refname());

    p_machineFullNameText->SetValue(p_slctRgstn->fullname());
    p_machineVendorText->SetValue(p_slctRgstn->vendor());
    p_machineModelText->SetValue(p_slctRgstn->model());
    p_machineProcessorText->SetValue(p_slctRgstn->proctype());

    int procs = p_slctRgstn->proccount();
    int nodes = p_slctRgstn->nodes();


    p_machineNumProcsSpin->SetValue(procs);
    p_machineNumNodesSpin->SetValue((nodes > 0) ? nodes : 1);


    int nshells;

    nshells = p_shellNames.size();

    for (int i = 0; i < nshells; i++)
    {
        p_remshellsCheckboxes[i]->SetValue(p_slctRgstn->hasRemShell(p_shellNames[i]));
    }

    p_queueAllctnAcctsCheckbox->SetValue(p_slctRgstn->launchOptions().find("AA") != string::npos);
    loadConfig(p_slctRgstn->refname());

    string tmp;
    string fullName = p_slctRgstn->fullname();
    string vendor = p_slctRgstn->vendor();
    STLUtil::toLower(vendor);

    bool emslflag = false;

    if ((vendor.find("sgi") == 0 || vendor.find("linux") == 0) &&
        fullName.find('.') != string::npos &&
        (fullName.find(".emsl.pnl.gov") == fullName.find('.') ||
         fullName.find(".pnl.gov") == fullName.find('.')))
    {
        string base = Ecce::ecceHome();
        base += "/siteconfig/";
        string path;
        path = base + "CONFIG.SGI";
        SFile test1 = path;
        path = base + "CONFIG.LINUX";
        SFile test2 = path;

        emslflag = test1.exists() && test2.exists();
    }


    string xname, lname;

    for (int i = 0; i < p_codePaths.size(); i++)
    {
        lname = xname = p_codeNames[i];
        STLUtil::toLower(lname);

        tmp = "";
        p_config->findValue(lname.c_str(), tmp);

        if ((tmp == "") && emslflag && p_slctRgstn->hasCode(xname.c_str()))
            tmp = "(EMSL default path)";

        p_codePaths[i]->SetValue((wxString)(tmp));
    }

    tmp = "";
    p_config->findValue("perlpath", tmp);

    if ((tmp == "") && emslflag)
    {
        tmp = "(EMSL default perl path)";
    }

    p_miscPathsText[0]->SetValue(_(tmp.c_str()));


    //  The queue manager PATH is genuinely admin-only -- that text
    //  field only exists in an admin invocation, which is why the
    //  size() guard in getSettings() is there too.
    if (p_adminFlag)
    {
        tmp = "";
        p_config->findValue("qmgrpath",tmp);
        p_miscPathsText[1]->SetValue((wxString)(tmp));
    }

    //  The QUEUES themselves load for everyone.  They used to be inside
    //  the admin branch above, which was harmless while the whole tab
    //  was hidden and became data loss the moment it was shown (#131):
    //  the tab came up empty however many queues the machine had, and
    //  saving then wrote that empty list back over them.
    loadQueues(p_slctRgstn->refname());
    fillQueues();
    showQueue();  // default to first if any exist


    p_formClearButton->Enable(true);
    p_machineChangeButton->Enable(true);
    p_machineDeleteButton->Enable(true);

    p_inCtrlUpdate = false;
}


void WxMachineRegister::clearForm()
{
    p_inCtrlUpdate = true;

    p_machineRefNameText->Clear();
    p_machineFullNameText->Clear();
    p_machineVendorText->Clear();
    p_machineModelText->Clear();
    p_machineProcessorText->Clear();

    int i, n;

    n = p_codePaths.size();

    for (i = 0; i < n; i++)
    {
        p_codePaths[i]->Clear();
    }

    n = p_miscPathsText.size();

    for (i = 0; i < n; i++)
    {
        p_miscPathsText[i]->Clear();
    }

    n = p_remshellsCheckboxes.size();

    for (i = 0; i < n; i++)
    {
        p_remshellsCheckboxes[i]->SetValue(false);
    }

    p_machineNumProcsSpin->SetValue(1);
    p_machineNumNodesSpin->SetValue(1);

    this->clearQueues();

    p_machineChangeButton->Enable(false);
    p_machineDeleteButton->Enable(false);
    p_formClearButton->Enable(false);

    p_inCtrlUpdate = false;
}


void WxMachineRegister::removeQueue()
{
    int k;

    string name = (string)(p_queueNameText->GetValue());
    STLUtil::stripLeadingAndTrailingWhiteSpace(name);

    if (name.empty())
    {
        displayMessage("You must supply a queue name.");
    }
    else
    {
        k = p_queuesChoicebox->GetSelection();
        string current = (string)(p_queuesChoicebox->GetString(k));

        int pos;
        for (pos=0; pos<p_qnames.size() && p_qnames[pos]!=name; pos++);

        if (pos < p_qnames.size())
        {
            int it;

            vector<string>::iterator sit = p_qnames.begin();
            for (it=0; it<pos; it++, sit++);
            if (it < p_qnames.size())
            {
                p_qnames.erase(sit);
            }

            vector<int>::iterator iit = p_minProcs.begin();
            for (it=0; it<pos; it++, iit++);
            if (it < p_minProcs.size())
            {
                p_minProcs.erase(iit);
            }

            iit = p_maxProcs.begin();
            for (it=0; it<pos; it++, iit++);
            if (it < p_maxProcs.size())
            {
                p_maxProcs.erase(iit);
            }

            iit = p_maxWall.begin();
            for (it=0; it<pos; it++, iit++);
            if (it < p_maxWall.size())
            {
                p_maxWall.erase(iit);
            }

            iit = p_maxMem.begin();
            for (it=0; it<pos; it++, iit++);
            if (it < p_maxMem.size())
            {
                p_maxMem.erase(iit);
            }

            iit = p_minScratch.begin();
            for (it=0; it<pos; it++, iit++);
            if (it < p_minScratch.size())
            {
                p_minScratch.erase(iit);
            }

            p_queuesChoicebox->Delete(k);
        }

        if (current == name)
        {
            showQueue();  // first
        }
    }
}


void WxMachineRegister::clearQueues()
{
    p_queueManagerChoicebox->SetSelection(0);
    p_queueAllctnAcctsCheckbox->SetValue(false);

    p_qnames.clear();
    p_minProcs.clear();
    p_maxProcs.clear();
    p_maxWall.clear();
    p_maxMem.clear();
    p_minScratch.clear();

    p_queuesChoicebox->Clear();
    p_queueMinProcsSpin->SetValue(1);
    p_queueMaxProcsSpin->SetValue(1);
    p_queueMaxWallSpin->SetValue(0);
    p_queueMaxMemorySpin->SetValue(0);
    p_queueMinScratchSpin->SetValue(0);
    p_queueNameText->Clear();
}


void WxMachineRegister::redo(string refName)
{
    this->reset();
    this->loadMachinesList();
    this->selectMachine(refName);
}


void WxMachineRegister::reset()
{
    RefMachine::finalize();

    //  Unconditional, for the same reason.  QueueManager caches its
    //  whole extent on first use, so without dropping it here a queue
    //  just written to disk is not visible again until the application
    //  is restarted.  That only ever mattered in admin mode before.
    QueueManager::finalize();

    p_machinesList->Clear();
}


vector<string> WxMachineRegister::getConfigFileNames(string refName) const
{
    vector<string> ret;
    string base;

    if (p_adminFlag)
    {
        base = Ecce::ecceHome();
        base += "/siteconfig";
    }
    else
    {
        base = Ecce::realUserPrefPath();
    }

    string siteBase = Ecce::ecceHome();
    siteBase += "/siteconfig";


    string path;
    SFile teste;

    // The main site default file
    path = base + "/submit.site";
    teste = path;

    if (teste.exists())
    {
        ret.push_back(path);
    }
    else if (p_prefillFromSite)
    {
        // Not overridden by the user -- fall back to the site copy so a
        // site machine's settings still pre-fill the form (#104).
        path = siteBase + "/submit.site";
        teste = path;

        if (teste.exists())
            ret.push_back(path);
    }

    // This is like CONFIG.columbo
    path = base + "/CONFIG." + refName;
    teste = path;

    if (teste.exists())
    {
        ret.push_back(path);
    }
    else if (p_prefillFromSite)
    {
        // A site machine (never a user machine) has no CONFIG.<name> in
        // the user's own prefs -- read the site one instead (#104).
        path = siteBase + "/CONFIG." + refName;
        teste = path;

        if (teste.exists())
            ret.push_back(path);
    }

    return ret;
}

//*****  2005.0523
//  Implements functionality of ConfigSvrs::checkForm(void)
bool WxMachineRegister::verifyInput()
{
    bool result = true;

    string refName = (string)(p_machineRefNameText->GetValue());
    string fullName = (string)(p_machineFullNameText->GetValue());

    STLUtil::stripLeadingAndTrailingWhiteSpace(refName);
    STLUtil::stripLeadingAndTrailingWhiteSpace(fullName);

    int k = p_queueManagerChoicebox->GetSelection();

    if ((refName == "") || (fullName == ""))
    {
        result = false;
        displayMessage("Both Machine and Name must be specified.");
    }
    else if (refName.find_first_of("/\t\r\n") != string::npos)
    {
        //  The name is part of file names and a tab-separated line.
        result = false;
        displayMessage("Name may not contain '/', tabs or line breaks.");
    }
    else if (k == 0 && p_qnames.size() > 0)
    {
        result = false;
        displayMessage("Queues have been specified but not the Queue Manager that is used.\n"
            "Please set the Queue Manager or delete the queues.");
    }
    else if (k > 0 && p_qnames.size() == 0)
    {
        result = false;
        displayMessage("You have set a Queue Manager but no queues.\n"
            "Please specify one or more queues or set the Queue Manager to None.");
    }
    else
    {
        bool found;
        int i, n;

        i = 0;
        n = p_remshellsCheckboxes.size();
        found = false;

        while ((i < n) && !found)
        {
            found = p_remshellsCheckboxes[i]->IsChecked();
            i++;
        }

        if (!found)
        {
            result = false;
            displayMessage("Please specify at least one remote shell.");
        }
    }

    return result;
}


void WxMachineRegister::displayMessage(string mesg)
{
    ewxMessageDialog* dlgMesg = new ewxMessageDialog(this, (wxString)mesg, "Alert", wxOK);
    dlgMesg->ShowModal();

    delete dlgMesg;
}


string WxMachineRegister::collectSettings() const
{
    typedef ProcessMachine PM;
    int i, n;
    string ret;
    string tmp;

    ret += PM::field("siteconfig", StringConverter::toString(p_adminFlag));
    ret += PM::field("machine", (string)p_machineFullNameText->GetValue());
    ret += PM::field("name", (string)p_machineRefNameText->GetValue());

    tmp = p_machineVendorText->GetValue();
    ret += PM::field("vendor", (tmp == "") ? "Unspecified" : tmp);
    tmp = p_machineModelText->GetValue();
    ret += PM::field("model", (tmp == "") ? "Unspecified" : tmp);
    tmp = p_machineProcessorText->GetValue();
    ret += PM::field("processor", (tmp == "") ? "Unspecified" : tmp);

    ret += PM::field("procs", StringConverter::toString(p_machineNumProcsSpin->GetValue()));
    ret += PM::field("nodes", StringConverter::toString(p_machineNumNodesSpin->GetValue()));

    ret += PM::field("ssh", StringConverter::toString(p_remshellsCheckboxes[0]->IsChecked()));
    ret += PM::field("sshftp", StringConverter::toString(p_remshellsCheckboxes[1]->IsChecked()));
    ret += PM::field("sshpass", StringConverter::toString(p_remshellsCheckboxes[2]->IsChecked()));

    // Applications - first the list of all known codes, then one by one
    // the path for each code.
    tmp = "";
    for (i = 0; i < p_codePaths.size(); i++)
    {
        if (i > 0)
            tmp += ",";
        tmp += p_codeNames[i];
    }
    ret += PM::field("registeredcodes", tmp);

    for (i = 0; i < p_codePaths.size(); i++)
    {
        tmp = p_codePaths[i]->GetValue();
        if (tmp.find("EMSL default") != string::npos)
            tmp = "EMSL";
        ret += PM::field(p_codeNames[i], tmp);
    }

    // Other paths
    tmp = (string)(p_miscPathsText[0]->GetValue());
    if (tmp.find("EMSL default") != string::npos)
        tmp = "EMSL";
    ret += PM::field("perlPath", tmp);

    // Only an "-admin" invocation will have a queue manager path
    tmp = "";
    if (p_miscPathsText.size() > 1)
        tmp = p_miscPathsText[1]->GetValue();
    ret += PM::field("qmgrPath", tmp);

    // Queue related stuff
    ret += PM::field("AA", StringConverter::toString(p_queueAllctnAcctsCheckbox->IsChecked()));
    ret += PM::field("qmgr", (string)p_queueManagerChoicebox->GetStringSelection());

    n = p_qnames.size();
    ret += PM::field("numQueues", StringConverter::toString(n));

    for (i = 0; i < n; i++)
    {
        tmp = "name|" + p_qnames[i] + ",";
        tmp += "minNodes|" + StringConverter::toString(p_minProcs[i]) + ",";
        tmp += "maxNodes|" + StringConverter::toString(p_maxProcs[i]) + ",";
        tmp += "maxCPU|" + StringConverter::toString(p_maxWall[i]) + ",";
        tmp += "maxMemory|" + StringConverter::toString(p_maxMem[i]) + ",";
        tmp += "minScratch|" + StringConverter::toString(p_minScratch[i]) + ",";
        ret += PM::field("q" + StringConverter::toString(i), tmp);
    }

    return ret;
}


void WxMachineRegister::loadConfig(string refName)
{


    int i, numFiles;
    int j, numChars;
    string line, key, value;
    char buf[MAXLINE];  // reading lines

    p_config->clear();

    vector<string> files = getConfigFileNames(refName);
    numFiles = files.size();

    for (i = 0; i < numFiles; i++)
    {
        ifstream is(files[i].c_str());

        while (!is.eof())
        {
            key = value = "";
            is.getline(buf, MAXLINE ,'\n');
            line = buf;
            STLUtil::stripLeadingWhiteSpace(line);

            if (!line.empty() && (line[0] != '#') && (line.find("//") != 0))
            {
                if (line.find(":") != string::npos)
                {
                    StringTokenizer tokens(line);
                    key = tokens.next(":");
                    value = tokens.next(": \t");
                }
                else if ((line.find("{") != string::npos) && (line.find("}") != string::npos))
                {
                    StringTokenizer tokens(line);
                    key = tokens.next("{");
                    STLUtil::stripLeadingAndTrailingWhiteSpace(key);
                    value = tokens.next("{");
                    numChars = value.length();

                    for (j = 0; j < numChars; j++)
                    {
                        if (value[j] == '{' || value[j] == '}')
                        {
                            value[j] = ' ';
                        }
                    }
                }
                else if (line.find("{") != string::npos)
                {
                    StringTokenizer tokens(line);
                    key = tokens.next("{ ");

                    //  Two faults here, both silent.  The condition parsed
                    //  as (!line.find("}")) != npos, which is always true, so
                    //  this ran to end of file and swallowed the rest of the
                    //  CONFIG; and line was never refreshed from buf, so what
                    //  accumulated was the opening line repeated once per
                    //  remaining line rather than the block's contents.
                    while (is.getline(buf,MAXLINE,'\n'))
                    {
                        line = buf;
                        if (line.find("}") != string::npos)
                        {
                            break;
                        }
                        value += line + "\n";
                    }
                }
                else
                {
                    StringTokenizer tokens(line);
                    key = tokens.next();
                    value = tokens.next();
                }

                if (key != "")
                {
                    STLUtil::toLower(key);
                    (*p_config)[key] = value;
                }
            }
        }

        is.close();
    }
}


void WxMachineRegister::loadQueues(string refName)
{
    p_qnames.clear();
    p_minProcs.clear();
    p_maxProcs.clear();
    p_maxWall.clear();
    p_maxMem.clear();
    p_minScratch.clear();

    RefMachine *mref = RefMachine::refLookup(refName);
    vector<string*> *qnames = mref->queues();

    if (qnames != NULL)
    {
        const QueueManager *qmgr = QueueManager::lookup(refName);

        if (qmgr)
        {
            p_queueManagerChoicebox->SetStringSelection((wxString)(qmgr->queueMgrName()));
            vector<string*> *qnames = mref->queues();

            for (int idx = 0; idx < qnames->size(); idx++)
            {
                const Queue *queue = qmgr->queue(*(*qnames)[idx]);

                p_qnames.push_back(*(*qnames)[idx]);
                p_minProcs.push_back(queue->minProcessors());
                p_maxProcs.push_back(queue->maxProcessors());
                p_maxWall.push_back(queue->runLimit());
                p_maxMem.push_back(queue->memLimit());
                p_minScratch.push_back(queue->scratchLimit());
            }
        }
    }
}


//   Show queue values for queue of specified name.
void WxMachineRegister::showQueue(string refName)
{
    //cout << "WxMachineRegister::showQueue(string)" << endl;

    p_queueNameText->SetValue("");

    if (p_qnames.size() > 0)
    {
        int pos;
        for (pos=0; pos<p_qnames.size() && p_qnames[pos]!=refName; pos++);

        if (pos >= p_qnames.size())
        {
            pos = 0;
        }

        p_queueNameText->SetValue(p_qnames[pos]);

        if (p_minProcs[pos]!=INT_MAX && p_minProcs[pos]!=0)
        {
            p_queueMinProcsSpin->SetValue(p_minProcs[pos]);
        }
        else
        {
            p_queueMinProcsSpin->SetValue(1);
        }

        if (p_maxProcs[pos]!=INT_MAX && p_maxProcs[pos]!=0)
        {
            p_queueMaxProcsSpin->SetValue(p_maxProcs[pos]);
        }
        else
        {
            p_queueMaxProcsSpin->SetValue(1);
        }

        if (p_maxWall[pos] != INT_MAX)
        {
            p_queueMaxWallSpin->SetValue(p_maxWall[pos]);
        }
        else
        {
            p_queueMaxWallSpin->SetValue(0);
        }

        if (p_maxMem[pos] != INT_MAX)
        {
            p_queueMaxMemorySpin->SetValue(MemoryUnits::mbToGB(p_maxMem[pos]));
        }
        else
        {
            p_queueMaxMemorySpin->SetValue(0);
        }

        if (p_minScratch[pos] != INT_MAX)
        {
            p_queueMinScratchSpin->SetValue(p_minScratch[pos]);
        }
        else
        {
            p_queueMinScratchSpin->SetValue(0);
        }
    }
    else
    {
        p_queueManagerChoicebox->SetSelection(0);

        p_queueNameText->SetValue("");
        p_queueMinProcsSpin->SetValue(1);
        p_queueMaxProcsSpin->SetValue(1);
        p_queueMaxWallSpin->SetValue(0);
        p_queueMaxMemorySpin->SetValue(0);
        p_queueMinScratchSpin->SetValue(0);
    }
}


//  The selected registration is deleted, not whatever name is in the form.
int WxMachineRegister::removeMachine(const string& refName)
{
    string settings = "type=delete";
    settings += ProcessMachine::field("siteconfig",
                                      StringConverter::toString(p_adminFlag));
    settings += ProcessMachine::field("name", refName);
    return ProcessMachine::run(settings);
}


void WxMachineRegister::notifyUpdate()
{
    // notify other apps that machine registration has been saved
    JMSPublisher *pub = new JMSPublisher("WxMachineRegister");

    JMSMessage *msg = pub->newMessage();
    pub->publish("ecce_machreg_changed", *msg);

    delete msg;
    delete pub;
}


void WxMachineRegister::fillQueues(void)
{
    p_queuesChoicebox->Clear();

    if (p_qnames.size() > 0)
    {
        for (int idx = 0; idx < p_qnames.size(); idx++)
        {
            p_queuesChoicebox->Append((wxString)(p_qnames[idx]));
        }

        p_queuesChoicebox->SetSelection(0);
    }
}


bool WxMachineRegister::confirmRemove(string refName)
{
    ewxMessageDialog* dlgMesg;
    wxString prompt, title;
    int style, result;

    prompt = "Are you sure you want to delete the registration for \'" + refName + "\'?\n";
    prompt += "This action cannot be undone.";
    title = "Confirm Delete";
    style = wxYES_NO | wxICON_QUESTION;

    dlgMesg = new ewxMessageDialog(this, prompt, title, style);
    result = dlgMesg->ShowModal();

    return (result == wxID_YES);
}


bool WxMachineRegister::confirmExit()
{
    ewxMessageDialog dlg(this, "Do you really want to quit?",
                         "Quit ECCE Machine Registration",
                         wxOK|wxCANCEL|wxICON_QUESTION, wxDefaultPosition);

    return (dlg.ShowModal() == wxID_OK);
}
