/**
 * @file
 *
 *  Register Machines' three tools that ask or simulate the scheduler (#212):
 *  discovering the machine's queues, previewing the job script from the form
 *  as it is now, and a test submission.  The dialogs are not modal while the
 *  test hook runs, and register their fields (disc:*, prev:*, test:*) so the
 *  hook can drive them.
 */

#include <atomic>
#include <memory>
#include <functional>
#include <thread>
#include <algorithm>
#include <limits.h>

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/checklst.h"
#include "wx/clipbrd.h"
#include "wx/notebook.h"
#include "wx/progdlg.h"
#include "wx/spinctrl.h"

#include "util/Ecce.H"
#include "tdat/ConfigFile.H"
#include "tdat/RefMachine.H"
#include "dsm/MachinePreferences.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"

#include "JobPreview.H"
#include "MemoryUnits.H"
#include "SchedulerQuery.H"
#include <wx/filedlg.h>
#include <wx/filename.h>
#include "WxMachineRegister.H"

using std::string;
using std::vector;

typedef MachineConfigDraft MCD;

static string stripped(const string& in)
{
    size_t a = in.find_first_not_of(" \t\r\n");
    if (a == string::npos)
        return "";
    return in.substr(a, in.find_last_not_of(" \t\r\n") - a + 1);
}

static string lowered(string s)
{
    for (size_t i = 0; i < s.size(); i++)
        s[i] = (char)tolower((unsigned char)s[i]);
    return s;
}

//  Runs `work` on a thread while the window waits, showing a progress dialog
//  once it has taken half a second.
static void runBusy(wxWindow* parent, const wxString& title,
                    const wxString& message, const std::function<void()>& work)
{
    std::atomic<bool> done(false);
    std::thread worker([&work, &done]() {
        try { work(); } catch (...) {}
        done = true;
    });
    wxBusyCursor busy;
    wxWindowDisabler disabler;
    wxProgressDialog* dlg = NULL;
    wxStopWatch watch;
    while (!done)
    {
        wxMilliSleep(25);
        if (dlg == NULL && watch.Time() > 500)
            dlg = new wxProgressDialog(title, message, 100, parent,
                                       wxPD_APP_MODAL);
        if (dlg != NULL)
            dlg->Pulse();
        wxTheApp->Yield(true);
    }
    worker.join();
    delete dlg;
}

//  A tint of `base` over the window colour, readable in a dark theme too.
static wxColour tint(const wxColour& base)
{
    wxColour bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    const int a = 70;     // percent of the window colour kept
    return wxColour((base.Red() * (100 - a) + bg.Red() * a) / 100,
                    (base.Green() * (100 - a) + bg.Green() * a) / 100,
                    (base.Blue() * (100 - a) + bg.Blue() * a) / 100);
}

static wxColour partColour(const string& part)
{
    if (part == "request") return tint(wxColour(230, 159, 0));
    if (part == "before") return tint(wxColour(86, 180, 233));
    if (part == "environment") return tint(wxColour(0, 158, 115));
    if (part == "command") return tint(wxColour(213, 94, 0));
    if (part == "after") return tint(wxColour(204, 121, 167));
    return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
}

static string partShort(const string& part)
{
    if (part == "environment") return "env";
    if (part == "ecce") return "ECCE";
    return part;
}

//  ---- shared helpers ---------------------------------------------------------

SchedulerQuery::Connection WxMachineRegister::connection()
{
    this->syncDraft();
    SchedulerQuery::Connection c;
    c.machine = stripped((string)p_fullName->GetValue());
    string refName = stripped((string)p_refName->GetValue());
    MachinePreferences *prefs = refName.empty() ? NULL :
                                MachinePreferences::lookup(refName);
    c.remShell = "ssh";
    if (prefs != NULL)
    {
        if (!prefs->getRemoteShell().empty())
            c.remShell = prefs->getRemoteShell();
        if (prefs->isOptionSupported("UN"))
            c.userName = prefs->getUsername();
    }

    //  The unsaved form decides the paths, as RefMachine reads them once saved.
    std::map<string, string> cfg;
    static const char* const keys[] = { "xappspath", "perlpath", "qmgrpath",
        "libpath", "sourcefile", "frontendmachine", "frontendbypass", "shell" };
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
    {
        string v;
        if (p_draft != NULL && p_draft->effective(keys[i], v, true) && !v.empty())
            cfg[keys[i]] = v;
    }
    c.locShell = cfg.count("shell") ? cfg["shell"] : "bash";
    c.shellPath = RefMachine::shellPathFor(cfg);
    c.libPath = cfg.count("libpath") ? cfg["libpath"] : "";
    c.sourceFile = cfg.count("sourcefile") ? cfg["sourcefile"] : "";
    c.frontendMachine = cfg.count("frontendmachine") ? cfg["frontendmachine"] : "";
    c.frontendBypass = cfg.count("frontendbypass") ? cfg["frontendbypass"] : "";
    return c;
}


