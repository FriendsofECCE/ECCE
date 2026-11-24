/**
 * @file
 *
 * Tools as tabs of the Organizer's window (#983 stage 2).  See ToolTabs.H
 * and docs/claude/services/tools-as-tabs.md.
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>

#include "wx/aui/auibook.h"
#include "wx/eventfilter.h"
#include "wx/aui/framemanager.h"
#include "wx/aui/tabart.h"
#include "wx/app.h"
#include "wx/dialog.h"
#include "wx/msgdlg.h"
#include "wx/timer.h"
#include "wx/weakref.h"
#include "wx/control.h"
#include "wx/dcclient.h"
#include "wx/frame.h"
#include "wx/menu.h"
#include "wx/accel.h"
#include "wx/panel.h"
#include "wx/scrolwin.h"
#include "wx/button.h"
#include "wx/dcmemory.h"
#include "wx/dcscreen.h"
#include "wx/image.h"
#include <set>
#include "wx/tokenzr.h"
#include "util/LiveCount.H"
#include "wx/settings.h"
#include "wx/sizer.h"
#include "wx/statusbr.h"
#include "wx/toolbar.h"

#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif

#include "util/MqttLink.H"
#include "util/WindowLayout.H"
#include "dsm/EDSIServerCentral.H"
#include "wxgui/InProcessTools.H"
#include "wxgui/ToolTabs.H"
#include "wx/splitter.h"

static void allocateChildren(wxWindow *body);
static void fitScrollBody(wxWindow *body);
static void allocateNeverSized(wxWindow *parent);

namespace {

const int ID_OPEN_IN_WINDOW = wxID_HIGHEST + 7301;
const int ID_MOVE_TO_TABS = wxID_HIGHEST + 7302;
const int ID_ALWAYS_WINDOW = wxID_HIGHEST + 7303;
const int ID_NEXT_TAB = wxID_HIGHEST + 7304;
const int ID_PREV_TAB = wxID_HIGHEST + 7305;

struct Tab {
  wxFrame *tool;          // 0 for the Organizer's tab until it is adopted
  wxAuiManager *mgr;
  wxMenuBar *menubar;
  wxWindow *page;         // 0 while the tool is in its own window
  wxString title;
  wxColour dot;
  std::string key;        // its WindowLayout row
  ToolTabs::SideFn side;  // fills the page's detail area (Set-up tab)
  bool pinned = false;    // the main tab: no close cross, never released
};

// wx 3.3 measures tabs on a wxReadOnlyDC.
#if wxCHECK_VERSION(3, 3, 0)
typedef wxReadOnlyDC TabSizeDC;
#else
typedef wxDC TabSizeDC;
#endif

// Not s_host: winsock2.h defines s_host as a macro.
wxFrame *s_hostFrame = 0;
wxAuiNotebook *s_book = 0;
std::vector<Tab>& tabs()
{
  static auto *t = new std::vector<Tab>;
  return *t;
}

Tab *findTool(const wxTopLevelWindow *tool)
{
  for (Tab& t : tabs())
    if (t.tool && t.tool == tool) return &t;
  return 0;
}

Tab *findPage(const wxWindow *page)
{
  for (Tab& t : tabs())
    if (page && t.page == page) return &t;
  return 0;
}

// The main (Organizer) tab.  Found by its flag: the notebook lets a tab be
// dragged, so it is not necessarily the first.
Tab *mainTab()
{
  for (Tab& t : tabs())
    if (t.pinned) return &t;
  return 0;
}

Tab *activeTab()
{
  if (!s_book || s_book->GetSelection() == wxNOT_FOUND) return 0;
  return findPage(s_book->GetPage(s_book->GetSelection()));
}

const wxString& dash()
{
  static const wxString d = wxString::FromUTF8(" \xe2\x80\x93 ");
  return d;
}

// The calculation a tab's tool has open, as its status bar shows it
// ("(HOME)/project/calc"); empty for the Organizer and tools without one.
wxString tabPath(const Tab *t)
{
  if (!t || !t->tool || t->pinned) return "";
  std::string ctx = InProcessTools::context(t->tool);
  if (ctx.find("http") != 0 && ctx.find("file://") != 0) return "";
  // Mapping reads the server's mount points: once per URL.
  static auto *names = new std::map<std::string, wxString>;
  auto it = names->find(ctx);
  if (it == names->end())
    it = names->emplace(ctx, wxString::FromUTF8(
             EDSIServerCentral().mapURLtoName(ctx).c_str())).first;
  return it->second;
}

// A long path loses its middle, never its last part: the calculation's
// name is what tells two tabs apart.
wxString shortPath(const wxString& path, size_t max = 60)
{
  if (path.length() <= max) return path;
  size_t tail = std::max(path.AfterLast('/').length() + 1, max * 2 / 3);
  if (tail + 4 >= path.length()) return path;
  size_t head = max > tail + 1 ? max - tail - 1 : 0;
  return path.Left(head) + wxString::FromUTF8("\xe2\x80\xa6") + path.Right(tail);
}

// Tab labels are the tool's name only; the tooltip says which calculation.
void refreshTabTips()
{
  if (!s_book) return;
  for (size_t i = 0; i < s_book->GetPageCount(); i++) {
    wxString path = tabPath(findPage(s_book->GetPage(i)));
    wxString tip = path.empty() ? wxString() : path + dash() + s_book->GetPageText(i);
    if (s_book->GetPageToolTip(i) != tip) s_book->SetPageToolTip(i, tip);
  }
}

// One step through the tabs, wrapping around.  A key may arrive both as the
// menu accelerator and through the key hook: a second step in the same
// event-loop pass is dropped.
bool s_cycling = false;
bool cycleTab(int step)
{
  if (!s_book || !s_hostFrame || s_book->GetPageCount() < 2) return false;
  if (s_cycling) return true;
  s_cycling = true;
  s_hostFrame->CallAfter([]() { s_cycling = false; });
  int n = (int)s_book->GetPageCount();
  int sel = s_book->GetSelection();
  int idx = ((sel < 0 ? 0 : sel) + step + n) % n;
  Tab *t = findPage(s_book->GetPage(idx));
  if (t && t->tool) ToolTabs::select(t->tool);
  else s_book->SetSelection(idx);
  return true;
}

// Ctrl+Tab / Ctrl+Shift+Tab, Ctrl+PgDn / Ctrl+PgUp; on macOS the Control
// key for Tab (Cmd+Tab is the system's application switcher) and the
// platform's Cmd+Shift+] / Cmd+Shift+[.
int tabKeyStep(const wxKeyEvent& ev)
{
  int k = ev.GetKeyCode();
  if (ev.AltDown()) return 0;
  if (k == WXK_TAB && ev.RawControlDown()) return ev.ShiftDown() ? -1 : 1;
  bool ctrl = ev.RawControlDown() || ev.ControlDown();
  if (ctrl && !ev.ShiftDown() && (k == WXK_PAGEDOWN || k == WXK_NUMPAD_PAGEDOWN)) return 1;
  if (ctrl && !ev.ShiftDown() && (k == WXK_PAGEUP || k == WXK_NUMPAD_PAGEUP)) return -1;
#ifdef __WXOSX__
  if (ev.CmdDown() && ev.ShiftDown() && (k == ']' || k == '}')) return 1;
  if (ev.CmdDown() && ev.ShiftDown() && (k == '[' || k == '{')) return -1;
#endif
  return 0;
}

wxColour dotFor(const wxString& title)
{
  if (title.Contains("Builder")) return wxColour(0xc0, 0x20, 0x20);
  if (title.Contains("Editor")) return wxColour(0x40, 0x90, 0x20);
  if (title.Contains("Organizer")) return wxColour(0x30, 0x60, 0xb0);
  return wxColour(0x70, 0x50, 0x90);
}


// A tool's windows in a tab: what they do not handle goes on to the tool's
// hidden frame, as it would in the tool's own window (event tables,
// EVT_CHILD_FOCUS, toolbar commands and their UI updates), never up to the
// notebook and the Organizer.
class ToolPage : public wxPanel
{
public:
  ToolPage(wxWindow *parent, wxFrame *tool, bool scrolls, bool side = false)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxTAB_TRAVERSAL | wxNO_BORDER), p_tool(tool)
  {
    // A Set-up tab: the tool's windows on the left, a detail area the tool
    // fills on the right; the left column folds away (Unsplit).
    wxWindow *bodyParent = this;
    if (side) {
      p_split = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition,
                                     wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3DSASH);
      p_split->SetMinimumPaneSize(200);
      p_split->SetSashGravity(0.0);
      bodyParent = p_split;
    }
    // The body is the tool's managed window; the status bar sits below it
    // (AUI would otherwise lay its panes over the bar).  A tool without an
    // AUI manager gets a scroller: a frame is fitted to its display
    // (ewxFrame::fitToScreen), a page is not, and its sizer minimum can be
    // taller than the page.
    if (scrolls) {
      wxScrolledWindow *sw = new wxScrolledWindow(
          bodyParent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
          wxTAB_TRAVERSAL | wxNO_BORDER);
      sw->SetScrollRate(10, 10);
      // Children arrive and are shown (wxGTK: at idle) after the body has
      // its size; see allocateChildren.
      sw->Bind(wxEVT_IDLE, [sw](wxIdleEvent& ev) {
        allocateChildren(sw);
        ev.Skip();
      });
      sw->Bind(wxEVT_SIZE, [sw](wxSizeEvent& ev) {
        fitScrollBody(sw);
        ev.Skip();
      });
      p_body = sw;
    } else {
      p_body = new wxPanel(bodyParent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxTAB_TRAVERSAL | wxNO_BORDER);
    }
    wxBoxSizer *sz = new wxBoxSizer(wxVERTICAL);
    if (p_split) {
      p_side = new wxPanel(p_split, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxTAB_TRAVERSAL | wxNO_BORDER);
      p_side->SetSizer(new wxBoxSizer(wxVERTICAL));
      p_split->SplitVertically(p_body, p_side, 520);
      sz->Add(p_split, 1, wxEXPAND);
    } else {
      sz->Add(p_body, 1, wxEXPAND);
    }
    SetSizer(sz);
  }
  wxWindow *body() const { return p_body; }
  wxWindow *side() const { return p_side; }
  bool folded() const { return p_split && !p_split->IsSplit(); }
  // The editor column as wide as its content (plus a scroll bar).
  void fitSash()
  {
    if (!p_split || !p_body->GetSizer()) return;
    p_sash = p_body->GetSizer()->CalcMin().x +
             wxSystemSettings::GetMetric(wxSYS_VSCROLL_X) + 4;
    if (p_split->IsSplit()) p_split->SetSashPosition(p_sash);
  }
  void fold(bool on)
  {
    if (!p_split || on == folded()) return;
    if (on) {
      p_sash = p_split->GetSashPosition();
      p_split->Unsplit(p_body);
    } else {
      p_split->SplitVertically(p_body, p_side, p_sash);
    }
  }
  // The tool's status bar into the page (and back on release).
  void takeStatusBar(wxFrame *tool)
  {
    wxStatusBar *sb = tool->GetStatusBar();
    if (!sb) return;
    sb->Reparent(this);
    GetSizer()->Add(sb, 0, wxEXPAND);
    sb->Show();
  }
  void giveStatusBar(wxFrame *tool)
  {
    wxStatusBar *sb = tool->GetStatusBar();
    if (!sb) return;
    GetSizer()->Detach(sb);
    sb->Reparent(tool);
  }
protected:
  bool TryAfter(wxEvent& ev) override
  {
    if (p_tool && ev.ShouldPropagate() && !p_tool->IsBeingDeleted()) {
      wxPropagateOnce once(ev, this);
      return p_tool->GetEventHandler()->ProcessEvent(ev);
    }
    return wxPanel::TryAfter(ev);
  }
private:
  wxFrame *p_tool;
  wxPanel *p_body;
  wxSplitterWindow *p_split = 0;
  wxPanel *p_side = 0;
  int p_sash = 520;
};


// Moves a window's client windows (not its menu bar, status bar, frame
// toolbar, floating panes or dialogs) and its sizer to another window; an
// AUI manager follows them.
void moveClient(wxWindow *from, wxWindow *to, wxAuiManager *mgr,
                wxWindow *keep = 0)
{
  wxFrame *frame = wxDynamicCast(from, wxFrame);
  if (mgr) mgr->UnInit();
  wxWindowList kids = from->GetChildren();
  for (wxWindow *w : kids) {
    if (w == keep || w->IsTopLevel() || wxDynamicCast(w, wxMenuBar)) continue;
    if (frame && ((wxWindow*)frame->GetStatusBar() == w ||
                  (wxWindow*)frame->GetToolBar() == w)) continue;
    w->Reparent(to);
  }
  if (wxSizer *s = from->GetSizer()) {
    from->SetSizer(0, false);
    to->SetSizer(s, true);
    s->SetContainingWindow(to);
  }
  if (mgr) {
    mgr->SetManagedWindow(to);
    mgr->Update();
  }
  to->Layout();
}


// The Window menu's Move to Tabs, present while the tool is in a window.
void setMoveToTabsItem(wxFrame *tool, wxMenuBar *mb, bool present)
{
  if (!mb) return;
  if (wxMenuItem *item = mb->FindItem(ID_MOVE_TO_TABS)) {
    if (!present) item->GetMenu()->Destroy(item);
    return;
  }
  if (!present) return;
  int idx = mb->FindMenu(_("Window"));
  wxMenu *menu = idx == wxNOT_FOUND ? new wxMenu : mb->GetMenu(idx);
  menu->Append(ID_MOVE_TO_TABS, _("Move to Tabs"),
               _("Show this tool as a tab of the Organizer's window"));
  if (idx == wxNOT_FOUND) mb->Insert(mb->GetMenuCount() - 1, menu, _("&Window"));
  tool->Bind(wxEVT_MENU, [tool](wxCommandEvent&) {
    ToolTabs::moveToTabs(tool);
  }, ID_MOVE_TO_TABS);
}


// The Window menu: every bar on the host lists the open tabs (#1069).  A
// bar that has a "Window" menu already gets the entries appended; else one
// is inserted before Help.  Looked up by title, never by position, since
// the tools find their menus by name.
const int WINDOW_TABS_MAX = 32;
int s_windowBase = 0;
struct WindowMenu {
  wxMenu *menu;
  bool created;        // made by us, removed whole on detach
  wxMenuItem *sep;     // below Next/Previous Tab, in a tool's own menu
};
std::map<wxMenuBar*, WindowMenu>& windowMenus()
{
  static auto *m = new std::map<wxMenuBar*, WindowMenu>;
  return *m;
}

// ECCE_DEBUG_MENUS=1: each bar operation on stderr, for platforms where a
// menu fault ends the program with no trace of the step (#1119).
void menuLog(const char *what, wxMenuBar *mb)
{
  static const bool on = getenv("ECCE_DEBUG_MENUS") != 0;
  if (on) fprintf(stderr, "ECCE menus: %s %p\n", what, (void*)mb);
}

void fillWindowMenu(wxMenuBar *mb)
{
  auto it = windowMenus().find(mb);
  if (it == windowMenus().end() || !s_book) return;
  wxMenu *menu = it->second.menu;
  menuLog("fill", mb);
  for (int i = 0; i < WINDOW_TABS_MAX; i++)
    if (menu->FindItem(s_windowBase + i)) menu->Delete(s_windowBase + i);
  for (size_t i = 0; i < s_book->GetPageCount() && i < (size_t)WINDOW_TABS_MAX; i++) {
    wxString label = s_book->GetPageText(i);
    wxString path = tabPath(findPage(s_book->GetPage(i)));
    if (!path.empty()) label += dash() + shortPath(path);
    label.Replace("&", "&&");
    menu->AppendCheckItem(s_windowBase + (int)i, label);
    menu->Check(s_windowBase + (int)i, (int)i == s_book->GetSelection());
  }
}

void attachWindowMenu(wxMenuBar *mb)
{
  menuLog("attach", mb);
  if (!mb || windowMenus().count(mb)) return;
  if (!s_windowBase) s_windowBase = wxWindow::NewControlId(WINDOW_TABS_MAX);
  int idx = mb->FindMenu(_("Window"));
  bool created = idx == wxNOT_FOUND;
  wxMenu *menu = created ? new wxMenu : mb->GetMenu(idx);
  if (created) {
    int help = mb->FindMenu(_("Help"));
    if (help == wxNOT_FOUND) mb->Append(menu, _("&Window"));
    else mb->Insert(help, menu, _("&Window"));
  }
#ifdef __WXOSX__
  // Cocoa's own Window menu is off (setHost), and with it its Minimize.
  if (!menu->FindItem(wxID_ICONIZE_FRAME)) {
    menu->Insert(0, wxID_ICONIZE_FRAME, _("Minimize\tCtrl+M"));
    menu->InsertSeparator(1);
  }
#endif
#ifdef __WXOSX__
  menu->Append(ID_NEXT_TAB, _("Next Tab\tCtrl+Shift+]"));
  menu->Append(ID_PREV_TAB, _("Previous Tab\tCtrl+Shift+["));
#elif defined(__WXGTK__)
  // GTK refuses Tab as a menu accelerator and wx then drops it: show the
  // page keys; Ctrl+Tab still works through the key hook.
  menu->Append(ID_NEXT_TAB, _("Next Tab\tCtrl+PgDn"));
  menu->Append(ID_PREV_TAB, _("Previous Tab\tCtrl+PgUp"));
#else
  menu->Append(ID_NEXT_TAB, _("Next Tab\tCtrl+Tab"));
  menu->Append(ID_PREV_TAB, _("Previous Tab\tCtrl+Shift+Tab"));
#endif
  wxMenuItem *sep = menu->AppendSeparator();
  windowMenus()[mb] = WindowMenu{menu, created, sep};
  fillWindowMenu(mb);
}

void detachWindowMenu(wxMenuBar *mb)
{
  menuLog("detach", mb);
  auto it = windowMenus().find(mb);
  if (it == windowMenus().end()) return;
  wxMenu *menu = it->second.menu;
  if (it->second.created) {
    int idx = mb->FindMenu(_("Window"));
    if (idx != wxNOT_FOUND && mb->GetMenu(idx) == menu) delete mb->Remove(idx);
  } else {
    for (int i = 0; i < WINDOW_TABS_MAX; i++)
      if (menu->FindItem(s_windowBase + i)) menu->Delete(s_windowBase + i);
    if (menu->FindItem(ID_NEXT_TAB)) menu->Delete(ID_NEXT_TAB);
    if (menu->FindItem(ID_PREV_TAB)) menu->Delete(ID_PREV_TAB);
    if (it->second.sep) menu->Delete(it->second.sep);
    if (menu->FindItem(wxID_ICONIZE_FRAME)) {
      menu->Delete(wxID_ICONIZE_FRAME);
      if (menu->GetMenuItemCount() && menu->FindItemByPosition(0)->IsSeparator())
        menu->Delete(menu->FindItemByPosition(0));
    }
  }
  windowMenus().erase(it);
}

wxString s_hostBaseTitle;

// "ECCE Organizer <ver> - <tab>": the window names the tab on show.  The
// Organizer's own title stays first, since tests and scripts find the
// window by it.
void updateHostTitle()
{
  if (!s_hostFrame || !s_book || s_book->GetSelection() < 0) return;
  Tab *t = findPage(s_book->GetPage(s_book->GetSelection()));
  wxString title = s_hostBaseTitle;
  if (t && !t->pinned && !title.empty())
    title += wxString::FromUTF8(" \xe2\x80\x93 ") + s_book->GetPageText(s_book->GetSelection());
  if (!title.empty() && s_hostFrame->GetTitle() != title) s_hostFrame->SetTitle(title);
}

// Puts a bar on the host (or none), moving the Window menu with it.
void putBar(wxMenuBar *mb)
{
  wxMenuBar *old = s_hostFrame->wxFrame::GetMenuBar();
  if (old != mb) {
    detachWindowMenu(old);
    menuLog("set", mb);
    s_hostFrame->SetMenuBar(mb);
    menuLog("set done", mb);
  }
  attachWindowMenu(mb);
  fillWindowMenu(mb);
}

// Placement of the active tab's menus.  SwapMenuBar: the main window's
// menu bar becomes the tab's.
void placeMenus(Tab *t)
{
  if (!t || !s_hostFrame) return;
  // Quit is the application's here (menuTarget); the tool's own quit is
  // the tab's close, so the item must not read as the tool's.
  if (!t->pinned && t->menubar)
    if (wxMenuItem *q = t->menubar->FindItem(wxID_EXIT)) {
      wxString label = q->GetItemLabel();
      const wxString accel = label.Contains("\t") ? "\t" + label.AfterFirst('\t') : "";
      if (label.BeforeFirst('\t') != _("&Quit ECCE"))
        q->SetItemLabel(_("&Quit ECCE") + accel);
    }
  if (ToolTabs::menuPlacement() == ToolTabs::SwapMenuBar) {
    Tab *org = mainTab();
    wxMenuBar *mb = t->menubar ? t->menubar : (org ? org->menubar : 0);
    putBar(mb);
  }
  updateHostTitle();
}


// The tool behind a tab acts as if its window had been activated: data
// notices go out under its name (InProcessTools::track), its own activate
// handlers run.
void activateTool(Tab *t)
{
  if (!t || !t->tool) return;
  wxActivateEvent ev(wxEVT_ACTIVATE, true, t->tool->GetId());
  ev.SetEventObject(t->tool);
  t->tool->GetEventHandler()->ProcessEvent(ev);
}


// Menu commands from the host's menu bar go to the frame of the tab whose
// menus are on it (the Organizer's included), as from its own menu bar.
wxFrame *menuTarget(int id)
{
  // Quit, Preferences and About belong to the application: the Organizer
  // answers them whatever tab is active (#1069).
  if (id == wxID_EXIT || id == wxID_PREFERENCES || id == wxID_ABOUT) {
    Tab *m = mainTab();
    return m ? m->tool : 0;
  }
  Tab *t = activeTab();
  if (!t || !t->tool) return 0;
  // The host has no handlers of its own: the Organizer also gets commands
  // that are on no menu (a button posting a menu id).
  if (t->pinned) return t->tool;
  if (!t->menubar) return 0;
  return t->menubar->FindItem(id) ? t->tool : 0;
}

void routeMenus(wxFrame *host)
{
  host->Bind(wxEVT_MENU, [](wxCommandEvent& ev) {
    int n = ev.GetId() - s_windowBase;
    if (s_windowBase && n >= 0 && n < WINDOW_TABS_MAX && s_book) {
      if (n < (int)s_book->GetPageCount()) s_book->SetSelection(n);
      return;
    }
    if (ev.GetId() == ID_NEXT_TAB || ev.GetId() == ID_PREV_TAB) {
      cycleTab(ev.GetId() == ID_NEXT_TAB ? 1 : -1);
      return;
    }
    if (ev.GetId() == wxID_ICONIZE_FRAME && s_hostFrame) {
      s_hostFrame->Iconize(true);
      return;
    }
    if (wxFrame *f = menuTarget(ev.GetId())) f->GetEventHandler()->ProcessEvent(ev);
    else ev.Skip();
  });
  // GTK sends the key hook to the top-level window, not up from the focused
  // window, so the tool's frame never sees it from a tab.
  host->Bind(wxEVT_CHAR_HOOK, [](wxKeyEvent& ev) {
    if (int step = tabKeyStep(ev))
      if (cycleTab(step)) return;
    Tab *t = activeTab();
    if (t && t->tool && t->tool->GetEventHandler()->ProcessEvent(ev)) return;
    ev.Skip();
  });
  host->Bind(wxEVT_UPDATE_UI, [](wxUpdateUIEvent& ev) {
    if (wxFrame *f = menuTarget(ev.GetId())) f->GetEventHandler()->ProcessEvent(ev);
    else ev.Skip();
  });
  auto menu = [](wxMenuEvent& ev) {
    // Rebuild only as the menu opens: rebuilding on highlight/close deletes
    // the check item GTK is about to select under the pointer (#1239).
    if (s_hostFrame && ev.GetEventType() == wxEVT_MENU_OPEN) {
      wxMenuBar *mb = s_hostFrame->wxFrame::GetMenuBar();
      auto it = windowMenus().find(mb);
      if (it != windowMenus().end() && ev.GetMenu() == it->second.menu)
        fillWindowMenu(mb);
    }
    Tab *t = activeTab();
    if (t && t->tool) t->tool->GetEventHandler()->ProcessEvent(ev);
    else ev.Skip();
  };
  host->Bind(wxEVT_MENU_OPEN, menu);
  host->Bind(wxEVT_MENU_CLOSE, menu);
  host->Bind(wxEVT_MENU_HIGHLIGHT, menu);
}


// The agreed Chrome-like strip: rounded active tab in the window colour,
// flat inactive tabs on a darker strip with thin separators, a coloured dot
// per tool, a close cross on every tab but the Organizer's.
class ChromeTabArt : public wxAuiGenericTabArt
{
public:
  wxAuiTabArt *Clone() override { return new ChromeTabArt(*this); }

  void DrawBackground(wxDC& dc, wxWindow*, const wxRect& rect) override
  {
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(strip()));
    dc.DrawRectangle(rect);
    dc.SetPen(wxPen(active()));
    dc.DrawLine(rect.x, rect.GetBottom(), rect.GetRight() + 1, rect.GetBottom());
  }

  wxSize GetTabSize(TabSizeDC& dc, wxWindow*, const wxString& caption,
                    const wxBitmapBundle&, bool, int closeState,
                    int *xExtent) override
  {
    dc.SetFont(m_normalFont);
    wxCoord tw, th;
    dc.GetTextExtent(caption, &tw, &th);
    int w = 14 + 10 + 8 + tw + 14;
    if (closeState != wxAUI_BUTTON_STATE_HIDDEN) w += 20;
    w = wxMax(110, wxMin(w, 260));
    *xExtent = w;
    return wxSize(w, wxMax(th + 16, 32));
  }

  void DrawTab(wxDC& dc, wxWindow *wnd, const wxAuiNotebookPage& page,
               const wxRect& in, int closeState, wxRect *outTab,
               wxRect *outButton, int *xExtent) override
  {
    Tab *t = findPage(page.window);
    bool closable = t && t->tool && !t->pinned;
    if (!closable) closeState = wxAUI_BUTTON_STATE_HIDDEN;
    wxSize sz = GetTabSize(dc, wnd, page.caption, page.bitmap, page.active,
                           closeState, xExtent);
    wxRect r(in.x, in.y + 4, sz.x, in.height - 4);

    if (page.active || page.hover) {
      dc.SetPen(*wxTRANSPARENT_PEN);
      dc.SetBrush(wxBrush(page.active ? active() : hover()));
      // Bottom corners below the strip: only the top is rounded.
      dc.DrawRoundedRectangle(r.x, r.y, r.width, r.height + 8, 7);
    } else {
      dc.SetPen(wxPen(strip().ChangeLightness(80)));
      dc.DrawLine(r.GetRight(), r.y + 7, r.GetRight(), r.GetBottom() - 6);
    }

    int cy = r.y + r.height / 2;
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(t && t->dot.IsOk() ? t->dot : dotFor(page.caption)));
    dc.DrawCircle(r.x + 14 + 5, cy, 5);

    int tx = r.x + 14 + 10 + 8;
    int right = r.GetRight() - 12 - (closeState != wxAUI_BUTTON_STATE_HIDDEN ? 20 : 0);
    dc.SetFont(m_normalFont);
    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    wxString text = wxControl::Ellipsize(page.caption, dc, wxELLIPSIZE_END,
                                         wxMax(10, right - tx));
    wxCoord tw, th;
    dc.GetTextExtent(text, &tw, &th);
    dc.DrawText(text, tx, cy - th / 2);

    *outButton = wxRect();
    if (closeState != wxAUI_BUTTON_STATE_HIDDEN) {
      wxRect b(r.GetRight() - 12 - 16, cy - 8, 16, 16);
      if (closeState == wxAUI_BUTTON_STATE_HOVER ||
          closeState == wxAUI_BUTTON_STATE_PRESSED) {
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(strip().ChangeLightness(85)));
        dc.DrawCircle(b.x + 8, b.y + 8, 8);
      }
      dc.SetPen(wxPen(wxColour(0x50, 0x50, 0x50), 1));
      dc.DrawLine(b.x + 5, b.y + 5, b.x + 11, b.y + 11);
      dc.DrawLine(b.x + 11, b.y + 5, b.x + 5, b.y + 11);
      // The clickable area is larger than the drawn cross: a 16 px target
      // was easy to miss with a trackpad.
      *outButton = b.Inflate(4, 4);
    }
    *outTab = r;
  }

private:
  static wxColour active()
  {
    return wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
  }
  static wxColour strip() { return active().ChangeLightness(86); }
  static wxColour hover() { return active().ChangeLightness(93); }
};


// Tab captions drop the window title's "ECCE " prefix (the window is
// already ECCE).
wxString tabCaption(const wxString& title)
{
  return title.StartsWith("ECCE ") ? title.Mid(5) : title;
}


// wxAuiTabCtrl only presses a tab's close cross when its last motion event
// left the pointer over it (m_hoverButton). A pointer arriving from the 3-D
// view or another window often sends none, so the first click on the cross
// only selected the tab and a second click closed it (#1234). A motion at
// the press point first makes the hover state current.
void hookTabCtrls()
{
  static std::set<wxWindow*> hooked;
  if (!s_book) return;
  for (wxWindow *child : s_book->GetChildren()) {
    wxAuiTabCtrl *tc = wxDynamicCast(child, wxAuiTabCtrl);
    if (!tc || !hooked.insert(tc).second) continue;
    tc->Bind(wxEVT_LEFT_DOWN, [tc](wxMouseEvent& ev) {
      wxMouseEvent move(wxEVT_MOTION);
      move.SetPosition(ev.GetPosition());
      move.SetEventObject(tc);
      tc->GetEventHandler()->ProcessEvent(move);
      ev.Skip();
    });
    tc->Bind(wxEVT_DESTROY, [tc](wxWindowDestroyEvent& ev) {
      hooked.erase(tc);
      ev.Skip();
    });
  }
}

void onPageChanged(wxAuiNotebookEvent& ev)
{
  hookTabCtrls();  // a split made a new tab strip
  Tab *t = activeTab();
  placeMenus(t);
  activateTool(t);
  refreshTabTips();
  ev.Skip();
}


bool s_closingTool = false;

void onPageClose(wxAuiNotebookEvent& ev)
{
  // The notebook would delete the page under a live tool: the tool closes
  // itself (its save question may say Cancel), and its Destroy takes the
  // tab away.
  ev.Veto();
  Tab *t = s_book ? findPage(s_book->GetPage(ev.GetSelection())) : 0;
  if (t && t->tool && !t->pinned) {
    wxFrame *tool = t->tool;
    // Closing a tool must never become the Organizer's Quit: the flag
    // stays up until the events the close queued have run.
    s_closingTool = true;
    s_hostFrame->CallAfter([tool]() {
      tool->Close();
      s_hostFrame->CallAfter([]() { s_closingTool = false; });
    });
  }
}


void fillTabMenu(wxMenu& menu)
{
  menu.Append(ID_OPEN_IN_WINDOW, _("Open in Window"),
              _("Move this tool into a window of its own"));
  menu.Append(ID_ALWAYS_WINDOW, _("Always open in a window"),
              _("Open this tool in a window from now on (Preferences > "
                "Windows and tabs becomes Custom)"));
}

// The tab menu's items: Open in Window now, or also from now on.
void tabMenuChoice(Tab *t, int id)
{
  if (!t || !t->tool || t->pinned) return;
  wxFrame *tool = t->tool;
  if (id == ID_ALWAYS_WINDOW && !t->key.empty())
    WindowLayout::setRow(t->key, false);
  if (id == ID_OPEN_IN_WINDOW || id == ID_ALWAYS_WINDOW)
    ToolTabs::release(tool);
}

void onTabRightUp(wxAuiNotebookEvent& ev)
{
  Tab *t = s_book ? findPage(s_book->GetPage(ev.GetSelection())) : 0;
  if (!t || !t->tool || t->pinned) return;
  wxMenu menu;
  fillTabMenu(menu);
  wxFrame *tool = t->tool;
  int id = s_book->GetPopupMenuSelectionFromUser(menu);
  // The tab may have gone while the menu was up.
  tabMenuChoice(findTool(tool), id);
}

// Whether the shown buttons whose label is in the comma list (plus any shown
// control at all, as "clipped") can be seen in the page's body or reached by
// scrolling it: its rectangle must end inside the scroller's virtual area,
// and either be in view or the area be scrollable.
wxString reachInfo(wxWindow *body, const wxString& labels)
{
  wxScrolledWindow *sw = wxDynamicCast(body, wxScrolledWindow);
  wxSize view = body->GetClientSize();
  wxSize virt = sw ? sw->GetVirtualSize() : view;
  bool canScroll = sw && (virt.y > view.y || virt.x > view.x);
  int clipped = 0;
  wxString out = wxString::Format("view=%dx%d virtual=%dx%d", view.x, view.y,
                                  virt.x, virt.y);
  // The scroller must be as tall as the content's minimum, or what lies
  // below its virtual area can never be scrolled to.
  if (sw && sw->GetSizer() && sw->GetSizer()->CalcMin().y > virt.y)
    out += " STALE";
  std::set<wxString> want;
  wxStringTokenizer tk(labels, ",");
  while (tk.HasMoreTokens()) want.insert(tk.GetNextToken());
  std::vector<wxWindow*> todo(body->GetChildren().begin(),
                              body->GetChildren().end());
  while (!todo.empty()) {
    wxWindow *w = todo.back();
    todo.pop_back();
    if (!w->IsShown()) continue;
    for (wxWindow *c : w->GetChildren()) todo.push_back(c);
    if (w->GetSize().x < 2 || w->GetSize().y < 2) continue;
    // Position in the body's unscrolled coordinates.
    wxPoint p = body->ScreenToClient(w->GetScreenPosition());
    if (sw) p = sw->CalcUnscrolledPosition(p);
    wxRect r(p, w->GetSize());
    bool inView = r.GetBottom() < (sw ? sw->GetViewStart().y * 10 + view.y : view.y) &&
                  r.y >= (sw ? sw->GetViewStart().y * 10 : 0);
    bool inArea = r.GetBottom() < virt.y + 1;
    bool reachable = inView || (canScroll && inArea);
    wxButton *b = wxDynamicCast(w, wxButton);
    if (b && want.count(wxStripMenuCodes(b->GetLabel())))
      out += wxString::Format(" %s=%s", wxStripMenuCodes(b->GetLabel()),
                              reachable ? "reachable" : "CLIPPED");
    else if (!reachable) clipped++;
  }
  if (want.count("dump")) {
    out += wxString::Format(" min=%dx%d |", body->GetSizer() ? body->GetSizer()->CalcMin().x : -1,
                            body->GetSizer() ? body->GetSizer()->CalcMin().y : -1);
    for (wxWindow *c : body->GetChildren())
      out += wxString::Format(" %s@%d,%d+%dx%d%s", c->GetClassInfo()->GetClassName(),
                              c->GetPosition().x, c->GetPosition().y, c->GetSize().x,
                              c->GetSize().y, c->IsShown() ? "" : "(h)");
  }
  return out + wxString::Format(" clipped=%d", clipped);
}


#ifdef __WXGTK__
// GTK state of a window: frozen (wx), widget/client mapped, visible,
// allocation; for diagnosing a page that is laid out but not drawn.
wxString gtkState(wxWindow *w)
{
  GtkWidget *wd = w->m_widget, *cl = w->m_wxwindow;
  GtkAllocation a = {0, 0, 0, 0};
  if (wd) gtk_widget_get_allocation(wd, &a);
  GtkAllocation c = {0, 0, 0, 0};
  if (cl) gtk_widget_get_allocation(cl, &c);
  GdkWindow *gw = cl ? gtk_widget_get_window(cl) : 0;
  return wxString::Format("%s[frz=%d map=%d/%d vis=%d/%d alloc=%d,%d+%dx%d cl=%d,%d+%dx%d gdkview=%d wx=%dx%d]",
      w->GetClassInfo()->GetClassName(), (int)w->IsFrozen(),
      wd ? (int)gtk_widget_get_mapped(wd) : -1, cl ? (int)gtk_widget_get_mapped(cl) : -1,
      wd ? (int)gtk_widget_get_visible(wd) : -1, cl ? (int)gtk_widget_get_visible(cl) : -1,
      a.x, a.y, a.width, a.height, c.x, c.y, c.width, c.height,
      gw ? (int)gdk_window_is_viewable(gw) : -1, w->GetSize().x, w->GetSize().y);
}

wxString gtkInfo(wxWindow *body)
{
  wxString out = " gtk:";
  for (wxWindow *w = body; w; w = w->GetParent()) {
    out += " " + gtkState(w);
    if (w->IsTopLevel()) break;
  }
  int frozen = 0, unmapped = 0, n = 0;
  wxString firstBad;
  std::vector<wxWindow*> todo(body->GetChildren().begin(), body->GetChildren().end());
  while (!todo.empty()) {
    wxWindow *w = todo.back();
    todo.pop_back();
    if (!w->IsShown()) continue;
    n++;
    for (wxWindow *c : w->GetChildren()) todo.push_back(c);
    if (w->IsFrozen()) frozen++;
    if (w->m_widget && !gtk_widget_get_mapped(w->m_widget)) {
      unmapped++;
      if (firstBad.empty()) firstBad = gtkState(w);
    }
  }
  out += wxString::Format(" | desc=%d frozen=%d unmapped=%d %s", n, frozen,
                          unmapped, firstBad);
  if (!body->GetChildren().empty())
    out += " first=" + gtkState(body->GetChildren().front());
  return out;
}
#endif


// Distinct colours in the page as the X server shows it: a page that is
// one flat colour is blank.
wxString paintInfo(wxWindow *page)
{
  wxSize sz = page->GetSize();
  wxPoint org = page->ClientToScreen(wxPoint(0, 0));
  wxBitmap bmp(sz.x, sz.y);
  {
    wxMemoryDC mem(bmp);
    wxScreenDC screen;
    mem.Blit(0, 0, sz.x, sz.y, &screen, org.x, org.y);
  }
  wxImage img = bmp.ConvertToImage();
  std::set<unsigned long> colours;
  const unsigned char *d = img.GetData();
  for (int i = 0; d && i < sz.x * sz.y; i++)
    colours.insert((d[3*i] << 16) | (d[3*i+1] << 8) | d[3*i+2]);
  return wxString::Format("colours=%d", (int)colours.size());
}

}


bool ToolTabs::enabled()
{
  return MqttLink::localDelivery() && WindowLayout::anyTab();
}


bool ToolTabs::wantsTab(const char *key)
{
  return s_book && WindowLayout::tab(key);
}


ToolTabs::MenuPlacement ToolTabs::menuPlacement()
{
  return SwapMenuBar;
}


void ToolTabs::setHost(wxFrame *host)
{
  if (s_hostFrame || !host) return;
#ifdef __WXOSX__
  // wx's automatic Window menu empties any menu titled "Window" natively each
  // time a bar is installed, behind wx's back; deleting our tab items from it
  // then raises an NSMenu exception (#1119).  The tabs' Window menu is ours.
  wxMenuBar::SetAutoWindowMenu(false);
#endif
  s_hostFrame = host;
  s_book = new wxAuiNotebook(host, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             wxAUI_NB_TOP | wxAUI_NB_TAB_MOVE |
                             wxAUI_NB_SCROLL_BUTTONS |
                             wxAUI_NB_CLOSE_ON_ALL_TABS | wxNO_BORDER);
  s_book->SetArtProvider(new ChromeTabArt);
  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(s_book, 1, wxEXPAND);
  host->SetSizer(sizer);
  routeMenus(host);
  // The notebook goes with the host; nothing may reach it afterwards.
  host->Bind(wxEVT_DESTROY, [host](wxWindowDestroyEvent& ev) {
    if (ev.GetEventObject() == host) { s_hostFrame = 0; s_book = 0; }
    ev.Skip();
  });
  // After the Organizer's own activate handler, which acts as Organizer.
  host->Bind(wxEVT_ACTIVATE, [host](wxActivateEvent& ev) {
    if (ev.GetActive())
      host->CallAfter([]() { activateTool(activeTab()); });
    ev.Skip();
  });
}


bool ToolTabs::adoptMain(wxFrame *org)
{
  if (!s_book || !org || mainTab()) return false;
  wxMenuBar *mb = org->wxFrame::GetMenuBar();
  if (mb) org->SetMenuBar(0);
  // A plain page, not a scroller: the Organizer's splitter sizes itself.
  ToolPage *page = new ToolPage(s_book, org, false);
  moveClient(org, page->body(), 0);
  tabs().push_back(Tab{org, 0, mb, page, _("Organizer"),
                       dotFor("Organizer"), "", {}, true});
  s_book->AddPage(page, _("Organizer"), true);
  hookTabCtrls();
  // Once per idle, over every shown window (see allocateNeverSized).
  wxTheApp->Bind(wxEVT_IDLE, [](wxIdleEvent& ev) {
    for (wxWindow *tlw : wxTopLevelWindows)
      if (tlw->IsShown()) allocateNeverSized(tlw);
    ev.Skip();
  });
  s_book->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, onPageChanged);
  s_book->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, onPageClose);
  s_book->Bind(wxEVT_AUINOTEBOOK_TAB_RIGHT_UP, onTabRightUp);
  // The Window menu lists the tabs in strip order.
  s_book->Bind(wxEVT_AUINOTEBOOK_DRAG_DONE, [](wxAuiNotebookEvent& ev) {
    if (s_hostFrame) fillWindowMenu(s_hostFrame->wxFrame::GetMenuBar());
    ev.Skip();
  });
  // wxAuiNotebook selects the page of any window that takes the focus.  A
  // hidden page's window gets it only from code (the Organizer refocusing
  // its tree after an update a tool's Save or Launch caused), never from the
  // user: keep the visible tab.  Bound handlers run before the notebook's.
  s_book->Bind(wxEVT_CHILD_FOCUS, [](wxChildFocusEvent& ev) {
    int idx = s_book->GetPageIndex(ev.GetWindow());
    if (idx != wxNOT_FOUND && idx != s_book->GetSelection()) return;
    ev.Skip();
  });
  placeMenus(mainTab());
  s_hostFrame->Layout();
  return true;
}


bool ToolTabs::open(wxFrame *tool, wxAuiManager *mgr, const wxString& title,
                    const char *key, SideFn side)
{
  if (!wantsTab(key)) return false;
  if (side && !findTool(tool)) {
    // adopt makes the page with a detail area when the Tab has a SideFn.
    tabs().push_back(Tab{tool, mgr, 0, 0, title, dotFor(title), key, side, false});
  }
  if (!adopt(tool, mgr, title)) return false;
  findTool(tool)->key = key;
  return true;
}


wxWindow *ToolTabs::sidePanel(const wxTopLevelWindow *tool)
{
  Tab *t = findTool(tool);
  return t && t->page ? static_cast<ToolPage*>(t->page)->side() : 0;
}


bool ToolTabs::fold(const wxTopLevelWindow *tool, int how)
{
  Tab *t = findTool(tool);
  if (!t || !t->page) return false;
  ToolPage *tp = static_cast<ToolPage*>(t->page);
  if (how >= 0) tp->fold(how == 1);
  return tp->folded();
}


void ToolTabs::allocate(wxWindow *w)
{
  allocateChildren(w);
  for (wxWindow *c : w->GetChildren())
    if (!c->IsTopLevel()) allocate(c);
}


wxTopLevelWindow *ToolTabs::activeTool()
{
  Tab *t = activeTab();
  return t ? t->tool : 0;
}


namespace {
std::map<wxString, ToolTabs::CommandFn>& commands()
{
  static auto *m = new std::map<wxString, ToolTabs::CommandFn>;
  return *m;
}
}


void ToolTabs::addCommand(const wxString& name, CommandFn fn)
{
  commands()[name] = fn;
}


bool ToolTabs::adopt(wxFrame *tool, wxAuiManager *mgr, const wxString& title)
{
  if (!s_book || !tool) return false;
  Tab *t = findTool(tool);
  if (t && t->page) return true;
  if (!t) {
    tabs().push_back(Tab{tool, mgr, 0, 0, title, dotFor(title), "", {}, false});
    t = &tabs().back();
  }
  if (wxMenuBar *mb = tool->wxFrame::GetMenuBar()) {
    tool->SetMenuBar(0);
    t->menubar = mb;
  }
  setMoveToTabsItem(tool, t->menubar, false);
  ToolPage *page = new ToolPage(s_book, tool, mgr == 0, (bool)t->side);
  moveClient(tool, page->body(), mgr);
  page->takeStatusBar(tool);
  t->page = page;
  if (t->side) {
    t->side(page->side());
    page->fitSash();
  }
  wxString caption = tool->GetTitle().empty() ? title : tool->GetTitle();
  t->dot = dotFor(caption);
  s_book->AddPage(page, tabCaption(caption), true);
  hookTabCtrls();
  // AddPage selected it; make sure the menus follow even if it was the
  // only change.
  placeMenus(activeTab());
  refreshTabTips();
  return true;
}


wxMenuBar *ToolTabs::menuBar(const wxTopLevelWindow *tool, wxMenuBar *own)
{
  if (own) return own;
  Tab *t = findTool(tool);
  return t && t->page ? t->menubar : 0;
}


bool ToolTabs::closingTool() { return s_closingTool; }

bool ToolTabs::hosts(const wxTopLevelWindow *tool)
{
  Tab *t = findTool(tool);
  return t && t->page;
}


wxTopLevelWindow *ToolTabs::screenWindow(const wxTopLevelWindow *tlw)
{
  return s_hostFrame && hosts(tlw) ? s_hostFrame
                                   : const_cast<wxTopLevelWindow*>(tlw);
}


wxSizer *ToolTabs::sizer(const wxTopLevelWindow *tool, wxSizer *own)
{
  if (own) return own;
  Tab *t = findTool(tool);
  return t && t->page ? static_cast<ToolPage*>(t->page)->body()->GetSizer() : 0;
}


wxTopLevelWindow *ToolTabs::pageTool(const wxWindow *w)
{
  Tab *t = findPage(w);
  return t ? t->tool : 0;
}


wxWindow *ToolTabs::client(const wxTopLevelWindow *tool)
{
  Tab *t = findTool(tool);
  return t && t->page ? static_cast<ToolPage*>(t->page)->body() : 0;
}


// GTK keeps its own allocation per widget, made when the parent's pizza is
// size-allocated.  Children reparented into a scroller body whose size never
// changes after they arrive (content fits, no scrollbar comes or goes) are
// never allocated: they stay at GTK's unallocated 1x1 and draw nothing,
// though wx has them laid out, and a queued resize does not reach them.
// Allocate those, and widgets whose GTK size differs from wx's, at their wx
// rectangle, as wxGTK does within a size-allocate.
static void allocateChildren(wxWindow *body)
{
#ifdef __WXGTK__
  bool any = false;
  for (wxWindow *w : body->GetChildren()) {
    GtkWidget *wd = w->m_widget;
    if (!wd || !w->IsShown() || !gtk_widget_get_visible(wd)) continue;
    GtkAllocation a;
    gtk_widget_get_allocation(wd, &a);
    wxRect r(w->GetPosition(), w->GetSize());
    // Also a widget whose place or size is stale: a static box that kept
    // its old height frames too little when its content grew, and the
    // rows below stay where they were.
    if ((a.x == r.x && a.y == r.y && a.width == r.width &&
         a.height == r.height) || r.width < 2 ||
        r.height < 2) continue;
    GtkRequisition req;
    gtk_widget_get_preferred_size(wd, &req, NULL);
    GtkAllocation na = {r.x, r.y, r.width, r.height};
    gtk_widget_size_allocate(wd, &na);
    any = true;
  }
  if (any) body->Refresh();
#else
  (void)body;
#endif
}


// GTK sizes a widget only when its parent is size-allocated: one created,
// reparented or shown inside a parent whose size then stays the same keeps
// GTK's 1x1 and draws nothing, though wx has laid it out (a desk built while
// its tab was in the background, Launch fields after a machine change).
// Each site that hit this was patched alone; this pass covers them all.
static void allocateNeverSized(wxWindow *parent)
{
#ifdef __WXGTK__
  for (wxWindow *w : parent->GetChildren()) {
    if (w->IsTopLevel() || !w->IsShown()) continue;
    GtkWidget *wd = w->m_widget;
    if (!wd || !gtk_widget_get_visible(wd)) continue;
    GtkAllocation a;
    gtk_widget_get_allocation(wd, &a);
    const wxSize sz = w->GetSize();
    if (a.width <= 1 && a.height <= 1 && sz.x > 1 && sz.y > 1) {
      const wxPoint pos = w->GetPosition();
      GtkAllocation na = {pos.x, pos.y, sz.x, sz.y};
      gtk_widget_size_allocate(wd, &na);
      w->Refresh();
    }
    allocateNeverSized(w);
  }
#else
  (void)parent;
#endif
}


// A scroller lays its sizer out at its virtual size, which nobody updates when
// content grows (a summary line appears): the rows then pile up in the old
// height and overlap.  Virtual size = the sizer's minimum, at least the view.
static void fitScrollBody(wxWindow *body)
{
  wxScrolledWindow *sw = wxDynamicCast(body, wxScrolledWindow);
  if (!sw || !sw->GetSizer()) return;
  wxSize min = sw->GetSizer()->CalcMin();
  wxSize view = sw->GetClientSize();
  wxSize want(wxMax(min.x, view.x), wxMax(min.y, view.y));
  if (sw->GetVirtualSize() != want) sw->SetVirtualSize(want);
}


bool ToolTabs::layout(wxTopLevelWindow *tool)
{
  Tab *t = findTool(tool);
  if (!t || !t->page) return false;
  wxWindow *body = static_cast<ToolPage*>(t->page)->body();
  fitScrollBody(body);
  body->Layout();
  allocateChildren(body);
  return true;
}


void ToolTabs::select(wxTopLevelWindow *tool)
{
  Tab *t = findTool(tool);
  if (!t || !t->page) return;
  int idx = s_book->GetPageIndex(t->page);
  if (idx != wxNOT_FOUND && idx != s_book->GetSelection())
    s_book->SetSelection(idx);
  if (s_hostFrame->IsIconized()) s_hostFrame->Iconize(false);
  s_hostFrame->Raise();
  // A frame's first Show lays its content out; a tab gets no such event.
  wxWindow *body = static_cast<ToolPage*>(t->page)->body();
  fitScrollBody(body);
  body->Layout();
  allocateChildren(body);
}


void ToolTabs::setTitle(wxTopLevelWindow *tool, const wxString& title)
{
  Tab *t = findTool(tool);
  if (!t || !t->page) return;
  // The Organizer's tab keeps its short caption; the window shows the title.
  if (t->pinned) { s_hostBaseTitle = title; updateHostTitle(); return; }
  // A Viewer can still have the Builder title when its tab is created.
  t->dot = dotFor(title);
  int idx = s_book->GetPageIndex(t->page);
  if (idx != wxNOT_FOUND) s_book->SetPageText(idx, tabCaption(title));
  updateHostTitle();
  refreshTabTips();
  fillWindowMenu(s_hostFrame->wxFrame::GetMenuBar());
}


void ToolTabs::contextChanged()
{
  static bool pending = false;
  if (!s_hostFrame || pending) return;
  pending = true;
  s_hostFrame->CallAfter([]() {
    pending = false;
    refreshTabTips();
    if (s_hostFrame) fillWindowMenu(s_hostFrame->wxFrame::GetMenuBar());
  });
}


void ToolTabs::release(wxTopLevelWindow *tlw, bool showFrame)
{
  Tab *t = findTool(tlw);
  if (!t || !t->page) return;
  // The host went first (an exception ended the main loop, #1119): the
  // page and the bar on the host went with it.
  if (!s_book || !s_hostFrame) {
    if (!showFrame)
      for (size_t i = 0; i < tabs().size(); i++)
        if (tabs()[i].tool == t->tool) { tabs().erase(tabs().begin() + i); break; }
    return;
  }
  wxFrame *tool = t->tool;
  wxWindow *page = t->page;
  wxSize size = page->GetSize();
  int idx = s_book->GetPageIndex(page);
  // Bring the tab that stays to the front before this one is taken apart,
  // or the window shows a blank page for a few frames on each close
  // (#1210). No Freeze: on GTK a frozen frame is shown black meanwhile.
  if (idx != wxNOT_FOUND && idx == s_book->GetSelection() &&
      s_book->GetPageCount() > 1)
    s_book->SetSelection(idx > 0 ? idx - 1 : 1);
  // The tab's menu bar leaves the main window first.
  if (s_hostFrame->wxFrame::GetMenuBar() == t->menubar)
    putBar(!t->pinned && mainTab() ? mainTab()->menubar : 0);
  else
    detachWindowMenu(t->menubar);
  t->page = 0;
  ToolPage *tp = static_cast<ToolPage*>(page);
  moveClient(tp->body(), tool, t->mgr);
  tp->giveStatusBar(tool);
  if (idx != wxNOT_FOUND) s_book->DeletePage(idx);
  // Closing the last tool tab could leave the notebook with no selection,
  // so nothing showed the Organizer's page again: its desk stayed blank
  // until a focus change (#1237). Select a page and make sure it is shown.
  if (s_book->GetPageCount() > 0) {
    if (s_book->GetSelection() == wxNOT_FOUND) s_book->SetSelection(0);
    wxWindow *shown = s_book->GetPage(s_book->GetSelection());
    if (shown && !shown->IsShown()) shown->Show();
  }
  placeMenus(activeTab());
  if (t->menubar) {
    tool->SetMenuBar(t->menubar);
    setMoveToTabsItem(tool, t->menubar, showFrame);
  }
  if (showFrame) {
    tool->SetClientSize(size);
    tool->Show(true);
    // The frame does not lay out its status bar again after a reparent.
    tool->SendSizeEvent();
    tool->SetStatusBar(tool->GetStatusBar());
    tool->Raise();
  } else {
    // The tool is being destroyed.
    for (size_t i = 0; i < tabs().size(); i++)
      if (tabs()[i].tool == tool) { tabs().erase(tabs().begin() + i); break; }
  }
}


namespace {
// Test hook "paintlog": every paint of a window on the host, in order, so a
// tab open or close shows whether a half-made state reached the screen.
class PaintLog : public wxEventFilter
{
public:
  int FilterEvent(wxEvent& ev) override
  {
    if (ev.GetEventType() == wxEVT_PAINT && s_hostFrame) {
      wxWindow *w = wxDynamicCast(ev.GetEventObject(), wxWindow);
      if (w && wxGetTopLevelParent(w) == s_hostFrame) {
        count++;
        wxString e = wxString::Format("%s@%dx%d", w->GetClassInfo()->GetClassName(),
                                      w->GetSize().x, w->GetSize().y);
        if (seq.empty() || seq.back().first != e) seq.push_back({e, 1});
        else seq.back().second++;
      }
    }
    return Event_Skip;
  }
  int count = 0;
  std::vector<std::pair<wxString, int>> seq;
};
PaintLog *s_paintLog = 0;
}

namespace {
wxWindowRef s_focusBefore;
wxString s_dialogResult = "none";
wxString s_popupResult = "none";
wxString s_floatName;
// Dialogs moved over the main window.  The tool's frame can be gone by the
// time the dialog closes (its own "save changes?" question), so closing
// must not look at the dialog's parent again.
std::set<wxDialog*> s_moved;

bool hostedParent(wxDialog *dlg)
{
  wxWindow *parent = dlg->GetParent();
  wxWindow *top = parent ? wxGetTopLevelParent(parent) : 0;
  wxTopLevelWindow *tlw = wxDynamicCast(top, wxTopLevelWindow);
  return s_hostFrame && tlw && ToolTabs::hosts(tlw);
}
}


void ToolTabs::dialogOpened(wxDialog *dlg)
{
  if (!hostedParent(dlg)) return;
  s_moved.insert(dlg);
  s_focusBefore = wxWindow::FindFocus();
#ifdef __WXGTK__
  // The hidden frame has no screen position: GTK would stack and centre
  // the dialog against it.
  GtkWidget *d = (GtkWidget*)dlg->GetHandle();
  GtkWidget *h = (GtkWidget*)s_hostFrame->GetHandle();
  if (d && h) {
    gtk_window_set_transient_for(GTK_WINDOW(d), GTK_WINDOW(h));
    gtk_window_set_position(GTK_WINDOW(d), GTK_WIN_POS_CENTER_ON_PARENT);
  }
#endif
  // Centre now from the best size, and again once shown at its real size.
  wxRect r = s_hostFrame->GetScreenRect();
  wxSize sz = dlg->GetBestSize();
  // A native GTK message box has no wx size; GTK centres it on its parent.
  if (sz.x > 1 && sz.y > 1)
    dlg->Move(r.x + (r.width - sz.x) / 2, r.y + (r.height - sz.y) / 2);
  wxWeakRef<wxDialog> ref(dlg);
  wxWindowRef host(s_hostFrame);
  dlg->CallAfter([ref, host]() {
    wxDialog *d = ref.get();
    if (!d || !host) return;
    wxRect r = s_hostFrame->GetScreenRect();
    wxSize sz = d->GetSize();
    if (sz.x > 1 && sz.y > 1)
      d->Move(r.x + (r.width - sz.x) / 2, r.y + (r.height - sz.y) / 2);
  });
}


wxWindow *ToolTabs::popupParent(wxWindow *w)
{
  wxWindow *top = w ? wxGetTopLevelParent(w) : 0;
  wxTopLevelWindow *tlw = wxDynamicCast(top, wxTopLevelWindow);
  wxWindow *page = (s_hostFrame && tlw && hosts(tlw)) ? client(tlw) : 0;
  return page ? page : w;
}


void ToolTabs::secondaryOpened(wxWindow *win, wxWindow *parent)
{
#ifdef __WXGTK__
  wxWindow *top = parent ? wxGetTopLevelParent(parent) : 0;
  wxTopLevelWindow *tlw = wxDynamicCast(top, wxTopLevelWindow);
  if (!s_hostFrame || !win || !tlw || (tlw != s_hostFrame && !hosts(tlw))) return;
  GtkWidget *w = (GtkWidget*)win->GetHandle();
  GtkWidget *h = (GtkWidget*)s_hostFrame->GetHandle();
  if (w && h && GTK_IS_WINDOW(w))
    gtk_window_set_transient_for(GTK_WINDOW(w), GTK_WINDOW(h));
#else
  (void)win; (void)parent;
#endif
}


wxString ToolTabs::describeSecondary(wxWindow *win)
{
  if (!s_hostFrame || !win) return "no window";
  bool mapped = false;
  const char *transient = "none";
#ifdef __WXGTK__
  GtkWidget *g = (GtkWidget*)win->GetHandle();
  if (g && GTK_IS_WINDOW(g)) {
    GtkWindow *tf = gtk_window_get_transient_for(GTK_WINDOW(g));
    if (tf) transient = (GtkWidget*)tf == (GtkWidget*)s_hostFrame->GetHandle()
                        ? "host" : "other";
    mapped = gtk_widget_get_mapped(g);
  }
#endif
  wxRect r = win->GetScreenRect();
  wxPoint c(r.x + r.width / 2, r.y + r.height / 2);
  return wxString::Format("transient=%s inside=%d mapped=%d", transient,
      (int)s_hostFrame->GetScreenRect().Contains(c), (int)mapped);
}


void ToolTabs::dialogClosed(wxDialog *dlg)
{
  if (!s_moved.erase(dlg)) return;
  wxWindowRef was = s_focusBefore;
  s_focusBefore = 0;
  // The Organizer itself may be the thing closing (a quit question).
  wxWindowRef host(s_hostFrame);
  wxTheApp->CallAfter([was, host]() {
    if (!host) return;
    if (was && was->IsShownOnScreen()) { was->SetFocus(); return; }
    if (s_book && s_book->GetCurrentPage()) s_book->GetCurrentPage()->SetFocus();
  });
}


void ToolTabs::moveToTabs(wxFrame *tool)
{
  Tab *t = findTool(tool);
  if (!t || t->page) return;
  tool->Hide();
  adopt(tool, t->mgr, t->title);
}


wxString ToolTabs::describe()
{
  wxString out;
  if (!s_book) return "no tabs";
  for (size_t i = 0; i < s_book->GetPageCount(); i++) {
    out += wxString::Format("%d:%s%s ", (int)i, s_book->GetPageText(i),
                            (int)i == s_book->GetSelection() ? "*" : "");
  }
  for (Tab& t : tabs())
    if (t.tool && !t.page)
      out += "window:" + t.tool->GetTitle() +
             (t.tool->IsShown() ? "" : "(hidden)") +
             (t.tool->wxFrame::GetMenuBar() ? "" : "(no menus)") + " ";
  out += "menus=";
  if (wxMenuBar *mb = s_hostFrame->wxFrame::GetMenuBar())
    for (size_t i = 0; i < mb->GetMenuCount(); i++)
      out += (i ? "," : "") + wxMenuItem::GetLabelText(mb->GetMenuLabel(i));
  return out;
}


bool ToolTabs::testCommand(const wxString& cmd, const wxString& arg,
                           wxString& outcome)
{
  auto c = commands().find(cmd);
  if (c != commands().end()) {
    outcome = c->second(arg);
    return true;
  }
  if (cmd == "tabs") {
    outcome = describe();
    if (arg == "dots" && s_book) {
      outcome += " dots=";
      for (size_t i = 0; i < s_book->GetPageCount(); i++) {
        Tab *t = findPage(s_book->GetPage(i));
        if (t) outcome += wxString::Format("%d:%s ", (int)i,
                         t->dot.GetAsString(wxC2S_HTML_SYNTAX));
      }
    }
  } else if (cmd == "livecount") {
    outcome = LiveCount::report();
  } else if (cmd == "mainwindow") {
    // The main window as the process sees it: the host (or none, the
    // Organizer's own frame), whether the Organizer's frame is hidden and
    // unmapped, the tab count, the title, the application's top window.
    wxWindow *top = wxTheApp ? wxTheApp->GetTopWindow() : 0;
    wxString topName = top ? wxString(top->GetClassInfo()->GetClassName())
                           : wxString("none");
    Tab *m = mainTab();
    if (!s_hostFrame || !m) {
      outcome = "host=none topwindow=" + topName;
      return true;
    }
    bool mapped = m->tool->IsShown();
#ifdef __WXGTK__
    if (GtkWidget *gw = (GtkWidget*)m->tool->GetHandle())
      mapped = gtk_widget_get_mapped(gw);
#endif
    outcome = wxString::Format(
        "host=%s organizer=%s mapped=%d tabs=%d title=%s topwindow=%s",
        s_hostFrame->GetClassInfo()->GetClassName(),
        m->tool->IsShown() ? "shown" : "hidden", (int)mapped,
        (int)s_book->GetPageCount(), s_hostFrame->GetTitle(), topName);
  } else if (cmd == "paintlog") {
    // "paintlog on" starts a fresh log; "paintlog" reports it.
    if (arg == "on") {
      if (!s_paintLog) { s_paintLog = new PaintLog; wxEvtHandler::AddFilter(s_paintLog); }
      s_paintLog->count = 0;
      s_paintLog->seq.clear();
      outcome = "on";
      return true;
    }
    if (!s_paintLog) { outcome = "off"; return true; }
    outcome = wxString::Format("paints=%d seq=", s_paintLog->count);
    for (auto& e : s_paintLog->seq) outcome += wxString::Format("%s*%d,", e.first, e.second);
  } else if (cmd == "quitlabel") {
    // The Quit item of the menu bar on the host now.
    wxMenuBar *mb = s_hostFrame ? s_hostFrame->wxFrame::GetMenuBar() : 0;
    wxMenuItem *q = mb ? mb->FindItem(wxID_EXIT) : 0;
    outcome = q ? q->GetItemLabelText().BeforeFirst('\t') : wxString("none");
  } else if (cmd == "windowmenu") {
    // The active bar's Window menu as "<n>:<tab>[*]"; with a number, as if
    // that entry were chosen (the tab changes).
    if (!s_hostFrame || !s_book) { outcome = "no host"; return true; }
    wxMenuBar *mb = s_hostFrame->wxFrame::GetMenuBar();
    fillWindowMenu(mb);
    long n = -1;
    if (arg.ToLong(&n)) {
      wxCommandEvent ev(wxEVT_MENU, s_windowBase + (int)n);
      ev.SetEventObject(s_hostFrame);
      s_hostFrame->GetEventHandler()->ProcessEvent(ev);
      mb = s_hostFrame->wxFrame::GetMenuBar();
      fillWindowMenu(mb);
    }
    auto it = windowMenus().find(mb);
    if (it == windowMenus().end()) { outcome = "no window menu"; return true; }
    for (int i = 0; i < WINDOW_TABS_MAX; i++)
      if (wxMenuItem *item = it->second.menu->FindItem(s_windowBase + i))
        outcome += wxString::Format("%d:%s%s ", i, item->GetItemLabelText(),
                                    item->IsChecked() ? "*" : "");
  } else if (cmd == "menu" || cmd == "hostclose") {
    // A menu command (a number, or exit|about|preferences) or the window
    // manager's close, on the host, as the user does it; queued, since a
    // question it opens runs a nested loop.
    if (!s_hostFrame) { outcome = "no host"; return true; }
    long id = wxID_NONE;
    if (cmd == "menu") {
      if (arg == "exit") id = wxID_EXIT;
      else if (arg == "about") id = wxID_ABOUT;
      else if (arg == "preferences") id = wxID_PREFERENCES;
      else if (arg == "minimize") id = wxID_ICONIZE_FRAME;
      else if (!arg.ToLong(&id)) { outcome = "bad id"; return true; }
    }
    wxFrame *host = s_hostFrame;
    host->CallAfter([host, id, cmd]() {
      if (cmd == "hostclose") { host->Close(); return; }
      wxCommandEvent ev(wxEVT_MENU, (int)id);
      ev.SetEventObject(host);
      host->GetEventHandler()->ProcessEvent(ev);
    });
    outcome = "queued";
  } else if (cmd == "tabsel") {
    outcome = wxString::Format("selected=%d", s_book ? s_book->GetSelection() : -1);
  } else if (cmd == "tabsave") {
    // File > Save of the tab in front, through the main window's menu bar.
    wxCommandEvent ev(wxEVT_MENU, wxID_SAVE);
    ev.SetEventObject(s_hostFrame);
    outcome = s_hostFrame && s_hostFrame->GetEventHandler()->ProcessEvent(ev)
              ? "handled" : "not handled";
  } else if (cmd == "tabclose" || cmd == "tabfocus") {
    // tabclose: the tab's close button; tabfocus: code focusing a window
    // in that page (the notebook must not switch to a hidden page).
    long n = -1;
    if (!s_book || !arg.ToLong(&n) || n < 0 ||
        n >= (long)s_book->GetPageCount()) {
      outcome = "no such tab";
      return true;
    }
    if (cmd == "tabclose") {
      wxAuiNotebookEvent ev(wxEVT_AUINOTEBOOK_PAGE_CLOSE, s_book->GetId());
      ev.SetSelection(n);
      ev.SetEventObject(s_book);
      s_book->GetEventHandler()->ProcessEvent(ev);
      outcome = "sent";
    } else {
      wxWindow *target = 0;
      std::vector<wxWindow*> todo(1, s_book->GetPage(n));
      while (!todo.empty() && !target) {
        wxWindow *w = todo.back();
        todo.pop_back();
        if (w != s_book->GetPage(n) && w->CanAcceptFocus()) target = w;
        for (wxWindow *c : w->GetChildren())
          if (!c->IsTopLevel()) todo.push_back(c);
      }
      if (target) target->SetFocus();
      outcome = target ? "focused" : "nothing to focus";
    }
  } else if (cmd == "tab" || cmd == "towindow") {
    long n = -1;
    if (!s_book || !arg.ToLong(&n) || n < 0 ||
        n >= (long)s_book->GetPageCount()) {
      outcome = "no such tab";
      return true;
    }
    if (cmd == "tab") {
      s_book->SetSelection(n);
    } else {
      Tab *t = findPage(s_book->GetPage(n));
      if (!t || !t->tool || t->pinned) { outcome = "not a tool tab"; return true; }
      release(t->tool);
    }
    outcome = describe();
  } else if (cmd == "tabinfo") {
    // The active tool tab's contents: what a screenshot should show.
    Tab *t = activeTab();
    ToolPage *tp = t && t->tool ? static_cast<ToolPage*>(t->page) : 0;
    if (!tp) { outcome = "no tool tab"; return true; }
    int shown = 0;
    for (wxWindow *w : tp->body()->GetChildren())
      if (w->IsShown() && w->GetSize().x > 1 && w->GetSize().y > 1) shown++;
    wxStatusBar *sb = t->tool->GetStatusBar();
    bool mapped = t->tool->IsShown();
#ifdef __WXGTK__
    // gtk_window_present maps a window without telling wx.
    if (GtkWidget *gw = (GtkWidget*)t->tool->GetHandle())
      mapped = gtk_widget_get_mapped(gw);
#endif
    outcome = wxString::Format(
        "page=%dx%d onscreen=%d children=%d shown=%d statusbar=%s frame=%s",
        tp->GetSize().x, tp->GetSize().y, (int)tp->IsShownOnScreen(),
        (int)tp->body()->GetChildren().size(), shown,
        !sb ? "none" : sb->GetParent() == tp ? "page" : "frame",
        mapped ? "mapped" : "hidden");
    outcome += " " + reachInfo(tp->body(), arg) + " " + paintInfo(tp);
#ifdef __WXGTK__
    if (arg.Contains("gtk")) outcome += gtkInfo(tp->body());
#endif
  } else if (cmd == "sbinfo") {
    // Status bar of the tool whose title contains arg, shown in a window;
    // bottom is in frame coordinates (menu bar included), client the frame height.
    for (Tab& t : tabs()) {
      if (!t.tool || t.page || !t.tool->GetTitle().Contains(arg)) continue;
      wxStatusBar *sb = t.tool->GetStatusBar();
      if (!sb) { outcome = "none"; return true; }
      wxPoint pos = sb->GetPosition();
      outcome = wxString::Format("parent=%s bottom=%d client=%d",
          sb->GetParent() == t.tool ? "frame" : "other",
          pos.y + sb->GetSize().y, t.tool->GetSize().y);
      return true;
    }
    outcome = "no such window";
  } else if (cmd == "tabtips") {
    // Each tab's tooltip, "<n>:<tip>|" (the Organizer's is empty).
    if (!s_book) { outcome = "no tabs"; return true; }
    refreshTabTips();
    for (size_t i = 0; i < s_book->GetPageCount(); i++)
      outcome += wxString::Format("%d:%s|", (int)i, s_book->GetPageToolTip(i));
  } else if (cmd == "nexttab" || cmd == "prevtab") {
    // Window > Next Tab / Previous Tab; answers the selected tab's index.
    if (!s_hostFrame || !s_book) { outcome = "no host"; return true; }
    wxCommandEvent ev(wxEVT_MENU, cmd == "nexttab" ? ID_NEXT_TAB : ID_PREV_TAB);
    ev.SetEventObject(s_hostFrame);
    s_hostFrame->GetEventHandler()->ProcessEvent(ev);
    outcome = wxString::Format("%d", s_book->GetSelection());
  } else if (cmd == "tabkey") {
    // The key hook as GTK delivers Ctrl+Tab (next), Ctrl+Shift+Tab (prev),
    // Ctrl+PgDn (pgdn) or Ctrl+PgUp (pgup); answers the selected index.
    if (!s_hostFrame || !s_book) { outcome = "no host"; return true; }
    wxKeyEvent ev(wxEVT_CHAR_HOOK);
    ev.m_keyCode = arg == "pgdn" ? WXK_PAGEDOWN : arg == "pgup" ? WXK_PAGEUP : WXK_TAB;
    ev.SetControlDown(true);
    ev.SetRawControlDown(true);
    ev.SetShiftDown(arg == "prev");
    ev.SetEventObject(s_hostFrame);
    s_hostFrame->GetEventHandler()->ProcessEvent(ev);
    outcome = wxString::Format("%d", s_book->GetSelection());
  } else if (cmd == "windowitems") {
    // Every item of the active bar's Window menu in order, "-" for a
    // separator, the shortcut in brackets, "*" when checked.
    if (!s_hostFrame) { outcome = "no host"; return true; }
    wxMenuBar *mb = s_hostFrame->wxFrame::GetMenuBar();
    fillWindowMenu(mb);
    auto it = windowMenus().find(mb);
    if (it == windowMenus().end()) { outcome = "no window menu"; return true; }
    for (wxMenuItem *item : it->second.menu->GetMenuItems()) {
      if (item->IsSeparator()) { outcome += "-|"; continue; }
      outcome += wxMenuItem::GetLabelText(item->GetItemLabel().BeforeFirst('\t'));
      // GTK keeps the shortcut apart from the label text.
      if (wxAcceleratorEntry *acc = item->GetAccel()) {
        outcome += "[" + acc->ToString() + "]";
        delete acc;
      }
      outcome += wxString(item->IsCheckable() && item->IsChecked() ? "*" : "") + "|";
    }
  } else if (cmd == "charhook") {
    // What GTK does for a key press: the hook goes to the main window.
    long key = 0;
    arg.ToLong(&key);
    wxKeyEvent ev(wxEVT_CHAR_HOOK);
    ev.m_keyCode = key;
    ev.SetEventObject(s_hostFrame);
    outcome = s_hostFrame->GetEventHandler()->ProcessEvent(ev) ? "handled"
                                                          : "not handled";
  } else if (cmd == "dialog") {
    // Opens a message dialog over the active tool's hidden frame, as its
    // own code does, measures it 1.5 s later and closes it; the
    // measurement is read with "dialoginfo".
    Tab *t = activeTab();
    if (!t || !t->tool) { outcome = "no tool tab"; return true; }
    wxFrame *tool = t->tool;
    s_dialogResult = "pending";
    s_hostFrame->CallAfter([tool]() {
      wxMessageDialog dlg(tool, "Test message", "Test dialog", wxOK);
      wxTimer timer;
      timer.Bind(wxEVT_TIMER, [&dlg, &timer](wxTimerEvent&) {
        wxRect hr = s_hostFrame->GetScreenRect();
        wxRect dr = dlg.GetScreenRect();
        bool transient = false, mapped = false;
#ifdef __WXGTK__
        GtkWidget *d = (GtkWidget*)dlg.GetHandle();
        GtkWindow *tf = d ? gtk_window_get_transient_for(GTK_WINDOW(d)) : 0;
        transient = tf && (GtkWidget*)tf == (GtkWidget*)s_hostFrame->GetHandle();
        mapped = d && gtk_widget_get_mapped(d);
        // wx knows no size for a native message box: ask GTK.
        if (d) {
          gint x = 0, y = 0;
          gtk_window_get_position(GTK_WINDOW(d), &x, &y);
          dr = wxRect(x, y, gtk_widget_get_allocated_width(d),
                      gtk_widget_get_allocated_height(d));
        }
#endif
        s_dialogResult = wxString::Format(
            "transient=%d mapped=%d inside=%d dialog=%d,%d,%d,%d "
            "organizer=%d,%d,%d,%d",
            (int)transient, (int)mapped, (int)hr.Contains(dr),
            dr.x, dr.y, dr.width, dr.height, hr.x, hr.y, hr.width, hr.height);
        timer.Stop();
#ifdef __WXGTK__
        // A native message box runs gtk_dialog_run, which EndModal does
        // not end: left open, its nested loop keeps Quit from returning.
        if (d) { gtk_dialog_response(GTK_DIALOG(d), GTK_RESPONSE_OK); return; }
#endif
        dlg.EndModal(wxID_OK);
      });
      timer.StartOnce(1500);
      dlg.ShowModal();
      if (s_dialogResult == "pending") s_dialogResult = "closed early";
      s_dialogResult += wxString::Format(" focus=%s",
          wxWindow::FindFocus() ? "set" : "none");
    });
    outcome = "started";
  } else if (cmd == "popup") {
    // Pops a menu up through popupParent over the active tool, as its own
    // code does, at a point inside the main window; "popupinfo" reads the
    // measurement taken 1 s later (popup window a page, menu mapped,
    // inside the main window, near the point), then the menu is closed.
    Tab *t = activeTab();
    if (!t || !t->tool) { outcome = "no tool tab"; return true; }
    wxFrame *tool = t->tool;
    s_popupResult = "pending";
    s_hostFrame->CallAfter([tool]() {
      wxMenu menu;
      menu.Append(wxID_ANY, "One");
      menu.Append(wxID_ANY, "Two");
      wxWindow *w = ToolTabs::popupParent(tool);
      wxPoint want = s_hostFrame->GetScreenRect().GetTopLeft() + wxPoint(300, 200);
      wxTimer timer;
      timer.Bind(wxEVT_TIMER, [&menu, &timer, w, want](wxTimerEvent&) {
        bool mapped = false, inside = false, near = false;
#ifdef __WXGTK__
        GtkWidget *m = (GtkWidget*)menu.m_menu;
        mapped = m && gtk_widget_get_mapped(m);
        GdkWindow *gw = m ? gtk_widget_get_window(m) : 0;
        if (gw && mapped) {
          gint x = 0, y = 0;
          gdk_window_get_origin(gw, &x, &y);
          wxRect mr(x, y, gtk_widget_get_allocated_width(m),
                    gtk_widget_get_allocated_height(m));
          inside = s_hostFrame->GetScreenRect().Contains(mr);
          near = abs(x - want.x) < 30 && abs(y - want.y) < 30;
        }
        if (m) gtk_menu_popdown(GTK_MENU(m));
#endif
        s_popupResult = wxString::Format(
            "parent=%s shown=%d mapped=%d inside=%d near=%d",
            w->IsKindOf(CLASSINFO(wxTopLevelWindow)) ? "frame" : "page",
            (int)w->IsShownOnScreen(), (int)mapped,
            (int)inside, (int)near);
        timer.Stop();
      });
      timer.StartOnce(1000);
      w->PopupMenu(&menu, w->ScreenToClient(want));
      if (s_popupResult == "pending") s_popupResult = "closed early";
    });
    outcome = "started";
  } else if (cmd == "popupinfo") {
    outcome = s_popupResult;
  } else if (cmd == "float") {
    // Floats the first docked pane of the active tool's AUI manager;
    // "floatinfo" says whether its frame is shown and transient for the
    // main window.
    Tab *t = activeTab();
    if (!t || !t->tool || !t->mgr) { outcome = "no aui tool tab"; return true; }
    wxAuiPaneInfoArray& panes = t->mgr->GetAllPanes();
    s_floatName.clear();
    for (size_t i = 0; i < panes.GetCount(); i++) {
      wxAuiPaneInfo& p = panes[i];
      if (p.window && p.IsShown() && !p.IsToolbar() && !p.IsFloating() &&
          p.dock_direction != wxAUI_DOCK_CENTER && !p.name.empty()) {
        s_floatName = p.name;
        p.Float();
        break;
      }
    }
    if (s_floatName.empty()) { outcome = "no pane"; return true; }
    t->mgr->Update();
    outcome = "floated " + s_floatName;
  } else if (cmd == "floatinfo") {
    Tab *t = activeTab();
    if (!t || !t->mgr || s_floatName.empty()) { outcome = "none"; return true; }
    wxAuiPaneInfo& p = t->mgr->GetPane(s_floatName);
    wxWindow *f = p.IsOk() ? p.frame : 0;
    bool transient = false, mapped = false;
#ifdef __WXGTK__
    GtkWidget *g = f ? (GtkWidget*)f->GetHandle() : 0;
    GtkWindow *tf = g ? gtk_window_get_transient_for(GTK_WINDOW(g)) : 0;
    transient = tf && (GtkWidget*)tf == (GtkWidget*)s_hostFrame->GetHandle();
    mapped = g && gtk_widget_get_mapped(g);
#endif
    outcome = wxString::Format("float=%s frame=%d shown=%d transient=%d mapped=%d",
        s_floatName, (int)(f != 0), (int)(f && f->IsShownOnScreen()),
        (int)transient, (int)mapped);
  } else if (cmd == "dialoginfo") {
    outcome = s_dialogResult;
  } else if (cmd == "layout") {
    // "layout show": the stored setting; "layout small|normal|wide" chooses
    // a preset; "layout KEY=tab|window" edits one row (then Custom).
    wxString a = arg.Strip(wxString::both);
    if (a == "show") a.clear();
    WindowLayout::Preset p;
    if (a.Contains("=")) {
      wxString v = a.AfterFirst('=');
      if (v != "tab" && v != "window") { outcome = "bad row"; return true; }
      WindowLayout::setRow(a.BeforeFirst('=').ToStdString(), v == "tab");
    } else if (!a.empty()) {
      if (!WindowLayout::presetFromName(a.ToStdString(), p) ||
          p == WindowLayout::Custom) { outcome = "bad preset"; return true; }
      WindowLayout::setPreset(p);
    }
    outcome = wxString::FromUTF8(WindowLayout::describe().c_str());
  } else if (cmd == "tabmenu" || cmd == "alwayswindow") {
    // "tabmenu N": the right-click menu's items on tab N;
    // "alwayswindow N": its Always open in a window, as a click does.
    long n = -1;
    if (!s_book || !arg.ToLong(&n) || n < 0 ||
        n >= (long)s_book->GetPageCount()) {
      outcome = "no such tab";
      return true;
    }
    Tab *t = findPage(s_book->GetPage(n));
    if (!t || !t->tool || t->pinned) { outcome = "not a tool tab"; return true; }
    if (cmd == "tabmenu") {
      wxMenu menu;
      fillTabMenu(menu);
      for (size_t i = 0; i < menu.GetMenuItemCount(); i++)
        outcome += (i ? "|" : "") + menu.FindItemByPosition(i)->GetItemLabelText();
    } else {
      tabMenuChoice(t, ID_ALWAYS_WINDOW);
      outcome = describe() + " " +
                wxString::FromUTF8(WindowLayout::describe().c_str());
    }
  } else if (cmd == "totabs") {
    for (Tab& t : tabs())
      if (t.tool && !t.page && t.tool->GetTitle().Contains(arg)) {
        moveToTabs(t.tool);
        outcome = describe();
        return true;
      }
    outcome = "no such window";
  } else {
    return false;
  }
  return true;
}
