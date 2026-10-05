/**
 *  @file
 *  @author Ken Swanson
 *
 *  Utility to allow registration functions for calculation servers; see
 *  the header.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <regex>
#include <set>

#include <sstream>
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/accel.h"
#include "wx/collpane.h"
#include "wx/filename.h"
#include "wx/artprov.h"
#include "wx/bmpbuttn.h"
#include "wx/radiobut.h"
#include "wx/statbmp.h"
#include "wx/utils.h"
#include "wx/display.h"
#include "wx/artprov.h"
#include "wx/statbmp.h"
#include "wx/listctrl.h"
#include "wx/notebook.h"
#include "wx/spinctrl.h"
#include "wx/statline.h"
#include "wx/timer.h"
#include "wx/hyperlink.h"
#include "wx/listbox.h"
#include "wx/dialog.h"

#include "util/BrowserHelp.H"
#include "util/Ecce.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/KeyValueReader.H"
#include "util/ProcessMachine.H"
#include "util/SFile.H"
#include "util/STLUtil.H"
#include "util/StringConverter.H"

#include "tdat/ConfigFile.H"
#include "tdat/Queue.H"
#include "tdat/QueueMgr.H"

#include "comm/RCommand.H"

#include "dsm/CodeFactory.H"
#include "dsm/MachinePreferences.H"
#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxScrolledWindow.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxWindowUtils.H"

#include "WxMachineRegister.H"
#include "MemoryUnits.H"

typedef MachineConfigDraft MCD;

static string plainTag(MCD::Tag t);

static const char* const TITLE = "ECCE Machine Registration";

static const int ID_NEW = wxID_HIGHEST + 301;
static const int ID_DELETE = wxID_HIGHEST + 302;

static string strip(const string& in)
{
    string s = in;
    STLUtil::stripLeadingAndTrailingWhiteSpace(s);
    return s;
}

static string lowerOf(const string& in)
{
    string s = in;
    STLUtil::toLower(s);
    return s;
}

static string withoutSlash(const string& path)
{
    string s = path;
    while (s.size() > 1 && s[s.size() - 1] == '/')
        s.erase(s.size() - 1);
    return s;
}

static bool remoteClient()
{
    const char* r = getenv("ECCE_REMOTE_SERVER");
    return r != NULL && *r != '\0';
}

//  The directory this session writes: siteconfig in admin mode, ~/.ECCE
//  otherwise.
static string editedDir(bool admin)
{
    return admin ? string(Ecce::ecceHome()) + "/siteconfig"
                 : withoutSlash(Ecce::realUserPrefPath());
}


WxMachineRegister::WxMachineRegister(wxWindow* parent, const bool admin)
    : ewxFrame(parent, wxID_ANY, TITLE, wxDefaultPosition, wxDefaultSize,
               wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|
               wxMINIMIZE_BOX|wxMAXIMIZE_BOX)
{
    p_adminFlag = admin;
    p_inCtrlUpdate = false;
    p_inListUpdate = false;
    p_closing = false;
    p_slctRgstn = NULL;
    p_draft = NULL;
    p_cfgPage = NULL;
    p_jobsTag = NULL;
    p_jobsUndoBox = NULL;
    p_jobsUndo = NULL;
    p_jobsCheck[0] = p_jobsCheck[1] = p_jobsCheck[2] = NULL;
    p_jobsMode = NULL;
    p_jobsIcon = NULL;
    p_advanced = NULL;
    p_advBtn = NULL;
    p_condorGrid = NULL;
    p_jobPage = NULL;
    p_cshTimer = NULL;
    p_rawDlg = NULL;
    p_wordsDlg = NULL;
    p_wordsList = NULL;
    p_wordsInsert = NULL;
    p_wordsWhere = NULL;
    p_codeSel = 0;
    p_codeList = NULL;
    p_codeTitle = NULL;
    p_codePage = NULL;
    p_codeAdvanced = NULL;
    p_codeAdvBtn = NULL;
    p_lastText = NULL;
    p_scripted = getenv("ECCE_MACHREG_SCRIPT") != NULL;
    p_codeNames = CodeFactory::getFullySupportedCodeNames();

    this->createControls();
    this->loadQueueManagerList();
    this->loadMachinesList();

    if (p_rows.empty())
        this->showNewMachine();
    else
        this->loadMachine(p_rows[0].name);

    if (!p_scripted)
    {
        Preferences prefs("MachineRegister");
        restoreSettings(prefs);
    }

    //  After the first machine is shown: its banner changes the size.
    this->growToFitSizer(true);
    this->Centre();
    this->keepOnScreen();

    ewxWindowUtils::setToolIcon(this, "MachineRegister");
}


WxMachineRegister::~WxMachineRegister()
{
    delete p_cshTimer;
    delete p_draft;
}


void WxMachineRegister::saveSettings(Preferences& prefs)
{
    ewxWindowUtils::saveWindowSettings(this, MACHREGISTER, prefs, false);
}


void WxMachineRegister::restoreSettings(Preferences& prefs)
{
    if (prefs.isValid())
        ewxWindowUtils::restoreWindowSettings(this, MACHREGISTER, prefs, false);
}


void WxMachineRegister::reg(const string& name, wxWindow* w)
{
    p_fields[name] = w;
}


wxWindow* WxMachineRegister::field(const string& name) const
{
    std::map<string, wxWindow*>::const_iterator it = p_fields.find(name);
    return it == p_fields.end() ? NULL : it->second;
}


//  ---- construction -------------------------------------------------------

ewxScrolledWindow* WxMachineRegister::newPage(wxWindow* parent, wxSizer* sizer)
{
    ewxScrolledWindow* page = new ewxScrolledWindow(parent, wxID_ANY,
        wxDefaultPosition, wxDefaultSize, wxVSCROLL|wxNO_BORDER|wxTAB_TRAVERSAL);
    page->SetScrollRate(0, 10);
    page->SetSizer(sizer);
    return page;
}


void WxMachineRegister::createControls()
{
    wxSizerFlags border = wxSizerFlags().Border();

    ewxPanel* panel = new ewxPanel(this, wxID_ANY, wxDefaultPosition,
                                   wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL);
    wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(panel, 1, wxEXPAND);
    this->SetSizer(frameSizer);

    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(root);

    wxBoxSizer* top = new wxBoxSizer(wxHORIZONTAL);
    root->Add(top, 1, wxEXPAND);

    //  Left: the machines, yours and the site's.
    wxBoxSizer* left = new wxBoxSizer(wxVERTICAL);
    left->Add(new ewxStaticText(panel, wxID_ANY, "Machines"), border);
    p_list = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition,
                            wxSize(230, -1), wxLC_REPORT|wxLC_SINGLE_SEL);
    p_list->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 140);
    p_list->InsertColumn(1, "From", wxLIST_FORMAT_LEFT, 70);
    left->Add(p_list, wxSizerFlags(1).Expand().Border(wxLEFT|wxRIGHT|wxBOTTOM));
    top->Add(left, 0, wxEXPAND);

    //  Right: one tab per kind of setting.
    wxBoxSizer* right = new wxBoxSizer(wxVERTICAL);
    p_book = new ewxNotebook(panel, wxID_ANY);
    p_book->AddPage(createMachinePage(p_book), "Machine");
    p_book->AddPage(createConnectionPage(p_book), "Connection");
    p_book->AddPage(createCodesPage(p_book), "Codes");
    p_book->AddPage(createJobScriptPage(p_book), "Job script");
    p_book->AddPage(createQueuesPage(p_book), "Queues");
    right->Add(p_book, wxSizerFlags(1).Expand().Border(wxTOP|wxRIGHT));

    p_storeNote = new wxStaticText(panel, wxID_ANY, "");
    right->Add(p_storeNote, wxSizerFlags().Border());
    top->Add(right, 1, wxEXPAND);

    root->Add(new wxStaticLine(panel), wxSizerFlags().Expand());

    //  Help at the left, the affirmative button last.
    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    p_helpButton = new ewxButton(panel, wxID_HELP, "&Help");
    p_deleteButton = new ewxButton(panel, ID_DELETE, "&Delete Machine");
    p_newButton = new ewxButton(panel, ID_NEW, "&New Machine");
    p_closeButton = new ewxButton(panel, wxID_CLOSE, "&Close");
    p_saveButton = new ewxButton(panel, wxID_SAVE, "&Save");
    buttons->Add(p_helpButton, border);
    buttons->AddStretchSpacer(1);
    buttons->Add(p_deleteButton, border);
    buttons->Add(p_newButton, border);
    buttons->AddSpacer(12);
    buttons->Add(p_closeButton, border);
    buttons->Add(p_saveButton, border);
    root->Add(buttons, wxSizerFlags().Expand());

    p_saveButton->SetDefault();
    p_saveButton->Enable(false);
    p_deleteButton->Enable(false);

    reg("help", p_helpButton);
    reg("delete", p_deleteButton);
    reg("new", p_newButton);
    reg("close", p_closeButton);
    reg("save", p_saveButton);

    wxAcceleratorEntry accel[1];
    accel[0].Set(wxACCEL_CTRL, (int)'S', wxID_SAVE);
    this->SetAcceleratorTable(wxAcceleratorTable(1, accel));

    this->Bind(wxEVT_CLOSE_WINDOW, &WxMachineRegister::onClose, this);
    p_book->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [this](wxBookCtrlEvent& e) {
        this->updateFooter();
        e.Skip();
    });
    p_list->Bind(wxEVT_LIST_ITEM_SELECTED, &WxMachineRegister::onListSelected, this);
    this->Bind(wxEVT_BUTTON, &WxMachineRegister::onSave, this, wxID_SAVE);
    this->Bind(wxEVT_BUTTON, &WxMachineRegister::onDelete, this, ID_DELETE);
    this->Bind(wxEVT_BUTTON, &WxMachineRegister::onNew, this, ID_NEW);
    this->Bind(wxEVT_BUTTON, &WxMachineRegister::onCloseButton, this, wxID_CLOSE);
    this->Bind(wxEVT_BUTTON, &WxMachineRegister::onHelp, this, wxID_HELP);
    this->Bind(wxEVT_MENU, &WxMachineRegister::onSave, this, wxID_SAVE);
    //  Every field's change event reaches the frame; one handler keeps the
    //  draft, the Save button and the title in step.
    this->Bind(wxEVT_TEXT, &WxMachineRegister::onFieldChanged, this);
    this->Bind(wxEVT_SPINCTRL, &WxMachineRegister::onFieldChanged, this);
    this->Bind(wxEVT_CHOICE, &WxMachineRegister::onFieldChanged, this);
    this->Bind(wxEVT_CHECKBOX, &WxMachineRegister::onFieldChanged, this);
    this->Bind(wxEVT_RADIOBUTTON, &WxMachineRegister::onFieldChanged, this);
    p_fullName->Bind(wxEVT_TEXT, &WxMachineRegister::onFullNameText, this);
    p_queueChoice->Bind(wxEVT_CHOICE, &WxMachineRegister::onQueueChoice, this);
    p_queueApply->Bind(wxEVT_BUTTON, &WxMachineRegister::onQueueApply, this);
    p_queueRemoveButton->Bind(wxEVT_BUTTON, &WxMachineRegister::onQueueRemove, this);
    p_queueClearButton->Bind(wxEVT_BUTTON, &WxMachineRegister::onQueueClear, this);

    this->InvalidateBestSize();
    this->GetSizer()->SetSizeHints(this);
    this->GetSizer()->Fit(this);
    this->growToFitSizer(true);
}


wxWindow* WxMachineRegister::createMachinePage(wxWindow* parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    ewxScrolledWindow* page = newPage(parent, sizer);
    wxSizerFlags border = wxSizerFlags().Border();

    //  Informational only: a message with an icon and no buttons (an
    //  info bar's Close button reads like the window's own).
    p_info = new wxPanel(page, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                         wxBORDER_SIMPLE);
    wxBoxSizer* infoRow = new wxBoxSizer(wxHORIZONTAL);
    infoRow->Add(new wxStaticBitmap(p_info, wxID_ANY,
        wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_MESSAGE_BOX)),
        wxSizerFlags().Border().CentreVertical());
    p_infoText = new wxStaticText(p_info, wxID_ANY, "");
    infoRow->Add(p_infoText, wxSizerFlags(1).Border().CentreVertical());
    p_info->SetSizer(infoRow);
    p_info->Show(false);
    sizer->Add(p_info, wxSizerFlags().Expand().Border(wxALL, 3));

    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 0, 0);
    grid->AddGrowableCol(1);
    sizer->Add(grid, wxSizerFlags().Expand());

    p_fullName = new ewxTextCtrl(page, wxID_ANY);
    p_fullName->SetToolTip("Required. The machine's fully-qualified name, "
                           "e.g. \"machine.anywhere.com\".");
    p_refName = new ewxTextCtrl(page, wxID_ANY);
    p_refName->SetToolTip("Required. The name ECCE uses for this machine, "
                          "e.g. \"curie-batch\".");
    grid->Add(new ewxStaticText(page, wxID_ANY, "Machine"),
              wxSizerFlags().Right().Border().CentreVertical());
    grid->Add(p_fullName, wxSizerFlags(1).Expand().Border().CentreVertical());
    grid->Add(new ewxStaticText(page, wxID_ANY, "Name"),
              wxSizerFlags().Right().Border().CentreVertical());
    grid->Add(p_refName, wxSizerFlags(1).Expand().Border().CentreVertical());

    p_vendor = new ewxTextCtrl(page, wxID_ANY);
    p_model = new ewxTextCtrl(page, wxID_ANY);
    p_proc = new ewxTextCtrl(page, wxID_ANY);
    wxBoxSizer* kind = new wxBoxSizer(wxHORIZONTAL);
    kind->Add(p_vendor, wxSizerFlags(1).Border().CentreVertical());
    kind->Add(new ewxStaticText(page, wxID_ANY, "Model"),
              wxSizerFlags().Border().CentreVertical());
    kind->Add(p_model, wxSizerFlags(1).Border().CentreVertical());
    kind->Add(new ewxStaticText(page, wxID_ANY, "Processor"),
              wxSizerFlags().Border().CentreVertical());
    kind->Add(p_proc, wxSizerFlags(1).Border().CentreVertical());
    grid->Add(new ewxStaticText(page, wxID_ANY, "Vendor"),
              wxSizerFlags().Right().Border().CentreVertical());
    grid->Add(kind, wxSizerFlags(1).Expand());

    p_procs = new ewxSpinCtrl(page, wxID_ANY, "1", wxDefaultPosition,
                              wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 1);
    p_nodes = new ewxSpinCtrl(page, wxID_ANY, "1", wxDefaultPosition,
                              wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 1);
    wxBoxSizer* counts = new wxBoxSizer(wxHORIZONTAL);
    counts->Add(p_procs, wxSizerFlags().Border().CentreVertical());
    counts->Add(new ewxStaticText(page, wxID_ANY, "Nodes"),
                wxSizerFlags().Border().CentreVertical());
    counts->Add(p_nodes, wxSizerFlags().Border().CentreVertical());
    grid->Add(new ewxStaticText(page, wxID_ANY, "Processors"),
              wxSizerFlags().Right().Border().CentreVertical());
    grid->Add(counts, wxSizerFlags(1));

    grid->Add(new ewxStaticText(page, wxID_ANY, "Connection"),
              wxSizerFlags().Right().Border().CentreVertical());
    grid->Add(new ewxStaticText(page, wxID_ANY, "ssh"),
              wxSizerFlags().Border().CentreVertical());

    //  #144: where jobs for a machine that names this host will run.
    p_localityText = new ewxStaticText(page, wxID_ANY, "");
    p_localityText->Show(false);
    sizer->Add(p_localityText, border);

    reg("fullname", p_fullName);
    reg("name", p_refName);
    reg("vendor", p_vendor);
    reg("model", p_model);
    reg("proc", p_proc);
    reg("procs", p_procs);
    reg("nodes", p_nodes);
    return page;
}


void WxMachineRegister::addHeading(wxWindow* page, wxSizer* sizer,
                                   const string& text)
{
    wxStaticText* t = new wxStaticText(page, wxID_ANY, text);
    wxFont f = t->GetFont();
    f.MakeBold();
    t->SetFont(f);
    sizer->Add(t, wxSizerFlags().Border(wxLEFT|wxRIGHT|wxTOP));
}


//  One key: [label] [control] [source tag] [...].  Checkboxes carry their
//  own label.  The control is registered under the lower-case key.
void WxMachineRegister::addCfgRow(wxWindow* page, wxFlexGridSizer* grid,
                                  const string& key, const string& label,
                                  CfgKind kind, const string& tip)
{
    CfgRow r;
    r.key = key;
    r.label = label;
    r.kind = kind;

    if (kind == CfgCheck || kind == CfgCheckYes)
        r.ctrl = new ewxCheckBox(page, wxID_ANY, label, wxDefaultPosition,
                                 wxDefaultSize, wxCHK_2STATE);
    else if (kind == CfgText)
        r.ctrl = new ewxTextCtrl(page, wxID_ANY);
    else
    {
        wxArrayString items;
        if (kind == CfgShell)
        {
            items.Add("bash"); items.Add("sh"); items.Add("csh");
            items.Add("tcsh");
        }
        else
        {
            items.Add("auto"); items.Add("yes"); items.Add("no");
        }
        r.ctrl = new ewxChoice(page, wxID_ANY, wxDefaultPosition,
                               wxSize(160, -1), items);
    }
    r.ctrl->SetToolTip(tip);
    reg(lowerOf(key), r.ctrl);

    if (grid == NULL)           // driven by other controls
    {
        r.ctrl->Hide();
        p_cfgRows.push_back(r);
        return;
    }

    if (kind == CfgCheck || kind == CfgCheckYes)
        grid->AddSpacer(0);
    else
    {
        r.name = new ewxStaticText(page, wxID_ANY, label);
        r.name->SetMinSize(wxSize(260, -1));
        grid->Add(r.name, wxSizerFlags().Right().Border().CentreVertical());
    }
    grid->Add(r.ctrl, wxSizerFlags(1).Expand().Border().CentreVertical());

    r.tag = new wxStaticText(page, wxID_ANY, "");
    r.tag->SetFont(r.tag->GetFont().Smaller());
    r.tag->SetMinSize(wxSize(r.tag->GetTextExtent("from server  ").x, -1));
    grid->Add(r.tag, wxSizerFlags().Border().CentreVertical());

    r.undo = makeUndo(page, r.undoBox);
    r.undo->Bind(wxEVT_BUTTON, &WxMachineRegister::onCfgUndo, this);
    grid->Add(r.undoBox, wxSizerFlags().Border().CentreVertical());

    reg("tag:" + lowerOf(key), r.tag);
    reg("undo:" + lowerOf(key), r.undo);
    p_cfgRows.push_back(r);
}


//  Lays out inside the tab only; the frame keeps its size.
void WxMachineRegister::setAdvanced(bool on)
{
    p_cfgPage->GetSizer()->Show(p_advanced, on);
    p_advBtn->SetLabel(wxString::FromUTF8(on ? "\xe2\x96\xbe Advanced"
                                             : "\xe2\x96\xb8 Advanced"));
    p_cfgPage->Layout();
    static_cast<wxScrolledWindow*>(p_cfgPage)->FitInside();
}


//  A small undo button in a box of fixed size, so the columns do not move
//  when it is hidden.
wxBitmapButton* WxMachineRegister::makeUndo(wxWindow* page, wxWindow*& box)
{
    box = new wxPanel(page, wxID_ANY, wxDefaultPosition, wxSize(34, 30));
    wxBitmapButton* b = new wxBitmapButton(box, wxID_ANY,
        wxArtProvider::GetBitmapBundle(wxART_UNDO, wxART_BUTTON),
        wxPoint(0, 0), wxDefaultSize, wxBU_EXACTFIT);
    b->SetToolTip("Use the site value");
    b->Hide();
    return b;
}


wxWindow* WxMachineRegister::createConnectionPage(wxWindow* parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    ewxScrolledWindow* page = newPage(parent, sizer);
    p_cfgPage = page;

    #define GRID(name) \
        wxFlexGridSizer* name = new wxFlexGridSizer(4, 0, 0); \
        name->AddGrowableCol(1); \
        sizer->Add(name, wxSizerFlags().Expand());

    addHeading(page, sizer, "Remote environment");
    GRID(env)
    addCfgRow(page, env, "shell", "Shell", CfgShell,
              "The shell ECCE starts on the remote machine to read the file "
              "below. bash is right unless that file is written for csh.");
    addCfgRow(page, env, "sourceFile", "Script run at login (e.g. module setup)", CfgText,
              "A file on the remote machine that sets up the environment "
              "(module commands, paths) before a job runs.");

    addHeading(page, sizer, "Connect through a login host");
    GRID(front)
    addCfgRow(page, front, "frontendMachine", "Log in via (gateway host)", CfgText,
              "Connect to this host first, then on to the machine. "
              "Leave empty to connect directly.");
    addCfgRow(page, front, "frontendBypass", "Connect directly from computers in", CfgText,
              "A domain, such as .example.org. When this computer is inside "
              "it, the login host is not used.");

    addHeading(page, sizer, "Paths on the remote machine");
    GRID(paths)
    addCfgRow(page, paths, "perlPath", "Perl program", CfgText,
              "Directory of the Perl 5 interpreter on the remote machine.");
    addCfgRow(page, paths, "qmgrPath", "Directory of sbatch, squeue, ...", CfgText, "");
    addCfgRow(page, paths, "libPath", "Library directory", CfgText,
              "Directory added to LD_LIBRARY_PATH on the remote machine.");
    addCfgRow(page, paths, "xappsPath", "X applications", CfgText,
              "Directory of X applications on the remote machine.");

    //  Normal mode is one line; the exceptions are chosen under Advanced.
    wxBoxSizer* jobHead = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(jobHead, wxSizerFlags().Expand());
    wxStaticText* jobTitle = new wxStaticText(page, wxID_ANY,
                                              "How jobs reach this machine");
    wxFont jf = jobTitle->GetFont();
    jf.MakeBold();
    jobTitle->SetFont(jf);
    jobHead->Add(jobTitle, wxSizerFlags().Border(wxLEFT|wxTOP));
    p_jobsIcon = new wxStaticBitmap(page, wxID_ANY,
        wxArtProvider::GetBitmapBundle(wxART_WARNING, wxART_BUTTON));
    jobHead->Add(p_jobsIcon, wxSizerFlags().Border(wxLEFT|wxTOP)
                                           .CentreVertical());
    p_jobsMode = new wxStaticText(page, wxID_ANY, "");
    jobHead->Add(p_jobsMode, wxSizerFlags().Border(wxLEFT|wxTOP)
                                           .CentreVertical());
    jobHead->AddStretchSpacer(1);
    addCfgRow(page, NULL, "noRemoteAccess", "", CfgCheck, "");
    addCfgRow(page, NULL, "userSubmit", "", CfgCheck, "");
    p_jobsTag = new wxStaticText(page, wxID_ANY, "");
    p_jobsTag->SetFont(p_jobsTag->GetFont().Smaller());
    p_jobsTag->SetMinSize(wxSize(p_jobsTag->GetTextExtent("from server  ").x, -1));
    jobHead->Add(p_jobsTag, wxSizerFlags().Border(wxTOP|wxBOTTOM|wxLEFT)
                                          .CentreVertical());
    p_jobsUndo = makeUndo(page, p_jobsUndoBox);
    p_jobsUndo->Bind(wxEVT_BUTTON, &WxMachineRegister::onCfgUndo, this);
    jobHead->Add(p_jobsUndoBox, wxSizerFlags().Border(wxTOP|wxRIGHT)
                                              .CentreVertical());
    reg("tag:jobs", p_jobsTag);
    reg("undo:jobs", p_jobsUndo);
    reg("jobs:mode", p_jobsMode);
    reg("jobs:icon", p_jobsIcon);

    //  Not a wxCollapsiblePane: that resizes the frame when it toggles.
    p_advBtn = new wxButton(page, wxID_ANY,
        wxString::FromUTF8("\xe2\x96\xb8 Advanced"),
        wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT|wxBORDER_NONE);
    p_advBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        this->setAdvanced(!p_advanced->IsShown());
    });
    reg("advanced:toggle", p_advBtn);
    sizer->Add(p_advBtn, wxSizerFlags().Border());
    p_advanced = new wxPanel(page);
    p_advanced->Hide();
    sizer->Add(p_advanced, wxSizerFlags().Expand().Border());
    wxWindow* adv = p_advanced;
    wxBoxSizer* advBox = new wxBoxSizer(wxVERTICAL);
    adv->SetSizer(advBox);

    wxStaticText* subTitle = new wxStaticText(adv, wxID_ANY,
                                              "Job submission");
    subTitle->SetFont(jf);
    advBox->Add(subTitle, wxSizerFlags().Border(wxLEFT|wxTOP));
    //  Normal is not a choice: it is what happens when neither is ticked.
    static const char* const jobText[3] = { "",
        "Submit jobs interactively",
        "Don't submit; only make the files" };
    static const char* const jobNote[3] = { "",
        "For sites that do not allow automated submission. ECCE copies the "
        "input and job script to the run directory; you log in, submit the "
        "job script yourself, and type the job ID into ECCE when asked, so "
        "it can follow the job.",
        "For machines ECCE cannot log in to, e.g. with two-factor "
        "authentication. ECCE writes the input and job script on this "
        "computer; you copy them to the machine, submit, and later import "
        "the output (Organizer > File > Import Calculation from Output "
        "File...)." };
    static const char* const jobName[3] = { "", "jobs:user", "jobs:none" };
    p_jobsCheck[0] = NULL;
    for (int i = 1; i < 3; i++)
    {
        p_jobsCheck[i] = new wxCheckBox(adv, wxID_ANY, jobText[i]);
        advBox->Add(p_jobsCheck[i], wxSizerFlags().Border(wxLEFT|wxTOP, 12));
        wxStaticText* note = new wxStaticText(adv, wxID_ANY, jobNote[i]);
        note->SetFont(note->GetFont().Smaller());
        note->SetForegroundColour(wxSystemSettings::GetColour(
                                  wxSYS_COLOUR_GRAYTEXT));
        note->Wrap(620);
        advBox->Add(note, wxSizerFlags().Border(wxLEFT, 36));
        reg(jobName[i], p_jobsCheck[i]);
    }
    wxFlexGridSizer* advGrid = new wxFlexGridSizer(4, 0, 0);
    advGrid->AddGrowableCol(1);
    advBox->Add(advGrid, wxSizerFlags().Expand().Border(wxTOP));
    addCfgRow(adv, advGrid, "singleConnect", "One connection for everything",
              CfgTri, "yes: send commands and files over one connection. "
              "auto: yes when this computer is outside the machine's domain.");
    addCfgRow(adv, advGrid, "checkScratch", "Check the scratch directory "
              "before a job starts", CfgCheckYes,
              "Verify that the scratch directory exists and is writable.");
    #undef GRID
    setAdvanced(false);
    return page;
}


wxWindow* WxMachineRegister::createCodesPage(wxWindow* parent)
{
    wxBoxSizer* outer = new wxBoxSizer(wxHORIZONTAL);
    ewxScrolledWindow* page = newPage(parent, outer);
    p_codePage = page;
    wxColour gray = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);

    //  The codes, with a star on those that have a program path.
    wxBoxSizer* left = new wxBoxSizer(wxVERTICAL);
    outer->Add(left, wxSizerFlags().Expand().Border());
    p_codeList = new wxListBox(page, wxID_ANY, wxDefaultPosition,
                               wxSize(170, 230));
    left->Add(p_codeList, wxSizerFlags(1).Expand());
    wxStaticText* star = new wxStaticText(page, wxID_ANY,
                                          "* has a program path");
    star->SetFont(star->GetFont().Smaller());
    star->SetForegroundColour(gray);
    left->Add(star, wxSizerFlags().Border(wxTOP, 4));
    reg("code:list", p_codeList);
    p_codeList->Bind(wxEVT_LISTBOX, [this](wxCommandEvent& e) {
        int row = p_codeList->GetSelection();
        if (row != wxNOT_FOUND && row < (int)p_codeShown.size())
            this->selectCode(p_codeShown[row]);
        e.Skip(false);
    });

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    outer->Add(sizer, wxSizerFlags(1).Expand());

    p_codeTitle = new wxStaticText(page, wxID_ANY, "");
    wxFont tf = p_codeTitle->GetFont().Bold();
    tf.SetFractionalPointSize(tf.GetFractionalPointSize() * 1.2);
    p_codeTitle->SetFont(tf);
    sizer->Add(p_codeTitle, wxSizerFlags().Border(wxLEFT|wxRIGHT|wxTOP));
    reg("code:title", p_codeTitle);

    wxFlexGridSizer* grid = new wxFlexGridSizer(4, 0, 0);
    grid->AddGrowableCol(1);
    sizer->Add(grid, wxSizerFlags().Expand());
    addCodeLine(page, grid, "code", "Program", "",
                "Where the program is installed on the machine. A code with "
                "no program is not offered for this machine.");

    //  Environment: gensub does not replace placeholders here.
    addBlock(page, sizer, "cenv", "Environment variables", page);
    wxStaticText* envHelp = new wxStaticText(page, wxID_ANY,
        "One \"NAME value\" per line, exported in the job script before the "
        "program starts. A name that contains PATH is added to the end of "
        "what the variable already holds; any other name replaces it.");
    envHelp->SetFont(envHelp->GetFont().Smaller());
    envHelp->SetForegroundColour(gray);
    envHelp->Wrap(560);
    sizer->Add(envHelp, wxSizerFlags().Border(wxLEFT|wxRIGHT));

    //  Command line.
    addBlock(page, sizer, "ccmd", "Command line", page);
    wxBoxSizer* cmdRow = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(cmdRow, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT));
    wxStaticText* cmdHelp = new wxStaticText(page, wxID_ANY,
        "When this is empty, ECCE's built-in command is used.");
    cmdHelp->SetFont(cmdHelp->GetFont().Smaller());
    cmdHelp->SetForegroundColour(gray);
    cmdRow->Add(cmdHelp, wxSizerFlags(1).CentreVertical());
    cmdRow->Add(wordsLink(page), wxSizerFlags().CentreVertical());
    reg("cmd:help", cmdHelp);

    //  Not a wxCollapsiblePane: that resizes the frame when it toggles.
    p_codeAdvBtn = new wxButton(page, wxID_ANY,
        wxString::FromUTF8("\xe2\x96\xb8 Advanced"),
        wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT|wxBORDER_NONE);
    p_codeAdvBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        this->setCodeAdvanced(!p_codeAdvanced->IsShown());
    });
    reg("code:advanced", p_codeAdvBtn);
    sizer->Add(p_codeAdvBtn, wxSizerFlags().Border());
    p_codeAdvanced = new wxPanel(page);
    p_codeAdvanced->Hide();
    sizer->Add(p_codeAdvanced, wxSizerFlags().Expand().Border());
    wxWindow* adv = p_codeAdvanced;
    wxBoxSizer* advBox = new wxBoxSizer(wxVERTICAL);
    adv->SetSizer(advBox);

    wxStaticText* advNote = new wxStaticText(adv, wxID_ANY,
        "Commands here replace the ones on the Job script tab, for this "
        "code only.");
    advNote->SetFont(advNote->GetFont().Smaller());
    advNote->SetForegroundColour(gray);
    advBox->Add(advNote, wxSizerFlags().Border(wxLEFT|wxRIGHT));
    addBlock(adv, advBox, "csetup", "Commands run before the program", page);
    addBlock(adv, advBox, "cwrapup", "Commands run after the program", page);
    wxFlexGridSizer* files = new wxFlexGridSizer(4, 0, 0);
    files->AddGrowableCol(1);
    advBox->Add(files, wxSizerFlags().Expand().Border(wxTOP));
    addCodeLine(adv, files, "files", "Files removed after the run",
                "FilesToRemove",
                "Names or patterns, separated by spaces, deleted from the "
                "run directory when the job has finished.");
    addCodeLine(adv, files, "prelim", "Files removed before the run",
                "PrelimFilesToRemove",
                "Names or patterns, separated by spaces, deleted from the "
                "run directory before the program starts.");
    setCodeAdvanced(false);
    return page;
}


//  One of the selected code's one-line settings.  The program line owns
//  every code's path box and shows the selected one.
void WxMachineRegister::addCodeLine(wxWindow* page, wxFlexGridSizer* grid,
                                    const string& id, const string& label,
                                    const string& suffix, const string& tip)
{
    CodeLine l;
    l.id = id;
    l.suffix = suffix;
    l.name = new ewxStaticText(page, wxID_ANY, label);
    l.name->SetMinSize(wxSize(190, -1));
    grid->Add(l.name, wxSizerFlags().Right().Border().CentreVertical());

    if (id == "code")
    {
        wxBoxSizer* col = new wxBoxSizer(wxVERTICAL);
        for (size_t i = 0; i < p_codeNames.size(); i++)
        {
            ewxTextCtrl* txt = new ewxTextCtrl(page, wxID_ANY);
            txt->SetToolTip(tip);
            txt->SetHint("e.g. /opt/" + lowerOf(p_codeNames[i]) + "/bin/" +
                         lowerOf(p_codeNames[i]));
            txt->Hide();
            col->Add(txt, wxSizerFlags().Expand());
            p_codePaths.push_back(txt);
            reg("code:" + lowerOf(p_codeNames[i]), txt);
        }
        grid->Add(col, wxSizerFlags(1).Expand().Border().CentreVertical());
        l.ctrl = p_codePaths.empty() ? NULL : p_codePaths[0];
    }
    else
    {
        l.ctrl = new ewxTextCtrl(page, wxID_ANY);
        l.ctrl->SetToolTip(tip);
        l.ctrl->SetHint("e.g. *.tmp core");
        grid->Add(l.ctrl, wxSizerFlags(1).Expand().Border().CentreVertical());
        reg("code:" + id, l.ctrl);
    }

    l.tag = new wxStaticText(page, wxID_ANY, "");
    l.tag->SetFont(l.tag->GetFont().Smaller());
    l.tag->SetMinSize(wxSize(l.tag->GetTextExtent("from server  ").x, -1));
    grid->Add(l.tag, wxSizerFlags().Border().CentreVertical());
    l.undo = makeUndo(page, l.undoBox);
    l.undo->Bind(wxEVT_BUTTON, [this, id](wxCommandEvent&) {
        this->codeLineUndo(id);
    });
    grid->Add(l.undoBox, wxSizerFlags().Border().CentreVertical());
    reg("tag:" + id, l.tag);
    reg("undo:" + id, l.undo);
    p_codeLines.push_back(l);
}


WxMachineRegister::CodeLine* WxMachineRegister::codeLine(const string& id)
{
    for (size_t i = 0; i < p_codeLines.size(); i++)
        if (p_codeLines[i].id == id)
            return &p_codeLines[i];
    return NULL;
}


string WxMachineRegister::codeLineKey(const CodeLine& l) const
{
    return p_codeNames.empty() ? string() : p_codeNames[p_codeSel] + l.suffix;
}


void WxMachineRegister::codeLineToControl(CodeLine& l)
{
    if (p_draft == NULL || p_codeNames.empty())
        return;
    string v;
    bool has = p_draft->effective(codeLineKey(l), v);
    if (l.id == "code")
    {
        l.ctrl = p_codePaths[p_codeSel];
        return;                 // the path boxes are filled with the machine
    }
    static_cast<wxTextCtrl*>(l.ctrl)->ChangeValue(has ? v : "");
}


void WxMachineRegister::codeLinesToControls()
{
    for (size_t i = 0; i < p_codeLines.size(); i++)
        codeLineToControl(p_codeLines[i]);
}


//  A key the machine itself sets: its own file or the site's CONFIG, not the
//  defaults every machine gets from submit.site or a vendor file.
static bool keySet(const MCD::KeyState& ks)
{
    if (ks.edit != MCD::Inherit)
        return true;
    for (size_t i = 0; i < ks.inherited.size(); i++)
        if (ks.inherited[i].source == "site" || ks.inherited[i].source == "user")
            return true;
    return false;
}


//  A code is listed when it is live, or when the machine already has a
//  setting for it (a retired code that a site still configures).
bool WxMachineRegister::codeInUse(const string& name) const
{
    static const char* const suffix[] = { "", "Command", "Environment",
        "FilesToRemove", "PrelimFilesToRemove" };
    if (p_draft == NULL)
        return false;
    for (size_t k = 0; k < 5; k++)
    {
        const MCD::KeyState* ks = p_draft->state(name + suffix[k]);
        if (ks != NULL && keySet(*ks))
            return true;
    }
    for (const char* x : { "_setup", "_wrapup" })
    {
        const MCD::KeyState* ks = p_draft->state(lowerOf(name) + x);
        if (ks != NULL && keySet(*ks))
            return true;
    }
    for (size_t i = 0; i < p_codeNames.size(); i++)
        if (p_codeNames[i] == name &&
            !strip((string)p_codePaths[i]->GetValue()).empty())
            return true;
    return false;
}


//  Rebuild the list for the loaded machine and show the selected code.
void WxMachineRegister::fillCodeList()
{
    static const std::set<string> retired = { "Gaussian-03", "Gaussian-98",
                                              "GAMESS-UK", "Amica" };
    p_codeList->Clear();
    p_codeShown.clear();
    for (size_t i = 0; i < p_codeNames.size(); i++)
        if (!retired.count(p_codeNames[i]) || codeInUse(p_codeNames[i]))
        {
            p_codeShown.push_back((int)i);
            p_codeList->Append(p_codeNames[i]);
        }
    if (p_codeShown.empty())
        return;
    if (std::find(p_codeShown.begin(), p_codeShown.end(), p_codeSel) ==
        p_codeShown.end())
        p_codeSel = p_codeShown[0];
    showCode();
}


//  Everything on the tab that depends on the selected code, except the
//  blocks (blocksToControls does those).
void WxMachineRegister::showCode()
{
    if (p_codeNames.empty())
        return;
    const string& name = p_codeNames[p_codeSel];
    for (size_t i = 0; i < p_codePaths.size(); i++)
        p_codePaths[i]->Show((int)i == p_codeSel);
    p_codeTitle->SetLabel(name);
    for (size_t r = 0; r < p_codeShown.size(); r++)
        if (p_codeShown[r] == p_codeSel)
            p_codeList->SetSelection((int)r);
    codeLinesToControls();

    //  Open Advanced when it holds something for this code.
    bool used = false;
    if (p_draft != NULL)
    {
        const char* keys[] = { "_setup", "_wrapup" };
        for (size_t k = 0; k < 2; k++)
        {
            const MCD::KeyState* ks = p_draft->state(lowerOf(name) + keys[k]);
            used = used || (ks != NULL && keySet(*ks));
        }
        for (size_t k = 0; k < p_codeLines.size(); k++)
        {
            if (p_codeLines[k].suffix.empty())
                continue;
            const MCD::KeyState* ks = p_draft->state(
                codeLineKey(p_codeLines[k]));
            used = used || (ks != NULL && keySet(*ks));
        }
    }
    if (used && !p_codeAdvanced->IsShown())
        setCodeAdvanced(true);
    else
    {
        p_codePage->Layout();
        static_cast<wxScrolledWindow*>(p_codePage)->FitInside();
    }
}


void WxMachineRegister::selectCode(int index)
{
    if (index < 0 || index >= (int)p_codeNames.size())
        return;
    //  What was typed belongs to the code it was typed for.
    if (p_draft != NULL && index != p_codeSel)
        this->syncDraft();
    p_codeSel = index;
    bool was = p_inCtrlUpdate;
    p_inCtrlUpdate = true;
    showCode();
    blocksToControls();
    p_inCtrlUpdate = was;
    updateDirty();
}


bool WxMachineRegister::selectCodeByName(const string& name)
{
    for (size_t r = 0; r < p_codeShown.size(); r++)
        if (lowerOf(p_codeNames[p_codeShown[r]]) == lowerOf(name))
        {
            selectCode(p_codeShown[r]);
            return true;
        }
    return false;
}


//  Lays out inside the tab only; the frame keeps its size.
void WxMachineRegister::setCodeAdvanced(bool on)
{
    p_codePage->GetSizer()->Show(p_codeAdvanced, on, true);
    p_codeAdvBtn->SetLabel(wxString::FromUTF8(on ? "\xe2\x96\xbe Advanced"
                                                 : "\xe2\x96\xb8 Advanced"));
    p_codePage->Layout();
    static_cast<wxScrolledWindow*>(p_codePage)->FitInside();
}


//  Tag, tooltip and undo button of the selected code's lines, and the stars.
void WxMachineRegister::codeLinesTags()
{
    if (p_draft == NULL || p_codeNames.empty())
        return;
    for (size_t i = 0; i < p_codeLines.size(); i++)
    {
        const CodeLine& l = p_codeLines[i];
        string key = codeLineKey(l);
        const MCD::KeyState* ks = p_draft->state(key);
        if (ks == NULL)
            continue;
        MCD::Tag t;
        string tip;
        cfgTagInfo(key, false, t, tip);
        wxString text = plainTag(t);
        if (l.tag->GetLabel() != text)
            l.tag->SetLabel(text);
        l.tag->SetForegroundColour(wxSystemSettings::GetColour(
            (t == MCD::TagYours || t == MCD::TagNoValue ||
             t == MCD::TagSiteEditing) ? wxSYS_COLOUR_WINDOWTEXT
                                       : wxSYS_COLOUR_GRAYTEXT));
        tip += "\nCONFIG key: " + ks->name;
        if (l.tag->GetToolTipText() != tip)
            l.tag->SetToolTip(tip);
        int uc = undoCase(key, false);
        if (l.undo->IsShown() != (uc != 0))
            l.undo->Show(uc != 0);
        l.undo->SetToolTip(undoTip(uc, "value"));
    }
    for (size_t r = 0; r < p_codeShown.size(); r++)
    {
        int i = p_codeShown[r];
        bool has = !strip((string)p_codePaths[i]->GetValue()).empty();
        wxString want = p_codeNames[i] + (has ? "  *" : "");
        if (p_codeList->GetString(r) != want)
            p_codeList->SetString(r, want);
    }
}


void WxMachineRegister::codeLineUndo(const string& id)
{
    CodeLine* l = codeLine(id);
    if (l == NULL || p_draft == NULL)
        return;
    this->syncDraft();
    string key = codeLineKey(*l);
    undoKey(key, p_draft->changed(key));
    p_inCtrlUpdate = true;
    if (id == "code")
    {
        string v;
        p_draft->effective(key, v);
        p_codePaths[p_codeSel]->SetValue(v);
    }
    else
        codeLineToControl(*l);
    p_inCtrlUpdate = false;
    this->updateDirty();
}


void WxMachineRegister::syncCodeLines(MCD* draft)
{
    for (size_t i = 0; i < p_codeLines.size(); i++)
    {
        const CodeLine& l = p_codeLines[i];
        if (l.suffix.empty() || p_codeNames.empty())
            continue;
        syncKey(draft, codeLineKey(l),
                (string)static_cast<wxTextCtrl*>(l.ctrl)->GetValue());
    }
}


//  Remember the last box typed in, so a placeholder can go there.
void WxMachineRegister::trackFocus(wxTextCtrl* t)
{
    t->Bind(wxEVT_SET_FOCUS, [this, t](wxFocusEvent& e) {
        p_lastText = t;
        e.Skip();
    });
}


//  The box a placeholder is inserted into: the one last focused if it can
//  be edited now, else the main box of the visible tab.
wxTextCtrl* WxMachineRegister::insertTarget(string& where) const
{
    auto describe = [this](wxTextCtrl* t, string& name) {
        for (size_t i = 0; i < p_blocks.size(); i++)
            if (p_blocks[i].user == t && p_blocks[i].id != "cenv")
            {
                name = (string)p_blocks[i].heading->GetLabel();
                return true;
            }
        return false;
    };
    auto usable = [](wxTextCtrl* t) {
        return t != NULL && t->IsEnabled() && t->IsShownOnScreen();
    };
    wxTextCtrl* t = p_lastText;
    if (usable(t) && describe(t, where))
        return t;
    const BlockRow* main = NULL;
    for (size_t i = 0; i < p_blocks.size(); i++)
    {
        const string& id = p_blocks[i].id;
        wxWindow* cur = p_book->GetCurrentPage();
        if ((id == "header" && cur == p_jobPage) ||
            (id == "ccmd" && cur == p_codePage))
            main = &p_blocks[i];
    }
    if (main != NULL && usable(main->user))
    {
        where = (string)main->heading->GetLabel();
        return main->user;
    }
    return NULL;
}


bool WxMachineRegister::insertWord(const string& word)
{
    string where;
    wxTextCtrl* t = insertTarget(where);
    if (t == NULL)
        return false;
    t->WriteText(word);
    p_lastText = t;
    return true;
}


wxWindow* WxMachineRegister::createJobScriptPage(wxWindow* parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    ewxScrolledWindow* page = newPage(parent, sizer);
    p_jobPage = page;

    p_jobNote = new wxStaticText(page, wxID_ANY, "");
    sizer->Add(p_jobNote, wxSizerFlags().Border());

    sizer->Add(wordsLink(page), wxSizerFlags().Border(wxLEFT|wxRIGHT));

    addBlock(page, sizer, "header", "");


    addBlock(page, sizer, "setup", "Commands run before the calculation");
    addBlock(page, sizer, "wrapup", "Commands run after the calculation");

    //  Only HTCondor needs it.
    wxFlexGridSizer* condor = new wxFlexGridSizer(4, 0, 0);
    condor->AddGrowableCol(1);
    sizer->Add(condor, wxSizerFlags().Expand().Border(wxTOP));
    addCfgRow(page, condor, "condorAllowTmp", "Allow a run directory under "
              "/tmp (HTCondor)", CfgCheck,
              "HTCondor gives every job a private /tmp. Tick this only if "
              "your pool does not, so that a run directory under /tmp or "
              "/var/tmp works.");
    p_cfgRows.back().gensubOnly = true;
    p_condorGrid = condor;

    ewxButton* adv = new ewxButton(page, wxID_ANY, "Advanced: edit file...");
    adv->SetToolTip("Edit the whole settings file for this machine as text");
    sizer->Add(adv, wxSizerFlags().Border());
    reg("edit-file", adv);
    adv->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { this->editFile(); });

    p_cshTimer = new wxTimer(this);
    this->Bind(wxEVT_TIMER, [this](wxTimerEvent&) { this->blocksCheckCsh(); });
    return page;
}


//  One block: heading, the inherited text (read-only), then the user's.
void WxMachineRegister::addBlock(wxWindow* page, wxSizer* sizer,
                                 const string& id, const string& title,
                                 wxWindow* scroll)
{
    BlockRow b;
    b.id = id;
    b.scroll = scroll != NULL ? scroll : page;
    b.box = new wxBoxSizer(wxVERTICAL);
    sizer->Add(b.box, wxSizerFlags().Expand());

    wxFont mono(wxFontInfo().Family(wxFONTFAMILY_TELETYPE));
    wxColour gray = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);

    b.heading = new wxStaticText(page, wxID_ANY, title);
    b.heading->SetFont(b.heading->GetFont().Bold());
    b.box->Add(b.heading, wxSizerFlags().Border(wxLEFT|wxRIGHT|wxTOP));

    b.siteLabel = new wxStaticText(page, wxID_ANY, "");
    b.siteLabel->SetFont(b.siteLabel->GetFont().Smaller());
    b.siteLabel->SetForegroundColour(gray);
    b.box->Add(b.siteLabel, wxSizerFlags().Border(wxLEFT|wxRIGHT));
    b.site = new ewxTextCtrl(page, wxID_ANY, "", wxDefaultPosition,
        wxSize(-1, 80), wxTE_MULTILINE|wxTE_READONLY|wxTE_DONTWRAP);
    b.site->SetFont(mono);
    b.site->SetForegroundColour(gray);
    b.box->Add(b.site, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT));

    wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
    b.box->Add(row, wxSizerFlags().Expand());
    b.yoursLabel = new wxStaticText(page, wxID_ANY, "Your text instead:");
    row->Add(b.yoursLabel, wxSizerFlags().Border().CentreVertical());
    b.copy = new ewxButton(page, wxID_ANY, "Copy site text to edit");
    b.copy->SetToolTip("Start from the site's text and change it. Your text "
                       "replaces the site's, it is not added to it.");
    row->Add(b.copy, wxSizerFlags().Border(wxTOP|wxBOTTOM|wxRIGHT)
                                   .CentreVertical());
    b.none = new wxCheckBox(page, wxID_ANY, "Use no text");
    b.none->SetToolTip("Ignore the site's text and put nothing here");
    row->Add(b.none, wxSizerFlags().Border().CentreVertical());
    row->AddStretchSpacer(1);
    b.tag = new wxStaticText(page, wxID_ANY, "");
    b.tag->SetFont(b.tag->GetFont().Smaller());
    b.tag->SetMinSize(wxSize(b.tag->GetTextExtent("from server  ").x, -1));
    row->Add(b.tag, wxSizerFlags().Border().CentreVertical());
    b.undo = makeUndo(page, b.undoBox);
    b.undo->Bind(wxEVT_BUTTON, [this, id](wxCommandEvent&) {
        this->blockUndo(id);
    });
    row->Add(b.undoBox, wxSizerFlags().Border(wxRIGHT).CentreVertical());

    b.user = new ewxTextCtrl(page, wxID_ANY, "", wxDefaultPosition,
        wxSize(-1, 90), wxTE_MULTILINE|wxTE_DONTWRAP);
    b.user->SetFont(mono);
    b.box->Add(b.user, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT));
    if (id != "cenv")
        trackFocus(b.user);

    b.note = new wxStaticText(page, wxID_ANY, "");
    b.note->SetFont(b.note->GetFont().Smaller());
    b.note->SetForegroundColour(wxSystemSettings::GetColour(
                                wxSYS_COLOUR_HOTLIGHT));
    b.box->Add(b.note, wxSizerFlags().Border(wxLEFT|wxRIGHT));

    b.copy->Bind(wxEVT_BUTTON, [this, id](wxCommandEvent&) {
        this->blockCopySite(id);
    });
    b.none->Bind(wxEVT_CHECKBOX, [this, id](wxCommandEvent& e) {
        this->blockNone(id);
        e.Skip();
    });

    reg("blk:" + id, b.user);
    reg("blk:" + id + ":site", b.site);
    reg("blk:" + id + ":label", b.siteLabel);
    reg("blk:" + id + ":none", b.none);
    reg("blk:" + id + ":copy", b.copy);
    reg("blk:" + id + ":csh", b.note);
    reg("tag:" + id, b.tag);
    reg("undo:" + id, b.undo);
    p_blocks.push_back(b);
}


WxMachineRegister::BlockRow* WxMachineRegister::block(const string& id)
{
    for (size_t i = 0; i < p_blocks.size(); i++)
        if (p_blocks[i].id == id)
            return &p_blocks[i];
    return NULL;
}


//  The header is the block named after the queue manager; "" when the job
//  runs without one.
string WxMachineRegister::headerKey() const
{
    string q = (string)p_qmgrChoice->GetStringSelection();
    string l = lowerOf(q);
    return (l.empty() || l == "none" || l == "shell") ? "" : l;
}


wxWindow* WxMachineRegister::createQueuesPage(wxWindow* parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    ewxScrolledWindow* page = newPage(parent, sizer);
    wxSizerFlags border = wxSizerFlags().Border();

    p_qmgrChoice = new ewxChoice(page, wxID_ANY, wxDefaultPosition,
                                 wxSize(200, -1));
    p_allocAccts = new ewxCheckBox(page, wxID_ANY, "Allocation accounts used",
                                   wxDefaultPosition, wxDefaultSize, wxCHK_2STATE);
    wxBoxSizer* row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new ewxStaticText(page, wxID_ANY, "Queue manager"),
              wxSizerFlags().Border().CentreVertical());
    row1->Add(p_qmgrChoice, wxSizerFlags().Border().CentreVertical());
    row1->AddSpacer(12);
    row1->Add(p_allocAccts, wxSizerFlags().Border().CentreVertical());
    sizer->Add(row1);

    p_queueChoice = new ewxChoice(page, wxID_ANY, wxDefaultPosition,
                                  wxSize(200, -1));
    wxBoxSizer* row2 = new wxBoxSizer(wxHORIZONTAL);
    row2->Add(new ewxStaticText(page, wxID_ANY, "Queues"),
              wxSizerFlags().Border().CentreVertical());
    row2->Add(p_queueChoice, wxSizerFlags().Border().CentreVertical());
    sizer->Add(row2);

    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 0, 0);
    grid->AddGrowableCol(1);
    p_queueName = new ewxTextCtrl(page, wxID_ANY);
    grid->Add(new ewxStaticText(page, wxID_ANY, "Name"),
              wxSizerFlags().Right().Border().CentreVertical());
    grid->Add(p_queueName, wxSizerFlags(1).Expand().Border().CentreVertical());

    //  Wall time is shown in hours and stored in minutes (README.Q); a
    //  fraction is fine because minutes are the finer unit.
    p_qMaxWall = new wxSpinCtrlDouble(page, wxID_ANY, "0", wxDefaultPosition,
                                      wxDefaultSize, wxSP_ARROW_KEYS, 0,
                                      100000, 0, 0.25);
    p_qMaxWall->SetDigits(2);
    reg("q-maxwall", p_qMaxWall);

    struct SpinRow { ewxSpinCtrl** spin; const char* label; const char* unit;
                     int min; const char* key; };
    SpinRow rows[] = {
        { &p_qMinProcs, "Min processors", "", 1, "q-minprocs" },
        { &p_qMaxProcs, "Max processors", "", 1, "q-maxprocs" },
        { NULL, "Max wall time", "h", 0, "" },
        { &p_qMaxMem, "Max memory", "GB", 0, "q-maxmem" },
        { &p_qMinScratch, "Min scratch", "GB", 0, "q-minscratch" },
    };
    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++)
    {
        wxWindow* spin = p_qMaxWall;
        if (rows[i].spin != NULL)
        {
            *rows[i].spin = new ewxSpinCtrl(page, wxID_ANY,
                wxString::Format("%d", rows[i].min), wxDefaultPosition,
                wxDefaultSize, wxSP_ARROW_KEYS, rows[i].min, 100000,
                rows[i].min);
            spin = *rows[i].spin;
            reg(rows[i].key, spin);
        }
        grid->Add(new ewxStaticText(page, wxID_ANY, rows[i].label),
                  wxSizerFlags().Right().Border().CentreVertical());
        wxBoxSizer* cell = new wxBoxSizer(wxHORIZONTAL);
        cell->Add(spin, wxSizerFlags().Border().CentreVertical());
        if (*rows[i].unit)
            cell->Add(new ewxStaticText(page, wxID_ANY, rows[i].unit),
                      wxSizerFlags().CentreVertical());
        grid->Add(cell, wxSizerFlags().CentreVertical());
    }
    sizer->Add(grid, wxSizerFlags().Expand());

    p_queueApply = new ewxButton(page, wxID_ANY, "Add Queue");
    p_queueRemoveButton = new ewxButton(page, wxID_ANY, "Remove Queue");
    p_queueClearButton = new ewxButton(page, wxID_ANY, "Remove All");
    wxBoxSizer* qbuttons = new wxBoxSizer(wxHORIZONTAL);
    qbuttons->Add(p_queueApply, border);
    qbuttons->Add(p_queueRemoveButton, border);
    qbuttons->Add(p_queueClearButton, border);
    sizer->Add(qbuttons);
    sizer->Add(new wxStaticText(page, wxID_ANY,
        "Queue changes are kept in the list until you press Save."),
        wxSizerFlags().Border());

    reg("qmgr", p_qmgrChoice);
    reg("aa", p_allocAccts);
    reg("queue", p_queueChoice);
    reg("q-name", p_queueName);
    reg("queue-apply", p_queueApply);
    reg("queue-remove", p_queueRemoveButton);
    reg("queue-clear", p_queueClearButton);
    return page;
}


//  Load up list of supported queue managers from file.
void WxMachineRegister::loadQueueManagerList()
{
    bool found = false;

    string path = Ecce::ecceHome();
    path.append("/siteconfig/QueueManagers");

    KeyValueReader reader(path.c_str());
    string key, value;

    p_qmgrChoice->Clear();
    p_qmgrChoice->Append("None");

    while (!found && (reader.getpair(key, value)))
    {
        if (key == "QueueManagers")
        {
            found = true;

            char *tokestr = strdup(value.c_str());
            char *tok = strtok(tokestr, " \t");

            while (tok)
            {
                p_qmgrChoice->Append(tok);
                tok = strtok((char*)0, " \t");
            }

            free(tokestr);
        }
    }

    p_qmgrChoice->SetSelection(0);
}


//  ---- sizing (#187) ------------------------------------------------------

/**
 *  #187: keep the frame's minimum in step with the pages and, with
 *  fitWholeForm, grow the frame to show the largest page whole, capped to
 *  the display.  Pages scroll vertically, so the notebook's minimum is only
 *  a small floor; the button row is always inside the frame's minimum.
 */
