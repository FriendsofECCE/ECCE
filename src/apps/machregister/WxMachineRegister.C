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

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/accel.h"
#include "wx/display.h"
#include "wx/artprov.h"
#include "wx/statbmp.h"
#include "wx/listctrl.h"
#include "wx/notebook.h"
#include "wx/spinctrl.h"
#include "wx/statline.h"

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


wxWindow* WxMachineRegister::createConnectionPage(wxWindow* parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    ewxScrolledWindow* page = newPage(parent, sizer);

    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 0, 0);
    grid->AddGrowableCol(1);
    sizer->Add(grid, wxSizerFlags().Expand());

    p_perlPath = new ewxTextCtrl(page, wxID_ANY);
    p_perlPath->SetToolTip("Directory of the Perl 5 interpreter on the "
                           "remote machine.");
    p_qmgrPath = new ewxTextCtrl(page, wxID_ANY);
    p_qmgrPath->SetToolTip("Directory holding the scheduler's commands on "
                           "the remote machine.");
    grid->Add(new ewxStaticText(page, wxID_ANY, "Perl on the remote machine"),
              wxSizerFlags().Right().Border().CentreVertical());
    grid->Add(p_perlPath, wxSizerFlags(1).Expand().Border().CentreVertical());
    grid->Add(new ewxStaticText(page, wxID_ANY, "Scheduler command directory"),
              wxSizerFlags().Right().Border().CentreVertical());
    grid->Add(p_qmgrPath, wxSizerFlags(1).Expand().Border().CentreVertical());

    p_connNote = new wxStaticText(page, wxID_ANY, "");
    sizer->Add(p_connNote, wxSizerFlags().Border());

    reg("perlpath", p_perlPath);
    reg("qmgrpath", p_qmgrPath);
    return page;
}


wxWindow* WxMachineRegister::createCodesPage(wxWindow* parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    ewxScrolledWindow* page = newPage(parent, sizer);

    sizer->Add(new wxStaticText(page, wxID_ANY,
        "Where each code is installed on the machine. A code with no path "
        "is not offered for this machine."), wxSizerFlags().Border());

    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 0, 0);
    grid->AddGrowableCol(1);
    sizer->Add(grid, wxSizerFlags().Expand());

    for (size_t i = 0; i < p_codeNames.size(); i++)
    {
        ewxTextCtrl* txt = new ewxTextCtrl(page, wxID_ANY);
        grid->Add(new ewxStaticText(page, wxID_ANY, p_codeNames[i]),
                  wxSizerFlags().Right().Border().CentreVertical());
        grid->Add(txt, wxSizerFlags(1).Expand().Border().CentreVertical());
        p_codePaths.push_back(txt);
        reg("code:" + lowerOf(p_codeNames[i]), txt);
    }
    return page;
}


