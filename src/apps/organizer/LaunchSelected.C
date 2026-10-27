#include "wx/wxprec.h"
#include "wx/wx.h"
#include "wx/listctrl.h"
#include "wx/progdlg.h"

#include <atomic>
#include <thread>

#include "LaunchSelected.H"
#include "dsm/MachinePreferences.H"
#include "dsm/TaskJob.H"
#include "tdat/RefMachine.H"
#include "dsm/AllocationProfiles.H"
#include "dsm/ResourceDescriptor.H"
#include "wxgui/WindowShot.H"
#include "wxgui/WxJMSSubscriber.H"
#include "wxgui/ewxDialog.H"

using std::string;
using std::vector;

namespace {

wxString U(const string& s) { return wxString::FromUTF8(s.c_str()); }

//  "0 4:0:00" (days h:m) as "4 h" style text; unset as "-".
string wallText(const string& w)
{
  int d = 0, h = 0, m = 0;
  if (sscanf(w.c_str(), "%d %d:%d", &d, &h, &m) < 3) return w.empty() ? "-" : w;
  h += d*24;
  char buf[32];
  snprintf(buf, sizeof buf, "%d:%02d h", h, m);
  return buf;
}

}

vector<LaunchSelected::Row> LaunchSelected::plan(const vector<TaskJob*>& tasks,
    const LaunchBatch::Target& target)
{
  //  Machines registered since the Organizer started, also by editing the
  //  files, must be found (#588).
  RefMachine::markUpdateUserPrefs();
  MachinePreferences::refresh(true);
  vector<Row> rows;
  for (TaskJob *t : tasks) {
    Row r;
    r.task = t;
    r.plan = LaunchBatch::plan(t, target);
    rows.push_back(r);
  }
  return rows;
}

