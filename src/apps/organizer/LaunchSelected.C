#include "wx/wxprec.h"
#include "wx/wx.h"
#include "wx/listctrl.h"
#include "wx/progdlg.h"

#include <atomic>
#include <thread>

#include "LaunchSelected.H"
#include "dsm/TaskJob.H"
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

vector<LaunchSelected::Row> LaunchSelected::plan(const vector<TaskJob*>& tasks)
{
  vector<Row> rows;
  for (TaskJob *t : tasks) {
    Row r;
    r.task = t;
    r.plan = LaunchBatch::plan(t);
    rows.push_back(r);
  }
  return rows;
}

bool LaunchSelected::confirm(wxWindow *parent, vector<Row>& rows,
    const std::function<void(const string&)>& openInLauncher, const string& shot)
{
  size_t ready = 0;
  for (const Row& r : rows) if (r.plan.ready) ready++;

  ewxDialog dlg(parent, wxID_ANY, "Launch Selected", wxDefaultPosition,
                wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxString head;
  head.Printf(ready == 1 ? "Submit %zu calculation with the launch settings it "
                           "already stores?"
                         : "Submit %zu calculations with the launch settings "
                           "they already store?", ready);
  top->Add(new wxStaticText(&dlg, wxID_ANY, head), 0, wxALL, 12);

  wxListCtrl *list = new wxListCtrl(&dlg, wxID_ANY, wxDefaultPosition,
                                    wxSize(760, 300), wxLC_REPORT | wxLC_SINGLE_SEL);
  const char *cols[] = {"Calculation", "Machine", "Queue", "Cores", "Wall time", "Check"};
  const int widths[] = {200, 100, 80, 60, 80, 220};
  for (int i = 0; i < 6; i++) list->AppendColumn(cols[i], wxLIST_FORMAT_LEFT, widths[i]);
  //  Ready rows first, skipped ones after, so the batch reads as the list.
  vector<size_t> order;
  for (size_t i = 0; i < rows.size(); i++) if (rows[i].plan.ready) order.push_back(i);
  for (size_t i = 0; i < rows.size(); i++) if (!rows[i].plan.ready) order.push_back(i);
  for (size_t k = 0; k < order.size(); k++) {
    const LaunchBatch::Plan& p = rows[order[k]].plan;
    long item = list->InsertItem(k, U(p.name));
    list->SetItem(item, 1, U(p.machine.empty() ? "-" : p.machine));
    list->SetItem(item, 2, U(p.queue.empty() ? "-" : p.queue));
    list->SetItem(item, 3, p.procs ? wxString::Format("%lu", p.procs) : wxString("-"));
    list->SetItem(item, 4, U(wallText(p.wall)));
    list->SetItem(item, 5, p.ready ? U("OK") : U("Skipped: " + p.problem));
    if (!p.ready)
      list->SetItemTextColour(item, wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
  }
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
  wxButton *go = new wxButton(&dlg, wxID_OK, "Launch");
  go->Enable(ready > 0);
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