wxWindow* WxMachineRegister::createJobScriptPage(wxWindow* parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    ewxScrolledWindow* page = newPage(parent, sizer);
    p_jobNote = new wxStaticText(page, wxID_ANY, "");
    sizer->Add(p_jobNote, wxSizerFlags().Border());
    return page;
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
        text = "This computer: jobs run locally when the login name is empty or ";
        text += Ecce::realUser();
        text += ";\nany other login name goes via ssh to " + machine +
                " as that user.";

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

MCD* WxMachineRegister::newDraft(const string& refName) const
{
    MCD::Mode mode = p_adminFlag ? MCD::AdminMode
                   : remoteClient() ? MCD::RemoteMode : MCD::UserMode;
    string site = string(Ecce::ecceHome()) + "/siteconfig/CONFIG." + refName;
    string user = withoutSlash(Ecce::realUserPrefPath()) + "/CONFIG." + refName;

    MCD* draft = new MCD(mode, site, user);

    vector<string> keys;
    for (size_t i = 0; i < p_codeNames.size(); i++)
        keys.push_back(p_codeNames[i]);
    keys.push_back("perlPath");
    keys.push_back("qmgrPath");
    draft->loadFiles(keys);
    return draft;
}


void WxMachineRegister::loadMachine(const string& refName)
{
    RefMachine* ref = RefMachine::refLookup(refName.c_str());
    int idx = findRow(refName);
    if (ref == NULL || idx < 0)
        return;

    p_slctRgstn = ref;
    p_loadedName = refName;
    p_loadedFrom = p_rows[idx].from;
    delete p_draft;
    p_draft = newDraft(refName);
    this->draftToControls();

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
    string v;
    p_draft->effective("perlPath", v);
    p_perlPath->SetValue(v);
    v = "";
    p_draft->effective("qmgrPath", v);
    p_qmgrPath->SetValue(v);

    if (p_slctRgstn != NULL)
    {
        this->loadQueues(p_slctRgstn->refname());
        this->fillQueues();
        this->showQueue();
    }
    else
        this->clearQueues();

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

void WxMachineRegister::syncKey(MCD* draft, const string& key, const string& text)
{
    string t = strip(text);
    const MCD::KeyState* ks = draft->state(key);
    if (ks == NULL)
        return;

    string cur;
    bool has = draft->effective(key, cur);
    if ((has && t == cur) || (!has && t.empty()))
        return;

    bool hasInh = !ks->inherited.empty() && ks->inherited.back().hasValue;
    string inh = hasInh ? ks->inherited.back().value : string();
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

    syncKeys(p_draft);
}


//  The managed CONFIG keys: each code's path, perlPath and qmgrPath.
void WxMachineRegister::syncKeys(MCD* draft)
{
    for (size_t i = 0; i < p_codePaths.size(); i++)
        syncKey(draft, p_codeNames[i], (string)p_codePaths[i]->GetValue());
    syncKey(draft, "perlPath", (string)p_perlPath->GetValue());
    syncKey(draft, "qmgrPath", (string)p_qmgrPath->GetValue());
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
    if (p_book == NULL || p_storeNote == NULL || p_connNote == NULL)
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

    wxString conn = "Not editable here yet. The remote shell (shell), the "
        "file to source (sourceFile) and the login host (frontendMachine, "
        "frontendBypass) are set in " + config + ".";
    if (p_connNote->GetLabel() != conn)
        { p_connNote->SetLabel(conn); p_connNote->Wrap(560); }

    string qm = (string)p_qmgrChoice->GetStringSelection();
    string lq = lowerOf(qm);
    wxString job = "Not editable here yet. The commands run before and after "
        "the code (setup, wrapup)";
    if (lq != "none" && lq != "shell" && !lq.empty())
        job += " and the " + qm + " header (" + lq + ")";
    job += " are set in " + config + ".";
    if (p_jobNote->GetLabel() != job)
        { p_jobNote->SetLabel(job); p_jobNote->Wrap(560); }

    string a = "sbatch", b = "squeue", dir = "/opt/slurm/bin";
    if (lq == "pbs" || lq == "sge") { a = "qsub"; b = "qstat"; dir = "/opt/pbs/bin"; }
    else if (lq == "lsf") { a = "bsub"; b = "bjobs"; dir = "/opt/lsf/bin"; }
    else if (lq == "moab") { a = "msub"; b = "showq"; dir = "/opt/moab/bin"; }
    else if (lq == "htcondor") { a = "condor_submit"; b = "condor_q"; dir = "/opt/condor/bin"; }
    p_qmgrPath->SetHint("where " + a + ", " + b + " are, e.g. " + dir +
                        "; empty if on PATH");
}


void WxMachineRegister::updateDirty()
{
    this->updateFooter();
    if (p_inCtrlUpdate || p_draft == NULL || p_closing)
        return;

    bool dirty = this->isDirty();
    p_saveButton->Enable(dirty && hasMinimalInput());
    wxString title = dirty ? wxString("*") + TITLE : wxString(TITLE);
    if ((string)this->GetTitle() != (string)title)
        this->SetTitle(title);
    this->updateQueueButtons();
}


void WxMachineRegister::onFieldChanged(wxCommandEvent& event)
{
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

    //  Values land on one line of CONFIG.<machine>, and "-" there means
    //  "no value".
    vector<ewxTextCtrl*> texts = p_codePaths;
    texts.push_back(p_perlPath);
    texts.push_back(p_qmgrPath);
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