bool LaunchSelected::confirm(wxWindow *parent, vector<Row>& rows,
    LaunchBatch::Target& target,
    const std::function<void(const string&)>& openInLauncher, const string& shot)
{
  ewxDialog dlg(parent, wxID_ANY, "Launch Selected", wxDefaultPosition,
                wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);

  //  "As set up" keeps each calculation's stored machine; a machine carries
  //  the settings over to it.
  wxBoxSizer *onRow = new wxBoxSizer(wxHORIZONTAL);
  onRow->Add(new wxStaticText(&dlg, wxID_ANY, "Launch on:"), 0,
             wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  wxChoice *machineChoice = new wxChoice(&dlg, wxID_ANY);
  machineChoice->SetToolTip("As set up submits each calculation with the machine "
                            "and settings it stores. A machine submits them all "
                            "there, with settings cut to its limits.");
  machineChoice->Append("As set up");
  vector<string> machines;
  for (int i = 0; i < MachinePreferences::countItems(); i++) {
    MachinePreferences *mp = MachinePreferences::getItemAt(i);
    if (mp && mp->getRegisteredMachine()) {
      machines.push_back(mp->getItemKey());
      machineChoice->Append(U(mp->getItemKey()));
    }
  }
  onRow->Add(machineChoice, 0, wxALIGN_CENTER_VERTICAL);
  wxStaticText *profileLabel = new wxStaticText(&dlg, wxID_ANY, "Profile:");
  wxChoice *profileChoice = new wxChoice(&dlg, wxID_ANY);
  onRow->Add(profileLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);
  onRow->Add(profileChoice, 0, wxALIGN_CENTER_VERTICAL);
  top->Add(onRow, 0, wxLEFT | wxRIGHT | wxTOP, 12);

  wxStaticText *headText = new wxStaticText(&dlg, wxID_ANY, "");
  top->Add(headText, 0, wxALL, 12);

  wxListCtrl *list = new wxListCtrl(&dlg, wxID_ANY, wxDefaultPosition,
                                    wxSize(960, 300), wxLC_REPORT | wxLC_SINGLE_SEL);
  const char *cols[] = {"Calculation", "Machine", "Queue", "Cores", "Wall time",
                        "Changed", "Check"};
  const int widths[] = {150, 80, 70, 55, 75, 420, 80};
  for (int i = 0; i < 7; i++) list->AppendColumn(cols[i], wxLIST_FORMAT_LEFT, widths[i]);
  vector<size_t> order;
  wxButton *go = new wxButton(&dlg, wxID_OK, "Launch");

  auto refill = [&]() {
    size_t ready = 0;
    for (const Row& r : rows) if (r.plan.ready) ready++;
    wxString head;
    if (target.any())
      head.Printf("Submit %zu on %s with the settings below?", ready, U(target.machine));
    else
      head.Printf(ready == 1 ? "Submit %zu calculation with the launch settings it "
                               "already stores?"
                             : "Submit %zu calculations with the launch settings "
                               "they already store?", ready);
    headText->SetLabel(head);
    list->DeleteAllItems();
    //  Ready rows first, skipped ones after, so the batch reads as the list.
    order.clear();
    for (size_t i = 0; i < rows.size(); i++) if (rows[i].plan.ready) order.push_back(i);
    for (size_t i = 0; i < rows.size(); i++) if (!rows[i].plan.ready) order.push_back(i);
    for (size_t k = 0; k < order.size(); k++) {
      const LaunchBatch::Plan& p = rows[order[k]].plan;
      long item = list->InsertItem(k, U(p.name));
      list->SetItem(item, 1, U(p.machine.empty() ? "-" : p.machine));
      list->SetItem(item, 2, U(p.queue.empty() ? "-" : p.queue));
      list->SetItem(item, 3, p.procs ? wxString::Format("%lu", p.procs) : wxString("-"));
      list->SetItem(item, 4, U(wallText(p.wall)));
      list->SetItem(item, 5, U(p.changes.empty() ? (p.ready && p.retarget ? "none" : "") : p.changes));
      list->SetItem(item, 6, p.ready ? U("OK") : U("Skipped: " + p.problem));
      if (!p.ready)
        list->SetItemTextColour(item, wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    }
    go->Enable(ready > 0);
  };

  auto replan = [&]() {
    for (Row& r : rows) r.plan = LaunchBatch::plan(r.task, target);
    refill();
  };

  //  The profile row only shows for a machine with more than one profile.
  auto showProfiles = [&]() {
    vector<string> names = target.any() ? LaunchBatch::profileNames(target.machine)
                                        : vector<string>();
    bool several = names.size() > 1;
    profileChoice->Clear();
    for (const string& n : names) profileChoice->Append(U(n));
    if (several && !profileChoice->SetStringSelection(U(target.profile)))
      profileChoice->SetStringSelection(U(AllocationProfiles::defaultName()));
    profileLabel->Show(several);
    profileChoice->Show(several);
    dlg.Layout();
  };

  {
    int sel = 0;
    for (size_t i = 0; i < machines.size(); i++)
      if (machines[i] == target.machine) sel = (int)i + 1;
    if (target.any() && sel == 0) target = LaunchBatch::Target();
    machineChoice->SetSelection(sel);
  }
  if (target.any()) replan(); else refill();
  showProfiles();

  machineChoice->Bind(wxEVT_CHOICE, [&](wxCommandEvent&) {
    int k = machineChoice->GetSelection();
    target = LaunchBatch::Target();
    if (k > 0) target.machine = machines[k - 1];
    showProfiles();
    replan();
  });
  profileChoice->Bind(wxEVT_CHOICE, [&](wxCommandEvent&) {
    int k = profileChoice->GetSelection();
    target.profile = k >= 0 ? string(profileChoice->GetString(k).ToUTF8()) : string();
    replan();
  });
  top->Add(list, 1, wxLEFT | wxRIGHT | wxEXPAND, 12);

  wxButton *openBtn = new wxButton(&dlg, wxID_ANY, "Open in Launcher");
  openBtn->Enable(false);
  top->Add(openBtn, 0, wxALL, 12);
  list->Bind(wxEVT_LIST_ITEM_SELECTED, [&](wxListEvent& e) {
    openBtn->Enable(!rows[order[e.GetIndex()]].plan.ready);
  });
  list->Bind(wxEVT_LIST_ITEM_DESELECTED, [&](wxListEvent&) { openBtn->Enable(false); });
  openBtn->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
    long sel = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel >= 0 && !rows[order[sel]].plan.ready)
      openInLauncher(rows[order[sel]].plan.url);
  });

  wxStdDialogButtonSizer *buttons = new wxStdDialogButtonSizer();
  go->SetDefault();
  buttons->AddButton(new wxButton(&dlg, wxID_CANCEL, "Cancel"));
  buttons->AddButton(go);
  buttons->Realize();
  top->Add(buttons, 0, wxALL | wxALIGN_RIGHT, 12);
  dlg.SetSizerAndFit(top);
  dlg.CentreOnParent();

  if (!shot.empty()) {
    dlg.Show();
    for (int i = 0; i < 5; i++) { dlg.Update(); wxTheApp->Yield(true); wxMilliSleep(50); }
    ecceClientShot(&dlg, U(shot));
    dlg.Hide();
    return false;
  }
  return dlg.ShowModal() == wxID_OK;
}

