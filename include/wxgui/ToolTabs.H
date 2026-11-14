/**
 * @file
 *
 * Tools as tabs of the Organizer's window (#983 stage 2, with
 * ECCE_ONE_PROCESS=1 and ECCE_LAYOUT=small).
 *
 * A tool stays the wxFrame it always was: in Tab mode its frame is kept
 * hidden and its client windows are reparented into a page of the main
 * window's notebook.  Events the page does not handle go on to the hidden
 * frame instead of up to the notebook, so the tool's event tables, its
 * wxAuiManager (re-pointed at the page) and its listeners work unchanged.
 * Open in Window moves the windows back into the frame and shows it.
 */
#ifndef TOOLTABS_HH
#define TOOLTABS_HH

#include "wx/string.h"

class wxAuiManager;
class wxFrame;
class wxMenuBar;
class wxTopLevelWindow;

class ToolTabs
{
public:
  // Where the active tool's menu bar goes.  SwapMenuBar: the main window's
  // menu bar is the active tab's (macOS does this natively).  InTab (later):
  // each tab draws its own menu bar under the strip.
  enum MenuPlacement { SwapMenuBar, InTab };

  // ECCE_LAYOUT: "small" puts tools in tabs; anything else (default, or
  // "wide") keeps today's separate windows.  Only with ECCE_ONE_PROCESS.
  static bool enabled();
  static MenuPlacement menuPlacement();

  // The Organizer's frame becomes the main window: its client windows go
  // into the first tab (no close button), a tab strip above them.
  static void setMainWindow(wxFrame *organizer);

  // Moves a tool into a new tab.  mgr is the tool's wxAuiManager, if it
  // has one.  Before the frame is first shown is best: the 3-D viewer's
  // window is then first realized inside the page.
  static bool adopt(wxFrame *tool, wxAuiManager *mgr, const wxString& title);
  // The tool's menu bar: in a tab it is on the main window, so the frame's
  // own GetMenuBar() is null; a tool's GetMenuBar() returns this.
  static wxMenuBar *menuBar(const wxTopLevelWindow *tool, wxMenuBar *own);
  static bool hosts(const wxTopLevelWindow *tool);
  // The tool's Show/Raise/SetTitle while hosted: select / retitle its tab.
  static void select(wxTopLevelWindow *tool);
  static void setTitle(wxTopLevelWindow *tool, const wxString& title);
  // Open in Window: the tool's windows go back into its frame, which is
  // shown; showFrame false when the frame is about to be destroyed.
  static void release(wxTopLevelWindow *tool, bool showFrame = true);
  // Move to Tabs, from the frame's Window menu.
  static void moveToTabs(wxFrame *tool);

  // Tests: "tabs" lists "<n>:<title>[*]" (* = active) and the menu bar's
  // menus; "tab <n>", "towindow <n>", "totabs <title part>" act on them.
  static wxString describe();
  static bool testCommand(const wxString& cmd, const wxString& arg,
                          wxString& outcome);
};

#endif