//  The file the form would save, as text; `extra` lines are appended.
bool WxMachineRegister::draftConfigText(string& text, string& err,
                                        const string& extra)
{
    this->syncDraft();
    string name = stripped((string)p_refName->GetValue());
    MCD* draft = p_draft;
    MCD* own = NULL;
    if (name != p_loadedName)
    {
        own = draft = newDraft(name);
        syncKeys(draft);
    }
    ConfigFile f;
    f.setSiteFile(p_adminFlag);
    f.load(draft->editedFile());
    bool ok = draft->applyTo(f, err);
    if (ok)
        text = f.text() + extra;
    delete own;
    return ok;
}


//  Codes that have a program path in the form as it is now.
vector<string> WxMachineRegister::codesWithPath()
{
    vector<string> out;
    this->syncDraft();
    for (size_t i = 0; i < p_codeNames.size(); i++)
    {
        string v;
        if (p_draft != NULL && p_draft->effective(lowered(p_codeNames[i]), v) &&
            !v.empty())
            out.push_back(p_codeNames[i]);
    }
    return out;
}


void WxMachineRegister::toolClosed(const string& prefix)
{
    for (std::map<string, wxWindow*>::iterator it = p_fields.begin();
         it != p_fields.end(); )
    {
        if (it->first.compare(0, prefix.size(), prefix) == 0)
            p_fields.erase(it++);
        else
            ++it;
    }
    p_toolDlg = NULL;
}


//  Shows `dlg` as the hook needs it (not modal) or to a person (modal).
void WxMachineRegister::showTool(wxDialog* dlg, const string& prefix)
{
    p_toolDlg = dlg;
    dlg->Bind(wxEVT_CLOSE_WINDOW, [this, dlg, prefix](wxCloseEvent&) {
        this->toolClosed(prefix);
        if (dlg->IsModal())
            dlg->EndModal(wxID_CANCEL);
        else
            dlg->Destroy();
    });
    dlg->CentreOnParent();
    if (p_scripted)
        dlg->Show();
    else
    {
        dlg->ShowModal();
        if (p_toolDlg == dlg)
            this->toolClosed(prefix);
        dlg->Destroy();
    }
}


//  ---- the job request controls the preview and the test share ---------------

namespace
{
    struct RequestPanel
    {
        wxChoice *code, *queue;
        wxSpinCtrl *nodes, *procs, *mem;
        wxSpinCtrlDouble *wall;
        wxTextCtrl *account;
        RequestPanel() : code(NULL), queue(NULL), nodes(NULL), procs(NULL),
                         mem(NULL), wall(NULL), account(NULL) {}
    };
}

//  A queue's defaults into the request fields: the processors, wall time and
//  memory the Launcher would start with.
static void queueDefaults(const vector<MCD::QueueRow>& queues,
                          const string& name, RequestPanel& p)
{
    for (size_t i = 0; i < queues.size(); i++)
    {
        if (queues[i].name != name)
            continue;
        const MCD::QueueRow& q = queues[i];
        p.procs->SetValue(q.defProcs != 0 && q.defProcs != (unsigned)INT_MAX
                          ? (int)q.defProcs : 1);
        p.wall->SetValue(q.defWall != 0 && q.defWall != (unsigned)INT_MAX
                         ? q.defWall / 60.0 : 1.0);
        p.mem->SetValue(q.defMem != 0 && q.defMem != (unsigned)INT_MAX
                        ? (int)MemoryUnits::mbToGB(q.defMem) : 0);
        return;
    }
}


