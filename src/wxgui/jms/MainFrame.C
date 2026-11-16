/**
 * @file
 *
 * The one-process main window; see MainFrame.H and
 * docs/claude/services/tools-as-tabs.md.
 */
#include "wx/frame.h"

#include "util/Preferences.H"
#include "wxgui/MainFrame.H"
#include "wxgui/ToolTabs.H"
#include "wxgui/ewxWindowUtils.H"

IMPLEMENT_CLASS(MainFrame, ewxFrame)

namespace {
MainFrame *s_instance = 0;
}


MainFrame::MainFrame()
  : ewxFrame(NULL, wxID_ANY, _("ECCE"), wxDefaultPosition, wxSize(800, 650))
{
  s_instance = this;
  Bind(wxEVT_CLOSE_WINDOW, &MainFrame::onClose, this);
}


MainFrame::~MainFrame()
{
  if (s_instance == this) s_instance = 0;
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
  SetTitle(org->GetTitle());
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


void MainFrame::bringToFront()
{
  if (IsIconized()) Iconize(false);
  Show(true);
  ewxRaiseWindow(this);
}


void MainFrame::openFiles(const std::vector<wxString>& paths)
{
  if (p_open) p_open(paths);
}
