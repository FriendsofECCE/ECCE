/**
 * @file
 *
 * The one-process main window; see MainFrame.H and
 * docs/claude/services/tools-as-tabs.md.
 */
#include <cstdio>
#include <cstdlib>
#include "wx/app.h"
#include "wx/evtloop.h"
#include "wx/frame.h"
#include "wx/timer.h"

#include "util/Preferences.H"
#include "wxgui/MainFrame.H"
#include "wxgui/ToolTabs.H"
#include "wxgui/ewxWindowUtils.H"

IMPLEMENT_CLASS(MainFrame, ewxFrame)

namespace {
MainFrame *s_instance = 0;
int s_raised = 0;
}


MainFrame::MainFrame()
  : ewxFrame(NULL, wxID_ANY, _("ECCE"), wxDefaultPosition, wxSize(800, 650))
{
  s_instance = this;
  Bind(wxEVT_CLOSE_WINDOW, &MainFrame::onClose, this);
}


namespace {
// Test hook (#1065): what keeps the process running after the host goes.
void reportLoop(const char *when)
{
  wxEventLoopBase *active = wxEventLoopBase::GetActive();
  wxEventLoopBase *main = wxTheApp ? wxTheApp->GetMainLoop() : 0;
  fprintf(stderr, "ECCE_TEST_ORGANIZER: %s: active loop %s, main running %d, "
          "exit on frame delete %d, pending %d, windows:", when,
          !active ? "none" : active == main ? "main" : "NESTED",
          main ? (int)main->IsRunning() : -1,
          wxTheApp ? (int)wxTheApp->GetExitOnFrameDelete() : -1,
          (int)wxPendingDelete.GetCount());
  for (wxWindow *w : wxTopLevelWindows)
    fprintf(stderr, " %s/'%s'%s%s%s",
            (const char *)wxString(w->GetClassInfo()->GetClassName()).utf8_str(),
            (const char *)w->GetLabel().utf8_str(),
            w->GetParent() ? "(child)" : "",
            static_cast<wxTopLevelWindow *>(w)->ShouldPreventAppExit()
                ? "(prevents exit)" : "",
            w->IsShown() ? "(shown)" : "");
  fprintf(stderr, "\n");
}
}


MainFrame::~MainFrame()
{
  if (s_instance == this) s_instance = 0;
  if (getenv("ECCE_TEST_ORGANIZER")) {
    reportLoop("host destroyed");
    wxTimer *t = new wxTimer(wxTheApp);
    wxTheApp->Bind(wxEVT_TIMER, [](wxTimerEvent&) {
      reportLoop("3 s after the host");
    }, t->GetId());
    t->StartOnce(3000);
  }
}


MainFrame *MainFrame::instance()
{
  return s_instance;
}


bool MainFrame::adoptMain(wxFrame *org)
{
  if (!ToolTabs::adoptMain(org)) return false;
  p_main = org;
  // The Organizer's frame restored its rectangle (or chose one) in its
  // constructor; it is never shown, so it is the window's.
  SetSize(org->GetSize());
  Move(org->GetPosition());
  ToolTabs::setTitle(org, org->GetTitle());
  return true;
}


void MainFrame::onClose(wxCloseEvent& ev)
{
  if (!p_shutdown && ev.CanVeto() && p_close && !p_close()) {
    ev.Veto();
    return;
  }
  shutdown();
}


void MainFrame::shutdown()
{
  if (p_shutdown) return;
  p_shutdown = true;
  // While it is still on screen: a hidden window has no geometry.  Same
  // prefix the Organizer's own frame used, so the size carries over when
  // the tab setting is switched.
  if (!IsIconized() && !IsMaximized()) {
    Preferences prefs("Organizer");
    saveWindowSettings("Organizer", prefs, true);
  }
  if (wxFrame *org = p_main) {
    p_main = 0;
    org->Destroy();
  }
  Destroy();
}


int MainFrame::raiseCount()
{
  return s_raised;
}


void MainFrame::bringToFront()
{
  s_raised++;
  if (IsIconized()) Iconize(false);
  Show(true);
  ewxRaiseWindow(this);
}


void MainFrame::openFiles(const std::vector<wxString>& paths)
{
  if (p_open) p_open(paths);
}