static void addRequestControls(WxMachineRegister* owner, wxDialog* dlg,
                               wxSizer* root, RequestPanel& p,
                               const vector<string>& codes,
                               const string& selectedCode,
                               const vector<MCD::QueueRow>& queues,
                               const string& selectedQueue,
                               const string& account,
                               const string& prefix, bool withCode,
                               std::function<void(const string&, wxWindow*)> reg)
{
    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 4, 8);
    root->Add(grid, wxSizerFlags().Border());

    p.code = new wxChoice(dlg, wxID_ANY);
    for (size_t i = 0; i < codes.size(); i++)
        p.code->Append(codes[i]);
    if (!p.code->SetStringSelection(selectedCode) && p.code->GetCount() > 0)
        p.code->SetSelection(0);
    if (withCode)
    {
        grid->Add(new wxStaticText(dlg, wxID_ANY, "Code"),
                  wxSizerFlags().CentreVertical());
        grid->Add(p.code);
    }
    else
        p.code->Hide();

    p.queue = new wxChoice(dlg, wxID_ANY);
    p.queue->Append("(none)");
    for (size_t i = 0; i < queues.size(); i++)
        p.queue->Append(queues[i].name);
    if (!p.queue->SetStringSelection(selectedQueue))
        p.queue->SetSelection(0);
    grid->Add(new wxStaticText(dlg, wxID_ANY, "Queue"),
              wxSizerFlags().CentreVertical());
    grid->Add(p.queue);

    p.nodes = new wxSpinCtrl(dlg, wxID_ANY, "1", wxDefaultPosition,
                             wxDefaultSize, wxSP_ARROW_KEYS, 1, 10000, 1);
    grid->Add(new wxStaticText(dlg, wxID_ANY, "Nodes"),
              wxSizerFlags().CentreVertical());
    grid->Add(p.nodes);

    p.procs = new wxSpinCtrl(dlg, wxID_ANY, "1", wxDefaultPosition,
                             wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 1);
    grid->Add(new wxStaticText(dlg, wxID_ANY, "Processors (total)"),
              wxSizerFlags().CentreVertical());
    grid->Add(p.procs);

    p.wall = new wxSpinCtrlDouble(dlg, wxID_ANY, "1", wxDefaultPosition,
                                  wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000, 1,
                                  0.25);
    p.wall->SetDigits(2);
    grid->Add(new wxStaticText(dlg, wxID_ANY, "Wall time (hours)"),
              wxSizerFlags().CentreVertical());
    grid->Add(p.wall);

    p.mem = new wxSpinCtrl(dlg, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize,
                           wxSP_ARROW_KEYS, 0, 100000, 0);
    p.mem->SetToolTip("0 requests no memory");
    grid->Add(new wxStaticText(dlg, wxID_ANY, "Memory (GB)"),
              wxSizerFlags().CentreVertical());
    grid->Add(p.mem);

    p.account = new ewxTextCtrl(dlg, wxID_ANY, wxString::FromUTF8(account.c_str()));
    p.account->SetHint("e.g. proj1");
    p.account->SetToolTip("The allocation account the job is charged to. "
        "Some machines refuse a job without one. Filled in from the Default "
        "account on the Queues tab.");
    grid->Add(new wxStaticText(dlg, wxID_ANY, "Account"),
              wxSizerFlags().CentreVertical());
    grid->Add(p.account, wxSizerFlags().Expand());

    queueDefaults(queues, selectedQueue, p);

    if (withCode)
        reg(prefix + "code", p.code);
    reg(prefix + "queue", p.queue);
    reg(prefix + "nodes", p.nodes);
    reg(prefix + "procs", p.procs);
    reg(prefix + "wall", p.wall);
    reg(prefix + "mem", p.mem);
    reg(prefix + "account", p.account);
    (void)owner;
}

static JobPreview::Request requestFrom(const RequestPanel& p, bool admin,
                                       const string& host, const string& fullName,
                                       const string& qmgr)
{
    JobPreview::Request r;
    r.host = host;
    r.fullName = fullName;
    r.qmgr = qmgr;
    r.code = (string)p.code->GetStringSelection();
    string q = (string)p.queue->GetStringSelection();
    r.queue = q == "(none)" ? "" : q;
    r.account = stripped((string)p.account->GetValue());
    r.nodes = (unsigned)p.nodes->GetValue();
    r.procs = (unsigned)p.procs->GetValue();
    r.wallHours = p.wall->GetValue();
    r.memMB = MemoryUnits::gbToMB((unsigned)p.mem->GetValue());
    r.admin = admin;
    return r;
}


//  ---- discovery ------------------------------------------------------------