void WxMachineRegister::growToFitSizer(bool fitWholeForm)
{
    if (this->GetSizer() == NULL || p_book == NULL)
        return;

    const int MIN_FORM_HEIGHT = 100;

    wxSize cap = this->maxClientSizeForDisplay();
    wxSize have = this->GetClientSize();

    wxSize natural(0, 0);
    for (size_t i = 0; i < p_book->GetPageCount(); i++)
    {
        wxWindow* page = p_book->GetPage(i);
        if (page->GetSizer() == NULL)
            continue;
        wxSize s = page->GetSizer()->GetMinSize();
        natural.x = wxMax(natural.x, s.x);
        natural.y = wxMax(natural.y, s.y);
    }

    //  Window best sizes are cached; the notebook's change must reach the
    //  frame's sizer.
    struct Invalidate {
        static void up(wxWindow* from, wxWindow* stop)
        {
            for (wxWindow* w = from; w != NULL && w != stop; w = w->GetParent())
                w->InvalidateBestSize();
            stop->InvalidateBestSize();
        }
    };

    p_book->SetMinSize(p_book->CalcSizeFromPage(natural));
    Invalidate::up(p_book, this);
    wxSize full = this->GetSizer()->GetMinSize();

    p_book->SetMinSize(p_book->CalcSizeFromPage(
        wxSize(natural.x, wxMin(natural.y, MIN_FORM_HEIGHT))));
    Invalidate::up(p_book, this);
    wxSize need = this->GetSizer()->GetMinSize();
    this->SetMinClientSize(need);

    wxSize target = fitWholeForm ? full : need;
    wxSize want(wxMax(target.x, have.x), wxMax(target.y, have.y));
    want.x = wxMax(need.x, wxMin(want.x, cap.x));
    want.y = wxMax(need.y, wxMin(want.y, cap.y));

    wxSize decoration = this->GetSize() - this->GetClientSize();
    this->SetMaxSize(wxSize(wxMax(want.x, cap.x) + decoration.x,
                            wxMax(want.y, cap.y) + decoration.y));

    if (want != have)
        this->SetClientSize(want);

    this->Layout();
    for (size_t i = 0; i < p_book->GetPageCount(); i++)
        p_book->GetPage(i)->FitInside();

    if (getenv("ECCE_DEBUG_MACHREGISTER_SIZE") != NULL)
    {
        fprintf(stderr,
                "[MACHREG_SIZE] cap=%dx%d need=%dx%d full=%dx%d "
                "frameClient=%dx%d frameOuter=%dx%d page=%dx%d\n",
                cap.x, cap.y, need.x, need.y, full.x, full.y,
                this->GetClientSize().x, this->GetClientSize().y,
                this->GetSize().x, this->GetSize().y, natural.x, natural.y);
    }
}


