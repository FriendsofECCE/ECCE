/**
 * @file
 *
 * Feedback while a tool starts (#1071), see ToolStartup.H.
 */
#include <cstdlib>

#include "wx/app.h"
#include "wx/button.h"
#include "wx/cursor.h"
#include "wx/frame.h"
#include "wx/gauge.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/utils.h"

#include "wxgui/WindowShot.H"

#include "ToolStartup.H"

namespace {

const long SHOW_AFTER = 1500;    // ms until the small window
const long GIVE_UP_AFTER = 30000;

// ECCE_TEST_TOOL_DELAY=<ms>: up() is held back that long since begin(),
// so a test can see the slow-start window with any tool.
long testDelay()
{
  const char *d = getenv("ECCE_TEST_TOOL_DELAY");
  return d ? atol(d) : 0;
}

string calcName(string url)
{
  while (!url.empty() && url.back() == '/') url.pop_back();
  size_t slash = url.rfind('/');
  return slash == string::npos ? url : url.substr(slash + 1);
}

string normal(string url)
{
  if (!url.empty() && url.back() != '/') url += '/';
  return url;
}

}


ToolStartup::ToolStartup(wxWindow *parent,
                         std::function<void(const string&)> setStatus)
  : p_parent(parent), p_setStatus(setStatus)
{
  p_timer.SetOwner(this);
  Bind(wxEVT_TIMER, &ToolStartup::tick, this);
}


ToolStartup::~ToolStartup()
{
  p_timer.Stop();
  setBusy(false);
  if (p_frame) p_frame->Destroy();
}


void ToolStartup::setBusy(bool on)
{
  if (on == p_busy) return;
  p_busy = on;
  if (on) wxBeginBusyCursor(); else wxEndBusyCursor();
}


int ToolStartup::starting() const
{
  int n = 0;
  for (const Entry& e : p_entries) if (!e.timedOut) n++;
  return n;
}


bool ToolStartup::begin(const string& app, const string& label,
                        const string& url, bool forceNew)
{
  string key = normal(url);
  if (!forceNew)
    for (const Entry& e : p_entries)
      if (e.app == app && e.url == key && !e.timedOut) return false;

  Entry e{app, label, key, calcName(url), wxGetLocalTimeMillis().GetValue(),
          false, false};
  p_entries.push_back(e);
  p_launches++;
  setBusy(true);
  refresh();
  if (!p_timer.IsRunning()) p_timer.Start(100);
  return true;
}


void ToolStartup::up(const string& app, const string& url)
{
  string key = normal(url);
  int hit = -1;
  for (size_t i = 0; i < p_entries.size(); i++) {
    if (p_entries[i].app != app) continue;
    if (p_entries[i].url == key) { hit = (int)i; break; }
    if (hit < 0) hit = (int)i;
  }
  if (hit < 0) return;
  long held = testDelay() -
              (wxGetLocalTimeMillis().GetValue() - p_entries[hit].started);
  if (held > 0) { p_entries[hit].deferred = true; return; }
  finish(hit);
}


void ToolStartup::finish(size_t i)
{
  p_entries.erase(p_entries.begin() + i);
  p_last = "up";
  if (starting() == 0) setBusy(false);
  if (p_entries.empty()) p_timer.Stop();
  refresh();
}


void ToolStartup::tick(wxTimerEvent&)
{
  long now = wxGetLocalTimeMillis().GetValue();
  for (size_t i = 0; i < p_entries.size(); i++)
    if (p_entries[i].deferred && now - p_entries[i].started >= testDelay()) {
      finish(i);
      return;
    }
  bool changed = false;
  for (Entry& e : p_entries)
    if (!e.timedOut && now - e.started >= GIVE_UP_AFTER) {
      e.timedOut = true;
      p_last = "timed out";
      changed = true;
    }
  if (changed && starting() == 0) setBusy(false);
  if (changed) refresh();
  else if (p_frame && p_frame->IsShown()) { if (p_gauge->IsShown()) p_gauge->Pulse(); }
  else if (starting() > 0)
    for (const Entry& e : p_entries)
      if (!e.timedOut && now - e.started >= SHOW_AFTER) { refresh(); break; }
}