void LaunchSelected::run(wxWindow *parent, vector<Row>& rows, bool progress)
{
  size_t total = 0, n = 0;
  for (const Row& r : rows) if (r.plan.ready) total++;

  //  As the Launcher: broker messages wait until the steps are done, and no
  //  input reaches the windows that show the calculations being launched.
  struct Defer {
    WxJMSSubscriber *s;
    Defer() : s(dynamic_cast<WxJMSSubscriber*>(wxTheApp)) { if (s) s->deferMessages(true); }
    ~Defer() { if (s) s->deferMessages(false); }
  } defer;
  wxWindowDisabler disabler;
  wxProgressDialog *pd = 0;
  if (progress)
    pd = new wxProgressDialog("Launch Selected", "Starting...", (int)total,
                              parent, wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_SMOOTH);
  bool aborted = false;

  for (Row& r : rows) {
    if (!r.plan.ready || aborted) continue;
    n++;
    r.attempted = true;
    if (pd) {
      string msg = "Submitting " + std::to_string(n) + " of " +
                   std::to_string(total) + " \xE2\x80\x93 " + r.plan.name;
      if (!pd->Update((int)n - 1, U(msg))) { aborted = true; r.attempted = false; break; }
    }
    if (!LaunchBatch::storeTarget(r.task, r.plan)) {
      r.outcome = "Unable to save launch settings for " + r.plan.name;
      LaunchBatch::recordFailure(r.task, r.outcome);
      continue;
    }
    LaunchBatch::Job job(r.task);
    string error;
    if (!job.begin(error)) {
      r.outcome = error;
      LaunchBatch::recordFailure(r.task, error);
      continue;
    }
    bool ok = true;
    while (ok && !job.done()) {
      std::atomic<bool> finished(false);
      bool stepOk = false;
      std::thread worker([&]() { stepOk = job.step(); finished = true; });
      while (!finished) {
        wxTheApp->Yield(true);
        wxMilliSleep(20);
      }
      worker.join();
      ok = stepOk;
      if (!ok) {
        r.outcome = job.message();
        LaunchBatch::recordFailure(r.task, r.outcome);
        break;
      }
      //  A launch check only warns: the batch goes on, the text is kept.
      string w = job.takeWarning();
      if (!w.empty()) {
        size_t cut = w.find("\nECCE-ORIGINAL-MESSAGE\n");
        r.outcome = w.substr(0, cut);
      }
    }
    if (ok) {
      r.submitted = r.task->getState() <= ResourceDescriptor::STATE_COMPLETED;
      if (!r.submitted && r.outcome.empty()) r.outcome = "the queue did not accept the job";
      if (r.submitted) LaunchBatch::clearFailure(r.task);
      else LaunchBatch::recordFailure(r.task, r.outcome);
    } else if (r.outcome.empty()) {
      r.outcome = "launch failed";
      LaunchBatch::recordFailure(r.task, r.outcome);
    }
  }
  if (pd) { pd->Update((int)total); delete pd; }
}

string LaunchSelected::report(const vector<Row>& rows)
{
  string s;
  for (const Row& r : rows) {
    s += r.plan.name + ": ";
    if (!r.plan.ready) s += "skipped: " + r.plan.problem;
    else if (!r.attempted) s += "not attempted";
    else if (r.submitted) s += "submitted" + (r.outcome.empty() ? string() : ": " + r.outcome);
    else s += "failed: " + r.outcome;
    if (r.plan.ready && !r.plan.changes.empty()) s += " [" + r.plan.changes + "]";
    s += "\n";
  }
  return s;
}

void LaunchSelected::showResults(wxWindow *parent, const vector<Row>& rows)
{
  size_t ok = 0, bad = 0;
  for (const Row& r : rows) {
    if (r.submitted) ok++;
    else if (r.attempted) bad++;
  }
  ewxDialog dlg(parent, wxID_ANY, "Launch Selected", wxDefaultPosition,
                wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxString head = wxString::Format("%zu submitted, %zu failed.", ok, bad);
  top->Add(new wxStaticText(&dlg, wxID_ANY, head), 0, wxALL, 12);
  wxListCtrl *list = new wxListCtrl(&dlg, wxID_ANY, wxDefaultPosition,
                                    wxSize(640, 280), wxLC_REPORT);
  list->AppendColumn("Calculation", wxLIST_FORMAT_LEFT, 220);
  list->AppendColumn("Result", wxLIST_FORMAT_LEFT, 90);
  list->AppendColumn("Details", wxLIST_FORMAT_LEFT, 300);
  long i = 0;
  for (const Row& r : rows) {
    long item = list->InsertItem(i++, U(r.plan.name));
    list->SetItem(item, 1, !r.plan.ready ? "Skipped" : r.submitted ? "Submitted"
                  : r.attempted ? "Failed" : "Not run");
    list->SetItem(item, 2, U(r.plan.ready ? r.outcome : r.plan.problem));
  }
  top->Add(list, 1, wxLEFT | wxRIGHT | wxEXPAND, 12);
  wxStdDialogButtonSizer *buttons = new wxStdDialogButtonSizer();
  wxButton *close = new wxButton(&dlg, wxID_OK, "Close");
  close->SetDefault();
  buttons->AddButton(close);
  buttons->Realize();
  top->Add(buttons, 0, wxALL | wxALIGN_RIGHT, 12);
  dlg.SetSizerAndFit(top);
  dlg.CentreOnParent();
  dlg.ShowModal();
}