//  Move the frame back fully onto its display; never resizes it.
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


//  The most the client area can be without the frame exceeding the usable
//  area of its display.  A pure query; growToFitSizer() applies it.
wxSize WxMachineRegister::maxClientSizeForDisplay()
{
    int dpyIdx = wxDisplay::GetFromWindow(this);
    wxDisplay display((unsigned)(dpyIdx == wxNOT_FOUND ? 0 : dpyIdx));
    wxRect avail = display.GetClientArea();

    wxSize decoration = this->GetSize() - this->GetClientSize();
    wxSize cap(avail.width - decoration.x, avail.height - decoration.y);
    if (cap.x <= 0)
        cap.x = avail.width;
    if (cap.y <= 0)
        cap.y = avail.height;

    return cap;
}


/**
 *  A machine registered as localhost, 127.0.0.1, ::1 or this host's own
 *  name runs jobs LOCALLY when the login name used for it is empty or
 *  your own, and over ssh to that name when it is anyone else's (#144).
 *  Shown where the name is typed, from RCommand::isRemote(), the function
 *  the launch uses.
 */
void WxMachineRegister::refreshLocality()
{
    if (p_localityText == NULL || p_fullName == NULL)
        return;

    string machine = (string)p_fullName->GetValue();
    string text = "";

    if (!RCommand::localityNote(machine, "ssh", "").empty())
    {
        //  Said in terms of launching, since that is the only thing a
        //  machine here is for; the queue manager may be set for localhost.
        text = "This computer. Calculations you launch on \"" + machine +
               "\" run here, on this computer,\neither directly or through "
               "the queue manager chosen on the Queues tab.\n"
               "This holds while the login name is empty or " +
               Ecce::realUser() + ";\nwith any other login name they run "
               "via ssh to " + machine + " as that user.";

        string refName = (string)p_refName->GetValue();
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
        this->growToFitSizer();
    }
}


