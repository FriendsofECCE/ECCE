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

#include "MemoryUnits.H"
#include "SchedulerQuery.H"
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
    std::thread worker([&work, &done]() { work(); done = true; });
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


//  ---- discovery ------------------------------------------------------------

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