// Status line and small window follow the entries: the newest tool that is
// still starting, else the newest that gave up, else nothing.
void ToolStartup::refresh()
{
  const Entry *cur = 0;
  for (const Entry& e : p_entries) if (!e.timedOut) cur = &e;
  const Entry *dead = 0;
  for (const Entry& e : p_entries) if (e.timedOut) dead = &e;

  string status;
  if (cur)
    status = "Opening the " + cur->label +
             (cur->calc.empty() ? "" : " for " + cur->calc) + "\xE2\x80\xA6";
  else if (dead)
    status = "The " + dead->label + " has not started.";
  p_status = status;
  p_setStatus(status);

  long now = wxGetLocalTimeMillis().GetValue();
  if (cur && now - cur->started >= SHOW_AFTER) {
    showWindow();
    p_text->SetLabel(wxString::FromUTF8(("Starting the " + cur->label +
                                         "\xE2\x80\xA6").c_str()));
    p_gauge->Show(true);
  } else if (dead && !cur) {
    showWindow();
    p_text->SetLabel(wxString::FromUTF8(
        ("The " + dead->label + " has not started.\n"
         "Help > Report a Problem... collects the log files.").c_str()));
    p_gauge->Hide();
  } else {
    hideWindow();
  }
  if (p_frame) p_frame->GetSizer()->Layout();
  if (p_frame) p_frame->Fit();
}


void ToolStartup::showWindow()
{
  if (!p_frame) {
    p_frame = new wxFrame(p_parent, wxID_ANY, "Starting",
                          wxDefaultPosition, wxDefaultSize,
                          wxCAPTION | wxCLOSE_BOX | wxFRAME_FLOAT_ON_PARENT |
                          wxFRAME_NO_TASKBAR);
    p_text = new wxStaticText(p_frame, wxID_ANY, "");
    p_gauge = new wxGauge(p_frame, wxID_ANY, 100, wxDefaultPosition,
                          wxSize(280, 12));
    wxButton *close = new wxButton(p_frame, wxID_CLOSE, "Close");
    close->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { dismiss(); });
    p_frame->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { dismiss(); });
    wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
    s->Add(p_text, 0, wxALL, 12);
    s->Add(p_gauge, 0, wxLEFT | wxRIGHT | wxEXPAND, 12);
    s->Add(close, 0, wxALL | wxALIGN_RIGHT, 12);
    p_frame->SetSizer(s);
    p_frame->CentreOnParent();
  }
  if (!p_frame->IsShown()) p_frame->Show(true);
}


// The user closed the window: a "has not started" note is then done with.
void ToolStartup::dismiss()
{
  bool note = false;
  for (size_t i = p_entries.size(); i-- > 0;)
    if (p_entries[i].timedOut) { p_entries.erase(p_entries.begin() + i); note = true; }
  hideWindow();
  if (note && starting() == 0) { p_status.clear(); p_setStatus(""); }
}


void ToolStartup::hideWindow()
{
  if (p_frame && p_frame->IsShown()) p_frame->Hide();
}


string ToolStartup::state() const
{
  return starting() > 0 ? "starting" : p_last;
}


string ToolStartup::report() const
{
  return "state=" + state() + " status=\"" + p_status + "\" busy=" +
         (p_busy ? "1" : "0") + " window=" +
         (p_frame && p_frame->IsShown() ? "shown" : "none") +
         " launches=" + std::to_string(p_launches);
}


bool ToolStartup::shot(const string& file)
{
  if (!p_frame || !p_frame->IsShown()) return false;
  return ecceWindowShot(p_frame, wxString::FromUTF8(file.c_str()));
}
