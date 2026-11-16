/**
 * @file
 *
 * Tools as tabs of the Organizer's window (#983 stage 2, with
 * ECCE_ONE_PROCESS=1; which tools: Preferences > Windows and tabs,
 * util/WindowLayout.H).
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
#include <functional>

class wxAuiManager;
class wxDialog;
class wxFrame;
class wxMenuBar;
class wxSizer;
class wxTopLevelWindow;
class wxWindow;

class ToolTabs
{
public:
  // Where the active tool's menu bar goes.  SwapMenuBar: the main window's
  // menu bar is the active tab's (macOS does this natively).  InTab (later):
  // each tab draws its own menu bar under the strip.
  enum MenuPlacement { SwapMenuBar, InTab };

  // Whether the main window gets a tab strip: one process, and some tool
  // set to Tab when the Organizer starts.
  static bool enabled();
  // Whether the tool with this WindowLayout key opens as a tab now.
  static bool wantsTab(const char *key);
  static MenuPlacement menuPlacement();

  // The main window (MainFrame) gets a tab strip and the menu routing.  It
  // has no menu bar of its own: the active tab's is on it.
  static void setHost(wxFrame *host);
  // The Organizer's frame (hidden from here on) becomes the main tab: its
  // client windows go into a page that has no close button and no scroller,
  // its menu bar is the host's while the tab is active.  Before its first Show.
  static bool adoptMain(wxFrame *org);

  // Moves a tool into a new tab.  mgr is the tool's wxAuiManager, if it
  // has one.  Before the frame is first shown is best: the 3-D viewer's
  // window is then first realized inside the page.
  static bool adopt(wxFrame *tool, wxAuiManager *mgr, const wxString& title);
  // A tool's opener: adopt if its Windows and tabs row says Tab.
  // Fills a Set-up tab's detail area (the panel right of the tool's windows).
  typedef std::function<void(wxWindow *area)> SideFn;
  // A test hook command (Organizer's ECCE_TEST_ORGANIZER): arg -> answer.
  typedef std::function<wxString(const wxString& arg)> CommandFn;

  // side: the page gets a detail area right of the tool's windows; side is
  // called with it each time the tool is (re)made a tab.
  static bool open(wxFrame *tool, wxAuiManager *mgr, const wxString& title,
                   const char *key, SideFn side = SideFn());
  static wxWindow *sidePanel(const wxTopLevelWindow *tool);
  // how: 1 fold the tool's column away, 0 unfold, -1 ask; returns folded.
  static bool fold(const wxTopLevelWindow *tool, int how);
  static wxTopLevelWindow *activeTool();
  // wxGTK: size-allocates windows made inside a shown page, recursively.
  static void allocate(wxWindow *w);
  static void addCommand(const wxString& name, CommandFn fn);
  // The tool's menu bar: in a tab it is on the main window, so the frame's
  // own GetMenuBar() is null; a tool's GetMenuBar() returns this.  Only
  // while hosted: wxFrameBase::SetMenuBar returns early when the bar equals
  // GetMenuBar(), so release could not put the bar back on the frame.
  static wxMenuBar *menuBar(const wxTopLevelWindow *tool, wxMenuBar *own);
  static bool hosts(const wxTopLevelWindow *tool);
  // The window that is on screen for a tool: the main window while the tool
  // is a tab (its own frame is hidden), else the tool's frame.  For sizes,
  // iconized checks, wxWindowDisabler and closing.
  static wxTopLevelWindow *screenWindow(const wxTopLevelWindow *tlw);
  // The tool's own sizer moves to its page: code that does GetSizer()->...
  // on the frame gets the page's through this (own: the frame's sizer).
  static wxSizer *sizer(const wxTopLevelWindow *tool, wxSizer *own);
  // Lays out the tool's page when it is hosted (the frame has no sizer then).
  static bool layout(wxTopLevelWindow *tool);
  // The window the tool's controls are children of while hosted (the
  // frame's FindWindow(id) no longer reaches them), else null.
  static wxWindow *client(const wxTopLevelWindow *tool);
  // The tool's Show/Raise/SetTitle while hosted: select / retitle its tab.
  static void select(wxTopLevelWindow *tool);
  static void setTitle(wxTopLevelWindow *tool, const wxString& title);
  // Open in Window: the tool's windows go back into its frame, which is
  // shown; showFrame false when the frame is about to be destroyed.
  static void release(wxTopLevelWindow *tool, bool showFrame = true);
  // Move to Tabs, from the frame's Window menu.
  static void moveToTabs(wxFrame *tool);

  // A modal dialog whose parent is a hosted tool's (hidden) frame: made
  // transient for the main window and centred over it, since the hidden
  // frame has no place on screen.  dialogClosed gives the focus back to
  // the control that had it.  Both are called by ewxModalGuard for every
  // modal dialog, so no call site needs to name another parent.
  static void dialogOpened(wxDialog *dlg);
  static void dialogClosed(wxDialog *dlg);

  // Popup menus: PopupMenu on a hosted tool's hidden frame has no place on
  // screen to attach to.  popupParent gives the tool's page for such a
  // window (else w itself): call popupParent(this)->PopupMenu(menu, pt),
  // with pt in that window's coordinates.  Menu commands still reach the
  // tool through the page.
  static wxWindow *popupParent(wxWindow *w);
  // A floating pane, tear-off or popup window made with a hosted tool's
  // hidden frame (or a child of it) as parent: made transient for the main
  // window, since the hidden frame is never on screen.
  static void secondaryOpened(wxWindow *win, wxWindow *parent);
  // Tests: "transient=host|other|none inside=0|1 mapped=0|1" for a window
  // (inside: its centre is within the main window).
  static wxString describeSecondary(wxWindow *win);

  // Tests: "tabs" lists "<n>:<title>[*]" (* = active) and the menu bar's
  // menus; "tab <n>", "towindow <n>", "totabs <title part>" act on them;
  // "layout show|<preset>|KEY=tab|window", "tabmenu <n>", "alwayswindow <n>".
  static wxString describe();
  static bool testCommand(const wxString& cmd, const wxString& arg,
                          wxString& outcome);
};

#endif
