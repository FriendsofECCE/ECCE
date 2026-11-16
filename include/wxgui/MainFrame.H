/**
 * @file
 *
 * The one-process main window (#1065, part of #983): a plain frame that
 * carries the tab strip of ToolTabs.  The Organizer is its first (pinned)
 * tab; the frame itself knows no chemistry.  Only made with
 * ECCE_ONE_PROCESS=1 and some tool set to Tab; otherwise the Organizer's own
 * frame is the main window, as before.
 */
#ifndef MAINFRAME_HH
#define MAINFRAME_HH

#include <functional>
#include <vector>

#include "wx/string.h"
#include "wxgui/ewxFrame.H"

class MainFrame : public ewxFrame
{
  DECLARE_CLASS(MainFrame)
public:
  MainFrame();
  ~MainFrame();

  // The host, or null when the Organizer is its own main window.
  static MainFrame *instance();

  // ToolTabs::adoptMain, then the window takes the Organizer frame's
  // geometry (restored from its settings) and title.
  bool adoptMain(wxFrame *org);

  // Asked on a close of the window (title-bar X, window manager): true
  // lets it go on to shutdown(), false vetoes.
  void setCloseHandler(std::function<bool()> fn) { p_close = fn; }

  // The end of the session's window: saves the geometry, destroys the
  // Organizer's frame, then this one.  The Organizer must go first: its
  // destructor still uses the windows that were in the page.
  void shutdown();

  // For #1069: a second start raises the running window; files opened from
  // the system (macOS) go to the handler.
  void bringToFront();
  void setOpenFilesHandler(std::function<void(const std::vector<wxString>&)> fn)
  { p_open = fn; }
  void openFiles(const std::vector<wxString>& paths);

private:
  void onClose(wxCloseEvent& ev);

  wxFrame *p_main = 0;
  bool p_shutdown = false;
  std::function<bool()> p_close;
  std::function<void(const std::vector<wxString>&)> p_open;
};

#endif
