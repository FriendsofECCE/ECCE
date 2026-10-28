#include "wx/wxprec.h"
#include "wx/wx.h"
#include "wx/listctrl.h"
#include "wx/progdlg.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include "ReconnectSelected.H"
#include "dsm/TaskJob.H"
#include "wxgui/WindowShot.H"
#include "wxgui/WxJMSSubscriber.H"
#include "wxgui/ewxDialog.H"

using std::string;
using std::vector;

namespace {
wxString U(const string& s) { return wxString::FromUTF8(s.c_str()); }
}

vector<ReconnectSelected::Row> ReconnectSelected::plan(const vector<TaskJob*>& tasks)
{
  vector<Row> rows;
  for (TaskJob *t : tasks) {
    Row r;
    r.task = t;
    r.plan = JobCatchUp::plan(t);
    rows.push_back(r);
  }
  std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
    if (a.eligible() != b.eligible()) return a.eligible();
    return a.eligible() && a.plan.machine < b.plan.machine;
  });
  return rows;
}

bool ReconnectSelected::confirm(wxWindow *parent, vector<Row>& rows, const string& shot)
{
  size_t ready = 0;
  for (const Row& r : rows) if (r.eligible()) ready++;

  ewxDialog dlg(parent, wxID_ANY, "Reconnect Selected", wxDefaultPosition,
                wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxString head;
  head.Printf(ready == 1 ? "Reconnect %zu calculation to its job?"
                         : "Reconnect %zu calculations to their jobs?", ready);
  top->Add(new wxStaticText(&dlg, wxID_ANY, head), 0, wxALL, 12);

  wxListCtrl *list = new wxListCtrl(&dlg, wxID_ANY, wxDefaultPosition,
                                    wxSize(700, 300), wxLC_REPORT | wxLC_SINGLE_SEL);
  const char *cols[] = {"Calculation", "Machine", "State", "Check"};
  const int widths[] = {200, 110, 130, 240};
  for (int i = 0; i < 4; i++) list->AppendColumn(cols[i], wxLIST_FORMAT_LEFT, widths[i]);
  for (size_t k = 0; k < rows.size(); k++) {
    const JobCatchUp::Plan& p = rows[k].plan;
    long item = list->InsertItem(k, U(p.name));
    list->SetItem(item, 1, U(p.machine.empty() ? "-" : p.machine));
    list->SetItem(item, 2, U(p.state));
    list->SetItem(item, 3, rows[k].eligible() ? U("OK") : U("Skipped: " + p.problem));
    if (!rows[k].eligible())
      list->SetItemTextColour(item, wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
  }
  top->Add(list, 1, wxLEFT | wxRIGHT | wxEXPAND, 12);

  wxStdDialogButtonSizer *buttons = new wxStdDialogButtonSizer();
  wxButton *go = new wxButton(&dlg, wxID_OK, "Reconnect");
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

void ReconnectSelected::run(wxWindow *parent, vector<Row>& rows, bool progress,
                            const std::function<bool(Row&, wxProgressDialog*)>& one)
{
  size_t total = 0, n = 0;
  for (const Row& r : rows) if (r.eligible()) total++;

  DeferMessages defer;
  wxProgressDialog *pd = 0;
  if (progress)
    pd = new wxProgressDialog("Reconnect Selected", "Starting...", (int)total,
                              parent, wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_SMOOTH);
  for (Row& r : rows) {
    if (!r.eligible()) continue;
    n++;
    if (pd) {
      string msg = "Reconnecting " + std::to_string(n) + " of " +
                   std::to_string(total) + " \xE2\x80\x93 " + r.plan.name;
      if (!pd->Update((int)n - 1, U(msg))) break;
    }
    r.attempted = true;
    r.ok = one(r, pd);
    if (pd && pd->WasCancelled()) break;
  }
  if (pd) { pd->Update((int)total); delete pd; }
}

ReconnectSelected::DeferMessages::DeferMessages()
{
  if (WxJMSSubscriber *s = dynamic_cast<WxJMSSubscriber*>(wxTheApp)) s->deferMessages(true);
}

ReconnectSelected::DeferMessages::~DeferMessages()
{
  if (WxJMSSubscriber *s = dynamic_cast<WxJMSSubscriber*>(wxTheApp)) s->deferMessages(false);
}

bool ReconnectSelected::openLogin(wxWindow *parent, wxProgressDialog *pd,
                                  const RCommandLogin& login, RCommand*& conn,
                                  string& why, bool *cancelled)
{
  struct State {
    std::mutex mu;
    bool done = false, abandoned = false;
    RCommand *conn = 0;
  };
  auto st = std::make_shared<State>();
  conn = 0;
  if (cancelled) *cancelled = false;
  //  Detached: a Cancel must not wait for a connect that may never return.
  std::thread([st, login]() {
    RCommand *c = login.open();
    std::lock_guard<std::mutex> lock(st->mu);
    if (st->abandoned) delete c;
    else st->conn = c;
    st->done = true;
  }).detach();

  const wxString msg = U("Connecting to " + login.machine + "...");
  wxWindowDisabler off(pd);
  std::unique_ptr<wxProgressDialog> own;
  const auto t0 = std::chrono::steady_clock::now();
  for (;;) {
    {
      std::lock_guard<std::mutex> lock(st->mu);
      if (st->done) break;
    }
    if (!pd && !own && std::chrono::steady_clock::now() - t0 > std::chrono::seconds(1))
      own.reset(new wxProgressDialog("ECCE", msg, 100, parent,
                                     wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_SMOOTH));
    wxProgressDialog *d = pd ? pd : own.get();
    if (d && !d->Pulse(msg)) {
      std::lock_guard<std::mutex> lock(st->mu);
      if (!st->done) {
        st->abandoned = true;
        why = "Cancelled while connecting to " + login.machine;
        if (cancelled) *cancelled = true;
        return false;
      }
      break;
    }
    wxTheApp->Yield(true);
    wxMilliSleep(50);
  }
  conn = st->conn;
  if (!conn) { why = "Unable to connect to " + login.machine; return false; }
  if (!conn->isOpen()) {
    why = conn->commError();
    delete conn;
    conn = 0;
    return false;
  }
  return true;
}

string ReconnectSelected::listing(const vector<Row>& rows)
{
  string s;
  for (const Row& r : rows)
    s += r.plan.name + "|" + r.plan.machine + "|" + r.plan.state + "|" +
         (r.eligible() ? string("ok") : r.plan.problem) + "\n";
  return s;
}

string ReconnectSelected::report(const vector<Row>& rows)
{
  string s;
  for (const Row& r : rows) {
    s += r.plan.name + ": ";
    if (!r.eligible()) s += "skipped: " + r.plan.problem;
    else if (!r.attempted) s += "not attempted";
    else if (r.ok) s += "reconnected";
    else s += "failed: " + r.outcome;
    s += "\n";
  }
  return s;
}

void ReconnectSelected::showResults(wxWindow *parent, const vector<Row>& rows)
{
  size_t ok = 0, bad = 0;
  for (const Row& r : rows) {
    if (r.attempted && r.ok) ok++;
    else if (r.attempted) bad++;
  }
  ewxDialog dlg(parent, wxID_ANY, "Reconnect Selected", wxDefaultPosition,
                wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  top->Add(new wxStaticText(&dlg, wxID_ANY,
           wxString::Format("%zu reconnected, %zu failed.", ok, bad)), 0, wxALL, 12);
  wxListCtrl *list = new wxListCtrl(&dlg, wxID_ANY, wxDefaultPosition,
                                    wxSize(640, 280), wxLC_REPORT);
  list->AppendColumn("Calculation", wxLIST_FORMAT_LEFT, 220);
  list->AppendColumn("Result", wxLIST_FORMAT_LEFT, 100);
  list->AppendColumn("Details", wxLIST_FORMAT_LEFT, 300);
  long i = 0;
  for (const Row& r : rows) {
    long item = list->InsertItem(i++, U(r.plan.name));
    list->SetItem(item, 1, !r.eligible() ? "Skipped" : r.ok ? "Reconnected"
                  : r.attempted ? "Failed" : "Not run");
    list->SetItem(item, 2, U(r.eligible() ? r.outcome : r.plan.problem));
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
