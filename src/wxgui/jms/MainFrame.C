/**
 * @file
 *
 * The one-process main window; see MainFrame.H and
 * docs/claude/services/tools-as-tabs.md.
 */
#include <cstdio>
#include <cstdlib>
#ifdef __GLIBC__
#include <execinfo.h>
#endif
#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif
#include "wx/app.h"
#include "wx/evtloop.h"
#include "wx/frame.h"
#include "wx/timer.h"

#include "util/Preferences.H"
#include "wxgui/MainFrame.H"
#include "wxgui/OneInstance.H"
#include "wxgui/InProcessTools.H"
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
void reportLoop(const char *when, bool trace = false)
{
  wxEventLoopBase *active = wxEventLoopBase::GetActive();
  wxEventLoopBase *main = wxTheApp ? wxTheApp->GetMainLoop() : 0;
  int gtkLevel = -1;
#ifdef __WXGTK__
  gtkLevel = (int)gtk_main_level();
#endif
  fprintf(stderr, "ECCE_TEST_ORGANIZER: %s: active loop %s, main running %d, "
          "exit on frame delete %d, pending %d, gtk level %d, windows:", when,
          !active ? "none" : active == main ? "main" : "NESTED",
          main ? (int)main->IsRunning() : -1,
          wxTheApp ? (int)wxTheApp->GetExitOnFrameDelete() : -1,
          (int)wxPendingDelete.GetCount(), gtkLevel);
  for (wxWindow *w : wxTopLevelWindows) {
    wxWindow *p = w->GetParent();
    fprintf(stderr, " %s/'%s'[parent %s%s]%s%s%s",
            (const char *)wxString(w->GetClassInfo()->GetClassName()).utf8_str(),
            (const char *)w->GetLabel().utf8_str(),
            p ? (const char *)wxString(p->GetClassInfo()->GetClassName()).utf8_str()
              : "none",
            p && p->IsBeingDeleted() ? " being deleted" : "",
            w->IsBeingDeleted() ? "(being deleted)" : "",
            static_cast<wxTopLevelWindow *>(w)->ShouldPreventAppExit()
                ? "(prevents exit)" : "",
            w->IsShown() ? "(shown)" : "");
  }
  fprintf(stderr, "\n");
#ifdef __GLIBC__
  if (!trace) return;
  // Which loop (wx, GTK or a dialog's) is running this code.
  void *frames[64];
  int n = backtrace(frames, 64);
  backtrace_symbols_fd(frames, n, 2);
#endif
}


// wx ends the main loop in ~wxTopLevelWindowBase when the last window goes,
// but after a tabbed Quit the loop was seen still running with no window
// left (#1065, only on a loaded host).  Ask again once the host is gone;
// windows that should keep the process alive still do.
void exitWhenNoWindowLeft()
{
  wxTimer *t = new wxTimer(wxTheApp);
  wxTheApp->Bind(wxEVT_TIMER, [t](wxTimerEvent&) {
    for (wxWindow *w : wxTopLevelWindows)
      if (!w->IsBeingDeleted() &&
          static_cast<wxTopLevelWindow *>(w)->ShouldPreventAppExit())
        return;
    wxEventLoopBase *main = wxTheApp->GetMainLoop();
    if (!main || !main->IsRunning()) return;  // a nested loop: next tick
    t->Stop();
    if (getenv("ECCE_TEST_ORGANIZER"))
      fprintf(stderr, "ECCE_TEST_ORGANIZER: no window left, ending the main loop\n");
    wxTheApp->ExitMainLoop();
  }, t->GetId());
  t->Start(500);
}
}


MainFrame::~MainFrame()
{
  if (s_instance == this) s_instance = 0;
  if (p_shutdown) exitWhenNoWindowLeft();
  if (getenv("ECCE_TEST_ORGANIZER")) {
    reportLoop("host destroyed");
    wxTimer *t = new wxTimer(wxTheApp);
    wxTheApp->Bind(wxEVT_TIMER, [](wxTimerEvent&) {
      reportLoop("3 s after the host", true);
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
  p_vetoable = ev.CanVeto();
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
  OneInstance::release();
  // A close that cannot be vetoed (session end, SIGTERM) must not leave
  // tool windows whose parent is going away.
  if (!p_vetoable) InProcessTools::closeAll(true);
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
  if (p_shutdown) return;
  s_raised++;
  if (IsIconized()) Iconize(false);
  Show(true);
  ewxRaiseWindow(this);
}


void MainFrame::openFiles(const std::vector<wxString>& paths)
{
  if (!p_shutdown && p_open) p_open(paths);
}