//  ---- the machine list ---------------------------------------------------

void WxMachineRegister::loadMachinesList()
{
    p_rows.clear();

    //  Your machines shadow site machines of the same name.
    vector<string> yours;
    if (!p_adminFlag)
    {
        vector<string> *u = RefMachine::referenceNames(RefMachine::userMachines);
        yours = *u;
        delete u;
    }
    vector<string> *s = RefMachine::referenceNames(RefMachine::siteMachines);
    vector<string> site = *s;
    delete s;

    string siteWord = remoteClient() ? "server" : "site";
    for (size_t i = 0; i < yours.size(); i++)
    {
        Row r; r.name = yours[i]; r.from = "yours";
        p_rows.push_back(r);
    }
    for (size_t i = 0; i < site.size(); i++)
    {
        if (std::find(yours.begin(), yours.end(), site[i]) != yours.end())
            continue;
        Row r; r.name = site[i]; r.from = siteWord;
        p_rows.push_back(r);
    }
    std::sort(p_rows.begin(), p_rows.end(),
              [](const Row& a, const Row& b) { return a.name < b.name; });

    p_inListUpdate = true;
    p_list->DeleteAllItems();
    for (size_t i = 0; i < p_rows.size(); i++)
    {
        long idx = p_list->InsertItem((long)i, p_rows[i].name);
        p_list->SetItem(idx, 1, p_rows[i].from);
    }
    p_inListUpdate = false;
}


//  Case-sensitive: machine names are, and a case-insensitive match once
//  selected "Tellurium" after "tellurium" was deleted.
int WxMachineRegister::findRow(const string& name) const
{
    for (size_t i = 0; i < p_rows.size(); i++)
        if (p_rows[i].name == name)
            return (int)i;
    return -1;
}


//  The form holds a machine that is not yours, so saving makes your copy.
bool WxMachineRegister::fromSite() const
{
    return !p_adminFlag && !p_loadedName.empty() && p_loadedFrom != "yours";
}