//  Fills in the selected code's program by asking the machine, over the
//  connection the other checks use, where its executable is.  The login
//  setup (Connection tab) runs first, so a module-loaded code is found.
void WxMachineRegister::findProgram()
{
    if (p_codeNames.empty())
        return;
    const string code = p_codeNames[p_codeSel];
    ewxTextCtrl* field = p_codePaths[p_codeSel];
    string machine = stripped((string)p_fullName->GetValue());
    if (machine.empty())
    {
        displayMessage("Enter the machine's host name on the Machine tab "
                       "first.");
        return;
    }
    SchedulerQuery::ProgramHelp h = SchedulerQuery::programHelp(code);
    SchedulerQuery::Connection conn = this->connection();
    string output, err;
    bool ran = false;
    string extra;
#ifdef _WIN32
    //  Windows installers put programs in folders not on PATH.  The roots
    //  are this computer's; on another machine they match nothing.
    {
        vector<string> roots;
        const char* vars[] = { "USERPROFILE", "LOCALAPPDATA", "ProgramFiles" };
        for (const char* v : vars)
        {
            const char* val = getenv(v);
            if (val && *val)
                roots.push_back(SchedulerQuery::localProgramPath(val));
        }
        if (getenv("LOCALAPPDATA"))
            roots.push_back(SchedulerQuery::localProgramPath(
                                getenv("LOCALAPPDATA")) + "/Programs");
        extra = SchedulerQuery::findInstallDirsScript(h, code, roots);
    }
#endif
    runBusy(this, "Find program", "Looking for " + code + " on " + machine +
            "...", [&]() {
        SchedulerQuery::Remote r(conn);
        string e;
        if (!r.open(e)) { err = e; return; }
        vector<string> argv;
        argv.push_back("sh");
        argv.push_back("-c");
        argv.push_back(SchedulerQuery::findProgramScript(h) + extra +
                       "exit 0\n");
        ran = r.run(argv, output, 60);
        if (!ran)
            err = output;
    });
    vector<string> found = SchedulerQuery::parseFound(output);
    if (!ran && found.empty())
    {
        displayMessage("Could not look on " + machine + ":\n" + err);
        return;
    }
    string names;
    for (size_t i = 0; i < h.names.size(); i++)
        names += (i ? ", " : "") + h.names[i];
    if (found.empty())
    {
        displayMessage("No " + names + " was found on " + machine + ". The "
            "program was left as it was. If " + code + " is loaded with "
            "'module load', put that command in the login setup on the "
            "Connection tab; otherwise enter the path by hand.");
        return;
    }
    string pick = found[0];
    if (found.size() > 1)
    {
        int at = 0;
        if (p_scripted)
        {
            fprintf(stderr, "[MACHREG] choose: %s\n",
                    SchedulerQuery::commandLine(found).c_str());
            p_lastMessage = "choose: " + SchedulerQuery::commandLine(found);
            if (p_choices.empty())
            {
                fprintf(stderr, "[MACHREG] FAIL unanswered choice\n");
                return;
            }
            at = p_choices.front();
            p_choices.pop_front();
            if (at < 0 || at >= (int)found.size())
                return;                         // cancelled
        }
        else
        {
            wxArrayString items;
            for (size_t i = 0; i < found.size(); i++)
                items.Add(wxString::FromUTF8(found[i].c_str()));
            at = wxGetSingleChoiceIndex("Several programs were found on " +
                     machine + ". Which one is " + code + "?",
                     "Find program", items, 0, this);
            if (at < 0)
                return;
        }
        pick = found[at];
    }
    string now = stripped((string)field->GetValue());
    if (!now.empty() && now != pick &&
        ask("Find program", "Replace " + now + " with " + pick + "?", "",
            wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION, "Replace", "Keep", "") != wxID_YES)
        return;
    field->SetValue(wxString::FromUTF8(pick.c_str()));
}


//  Browse...: the program picked in a file dialog on this computer.
void WxMachineRegister::browseProgram()
{
    if (p_codeNames.empty())
        return;
    ewxTextCtrl* field = p_codePaths[p_codeSel];
    string now = stripped((string)field->GetValue());
    wxString dir;
#ifdef _WIN32
    if (now.size() > 2 && now[0] == '/' && now[2] == '/')
        dir = wxString::Format("%c:%s", toupper(now[1]),
                               wxString::FromUTF8(now.substr(2).c_str()));
#else
    dir = wxString::FromUTF8(now.c_str());
#endif
    if (!dir.empty())
        dir = wxFileName(dir).GetPath();
    wxFileDialog dlg(this, "Program for " + p_codeNames[p_codeSel], dir, "",
#ifdef _WIN32
                     "Programs (*.exe)|*.exe|All files (*.*)|*.*",
#else
                     wxFileSelectorDefaultWildcardStr,
#endif
                     wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK)
        return;
    string pick = SchedulerQuery::localProgramPath(
                      string(dlg.GetPath().ToUTF8()));
    field->SetValue(wxString::FromUTF8(pick.c_str()));
}