void WxMachineRegister::onListSelected(wxListEvent& event)
{
    if (p_inListUpdate)
        return;

    int idx = event.GetIndex();
    if (idx < 0 || idx >= (int)p_rows.size())
        return;
    string name = p_rows[idx].name;
    if (name == p_loadedName)
        return;

    if (!resolveUnsaved("Discard Changes"))
    {
        int prev = findRow(p_loadedName);
        p_inListUpdate = true;
        if (prev >= 0)
            p_list->SetItemState(prev, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        else
            p_list->SetItemState(idx, 0, wxLIST_STATE_SELECTED);
        p_inListUpdate = false;
        return;
    }
    this->loadMachine(name);
}


bool WxMachineRegister::selectMachine(string refName)
{
    int idx = findRow(refName);
    if (idx < 0)
        return false;
    if (refName == p_loadedName)
        return true;
    if (!resolveUnsaved("Discard Changes"))
        return false;
    this->loadMachine(refName);
    return true;
}


bool WxMachineRegister::showPage(const string& name)
{
    static const char* names[] = { "machine", "connection", "codes", "job",
                                   "queues" };
    for (size_t i = 0; i < p_book->GetPageCount() && i < 5; i++)
        if (name == names[i])
        {
            p_book->SetSelection(i);
            return true;
        }
    return false;
}


//  ---- loading a machine ---------------------------------------------------

//  Every CONFIG key of one code, in the spelling the readers document.
static vector<string> codeKeys(const string& code)
{
    vector<string> k;
    k.push_back(code);
    k.push_back(code + "Command");
    k.push_back(code + "Environment");
    k.push_back(code + "FilesToRemove");
    k.push_back(code + "PrelimFilesToRemove");
    k.push_back(lowerOf(code) + "_setup");
    k.push_back(lowerOf(code) + "_wrapup");
    return k;
}


MCD* WxMachineRegister::newDraft(const string& refName) const
{
    MCD::Mode mode = p_adminFlag ? MCD::AdminMode
                   : remoteClient() ? MCD::RemoteMode : MCD::UserMode;
    string site = string(Ecce::ecceHome()) + "/siteconfig/CONFIG." + refName;
    string user = withoutSlash(Ecce::realUserPrefPath()) + "/CONFIG." + refName;

    MCD* draft = new MCD(mode, site, user);

    //  Provenance from gensub itself, so the tags are what a job gets.
    string err;
    string out = explain(refName, err);
    if (err.empty())
        draft->loadExplain(out, err);
    if (!err.empty())
    {
        fprintf(stderr, "[MACHREG] %sexplain failed: %s\n",
                p_scripted ? "FAIL " : "", err.c_str());
        vector<string> keys;
        for (size_t i = 0; i < p_codeNames.size(); i++)
            for (const string& k : codeKeys(p_codeNames[i]))
                keys.push_back(k);
        for (size_t i = 0; i < p_cfgRows.size(); i++)
            keys.push_back(p_cfgRows[i].key);
        keys.push_back("setup");
        keys.push_back("wrapup");
        for (int i = 1; i < (int)p_qmgrChoice->GetCount(); i++)
            keys.push_back((string)p_qmgrChoice->GetString(i));
        draft->loadFiles(keys);
    }
    for (size_t i = 0; i < p_codeNames.size(); i++)
        for (const string& k : codeKeys(p_codeNames[i]))
            draft->ensureKey(k);
    for (size_t i = 0; i < p_cfgRows.size(); i++)
        draft->ensureKey(p_cfgRows[i].key);
    draft->ensureKey("setup");
    draft->ensureKey("wrapup");
    for (int i = 1; i < (int)p_qmgrChoice->GetCount(); i++)
        draft->ensureKey((string)p_qmgrChoice->GetString(i));
    return draft;
}


//  GENSUB_EXPLAIN=1 gensub: the effective value of every key and the layers
//  under it.  In admin mode the user's own files are hidden from it.
string WxMachineRegister::explain(const string& refName, string& err) const
{
    string dir = (string)wxFileName::GetTempDir() + "/ecce-machreg-XXXXXX";
    vector<char> tmpl(dir.begin(), dir.end());
    tmpl.push_back('\0');
    if (mkdtemp(&tmpl[0]) == NULL)
    {
        err = "cannot make a temporary directory";
        return "";
    }
    dir = &tmpl[0];

    string params = dir + "/params";
    {
        std::ofstream f(params.c_str());
        f << " -H " << (refName.empty() ? "unnamed" : refName) << "\n"
          << " -Q Shell\n -c NWChem\n -d localhost\n -n 1\n -N 1\n"
          << " -r " << dir << "\n -i a\n -o a\n -f " << dir << "/submit__x\n";
    }
    wxString cmd = "perl \"" + string(Ecce::ecceHome()) +
                   "/scripts/gensub\" -p \"" + params + "\"";
    wxExecuteEnv env;
    wxGetEnvMap(&env.env);
    env.env["GENSUB_EXPLAIN"] = "1";
    if (p_adminFlag)
        env.env["ECCE_REALUSERHOME"] = dir;

    wxArrayString lines, errors;
    long rc = wxExecute(cmd, lines, errors, wxEXEC_SYNC, &env);
    string out;
    for (size_t i = 0; i < lines.GetCount(); i++)
        out += (string)lines[i] + "\n";
    if (rc != 0)
    {
        err = "gensub exited with " + std::to_string(rc);
        if (errors.GetCount() > 0)
            err += ": " + (string)errors[0];
    }

    unlink(params.c_str());
    rmdir(dir.c_str());
    return out;
}


void WxMachineRegister::loadMachine(const string& refName)
{
    RefMachine* ref = RefMachine::refLookup(refName.c_str());
    int idx = findRow(refName);
    if (ref == NULL || idx < 0)
        return;

    bool another = refName != p_loadedName;
    p_slctRgstn = ref;
    p_loadedName = refName;
    p_loadedFrom = p_rows[idx].from;
    delete p_draft;
    p_draft = newDraft(refName);
    this->draftToControls();
    if (another)                  // a pane opened by hand stays on a save
        setAdvanced(jobsIndex() != 0);

    p_inListUpdate = true;
    p_list->SetItemState(idx, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    p_list->EnsureVisible(idx);
    p_inListUpdate = false;
}


void WxMachineRegister::showNewMachine()
{
    p_slctRgstn = NULL;
    p_loadedName = "";
    p_loadedFrom = "";
    p_inListUpdate = true;
    for (long i = 0; i < p_list->GetItemCount(); i++)
        p_list->SetItemState(i, 0, wxLIST_STATE_SELECTED);
    p_inListUpdate = false;
    delete p_draft;
    p_draft = newDraft("");
    this->draftToControls();
}


//  Fill every widget from the registration and the draft, then take the
//  result as the baseline so nothing counts as changed.
void WxMachineRegister::draftToControls()
{
    p_inCtrlUpdate = true;

    if (p_slctRgstn != NULL)
    {
        p_refName->SetValue(p_slctRgstn->refname());
        p_fullName->SetValue(p_slctRgstn->fullname());
        p_vendor->SetValue(p_slctRgstn->vendor());
        p_model->SetValue(p_slctRgstn->model());
        p_proc->SetValue(p_slctRgstn->proctype());
        int nodes = p_slctRgstn->nodes();
        p_procs->SetValue(p_slctRgstn->proccount());
        p_nodes->SetValue((nodes > 0) ? nodes : 1);
        p_allocAccts->SetValue(
            p_slctRgstn->launchOptions().find("AA") != string::npos);
    }
    else
    {
        p_refName->Clear();
        p_fullName->Clear();
        p_vendor->Clear();
        p_model->Clear();
        p_proc->Clear();
        p_procs->SetValue(1);
        p_nodes->SetValue(1);
        p_allocAccts->SetValue(false);
    }
    p_autoRefName = "";

    for (size_t i = 0; i < p_codePaths.size(); i++)
    {
        string v;
        p_draft->effective(p_codeNames[i], v);
        p_codePaths[i]->SetValue(v);
    }
    this->fillCodeList();
    this->cfgToControls();

    if (p_slctRgstn != NULL)
    {
        this->loadQueues(p_slctRgstn->refname());
        this->fillQueues();
        this->showQueue();
    }
    else
        this->clearQueues();
    this->blocksToControls();

    p_deleteButton->Enable(p_slctRgstn != NULL &&
                           (p_adminFlag || p_loadedFrom == "yours"));
    p_inCtrlUpdate = false;

    this->showSiteBanner();
    this->refreshLocality();
    this->syncDraft();
    p_draft->markSaved();
    this->updateDirty();
}


void WxMachineRegister::showSiteBanner()
{
    if (fromSite())
    {
        //  The info bar does not wrap, so the line breaks are explicit.
        string site = remoteClient() ? "server" : "site";
        string what = remoteClient()
            ? "'" + p_loadedName + "' is published by the ECCE server,\n"
              "shared by everyone using it."
            : "'" + p_loadedName + "' is a site machine, shared by everyone\n"
              "using this installation.";
        p_infoText->SetLabel(what + " Saving stores your own copy;\n"
                             "deleting your copy brings the " + site +
                             " one back.");
        p_info->Show(true);
        p_info->Layout();
    }
    else
        p_info->Show(false);
    this->growToFitSizer();
}


//  ---- queues --------------------------------------------------------------

void WxMachineRegister::clearQueues()
{
    p_qmgrChoice->SetSelection(0);
    p_allocAccts->SetValue(false);
    p_queues.clear();
    p_queueChoice->Clear();
    p_qMinProcs->SetValue(1);
    p_qMaxProcs->SetValue(1);
    p_qMaxWall->SetValue(0);
    p_qMaxMem->SetValue(0);
    p_qMinScratch->SetValue(0);
    p_queueName->Clear();
    p_queueFormBase = queueFormRow();
}


void WxMachineRegister::loadQueues(const string& refName)
{
    p_queues.clear();
    p_qmgrChoice->SetSelection(0);

    RefMachine *mref = RefMachine::refLookup(refName);
    vector<string*> *qnames = mref ? mref->queues() : NULL;
    if (qnames == NULL)
        return;

    const QueueManager *qmgr = QueueManager::lookup(refName);
    if (qmgr == NULL)
        return;

    p_qmgrChoice->SetStringSelection((wxString)(qmgr->queueMgrName()));
    for (size_t idx = 0; idx < qnames->size(); idx++)
    {
        const Queue *queue = qmgr->queue(*(*qnames)[idx]);
        MCD::QueueRow r;
        r.name = *(*qnames)[idx];
        r.minProcs = queue->minProcessors();
        r.maxProcs = queue->maxProcessors();
        r.maxWall = queue->runLimit();
        r.maxMem = queue->memLimit();
        r.minScratch = queue->scratchLimit();
        p_queues.push_back(r);
    }
}


void WxMachineRegister::fillQueues()
{
    p_queueChoice->Clear();
    for (size_t idx = 0; idx < p_queues.size(); idx++)
        p_queueChoice->Append((wxString)(p_queues[idx].name));
    if (!p_queues.empty())
        p_queueChoice->SetSelection(0);
}


//  Show the values of the named queue, or the first.
void WxMachineRegister::showQueue(const string& name)
{
    bool was = p_inCtrlUpdate;
    p_inCtrlUpdate = true;

    p_queueName->SetValue("");
    if (!p_queues.empty())
    {
        size_t pos = 0;
        while (pos < p_queues.size() && p_queues[pos].name != name)
            pos++;
        if (pos >= p_queues.size())
            pos = 0;
        const MCD::QueueRow& r = p_queues[pos];

        p_queueName->SetValue(r.name);
        p_qMinProcs->SetValue(r.minProcs != (unsigned)INT_MAX && r.minProcs != 0
                              ? r.minProcs : 1);
        p_qMaxProcs->SetValue(r.maxProcs != (unsigned)INT_MAX && r.maxProcs != 0
                              ? r.maxProcs : 1);
        p_qMaxWall->SetValue(r.maxWall != (unsigned)INT_MAX
                             ? r.maxWall / 60.0 : 0.0);
        p_qMaxMem->SetValue(r.maxMem != (unsigned)INT_MAX
                            ? MemoryUnits::mbToGB(r.maxMem) : 0);
        p_qMinScratch->SetValue(r.minScratch != (unsigned)INT_MAX
                                ? MemoryUnits::mbToGB(r.minScratch) : 0);
        p_queueChoice->SetSelection((int)pos);
    }
    else
    {
        p_qmgrChoice->SetSelection(0);
        p_qMinProcs->SetValue(1);
        p_qMaxProcs->SetValue(1);
        p_qMaxWall->SetValue(0);
        p_qMaxMem->SetValue(0);
        p_qMinScratch->SetValue(0);
    }
    p_queueFormBase = queueFormRow();
    p_inCtrlUpdate = was;
}


MCD::QueueRow WxMachineRegister::queueFormRow() const
{
    MCD::QueueRow r;
    r.name = strip((string)p_queueName->GetValue());
    r.minProcs = p_qMinProcs->GetValue();
    r.maxProcs = p_qMaxProcs->GetValue();
    r.maxWall = (unsigned)(p_qMaxWall->GetValue() * 60.0 + 0.5);
    r.maxMem = MemoryUnits::gbToMB(p_qMaxMem->GetValue());
    r.minScratch = MemoryUnits::gbToMB(p_qMinScratch->GetValue());
    return r;
}


//  An edit in the queue form that Add/Update Queue has not applied yet
//  counts as unsaved.
bool WxMachineRegister::queueFormDiffers() const
{
    return !(queueFormRow() == p_queueFormBase);
}


void WxMachineRegister::updateQueueButtons()
{
    string name = strip((string)p_queueName->GetValue());
    bool exists = false;
    for (size_t i = 0; i < p_queues.size(); i++)
        if (p_queues[i].name == name)
            exists = true;

    wxString label = exists ? "Update Queue" : "Add Queue";
    if (p_queueApply->GetLabel() != label)
        p_queueApply->SetLabel(label);
    p_queueApply->Enable(!name.empty() && (!exists || queueFormDiffers()));
    p_queueRemoveButton->Enable(exists);
    p_queueClearButton->Enable(!p_queues.empty());
}


bool WxMachineRegister::applyQueueForm()
{
    MCD::QueueRow r = queueFormRow();

    if (r.name.empty())
    {
        displayMessage("You must supply a queue name.");
        return false;
    }
    if (!std::regex_match(r.name, std::regex("^[A-Za-z0-9_.-]+$")))
    {
        //  <machine>.Q separates queue names with spaces and keys with
        //  '|', so a name cannot hold either (#131); processmachine
        //  refuses the same set.
        displayMessage("Queue name '" + r.name + "' is not valid.\n"
            "Queue names may contain only letters, digits, '_', '.' and '-'.");
        return false;
    }

    size_t it = 0;
    while (it < p_queues.size() && p_queues[it].name != r.name)
        it++;
    if (it < p_queues.size())
        p_queues[it] = r;
    else
    {
        p_queues.push_back(r);
        p_queueChoice->Append((wxString)r.name);
    }
    p_queueChoice->SetStringSelection((wxString)r.name);
    p_queueFormBase = queueFormRow();
    this->updateDirty();
    return true;
}


void WxMachineRegister::removeQueue()
{
    string name = strip((string)p_queueName->GetValue());
    if (name.empty())
    {
        displayMessage("You must supply a queue name.");
        return;
    }
    for (size_t pos = 0; pos < p_queues.size(); pos++)
    {
        if (p_queues[pos].name == name)
        {
            p_queues.erase(p_queues.begin() + pos);
            break;
        }
    }
    this->fillQueues();
    this->showQueue();
    this->updateDirty();
}


void WxMachineRegister::onQueueChoice(wxCommandEvent& event)
{
    if (event.GetString() != p_queueName->GetValue())
        this->showQueue((string)(event.GetString()));
    this->updateDirty();
    event.Skip(false);
}


void WxMachineRegister::onQueueApply(wxCommandEvent&)
{
    this->applyQueueForm();
}


void WxMachineRegister::onQueueRemove(wxCommandEvent&)
{
    this->removeQueue();
}


void WxMachineRegister::onQueueClear(wxCommandEvent&)
{
    this->clearQueues();
    this->updateDirty();
}


//  ---- dirty tracking ------------------------------------------------------

//  The value the inherited layers give (false: none, or the last cleared it).
static bool inheritedValue(const MCD::KeyState& ks, bool cppOnly, string& v)
{
    for (size_t i = ks.inherited.size(); i-- > 0; )
    {
        const MCD::Layer& l = ks.inherited[i];
        if (cppOnly && (l.source == "submit.site" || l.source == "vendor"))
            continue;
        v = l.value;
        return l.hasValue;
    }
    return false;
}


//  ---- the Connection tab's keys ---------------------------------------------

static bool isOneOf(const string& v, const char* a, const char* b)
{
    string l = lowerOf(strip(v));
    return l == a || l == b;
}

static void qmgrCommandNames(const string& lq, string& a, string& b,
                             string& dir)
{
    a = "sbatch"; b = "squeue"; dir = "/opt/slurm/bin";
    if (lq == "pbs" || lq == "sge") { a = "qsub"; b = "qstat"; dir = "/opt/pbs/bin"; }
    else if (lq == "lsf") { a = "bsub"; b = "bjobs"; dir = "/opt/lsf/bin"; }
    else if (lq == "moab") { a = "msub"; b = "showq"; dir = "/opt/moab/bin"; }
    else if (lq == "htcondor") { a = "condor_submit"; b = "condor_q"; dir = "/opt/condor/bin"; }
}

static string qmgrCommands(const string& lq)
{
    string a, b, dir;
    qmgrCommandNames(lq, a, b, dir);
    return "Directory holding " + a + ", " + b + " and the other scheduler "
           "commands";
}


WxMachineRegister::CfgRow* WxMachineRegister::cfgRow(const string& key)
{
    string k = lowerOf(key);
    for (size_t i = 0; i < p_cfgRows.size(); i++)
        if (lowerOf(p_cfgRows[i].key) == k)
            return &p_cfgRows[i];
    return NULL;
}


//  What applies when no layer sets the key (RefMachine's own defaults).
string WxMachineRegister::cfgDefault(const CfgRow& r) const
{
    switch (r.kind)
    {
        case CfgShell: return "bash";
        case CfgCheck: return "false";
        case CfgCheckYes: return "true";
        case CfgTri: return "no";
        default: return "";
    }
}


//  One spelling per meaning, as RefMachine reads the values.
string WxMachineRegister::cfgCanon(const CfgRow& r, const string& raw) const
{
    switch (r.kind)
    {
        case CfgCheck:
            return isOneOf(raw, "true", "yes") ? "true" : "false";
        case CfgCheckYes:
            return isOneOf(raw, "false", "no") ? "false" : "true";
        case CfgTri:
            return isOneOf(raw, "true", "yes") ? "yes"
                 : isOneOf(raw, "false", "no") ? "no" : "auto";
        default:
            return strip(raw);
    }
}


string WxMachineRegister::cfgCtrlText(const CfgRow& r) const
{
    switch (r.kind)
    {
        case CfgCheck:
        case CfgCheckYes:
            return static_cast<wxCheckBox*>(r.ctrl)->IsChecked() ? "true"
                                                                 : "false";
        case CfgShell:
        case CfgTri:
            return (string)static_cast<wxChoice*>(r.ctrl)->GetStringSelection();
        default:
            return strip((string)static_cast<wxTextCtrl*>(r.ctrl)->GetValue());
    }
}


string WxMachineRegister::cfgHint(const CfgRow& r) const
{
    const MCD::KeyState* ks = p_draft ? p_draft->state(r.key) : NULL;
    if (ks != NULL && ks->edit == MCD::Clear)
        return "(no value)";
    string k = lowerOf(r.key);
    if (k == "sourcefile") return "e.g. /etc/profile.d/modules.sh";
    if (k == "frontendmachine") return "e.g. login.example.org";
    if (k == "frontendbypass") return "e.g. .example.org";
    if (k == "perlpath") return "e.g. /usr/bin/perl";
    if (k == "libpath") return "e.g. /opt/lib";
    if (k == "xappspath") return "e.g. /usr/X11R6/bin";
    if (k == "qmgrpath")
    {
        string a, b, dir;
        qmgrCommandNames(lowerOf((string)p_qmgrChoice->GetStringSelection()),
                         a, b, dir);
        return "e.g. " + dir;
    }
    return "";
}


void WxMachineRegister::cfgToControl(const CfgRow& r)
{
    string v;
    bool has = p_draft->effective(r.key, v, !r.gensubOnly);
    string show = cfgCanon(r, has ? v : cfgDefault(r));
    switch (r.kind)
    {
        case CfgText:
        {
            wxTextCtrl* t = static_cast<wxTextCtrl*>(r.ctrl);
            t->SetValue(has ? v : "");
            t->SetHint(cfgHint(r));
            break;
        }
        case CfgCheck:
        case CfgCheckYes:
            static_cast<wxCheckBox*>(r.ctrl)->SetValue(show == "true");
            break;
        default:
        {
            wxChoice* c = static_cast<wxChoice*>(r.ctrl);
            if (!c->SetStringSelection(show))
            {
                c->Append(show);        // a shell this list does not know
                c->SetStringSelection(show);
            }
        }
    }
}


void WxMachineRegister::cfgToControls()
{
    for (size_t i = 0; i < p_cfgRows.size(); i++)
        cfgToControl(p_cfgRows[i]);
    jobsToRadios();

    //  Legacy: shown only when something sets it.
    if (CfgRow* x = cfgRow("xappsPath"))
    {
        string v;
        const MCD::KeyState* ks = p_draft->state(x->key);
        bool show = p_draft->effective(x->key, v, true) ||
                    (ks != NULL && ks->edit != MCD::Inherit);
        x->name->Show(show);
        x->ctrl->Show(show);
        x->tag->Show(show);
        x->undoBox->Show(show);
        p_cfgPage->Layout();
    }
}


static string layerTagName(const MCD::Layer& l, bool remote)
{
    if (remote && l.source != "user")
        return "server";
    return l.source == "site" ? "site" : "site defaults";
}


static string plainTag(MCD::Tag t)
{
    switch (t)
    {
        case MCD::TagServer: return "from server";
        case MCD::TagSite:
        case MCD::TagSiteDefaults: return "from site";
        case MCD::TagYours:
        case MCD::TagNoValue: return "your value";
        case MCD::TagSiteEditing: return "site value";
        default: return "not set";
    }
}


//  The tag and its tooltip: the file the value is in, and what it overrides.
void WxMachineRegister::cfgTagInfo(const string& key, bool cppOnly,
                                   MCD::Tag& t, string& tip) const
{
    const MCD::KeyState* ks = p_draft->state(key);
    t = MCD::TagDefault;
    tip = "Not set; ECCE's built-in default applies";
    if (ks == NULL)
        return;
    t = p_draft->tag(key, cppOnly);

    string inh, inhFile, inhTag;
    bool hasInh = inheritedValue(*ks, cppOnly, inh);
    for (size_t k = ks->inherited.size(); k-- > 0 && inhTag.empty(); )
        if (!cppOnly || !(ks->inherited[k].source == "submit.site" ||
                          ks->inherited[k].source == "vendor"))
        {
            inhTag = layerTagName(ks->inherited[k], remoteClient());
            inhFile = ks->inherited[k].file;
        }
    string file = p_draft->editedFile();
    string shown = inh.find('\n') == string::npos ? " \"" + inh + "\"" : "";
    string over = hasInh ? "\nOverrides the " + inhTag + " value" + shown +
                           " (" + inhFile + ")" : "";
    switch (t)
    {
        case MCD::TagDefault: break;
        case MCD::TagServer:
            tip = "Published by the ECCE server: " + inhFile + "\nChanges "
                  "go to your own settings on this computer";
            break;
        case MCD::TagSite:
        case MCD::TagSiteDefaults:
            tip = "From " + inhFile;
            break;
        case MCD::TagYours:
            tip = "Your value, stored in " + file + over;
            break;
        case MCD::TagSiteEditing:
            tip = "Stored in " + file;
            break;
        case MCD::TagNoValue:
            tip = "You cleared it; stored in " + file + over;
            break;
    }
}


//  What undo does for a key: 1 puts back the value as last loaded or saved
//  (there is an unsaved change); 2 drops the user's saved value so the
//  site's applies; 0 nothing, and no undo button.
int WxMachineRegister::undoCase(const string& key, bool cppOnly) const
{
    const MCD::KeyState* ks = p_draft ? p_draft->state(key) : NULL;
    if (ks == NULL)
        return 0;
    if (p_draft->changed(key))
        return 1;
    string inh;
    return ks->edit != MCD::Inherit && inheritedValue(*ks, cppOnly, inh) ? 2 : 0;
}


string WxMachineRegister::undoTip(int undoCase, const string& what) const
{
    if (undoCase == 1)
        return "Undo this change";
    return p_adminFlag ? "Use the site default"
         : remoteClient() ? "Use the server " + what
         : "Use the site " + what;
}


void WxMachineRegister::undoKey(const string& key, bool revertOnly)
{
    if (revertOnly)
    {
        if (p_draft->changed(key))
            p_draft->revert(key);
    }
    else
        p_draft->useInherited(key);
}


//  Refresh each tag, tooltip and undo button from the draft.
void WxMachineRegister::cfgTags()
{
    if (p_draft == NULL)
        return;
    for (size_t i = 0; i < p_cfgRows.size(); i++)
    {
        const CfgRow& r = p_cfgRows[i];
        if (r.tag == NULL)
            continue;
        const MCD::KeyState* ks = p_draft->state(r.key);
        if (ks == NULL)
            continue;
        MCD::Tag t;
        string tip;
        cfgTagInfo(r.key, !r.gensubOnly, t, tip);
        wxString text = plainTag(t);
        if (r.tag->GetLabel() != text)
            r.tag->SetLabel(text);
        r.tag->SetForegroundColour(wxSystemSettings::GetColour(
            (t == MCD::TagYours || t == MCD::TagNoValue ||
             t == MCD::TagSiteEditing) ? wxSYS_COLOUR_WINDOWTEXT
                                       : wxSYS_COLOUR_GRAYTEXT));
        if (r.tag->GetToolTipText() != tip)
            r.tag->SetToolTip(tip);
        int uc = undoCase(r.key, !r.gensubOnly);
        if (r.undo->IsShown() != (uc != 0))
            r.undo->Show(uc != 0);
        r.undo->SetToolTip(undoTip(uc, "value"));
        if (r.kind == CfgText)
            static_cast<wxTextCtrl*>(r.ctrl)->SetHint(cfgHint(r));
    }

    //  The job-handling group shows the two keys as one.
    CfgRow* a = cfgRow("noRemoteAccess");
    CfgRow* b = cfgRow("userSubmit");
    if (a && b && p_jobsTag)
    {
        MCD::Tag ta, tb;
        string tipa, tipb;
        cfgTagInfo(a->key, true, ta, tipa);
        cfgTagInfo(b->key, true, tb, tipb);
        bool own = ta != MCD::TagDefault && (ta == MCD::TagYours ||
                   ta == MCD::TagNoValue || ta == MCD::TagSiteEditing);
        bool ownb = tb == MCD::TagYours || tb == MCD::TagNoValue ||
                    tb == MCD::TagSiteEditing;
        MCD::Tag t = (own || ownb) ? (own ? ta : tb)
                   : ta != MCD::TagDefault ? ta : tb;
        string tip = ta == MCD::TagDefault ? tipb
                   : tb == MCD::TagDefault ? tipa : tipa + "\n" + tipb;
        wxString text = plainTag(t);
        if (p_jobsTag->GetLabel() != text)
            p_jobsTag->SetLabel(text);
        p_jobsTag->SetForegroundColour(wxSystemSettings::GetColour(
            (own || ownb) ? wxSYS_COLOUR_WINDOWTEXT : wxSYS_COLOUR_GRAYTEXT));
        if (p_jobsTag->GetToolTipText() != tip)
            p_jobsTag->SetToolTip(tip);
        jobsLine();
        int ua = undoCase(a->key, true), ub = undoCase(b->key, true);
        int uc = (ua == 1 || ub == 1) ? 1 : (ua || ub) ? 2 : 0;
        if (p_jobsUndo->IsShown() != (uc != 0))
            p_jobsUndo->Show(uc != 0);
        p_jobsUndo->SetToolTip(undoTip(uc, "value"));
    }
}


//  noRemoteAccess wins over userSubmit, as Launch applies them.
int WxMachineRegister::jobsIndex() const
{
    CfgRow* a = const_cast<WxMachineRegister*>(this)->cfgRow("noRemoteAccess");
    CfgRow* b = const_cast<WxMachineRegister*>(this)->cfgRow("userSubmit");
    if (a && cfgCtrlText(*a) == "true")
        return 2;
    return b && cfgCtrlText(*b) == "true" ? 1 : 0;
}


void WxMachineRegister::jobsToRadios()
{
    if (p_jobsCheck[1] == NULL)
        return;
    int idx = jobsIndex();
    p_jobsCheck[1]->SetValue(idx == 1);
    p_jobsCheck[2]->SetValue(idx == 2);
    p_jobsCheck[1]->Enable(idx != 2);
    p_jobsCheck[2]->Enable(idx != 1);
    //  Something other than normal: show the choice.
    if (idx != 0 && p_advanced != NULL && !p_advanced->IsShown())
        setAdvanced(true);
    jobsLine();
}


//  The read-only line on the main area: the mode in words, the exceptions
//  with a warning icon.
void WxMachineRegister::jobsLine()
{
    if (p_jobsMode == NULL)
        return;
    int idx = jobsIndex();
    wxString text = idx == 0 ? "ECCE submits the job"
                  : idx == 1 ? "Interactive submission"
                  : "Files only, not submitted";
    bool layout = false;
    if (p_jobsMode->GetLabel() != text)
        { p_jobsMode->SetLabel(text); layout = true; }
    if (p_jobsIcon->IsShown() != (idx != 0))
        { p_jobsIcon->Show(idx != 0); layout = true; }
    if (layout)
        p_cfgPage->Layout();
}


//  The radios are the editor, the two hidden checkboxes carry the keys.
void WxMachineRegister::jobsFromRadios()
{
    if (p_jobsCheck[1] == NULL)
        return;
    int sel = p_jobsCheck[2]->GetValue() ? 2 : p_jobsCheck[1]->GetValue() ? 1 : 0;
    //  Ticking one disables the other.
    p_jobsCheck[1]->Enable(sel != 2);
    p_jobsCheck[2]->Enable(sel != 1);
    if (sel == jobsIndex())
        return;                 // e.g. both keys set: leave them as they are
    static_cast<wxCheckBox*>(cfgRow("noRemoteAccess")->ctrl)->SetValue(sel == 2);
    static_cast<wxCheckBox*>(cfgRow("userSubmit")->ctrl)->SetValue(sel == 1);
}


//  Back to the inherited value; "jobs" is both job-handling keys.
void WxMachineRegister::cfgUndo(const string& key)
{
    if (p_draft == NULL)
        return;
    if (block(key) != NULL)
    {
        this->blockUndo(key);
        return;
    }
    if (codeLine(key) != NULL)
    {
        this->codeLineUndo(key);
        return;
    }
    this->syncDraft();
    vector<string> ks;
    if (lowerOf(key) == "jobs")
    {
        ks.push_back("noRemoteAccess");
        ks.push_back("userSubmit");
    }
    else
        ks.push_back(key);
    bool anyChanged = false;
    for (size_t i = 0; i < ks.size(); i++)
        anyChanged = anyChanged || p_draft->changed(ks[i]);
    p_inCtrlUpdate = true;
    for (size_t i = 0; i < ks.size(); i++)
        if (CfgRow* r = cfgRow(ks[i]))
        {
            undoKey(r->key, anyChanged);
            cfgToControl(*r);
        }
    jobsToRadios();
    p_inCtrlUpdate = false;
    this->updateDirty();
}


void WxMachineRegister::onCfgUndo(wxCommandEvent& event)
{
    wxObject* o = event.GetEventObject();
    if (o == p_jobsUndo)
        cfgUndo("jobs");
    for (size_t i = 0; i < p_cfgRows.size(); i++)
        if (p_cfgRows[i].undo == o)
            cfgUndo(p_cfgRows[i].key);
}


void WxMachineRegister::syncKey(MCD* draft, const string& key,
                                const string& text, bool cppOnly)
{
    string t = strip(text);
    const MCD::KeyState* ks = draft->state(key);
    if (ks == NULL)
        return;

    string cur;
    bool has = draft->effective(key, cur, cppOnly);
    if ((has && t == cur) || (!has && t.empty()))
        return;

    string inh;
    bool hasInh = inheritedValue(*ks, cppOnly, inh);
    if (t.empty())
    {
        //  An emptied field overrides an inherited value with "no value".
        if (hasInh)
            draft->clear(key);
        else
            draft->useInherited(key);
    }
    else if (hasInh && t == inh)
        draft->useInherited(key);
    else
        draft->setValue(key, t);
}


//  The other kinds hold a fixed set of values; "true" and "yes" are one
//  value, so they are compared in canonical form and written as chosen.
void WxMachineRegister::syncCfg(MCD* draft, const CfgRow& r)
{
    bool cpp = !r.gensubOnly;
    if (r.kind == CfgText)
    {
        syncKey(draft, r.key, cfgCtrlText(r), cpp);
        return;
    }
    const MCD::KeyState* ks = draft->state(r.key);
    if (ks == NULL)
        return;

    string t = cfgCtrlText(r), cur;
    bool has = draft->effective(r.key, cur, cpp);
    if (t == cfgCanon(r, has ? cur : cfgDefault(r)))
        return;

    string inh;
    bool hasInh = inheritedValue(*ks, cpp, inh);
    if (t == cfgCanon(r, hasInh ? inh : cfgDefault(r)))
        draft->useInherited(r.key);
    else
        draft->setValue(r.key, t);
}


//  Push the form into the draft; what differs from the baseline is "unsaved".
void WxMachineRegister::syncDraft()
{
    if (p_draft == NULL)
        return;

    MCD::Content& c = p_draft->content();
    c.line.name = strip((string)p_refName->GetValue());
    c.line.host = strip((string)p_fullName->GetValue());
    c.line.vendor = strip((string)p_vendor->GetValue());
    c.line.model = strip((string)p_model->GetValue());
    c.line.proc = strip((string)p_proc->GetValue());
    c.line.procs = p_procs->GetValue();
    c.line.nodes = p_nodes->GetValue();
    c.line.allocationAccount = p_allocAccts->IsChecked();
    c.qmgr = (string)p_qmgrChoice->GetStringSelection();
    c.queues = p_queues;

    this->blocksRetarget();
    syncKeys(p_draft);
}


//  The managed CONFIG keys: each code's path and the Connection tab's.
void WxMachineRegister::syncKeys(MCD* draft)
{
    for (size_t i = 0; i < p_codePaths.size(); i++)
        syncKey(draft, p_codeNames[i], (string)p_codePaths[i]->GetValue());
    jobsFromRadios();
    for (size_t i = 0; i < p_cfgRows.size(); i++)
        syncCfg(draft, p_cfgRows[i]);
    this->syncCodeLines(draft);
    this->syncBlocks(draft);
}


bool WxMachineRegister::isDirty()
{
    this->syncDraft();
    return p_draft != NULL && (p_draft->isDirty() || queueFormDiffers());
}


bool WxMachineRegister::hasMinimalInput()
{
    return !strip((string)p_fullName->GetValue()).empty()
        && !strip((string)p_refName->GetValue()).empty();
}


//  The files Save writes for the visible tab and machine.
string WxMachineRegister::editedBase() const
{
    return p_adminFlag ? string(Ecce::ecceHome()) + "/siteconfig" : "~/.ECCE";
}


void WxMachineRegister::updateFooter()
{
    if (p_book == NULL || p_storeNote == NULL || p_cfgRows.empty())
        return;

    string base = editedBase();
    string name = strip((string)p_refName->GetValue());
    if (name.empty())
        name = "<name>";
    string config = base + "/CONFIG." + name;

    string files;
    switch (p_book->GetSelection())
    {
        case 0: files = base + (p_adminFlag ? "/Machines" : "/MyMachines"); break;
        case 4: files = base + "/Queues and " + base + "/" + name + ".Q"; break;
        default: files = config; break;
    }
    wxString note = "Saved in " + files;
    if (p_storeNote->GetLabel() != note)
        p_storeNote->SetLabel(note);

    string qm = (string)p_qmgrChoice->GetStringSelection();
    string lq = lowerOf(qm);
    if (CfgRow* q = cfgRow("qmgrPath"))
    {
        static_cast<wxTextCtrl*>(q->ctrl)->SetHint(cfgHint(*q));
        string a, b, dir;
        qmgrCommandNames(lq, a, b, dir);
        wxString lab = "Directory of " + a + ", " + b + ", ...";
        if (q->name->GetLabel() != lab)
            q->name->SetLabel(lab);
        q->ctrl->SetToolTip(qmgrCommands(lq) + " on the remote machine. "
                            "Leave empty if they are on the default PATH.");
    }
}


void WxMachineRegister::updateDirty()
{
    this->updateFooter();
    if (p_inCtrlUpdate || p_draft == NULL || p_closing)
        return;

    bool dirty = this->isDirty();
    this->cfgTags();
    this->blocksTags();
    this->codeLinesTags();
    p_saveButton->Enable(dirty && hasMinimalInput());
    wxString title = dirty ? wxString("*") + TITLE : wxString(TITLE);
    if ((string)this->GetTitle() != (string)title)
        this->SetTitle(title);
    this->updateQueueButtons();
}


void WxMachineRegister::onFieldChanged(wxCommandEvent& event)
{
    if (p_cshTimer != NULL && !p_inCtrlUpdate)
        p_cshTimer->StartOnce(600);
    this->updateDirty();
    event.Skip();
}


void WxMachineRegister::onFullNameText(wxCommandEvent& event)
{
    string refName = (string)(p_fullName->GetValue());
    size_t chpos = refName.find('.');

    //  Don't cut an IP address at its first '.' -- 127.0.0.1 must stay
    //  127.0.0.1, not become "127" (matches RunMgmt::registerLocalMachine).
    bool isAddress = refName.find_first_not_of("0123456789.") == string::npos;

    if (chpos != string::npos && !isAddress)
        refName = refName.substr(0, chpos);

    //  Follow the machine only while Name is empty or still the value
    //  filled in here, so a name the user typed is never overwritten.
    if (!p_inCtrlUpdate) {
        string current = (string)(p_refName->GetValue());
        if (current.empty() || current == p_autoRefName) {
            p_autoRefName = refName;
            p_refName->SetValue(refName);
        }
    }

    this->refreshLocality();
    event.Skip();
}


//  Save / discard / cancel.  Returns the dialog's id.
int WxMachineRegister::confirmUnsaved(const string& discardLabel)
{
    string name = p_loadedName.empty() ? strip((string)p_refName->GetValue())
                                       : p_loadedName;
    string what = name.empty() ? "the new machine" : "'" + name + "'";
    return this->ask("Unsaved Changes", "Save changes to " + what + "?",
                     "Your changes are lost if you do not save them.",
                     wxYES_NO|wxCANCEL|wxICON_QUESTION,
                     "Save", discardLabel, "Cancel");
}


//  True when it is fine to replace what the form holds.
bool WxMachineRegister::resolveUnsaved(const string& discardLabel)
{
    if (!this->isDirty())
        return true;
    int answer = this->confirmUnsaved(discardLabel);
    if (answer == wxID_CANCEL)
        return false;
    if (answer == wxID_YES)
        return this->save();
    return true;
}


//  ---- buttons -------------------------------------------------------------

void WxMachineRegister::onClose(wxCloseEvent& event)
{
    if (event.CanVeto() && !p_closing && !this->resolveUnsaved("Close without Saving"))
    {
        event.Veto();
        return;
    }

    p_closing = true;
    if (!p_scripted)
    {
        Preferences prefs("MachineRegister");
        saveSettings(prefs);
    }
    this->Destroy();
}


void WxMachineRegister::onCloseButton(wxCommandEvent&)
{
    this->Close();
}


void WxMachineRegister::onSave(wxCommandEvent&)
{
    if (p_saveButton->IsEnabled())
        this->save();
}


void WxMachineRegister::onNew(wxCommandEvent&)
{
    if (!this->resolveUnsaved("Discard Changes"))
        return;
    this->showNewMachine();
}


void WxMachineRegister::onDelete(wxCommandEvent&)
{
    this->deleteMachine();
}


void WxMachineRegister::onHelp(wxCommandEvent&)
{
    BrowserHelp help;
    help.showPage(help.URL("ConfigSvrs"));
}


//  ---- saving --------------------------------------------------------------

bool WxMachineRegister::verifyInput()
{
    string refName = strip((string)p_refName->GetValue());
    string fullName = strip((string)p_fullName->GetValue());
    int k = p_qmgrChoice->GetSelection();

    if (refName.empty() || fullName.empty())
    {
        displayMessage("Both Machine and Name must be specified.");
        return false;
    }
    if (refName.find_first_of("/\t\r\n") != string::npos)
    {
        //  The name is part of file names and a tab-separated line.
        displayMessage("Name may not contain '/', tabs or line breaks.");
        return false;
    }
    if (k == 0 && p_queues.size() > 0)
    {
        displayMessage("Queues have been specified but not the Queue Manager that is used.\n"
            "Please set the Queue Manager or delete the queues.");
        return false;
    }
    if (k > 0 && p_queues.size() == 0)
    {
        displayMessage("You have set a Queue Manager but no queues.\n"
            "Please specify one or more queues or set the Queue Manager to None.");
        return false;
    }

    //  ConfigFile cannot write a block with "}" in column 0.
    for (size_t i = 0; i < p_blocks.size(); i++)
    {
        string t = (string)p_blocks[i].user->GetValue();
        if (t.compare(0, 1, "}") == 0 || t.find("\n}") != string::npos)
        {
            displayMessage("A line in the job script text may not start with "
                           "\"}\"; put a space before it.");
            return false;
        }
    }

    //  Values land on one line of CONFIG.<machine>, and "-" there means
    //  "no value".
    vector<ewxTextCtrl*> texts = p_codePaths;
    for (size_t i = 0; i < p_codeLines.size(); i++)
        if (p_codeLines[i].id != "code")
            texts.push_back(static_cast<ewxTextCtrl*>(p_codeLines[i].ctrl));
    for (size_t i = 0; i < p_cfgRows.size(); i++)
        if (p_cfgRows[i].kind == CfgText)
            texts.push_back(static_cast<ewxTextCtrl*>(p_cfgRows[i].ctrl));
    for (size_t i = 0; i < texts.size(); i++)
    {
        string v = strip((string)texts[i]->GetValue());
        if (v.find_first_of("\r\n") != string::npos || v == "-")
        {
            displayMessage("A path may not contain a line break and may not "
                           "be just \"-\".");
            return false;
        }
    }
    return true;
}


//  The values processmachine writes into Machines/MyMachines and the queue
//  files.  CONFIG.<machine> itself is written by ConfigFile (config=external).
string WxMachineRegister::collectSettings() const
{
    typedef ProcessMachine PM;
    string ret;
    string tmp;

    ret += PM::field("siteconfig", StringConverter::toString(p_adminFlag));
    ret += PM::field("config", "external");
    ret += PM::field("machine", strip((string)p_fullName->GetValue()));
    ret += PM::field("name", strip((string)p_refName->GetValue()));

    tmp = p_vendor->GetValue();
    ret += PM::field("vendor", (tmp == "") ? "Unspecified" : tmp);
    tmp = p_model->GetValue();
    ret += PM::field("model", (tmp == "") ? "Unspecified" : tmp);
    tmp = p_proc->GetValue();
    ret += PM::field("processor", (tmp == "") ? "Unspecified" : tmp);

    ret += PM::field("procs", StringConverter::toString(p_procs->GetValue()));
    ret += PM::field("nodes", StringConverter::toString(p_nodes->GetValue()));
    ret += PM::field("ssh", "true");

    //  The list of all known codes, then each code's effective path: the
    //  Machines line lists a code when it has one, inherited or yours.
    tmp = "";
    for (size_t i = 0; i < p_codePaths.size(); i++)
    {
        if (i > 0)
            tmp += ",";
        tmp += p_codeNames[i];
    }
    ret += PM::field("registeredcodes", tmp);
    for (size_t i = 0; i < p_codePaths.size(); i++)
        ret += PM::field(p_codeNames[i], strip((string)p_codePaths[i]->GetValue()));

    ret += PM::field("AA", StringConverter::toString(p_allocAccts->IsChecked()));
    ret += PM::field("qmgr", (string)p_qmgrChoice->GetStringSelection());

    size_t n = p_queues.size();
    ret += PM::field("numQueues", StringConverter::toString((int)n));
    for (size_t i = 0; i < n; i++)
    {
        tmp = "name|" + p_queues[i].name + ",";
        tmp += "minNodes|" + StringConverter::toString((int)p_queues[i].minProcs) + ",";
        tmp += "maxNodes|" + StringConverter::toString((int)p_queues[i].maxProcs) + ",";
        tmp += "maxCPU|" + StringConverter::toString((int)p_queues[i].maxWall) + ",";
        tmp += "maxMemory|" + StringConverter::toString((int)p_queues[i].maxMem) + ",";
        tmp += "minScratch|" + StringConverter::toString((int)p_queues[i].minScratch) + ",";
        ret += PM::field("q" + StringConverter::toString((int)i), tmp);
    }

    return ret;
}


//  Apply the draft's edits to CONFIG.<name> in the edited layer.  A new
//  name gets a draft of its own, since the loaded one belongs to the old.
bool WxMachineRegister::writeConfig(const string& name, string& err)
{
    MCD* draft = p_draft;
    MCD* own = NULL;

    if (name != p_loadedName)
    {
        own = draft = newDraft(name);
        syncKeys(draft);
    }

    ConfigFile f;
    f.setSiteFile(p_adminFlag);
    bool ok = f.load(draft->editedFile());
    if (!ok)
        err = "Cannot read " + draft->editedFile();
    else
        ok = draft->applyTo(f, err) && f.save(&err);
    if (!ok)
        err = "The machine was registered, but its settings file was not "
              "written (" + draft->editedFile() + "): " + err;

    delete own;
    return ok;
}


bool WxMachineRegister::save()
{
    //  Q9: a queue form that was edited but not applied.
    if (queueFormDiffers())
    {
        MCD::QueueRow r = queueFormRow();
        if (r.name.empty())
            this->showQueue();
        else
        {
            int answer = this->ask("Queue Not Applied",
                "Apply the queue form to '" + r.name + "' first?",
                "The queue form has changes that were not added to the "
                "queue list.", wxYES_NO|wxCANCEL|wxICON_QUESTION,
                "Apply", "Discard", "Cancel");
            if (answer == wxID_CANCEL)
                return false;
            if (answer == wxID_YES)
            {
                if (!this->applyQueueForm())
                    return false;
            }
            else
                this->showQueue((string)p_queueChoice->GetStringSelection());
        }
    }

    this->syncDraft();
    this->blocksCheckCsh();
    if (!this->verifyInput())
        return false;

    string name = strip((string)p_refName->GetValue());
    int status = ProcessMachine::run("type=accept" + collectSettings());
    if (status != 0)
    {
        displayMessage("Unable to save changes to machine registration!");
        return false;
    }

    string err;
    if (!this->writeConfig(name, err))
    {
        displayMessage(err);
        return false;
    }

    this->redo(name);
    this->notifyUpdate();
    return true;
}


//  Re-read everything from disk and show the machine again.
void WxMachineRegister::redo(const string& refName)
{
    RefMachine::finalize();
    //  QueueManager caches its whole extent on first use; without dropping
    //  it a queue just written is not visible until the app restarts.
    QueueManager::finalize();

    this->loadMachinesList();
    if (findRow(refName) >= 0)
        this->loadMachine(refName);
    else
        this->showNewMachine();
}


//  What a delete removes, one line each, for the confirmation.
string WxMachineRegister::removalList(const string& refName) const
{
    string base = editedDir(p_adminFlag);
    string list = "  " + base + (p_adminFlag ? "/Machines" : "/MyMachines") +
                  " (the line for '" + refName + "')\n";

    SFile config(base + "/CONFIG." + refName);
    if (config.exists())
        list += "  " + base + "/CONFIG." + refName +
                " (including any settings you wrote by hand)\n";
    SFile qfile(base + "/" + refName + ".Q");
    if (qfile.exists())
        list += "  " + base + "/" + refName + ".Q\n";

    std::ifstream queues((base + "/Queues").c_str());
    string line;
    bool listed = false;
    while (std::getline(queues, line))
        if (line.compare(0, refName.size() + 1, refName + "|") == 0)
            listed = true;
    if (listed)
        list += "  " + base + "/Queues (the lines for '" + refName + "')\n";
    return list;
}


bool WxMachineRegister::deleteMachine()
{
    if (p_loadedName.empty() || !p_deleteButton->IsEnabled())
        return false;
    string refName = p_loadedName;

    string detail = "These are removed:\n" + removalList(refName);
    vector<string> *siteNames = RefMachine::referenceNames(RefMachine::siteMachines);
    bool shadowing = !p_adminFlag &&
        std::find(siteNames->begin(), siteNames->end(), refName) != siteNames->end();
    delete siteNames;
    if (shadowing)
        detail += "\nThe site version of '" + refName + "' is used again "
                  "afterwards.";
    detail += "\nThis cannot be undone.";

    int answer = this->ask("Delete Machine",
        "Delete the registration for '" + refName + "'?", detail,
        wxYES_NO|wxICON_WARNING, "Delete", "Cancel", "");
    if (answer != wxID_YES)
        return false;

    string settings = "type=delete";
    settings += ProcessMachine::field("siteconfig",
                                      StringConverter::toString(p_adminFlag));
    settings += ProcessMachine::field("name", refName);
    if (ProcessMachine::run(settings) != 0)
    {
        displayMessage("Unable to delete registered machine!");
        return false;
    }

    this->redo(refName);
    this->notifyUpdate();
    return true;
}


//  ---- the Job script tab -----------------------------------------------------

//  Where the inherited text comes from, in words.
static string blockSource(const MCD::Layer& l, bool remote)
{
    string name = l.file;
    size_t slash = name.rfind('/');
    if (slash != string::npos)
        name = name.substr(slash + 1);
    if (remote && l.source != "user")
        return "the ECCE server: " + name;
    if (l.source == "site")
        return "the site: " + name;
    return "the site's default: " + name;
}


//  The header block follows the queue manager chosen on the Queues tab.
void WxMachineRegister::blocksRetarget()
{
    BlockRow* h = block("header");
    if (h == NULL || p_draft == NULL || headerKey() == h->key)
        return;
    syncBlock(p_draft, *h);
    bool was = p_inCtrlUpdate;
    p_inCtrlUpdate = true;
    blockToControl(*h);
    p_inCtrlUpdate = was;
}


void WxMachineRegister::blocksToControls()
{
    for (size_t i = 0; i < p_blocks.size(); i++)
        blockToControl(p_blocks[i]);
    blocksCheckCsh();
}


void WxMachineRegister::blockToControl(BlockRow& b)
{
    bool header = b.id == "header";
    b.key = blockKey(b.id);
    string qm = (string)p_qmgrChoice->GetStringSelection();

    if (header)
    {
        b.heading->SetLabel("Request lines for " + qm);
        p_jobNote->SetLabel(b.key.empty()
            ? "No queue manager is set (Queues tab): the job runs directly on "
              "the machine and needs no request lines."
            : "Queue manager: " + qm + " (change it on the Queues tab). The "
              "lines below go at the top of the job script and tell " + qm +
              " what the job needs.");
        p_jobNote->Wrap(600);
    }

    const MCD::KeyState* ks = b.key.empty() ? NULL : p_draft->state(b.key);
    b.box->ShowItems(ks != NULL);
    if (header)
    {
        if (p_condorGrid != NULL)
            p_condorGrid->ShowItems(lowerOf(qm) == "htcondor");
    }
    if (ks != NULL)
    {
        b.heading->SetToolTip("CONFIG key: " + ks->name);
        string inh;
        bool hasInh = inheritedValue(*ks, false, inh);
        if (hasInh)
            b.siteLabel->SetLabel("From " + blockSource(
                ks->inherited.back(), remoteClient()) + " (read-only)");
        else
            b.siteLabel->SetLabel(p_adminFlag
                ? "Nothing from the site's defaults."
                : "The site gives no text here.");
        b.site->ChangeValue(hasInh ? inh : "");
        b.site->Show(hasInh);

        bool cleared = ks->edit == MCD::Clear;
        b.user->ChangeValue(ks->edit == MCD::Set ? ks->value : "");
        b.user->Enable(!cleared);
        b.user->SetHint(cleared ? "(no text)" : hasInh
            ? "Empty: the text above is used" : "");
        b.none->SetValue(cleared);
        b.none->Show(hasInh || cleared);
        b.copy->Enable(hasInh && !cleared);
        b.copy->Show(hasInh);
        b.note->Show(!b.note->GetLabel().empty());
    }
    relayout(b);
}


void WxMachineRegister::relayout(const BlockRow& b)
{
    if (b.scroll != NULL)
    {
        b.scroll->Layout();
        static_cast<wxScrolledWindow*>(b.scroll)->FitInside();
    }
}


//  The CONFIG key a block edits.  The Codes tab's blocks follow the code
//  selected in the list.  gensub reads <Code>Environment and <Code>Command
//  with the code's own spelling and lower-cases the whole key, so the case
//  does not matter; _setup and _wrapup are written in lower case.
string WxMachineRegister::blockKey(const string& id) const
{
    if (id == "header")
        return headerKey();
    if (id[0] != 'c' || p_codeNames.empty())
        return id;
    const string& code = p_codeNames[p_codeSel];
    if (id == "cenv") return code + "Environment";
    if (id == "ccmd") return code + "Command";
    if (id == "csetup") return lowerOf(code) + "_setup";
    return lowerOf(code) + "_wrapup";
}


void WxMachineRegister::syncBlock(MCD* draft, const BlockRow& b)
{
    if (b.key.empty() || draft->state(b.key) == NULL)
        return;
    string t = strip((string)b.user->GetValue());
    if (b.none->IsChecked() && b.none->IsShown())
    {
        if (!draft->clear(b.key))
            draft->useInherited(b.key);
    }
    else if (t.empty())
        draft->useInherited(b.key);
    else
        draft->setValue(b.key, t);
}


void WxMachineRegister::syncBlocks(MCD* draft)
{
    for (size_t i = 0; i < p_blocks.size(); i++)
        syncBlock(draft, p_blocks[i]);
}


void WxMachineRegister::blocksTags()
{
    if (p_draft == NULL)
        return;
    for (size_t i = 0; i < p_blocks.size(); i++)
    {
        const BlockRow& b = p_blocks[i];
        const MCD::KeyState* ks = b.key.empty() ? NULL : p_draft->state(b.key);
        if (ks == NULL)
            continue;
        MCD::Tag t;
        string tip;
        cfgTagInfo(b.key, false, t, tip);
        wxString text = plainTag(t);
        if (b.tag->GetLabel() != text)
            b.tag->SetLabel(text);
        b.tag->SetForegroundColour(wxSystemSettings::GetColour(
            (t == MCD::TagYours || t == MCD::TagNoValue ||
             t == MCD::TagSiteEditing) ? wxSYS_COLOUR_WINDOWTEXT
                                       : wxSYS_COLOUR_GRAYTEXT));
        tip += "\nCONFIG key: " + ks->name;
        if (b.tag->GetToolTipText() != tip)
            b.tag->SetToolTip(tip);
        int uc = undoCase(b.key, false);
        if (b.undo->IsShown() != (uc != 0))
            b.undo->Show(uc != 0);
        b.undo->SetToolTip(undoTip(uc, "text"));
        b.copy->Enable(b.copy->IsShown() && !b.none->IsChecked());
    }
}


//  Fills the box with the inherited text, which then replaces it.
void WxMachineRegister::blockCopySite(const string& id)
{
    BlockRow* b = block(id);
    const MCD::KeyState* ks = b && p_draft ? p_draft->state(b->key) : NULL;
    string inh;
    if (ks == NULL || !inheritedValue(*ks, false, inh))
        return;
    string mine = strip((string)b->user->GetValue());
    if (!mine.empty() && mine != strip(inh) &&
        this->ask("Copy Site Text", "Replace your text with the site text?",
                  "What you typed in this box is lost.",
                  wxYES_NO|wxICON_QUESTION, "Replace", "Keep", "") != wxID_YES)
        return;
    b->none->SetValue(false);
    b->user->Enable(true);
    b->user->SetValue(inh);
    this->updateDirty();
}


void WxMachineRegister::blockUndo(const string& id)
{
    BlockRow* b = block(id);
    if (b == NULL || p_draft == NULL)
        return;
    this->syncDraft();
    undoKey(b->key, p_draft->changed(b->key));
    p_inCtrlUpdate = true;
    blockToControl(*b);
    p_inCtrlUpdate = false;
    this->updateDirty();
}


void WxMachineRegister::blockNone(const string& id)
{
    BlockRow* b = block(id);
    if (b == NULL)
        return;
    bool none = b->none->IsChecked();
    if (none)
    {
        bool was = p_inCtrlUpdate;
        p_inCtrlUpdate = true;
        b->user->ChangeValue("");
        p_inCtrlUpdate = was;
    }
    b->user->Enable(!none);
    b->copy->Enable(!none);
}


//  csh text in a block.  gensub stops at the same lines when it makes a job
//  script, so say so before saving.  firstLine: the file line the text
//  starts on, so the numbers count within the block.
static void cshNotices(const string& text, int firstLine, vector<string>& out)
{
    char tmpl[] = "/tmp/ecce-machreg-csh-XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0)
        return;
    FILE* f = fdopen(fd, "w");
    fputs(text.c_str(), f);
    fclose(f);

    wxString cmd = "perl \"" + string(Ecce::ecceHome()) +
                   "/scripts/ecce-csh2sh\" --check \"" + tmpl + "\"";
    wxArrayString lines, errors;
    wxExecute(cmd, lines, errors, wxEXEC_SYNC);
    unlink(tmpl);

    static const std::regex lineRe("^\\s+line (\\d+) \\[[^\\]]*\\]\\s+(.*)$");
    static const std::regex wantRe("^\\s+(?:->|write:)\\s+(.*)$");
    for (size_t i = 0; i < lines.GetCount(); i++)
    {
        std::smatch m;
        string l = (string)lines[i];
        if (std::regex_match(l, m, lineRe))
            out.push_back("Line " + std::to_string(atoi(m[1].str().c_str()) -
                          firstLine + 1) + " is csh: " + m[2].str());
        else if (!out.empty() && std::regex_match(l, m, wantRe))
            out.back() += "\n    write: " + m[1].str();
    }
}


//  gensub's doEnvironment() takes the first two blank-separated words of a
//  line as NAME and value and drops the rest.
static string envNotice(const string& text)
{
    string msg;
    std::istringstream in(text);
    string line;
    int n = 0, found = 0;
    while (std::getline(in, line))
    {
        n++;
        string l = strip(line);
        if (l.empty())
            continue;
        size_t sp = l.find(' ');
        string rest = sp == string::npos ? "" : strip(l.substr(sp));
        string what;
        if (sp == string::npos)
            what = "has no value after the name";
        else if (rest.find(' ') != string::npos)
            what = "has a value with spaces; only \"" +
                   rest.substr(0, rest.find(' ')) + "\" is used";
        if (what.empty())
            continue;
        if (++found <= 3)
            msg += (msg.empty() ? "" : "\n") + string("Line ") +
                   std::to_string(n) + " " + what + ".";
    }
    if (found > 3)
        msg += "\n... and " + std::to_string(found - 3) + " more";
    return msg;
}


void WxMachineRegister::blocksCheckCsh()
{
    static const char* ids[] = { "setup", "wrapup", "csetup", "cwrapup",
                                 "cenv" };
    for (size_t k = 0; k < sizeof(ids) / sizeof(ids[0]); k++)
    {
        BlockRow* b = block(ids[k]);
        if (b == NULL)
            continue;
        string t = b->user->IsEnabled() ? strip((string)b->user->GetValue())
                                        : string();
        string msg;
        if (b->id == "cenv")
            msg = envNotice(t);
        else
        {
            vector<string> found;
            if (!t.empty())
                cshNotices(b->id + " {\n" + t + "\n}\n", 2, found);
            for (size_t i = 0; i < found.size() && i < 3; i++)
                msg += (i ? "\n" : "") + found[i];
            if (found.size() > 3)
                msg += "\n... and " + std::to_string(found.size() - 3) +
                       " more";
            if (!msg.empty())
                msg += "\nJob scripts are POSIX sh; the job script cannot be "
                       "made until this is fixed.";
        }
        if (b->note->GetLabel() != wxString(msg))
        {
            b->note->SetLabel(msg);
            b->note->Show(!msg.empty());
            relayout(*b);
        }
    }
}


//  The link under the text boxes the placeholders work in.
wxHyperlinkCtrl* WxMachineRegister::wordsLink(wxWindow* page)
{
    wxHyperlinkCtrl* link = new wxHyperlinkCtrl(page, wxID_ANY,
        "Placeholders: $queue, $nodes, ...", "");
    link->SetToolTip("The $words ECCE replaces when a job is submitted, "
                     "and how to insert them");
    link->Bind(wxEVT_HYPERLINK, [this](wxHyperlinkEvent&) {
        this->showWords();
    });
    reg(p_codePage == NULL || page != p_codePage ? "header:variables"
                                                 : "cmd:variables", link);
    return link;
}


//  What gensub's provideVariables() replaces, as a table.  It is applied to
//  the scheduler header, setup, wrap-up and the program's command line, not
//  to the environment.
void WxMachineRegister::showWords()
{
    if (p_wordsDlg != NULL)
        return;
    static const char* const words[][2] = {
        { "$queue", "the queue chosen in the Launcher" },
        { "$nodes", "number of nodes" },
        { "$totalprocs", "total number of processors" },
        { "$ppn", "processors per node" },
        { "$wallTime", "wall time as h:m:s, e.g. 02:00:00" },
        { "$wallHrMin", "wall time as hours and minutes" },
        { "$wallSeconds", "wall time in seconds" },
        { "$cpuTime", "CPU time" },
        { "$memory", "memory, a number in the queue's memory unit" },
        { "$memoryM", "$memory with an M after it, for queues counted in MB" },
        { "$mem_x_1024", "memory times 1024" },
        { "$scratchSpace", "scratch space" },
        { "$scratchDir", "scratch directory" },
        { "$runDir", "the run directory" },
        { "$inFile", "name of the input file" },
        { "$outFile", "name of the output file" },
        { "$submitFile", "name of the job script" },
        { "$account", "allocation account" },
        { "$host", "name of this machine" },
        { "$code", "the code that runs" },
        { "$qMgr", "the queue manager" },
        { "$mdSystemName", "name of the molecular dynamics system" },
        { "$mdCalcName", "name of the molecular dynamics calculation" },
    };
    wxDialog* dlg = new wxDialog(this, wxID_ANY, "Placeholders",
        wxDefaultPosition, wxSize(600, 760),
        wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    dlg->SetSizer(root);

    wxStaticText* eg = new wxStaticText(dlg, wxID_ANY,
        "ECCE replaces each placeholder when the job is submitted: "
        "\"#SBATCH --time=$wallTime\" becomes \"#SBATCH --time=02:00:00\".");
    eg->Wrap(560);
    root->Add(eg, wxSizerFlags().Border());

    wxListCtrl* list = new wxListCtrl(dlg, wxID_ANY, wxDefaultPosition,
                                      wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL);
    list->InsertColumn(0, "Placeholder", wxLIST_FORMAT_LEFT, 140);
    list->InsertColumn(1, "What it becomes", wxLIST_FORMAT_LEFT, 400);
    for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); i++)
    {
        long r = list->InsertItem(i, words[i][0]);
        list->SetItem(r, 1, words[i][1]);
    }
    root->Add(list, wxSizerFlags(1).Expand().Border(wxLEFT|wxRIGHT));
    wxStaticText* note = new wxStaticText(dlg, wxID_ANY,
        "A request line whose placeholder is empty is left out. The "
        "placeholders work in the request lines, the setup and wrap-up "
        "commands and a command line, not in the environment variables.");
    note->Wrap(560);
    root->Add(note, wxSizerFlags().Border());

    wxStaticText* where = new wxStaticText(dlg, wxID_ANY, "");
    root->Add(where, wxSizerFlags().Border(wxLEFT|wxRIGHT));
    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    wxButton* insert = new ewxButton(dlg, wxID_ANY, "&Insert");
    wxButton* close = new ewxButton(dlg, wxID_CLOSE, "&Close");
    buttons->Add(insert, wxSizerFlags().Border());
    buttons->AddStretchSpacer(1);
    buttons->Add(close, wxSizerFlags().Border());
    root->Add(buttons, wxSizerFlags().Expand());

    p_wordsList = list;
    p_wordsInsert = insert;
    p_wordsWhere = where;

    //  Says where Insert would put the word, or why it cannot.
    auto refresh = [this]() {
        string name;
        wxTextCtrl* t = insertTarget(name);
        p_wordsInsert->Enable(t != NULL);
        p_wordsInsert->SetToolTip(t != NULL ? wxString("Insert the selected "
            "placeholder at the cursor") : wxString("There is no text box on "
            "this tab to insert into. Open the Job script tab (with a queue "
            "manager set) or the Codes tab."));
        p_wordsWhere->SetLabel(t != NULL
            ? "Insert goes into: " + name + " (double-click a placeholder)"
            : "Nothing to insert into on this tab.");
    };
    auto doInsert = [this, refresh](long row) {
        if (row < 0)
            return;
        insertWord((string)p_wordsList->GetItemText(row));
        refresh();
    };
    insert->Bind(wxEVT_BUTTON, [this, doInsert](wxCommandEvent&) {
        doInsert(p_wordsList->GetNextItem(-1, wxLIST_NEXT_ALL,
                                          wxLIST_STATE_SELECTED));
    });
    list->Bind(wxEVT_LIST_ITEM_ACTIVATED, [doInsert](wxListEvent& e) {
        doInsert(e.GetIndex());
    });
    refresh();

    bool modal = !p_scripted;
    auto finish = [this, dlg, modal]() {
        p_wordsDlg = NULL;
        p_wordsList = NULL;
        p_wordsInsert = NULL;
        p_wordsWhere = NULL;
        p_fields.erase("words:dialog");
        if (modal)
            dlg->EndModal(wxID_CLOSE);
        else
            dlg->Destroy();
    };
    close->Bind(wxEVT_BUTTON, [finish](wxCommandEvent&) { finish(); });
    dlg->Bind(wxEVT_CLOSE_WINDOW, [finish](wxCloseEvent&) { finish(); });
    reg("words:dialog", dlg);
    reg("words:close", close);
    reg("words:insert", insert);
    reg("words:where", where);
    p_wordsDlg = dlg;
    dlg->CentreOnParent();
    if (modal)
    {
        dlg->ShowModal();
        dlg->Destroy();
    }
    else
    {
        dlg->Show();
        refresh();
    }
}


//  ---- Advanced: edit file -----------------------------------------------------

string WxMachineRegister::rawFileText() const
{
    ConfigFile f;
    f.load(p_draft->editedFile());
    if (f.exists())
        return f.text();
    return "# Settings for " + p_loadedName + " (Register Machines)\n";
}


//  Problems in `text`; errors stop the save, the rest are warnings.
string WxMachineRegister::rawFileCheck(const string& text, bool& errors) const
{
    errors = false;
    ConfigFile f;
    f.parse(text);
    string report;
    for (size_t i = 0; i < f.warnings().size(); i++)
    {
        const string& w = f.warnings()[i];
        bool bad = w.find("not closed") != string::npos;
        errors = errors || bad;
        report += string(bad ? "Error: " : "Warning: ") + w + "\n";
    }

    //  A key some other layer (or this form) knows; the file's own keys do
    //  not make themselves known.
    std::set<string> known;
    vector<string> dk = p_draft->keys();
    for (size_t i = 0; i < dk.size(); i++)
        if (!p_draft->state(dk[i])->inherited.empty())
            known.insert(dk[i]);
    for (size_t i = 0; i < p_cfgRows.size(); i++)
        known.insert(lowerOf(p_cfgRows[i].key));
    known.insert("setup");
    known.insert("wrapup");
    for (int i = 1; i < (int)p_qmgrChoice->GetCount(); i++)
        known.insert(lowerOf((string)p_qmgrChoice->GetString(i)));
    static const char* const per[] = { "", "command", "environment", "_setup",
        "_wrapup", "_loophole", "filestoremove", "prelimfilestoremove" };
    for (size_t i = 0; i < p_codeNames.size(); i++)
        for (size_t j = 0; j < sizeof(per) / sizeof(per[0]); j++)
            known.insert(lowerOf(p_codeNames[i]) + per[j]);
    for (size_t i = 0; i < f.entries().size(); i++)
        if (known.find(f.entries()[i].lkey) == known.end())
            report += "Warning: line " + std::to_string(f.entries()[i].first + 1)
                      + ": '" + f.entries()[i].key + "' is not a setting "
                      "ECCE reads.\n";

    vector<string> csh;
    cshNotices(text, 1, csh);
    for (size_t i = 0; i < csh.size(); i++)
        report += "Warning: " + csh[i] + "\n";
    if (report.empty())
        report = "No problems found.\n";
    return report;
}


bool WxMachineRegister::rawFileSave(const string& text, string& err)
{
    ConfigFile f;
    f.setSiteFile(p_adminFlag);
    if (!f.load(p_draft->editedFile()))
    {
        err = "Cannot read " + p_draft->editedFile();
        return false;
    }
    f.setText(text);
    return f.save(&err);
}


void WxMachineRegister::rawFileClosed(bool saved)
{
    for (std::map<string, wxWindow*>::iterator it = p_fields.begin();
         it != p_fields.end(); )
        if (it->first.compare(0, 4, "raw:") == 0)
            p_fields.erase(it++);
        else
            ++it;
    p_rawDlg = NULL;
    if (saved)
    {
        this->redo(p_loadedName);
        this->notifyUpdate();
    }
}


//  The form and a raw edit are never merged: unsaved form changes are saved
//  or dropped first.
void WxMachineRegister::editFile()
{
    if (p_rawDlg != NULL || p_draft == NULL)
        return;
    if (p_loadedName.empty())
    {
        displayMessage("Save the machine first; then its settings file can "
                       "be edited.");
        return;
    }
    if (!this->resolveUnsaved("Discard Changes"))
        return;
    //  Discarded: the file, not the form, is what is edited now.
    if (this->isDirty())
        this->loadMachine(p_loadedName);

    wxDialog* dlg = new wxDialog(this, wxID_ANY,
        "Edit " + p_draft->editedFile(), wxDefaultPosition, wxSize(720, 560),
        wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    dlg->SetSizer(root);

    wxStaticText* intro = new wxStaticText(dlg, wxID_ANY,
        "The settings file for '" + p_loadedName + "', as text. Check looks "
        "for mistakes; Save writes the file and shows the form again.");
    intro->Wrap(680);
    root->Add(intro, wxSizerFlags().Border());
    wxTextCtrl* text = new wxTextCtrl(dlg, wxID_ANY, rawFileText(),
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE|wxTE_DONTWRAP|wxHSCROLL);
    text->SetFont(wxFont(wxFontInfo().Family(wxFONTFAMILY_TELETYPE)));
    root->Add(text, wxSizerFlags(3).Expand().Border());
    wxTextCtrl* report = new wxTextCtrl(dlg, wxID_ANY, "", wxDefaultPosition,
        wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_DONTWRAP);
    root->Add(report, wxSizerFlags(1).Expand().Border(wxLEFT|wxRIGHT));
    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    wxButton* check = new ewxButton(dlg, wxID_ANY, "&Check");
    wxButton* cancel = new ewxButton(dlg, wxID_CANCEL, "Cancel");
    wxButton* save = new ewxButton(dlg, wxID_SAVE, "&Save");
    buttons->Add(check, wxSizerFlags().Border());
    buttons->AddStretchSpacer(1);
    buttons->Add(cancel, wxSizerFlags().Border());
    buttons->Add(save, wxSizerFlags().Border());
    root->Add(buttons, wxSizerFlags().Expand());
    save->SetDefault();

    bool modal = !p_scripted;
    auto finish = [this, dlg, modal](bool saved) {
        this->rawFileClosed(saved);
        if (modal)
            dlg->EndModal(saved ? wxID_OK : wxID_CANCEL);
        else
            dlg->Destroy();
    };
    check->Bind(wxEVT_BUTTON, [this, text, report](wxCommandEvent&) {
        bool errors;
        report->SetValue(this->rawFileCheck((string)text->GetValue(), errors));
    });
    save->Bind(wxEVT_BUTTON, [this, text, report, finish](wxCommandEvent&) {
        bool errors;
        string r = this->rawFileCheck((string)text->GetValue(), errors);
        report->SetValue(r);
        if (errors)
        {
            this->p_lastMessage = "not saved: " + r;
            return;
        }
        string err;
        if (!this->rawFileSave((string)text->GetValue(), err))
        {
            report->SetValue("Error: " + err + "\n");
            this->p_lastMessage = err;
            return;
        }
        finish(true);
    });
    cancel->Bind(wxEVT_BUTTON, [finish](wxCommandEvent&) { finish(false); });
    dlg->Bind(wxEVT_CLOSE_WINDOW, [finish](wxCloseEvent&) { finish(false); });

    reg("raw:text", text);
    reg("raw:report", report);
    reg("raw:check", check);
    reg("raw:save", save);
    reg("raw:cancel", cancel);
    p_rawDlg = dlg;
    dlg->CentreOnParent();
    if (modal)
    {
        dlg->ShowModal();
        dlg->Destroy();
    }
    else
        dlg->Show();
}


//  ---- dialogs ---------------------------------------------------------------

//  The hook answers prompts from a queue; without one a scripted run cancels,
//  so a prompt nobody expected cannot hang a headless test.
int WxMachineRegister::ask(const string& title, const string& message,
                           const string& ext, long style, const string& yes,
                           const string& no, const string& cancel)
{
    if (p_scripted)
    {
        fprintf(stderr, "[MACHREG] prompt: %s: %s\n", title.c_str(),
                message.c_str());
        p_lastMessage = title + ": " + message + " " + ext;
        if (!p_answers.empty())
        {
            int a = p_answers.front();
            p_answers.pop_front();
            return a;
        }
        fprintf(stderr, "[MACHREG] FAIL unanswered prompt\n");
        return wxID_CANCEL;
    }

    wxMessageDialog dlg(this, message, title, style);
    if (!ext.empty())
        dlg.SetExtendedMessage(ext);
    if (style & wxCANCEL)
        dlg.SetYesNoCancelLabels(wxString(yes), wxString(no), wxString(cancel));
    else
        dlg.SetYesNoLabels(wxString(yes), wxString(no));
    return dlg.ShowModal();
}


void WxMachineRegister::displayMessage(const string& mesg)
{
    if (p_scripted)
    {
        fprintf(stderr, "[MACHREG] alert: %s\n", mesg.c_str());
        p_lastMessage = mesg;
        return;
    }
    wxMessageDialog dlg(this, mesg, "Machine Registration",
                        wxOK|wxICON_INFORMATION);
    dlg.ShowModal();
}


//  Tell the other apps that machine registration has been saved.
void WxMachineRegister::notifyUpdate()
{
    if (p_scripted)
        return;

    JMSPublisher *pub = new JMSPublisher("WxMachineRegister");

    JMSMessage *msg = pub->newMessage();
    pub->publish("ecce_machreg_changed", *msg);

    delete msg;
    delete pub;
}