void WxMachineRegister::discoverQueues()
{
    string qmgr = (string)p_qmgrChoice->GetStringSelection();
    string machine = stripped((string)p_fullName->GetValue());
    if (qmgr.empty() || qmgr == "None")
    {
        displayMessage("Choose a queue manager first. Discovery asks that "
                       "queue manager which queues the machine has.");
        return;
    }
    if (!SchedulerQuery::canDiscover(qmgr))
    {
        displayMessage("Queues cannot be discovered for " + qmgr + ". Enter "
                       "them by hand.");
        return;
    }
    if (machine.empty())
    {
        displayMessage("Enter the machine's host name on the Machine tab "
                       "first.");
        return;
    }

    SchedulerQuery::Connection conn = this->connection();
    vector<SchedulerQuery::Queue> found;
    string commands, err;
    bool ok = false;
    runBusy(this, "Discover queues", "Asking " + machine + " for its queues...",
        [&]() {
            SchedulerQuery::Remote r(conn);
            string e;
            if (!r.open(e)) { err = e; return; }
            ok = SchedulerQuery::discover(r, qmgr, found, commands, err);
        });
    if (ok && !found.empty())
    {
        p_discovered.clear();
        for (size_t i = 0; i < found.size(); i++)
            p_discovered.insert(found[i].name);
    }

    wxDialog* dlg = new wxDialog(this, wxID_ANY, "Discover queues",
                                 wxDefaultPosition, wxDefaultSize,
                                 wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    dlg->SetSizer(root);
    wxColour gray = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
    wxFont mono(wxFontInfo().Family(wxFONTFAMILY_TELETYPE));

    wxStaticText* head = new wxStaticText(dlg, wxID_ANY, ok
        ? "Queues that " + qmgr + " reports on " + machine + ". Tick the ones "
          "to add. Limits are filled in; the defaults are left empty."
        : qmgr + " on " + machine + " did not give a list of queues.");
    head->Wrap(560);
    root->Add(head, wxSizerFlags().Border());
    wxStaticText* cmd = new wxStaticText(dlg, wxID_ANY,
        commands.empty() ? string() : "Command: " + commands);
    cmd->SetForegroundColour(gray);
    cmd->SetFont(mono);
    root->Add(cmd, wxSizerFlags().Border(wxLEFT|wxRIGHT));
    reg("disc:command", cmd);

    wxCheckListBox* list = new wxCheckListBox(dlg, wxID_ANY, wxDefaultPosition,
                                              wxSize(560, 200));
    list->SetFont(mono);
    size_t width = 0;
    for (size_t i = 0; i < found.size(); i++)
        width = std::max(width, found[i].name.size());
    vector<string> names;
    for (size_t i = 0; i < found.size(); i++)
    {
        string label = found[i].name +
            string(width - found[i].name.size() + 2, ' ') + found[i].detail;
        list->Append(label);
        list->Check(i, true);
        names.push_back(found[i].name);
    }
    wxTextCtrl* errText = new wxTextCtrl(dlg, wxID_ANY, err, wxDefaultPosition,
        wxSize(560, 120), wxTE_MULTILINE|wxTE_READONLY|wxTE_DONTWRAP);
    errText->SetFont(mono);
    if (ok)
    {
        root->Add(list, wxSizerFlags(1).Expand().Border());
        errText->Hide();
    }
    else
    {
        list->Hide();
        root->Add(errText, wxSizerFlags(1).Expand().Border());
    }
    reg("disc:list", list);
    reg("disc:error", errText);

    wxStaticText* status = new wxStaticText(dlg, wxID_ANY, "");
    status->SetForegroundColour(gray);
    root->Add(status, wxSizerFlags().Border(wxLEFT|wxRIGHT));
    reg("disc:status", status);
    int existing = 0;
    for (size_t i = 0; i < found.size(); i++)
        for (size_t k = 0; k < p_queues.size(); k++)
            if (p_queues[k].name == found[i].name)
                existing++;
    if (ok && existing > 0)
        status->SetLabel(std::to_string(existing) + " of these are in the "
            "list already; adding them updates their limits and keeps their "
            "defaults.");
    status->Wrap(560);

    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    wxButton* none = new ewxButton(dlg, wxID_ANY, "Select None");
    wxButton* all = new ewxButton(dlg, wxID_ANY, "Select All");
    wxButton* cancel = new ewxButton(dlg, wxID_CANCEL, ok ? "Cancel" : "Close");
    wxButton* add = new ewxButton(dlg, wxID_ANY, "Add Queues");
    add->Enable(ok);
    all->Enable(ok);
    none->Enable(ok);
    buttons->Add(all, wxSizerFlags().Border());
    buttons->Add(none, wxSizerFlags().Border());
    buttons->AddStretchSpacer(1);
    buttons->Add(cancel, wxSizerFlags().Border());
    buttons->Add(add, wxSizerFlags().Border());
    root->Add(buttons, wxSizerFlags().Expand());
    add->SetDefault();
    reg("disc:add", add);
    reg("disc:cancel", cancel);
    reg("disc:all", all);
    reg("disc:none", none);

    all->Bind(wxEVT_BUTTON, [list](wxCommandEvent&) {
        for (unsigned i = 0; i < list->GetCount(); i++) list->Check(i, true);
    });
    none->Bind(wxEVT_BUTTON, [list](wxCommandEvent&) {
        for (unsigned i = 0; i < list->GetCount(); i++) list->Check(i, false);
    });
    cancel->Bind(wxEVT_BUTTON, [dlg](wxCommandEvent&) { dlg->Close(); });
    add->Bind(wxEVT_BUTTON, [this, dlg, list, found](wxCommandEvent&) {
        vector<SchedulerQuery::Queue> picked;
        for (unsigned i = 0; i < list->GetCount() && i < found.size(); i++)
            if (list->IsChecked(i))
                picked.push_back(found[i]);
        if (!picked.empty())
            this->addDiscovered(picked);
        dlg->Close();
    });

    dlg->SetMinSize(wxSize(600, 340));
    dlg->Fit();
    this->showTool(dlg, "disc:");
}


//  The limits the scheduler reported into the queue list: a queue that is
//  there keeps its defaults, a new one has none.
void WxMachineRegister::addDiscovered(const vector<SchedulerQuery::Queue>& picked)
{
    for (size_t i = 0; i < picked.size(); i++)
    {
        const SchedulerQuery::Queue& f = picked[i];
        size_t at = 0;
        while (at < p_queues.size() && p_queues[at].name != f.name)
            at++;
        if (at == p_queues.size())
        {
            p_queues.push_back(MCD::QueueRow());
            p_queues.back().name = f.name;
        }
        MCD::QueueRow& r = p_queues[at];
        r.minProcs = 1;
        r.maxProcs = f.maxProcs != 0 ? f.maxProcs : 1;
        r.maxWall = f.maxWallMin;
        r.maxMem = f.maxMemMB;
        //  defaults stay as they were; a default beyond a new limit would
        //  be refused by Add Queue, so clamp what was there.
        if (r.maxProcs && r.defProcs > r.maxProcs) r.defProcs = 0;
        if (r.maxWall && r.defWall > r.maxWall) r.defWall = 0;
        if (r.maxMem && r.defMem > r.maxMem) r.defMem = 0;
    }
    this->fillQueues();
    this->showQueue(picked.front().name);
    p_queueFormBase = queueFormRow();
    this->updateDirty();
}


//  ---- preview --------------------------------------------------------------

void WxMachineRegister::previewJobScript(const string& wantedCode)
{
    vector<string> codes = this->codesWithPath();
    if (codes.empty())
    {
        displayMessage("No code has a program path on this machine yet. Set "
                       "one on the Codes tab (Program) first; the job script "
                       "needs it.");
        return;
    }
    string code = wantedCode;
    if (code.empty() && p_codeSel >= 0 && p_codeSel < (int)p_codeNames.size())
        code = p_codeNames[p_codeSel];
    string queue = (string)p_queueChoice->GetStringSelection();
    string qmgr = (string)p_qmgrChoice->GetStringSelection();
    string accounts = this->defaultAccount();

    wxDialog* dlg = new wxDialog(this, wxID_ANY, "Preview job script",
                                 wxDefaultPosition, wxDefaultSize,
                                 wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    dlg->SetSizer(root);
    wxColour gray = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
    wxFont mono(wxFontInfo().Family(wxFONTFAMILY_TELETYPE));

    wxStaticText* intro = new wxStaticText(dlg, wxID_ANY,
        "The job script ECCE would write for this request, from the form as it "
        "is now, saved or not. Nothing is submitted. The run directory, input "
        "and output names are examples.");
    intro->SetForegroundColour(gray);
    intro->Wrap(640);
    root->Add(intro, wxSizerFlags().Border());

    std::shared_ptr<RequestPanel> panel = std::make_shared<RequestPanel>();
    addRequestControls(this, dlg, root, *panel, codes, code, p_queues, queue,
                       accounts, "prev:", true,
                       [this](const string& n, wxWindow* w) { this->reg(n, w); });

    wxButton* update = new ewxButton(dlg, wxID_ANY, "Show Script");
    root->Add(update, wxSizerFlags().Border(wxLEFT|wxRIGHT));
    reg("prev:update", update);

    //  Legend: the colour and gutter label of each part, and the tab it
    //  comes from.
    wxFlexGridSizer* legend = new wxFlexGridSizer(2, 2, 8);
    static const char* const parts[] = { "request", "before", "environment",
                                         "command", "after", "ecce" };
    for (size_t i = 0; i < 6; i++)
    {
        wxStaticText* sw = new wxStaticText(dlg, wxID_ANY,
            " " + wxString(partShort(parts[i])) + " ",
            wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
        sw->SetBackgroundColour(partColour(parts[i]));
        sw->SetFont(mono);
        legend->Add(sw, wxSizerFlags().Expand());
        legend->Add(new wxStaticText(dlg, wxID_ANY,
                    JobPreview::partTitle(parts[i])),
                    wxSizerFlags().CentreVertical());
    }
    root->Add(legend, wxSizerFlags().Border());
    wxStaticText* layers = new wxStaticText(dlg, wxID_ANY,
        "After the name: user = your setting, site = the site's, built-in = "
        "ECCE's own text.");
    layers->SetForegroundColour(gray);
    root->Add(layers, wxSizerFlags().Border(wxLEFT|wxRIGHT));

    wxTextCtrl* text = new wxTextCtrl(dlg, wxID_ANY, "", wxDefaultPosition,
        wxSize(720, 320),
        wxTE_MULTILINE|wxTE_READONLY|wxTE_DONTWRAP|wxTE_RICH2);
    text->SetFont(mono);
    root->Add(text, wxSizerFlags(1).Expand().Border());
    reg("prev:text", text);

    wxStaticText* status = new wxStaticText(dlg, wxID_ANY, "");
    status->SetForegroundColour(gray);
    root->Add(status, wxSizerFlags().Border(wxLEFT|wxRIGHT));
    reg("prev:status", status);

    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    wxButton* copy = new ewxButton(dlg, wxID_ANY, "Copy Script");
    copy->SetToolTip("Copies the script without the labels");
    wxButton* close = new ewxButton(dlg, wxID_CANCEL, "Close");
    buttons->Add(copy, wxSizerFlags().Border());
    buttons->AddStretchSpacer(1);
    buttons->Add(close, wxSizerFlags().Border());
    root->Add(buttons, wxSizerFlags().Expand());
    reg("prev:copy", copy);
    reg("prev:close", close);

    auto lines = std::make_shared<vector<JobPreview::Line> >();
    auto show = [this, panel, text, status, lines, qmgr]() {
        string cfg, err;
        if (!this->draftConfigText(cfg, err))
        {
            text->SetValue("");
            status->SetLabel("Cannot build the settings file: " + err);
            return;
        }
        string host = stripped((string)p_refName->GetValue());
        if (host.empty())
            host = "unnamed";
        JobPreview::Request req = requestFrom(*panel, p_adminFlag, host,
            stripped((string)p_fullName->GetValue()), qmgr);
        req.configText = cfg;
        string gerr;
        text->Clear();
        lines->clear();
        if (!JobPreview::generate(req, *lines, gerr))
        {
            text->SetValue(gerr);
            status->SetLabel("No script could be made:");
            return;
        }
        size_t width = 0;
        vector<string> gutter;
        for (size_t i = 0; i < lines->size(); i++)
        {
            const JobPreview::Line& l = (*lines)[i];
            string g = partShort(l.part);
            if (l.part != "ecce" && !l.layer.empty())
                g += " (" + l.layer + ")";
            gutter.push_back(g);
            width = std::max(width, g.size());
        }
        for (size_t i = 0; i < lines->size(); i++)
        {
            const JobPreview::Line& l = (*lines)[i];
            string row = gutter[i] + string(width - gutter[i].size(), ' ') +
                         " | " + l.text + "\n";
            long from = text->GetLastPosition();
            text->AppendText(wxString::FromUTF8(row.c_str()));
            if (l.part != "ecce")
                text->SetStyle(from, text->GetLastPosition(),
                               wxTextAttr(wxNullColour, partColour(l.part)));
        }
        text->ShowPosition(0);
        status->SetLabel(std::to_string(lines->size()) + " lines.");
    };
    update->Bind(wxEVT_BUTTON, [show](wxCommandEvent&) { show(); });
    panel->queue->Bind(wxEVT_CHOICE, [this, panel, show](wxCommandEvent&) {
        string q = (string)panel->queue->GetStringSelection();
        queueDefaults(p_queues, q, *panel);
        show();
    });
    panel->code->Bind(wxEVT_CHOICE, [show](wxCommandEvent&) { show(); });
    copy->Bind(wxEVT_BUTTON, [lines](wxCommandEvent&) {
        if (wxTheClipboard->Open())
        {
            wxTheClipboard->SetData(new wxTextDataObject(
                wxString::FromUTF8(JobPreview::plainText(*lines).c_str())));
            wxTheClipboard->Close();
        }
    });
    close->Bind(wxEVT_BUTTON, [dlg](wxCommandEvent&) { dlg->Close(); });

    show();
    dlg->Fit();
    this->showTool(dlg, "prev:");
}


//  ---- test submission ----------------------------------------------------------

void WxMachineRegister::testSubmission()
{
    string qmgr = (string)p_qmgrChoice->GetStringSelection();
    string machine = stripped((string)p_fullName->GetValue());
    SchedulerQuery::TestPlan plan = SchedulerQuery::testPlan(qmgr);
    if (!plan.known)
    {
        displayMessage("Choose a queue manager first. The test hands a small "
                       "script to that queue manager.");
        return;
    }
    if (machine.empty())
    {
        displayMessage("Enter the machine's host name on the Machine tab "
                       "first.");
        return;
    }

    string queue = (string)p_queueChoice->GetStringSelection();
    wxDialog* dlg = new wxDialog(this, wxID_ANY, "Test submission",
                                 wxDefaultPosition, wxDefaultSize,
                                 wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    dlg->SetSizer(root);
    wxColour gray = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
    wxFont mono(wxFontInfo().Family(wxFONTFAMILY_TELETYPE));

    wxStaticText* intro = new wxStaticText(dlg, wxID_ANY,
        "Copies a small script to " + machine + " and asks " + qmgr + " whether "
        "it would accept it: " + plan.what + ". The script holds this "
        "machine's request lines for the queue below and no calculation; "
        "nothing ECCE runs for you is started. This connects to the machine.");
    intro->SetForegroundColour(gray);
    intro->Wrap(640);
    root->Add(intro, wxSizerFlags().Border());

    std::shared_ptr<RequestPanel> panel = std::make_shared<RequestPanel>();
    vector<string> codes;
    addRequestControls(this, dlg, root, *panel, codes, "", p_queues, queue,
                       this->defaultAccount(), "test:", false,
                       [this](const string& n, wxWindow* w) { this->reg(n, w); });

    wxCheckBox* hold = NULL;
    if (plan.holdAndCancel)
    {
        hold = new wxCheckBox(dlg, wxID_ANY,
            "This queue manager has no dry run. Submit the script on hold and "
            "cancel it at once.");
        root->Add(hold, wxSizerFlags().Border());
        reg("test:hold", hold);
    }

    wxButton* run = new ewxButton(dlg, wxID_ANY, "Run Test");
    run->Enable(hold == NULL);
    root->Add(run, wxSizerFlags().Border(wxLEFT|wxRIGHT));
    reg("test:run", run);
    if (hold != NULL)
        hold->Bind(wxEVT_CHECKBOX, [run, hold](wxCommandEvent&) {
            run->Enable(hold->IsChecked());
        });

    root->Add(new wxStaticText(dlg, wxID_ANY, "Answer from " + qmgr),
              wxSizerFlags().Border(wxLEFT|wxRIGHT|wxTOP));
    wxTextCtrl* result = new wxTextCtrl(dlg, wxID_ANY, "", wxDefaultPosition,
        wxSize(720, 240), wxTE_MULTILINE|wxTE_READONLY|wxTE_DONTWRAP);
    result->SetFont(mono);
    root->Add(result, wxSizerFlags(1).Expand().Border());
    reg("test:result", result);
    wxStaticText* verdict = new wxStaticText(dlg, wxID_ANY, "");
    wxFont vf = verdict->GetFont();
    vf.MakeBold();
    verdict->SetFont(vf);
    root->Add(verdict, wxSizerFlags().Border(wxLEFT|wxRIGHT));
    reg("test:verdict", verdict);

    wxButton* close = new ewxButton(dlg, wxID_CANCEL, "Close");
    root->Add(close, wxSizerFlags().Right().Border());
    reg("test:close", close);
    close->Bind(wxEVT_BUTTON, [dlg](wxCommandEvent&) { dlg->Close(); });

    run->Bind(wxEVT_BUTTON, [this, panel, result, verdict, qmgr, machine](wxCommandEvent&) {
        string cfg, err;
        vector<string> codes = this->codesWithPath();
        //  The request lines do not depend on the code, but gensub wants a
        //  code with a path: borrow a path for the temporary file only.
        string code = codes.empty() ? string("NWChem") : codes[0];
        string extra = codes.empty() ? "\n" + code + ": /bin/true\n" : "";
        if (!this->draftConfigText(cfg, err, extra))
        {
            result->SetValue("Cannot build the settings file: " + err);
            return;
        }
        SchedulerQuery::Connection conn = this->connection();
        string host = stripped((string)p_refName->GetValue());
        if (host.empty())
            host = "unnamed";
        JobPreview::Request req = requestFrom(*panel, p_adminFlag, host,
                                              machine, qmgr);
        req.code = code;
        req.configText = cfg;
        SchedulerQuery::TestResult tr;
        string terr, scriptErr;
        bool ok = false;
        result->SetValue("Asking " + machine + "...");
        verdict->SetLabel("");
        runBusy(this, "Test submission", "Asking " + machine + "...", [&]() {
            SchedulerQuery::Remote r(conn);
            string e;
            if (!r.open(e))
            {
                terr = "Could not log in to " + machine + ".\n" + e;
                return;
            }
            //  The run directory must exist on the machine for HTCondor.
            req.runDir = r.home();
            vector<JobPreview::Line> lines;
            if (!JobPreview::generate(req, lines, scriptErr))
                return;
            string script = "#!/bin/sh\n";
            for (size_t i = 0; i < lines.size(); i++)
                if (lines[i].part == "request")
                    script += lines[i].text + "\n";
            script += "exit 0\n";
            ok = SchedulerQuery::testSubmission(r, qmgr, script, tr, terr);
        });
        if (!scriptErr.empty())
        {
            result->SetValue("No script could be made:\n" + scriptErr);
            verdict->SetLabel("Not tested");
            return;
        }
        if (!ok)
        {
            string text = "The test did not run.\n";
            if (!tr.commands.empty())
                text += "\nCommand run on " + machine + ":\n" + tr.commands +
                        "\n";
            text += "\n" + (terr.empty() ? string("No reason was given; the "
                    "connection may have been refused or timed out.") : terr)
                    + "\n";
            result->SetValue(wxString::FromUTF8(text.c_str()));
            verdict->SetLabel("Not tested");
            return;
        }
        string text = "Command run on " + machine + ":\n" + tr.commands +
                      "\n\nThe scheduler's answer:\n" + tr.answer + "\n";
        result->SetValue(wxString::FromUTF8(text.c_str()));
        string v;
        if (!tr.accepted)
            v = qmgr + " did not accept the script.";
        else if (tr.cancelled)
            v = "Job " + tr.jobId + " was submitted on hold and cancelled.";
        else
            v = qmgr + " accepted the script; nothing was submitted" +
                (tr.jobId.empty() ? string() : " (job " + tr.jobId +
                 " would have started)") + ".";
        verdict->SetLabel(wxString::FromUTF8(v.c_str()));
    });

    dlg->Fit();
    this->showTool(dlg, "test:");
}
