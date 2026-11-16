/**
 * @file
 *
 * Tools as tabs of the Organizer's window (#983 stage 2).  See ToolTabs.H
 * and docs/claude/services/tools-as-tabs.md.
 */
#include <cstdlib>
#include <map>
#include <vector>

#include "wx/aui/auibook.h"
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
#include "wx/panel.h"
#include "wx/scrolwin.h"
#include "wx/button.h"
#include "wx/dcmemory.h"
#include "wx/dcscreen.h"
#include "wx/image.h"
#include <set>
#include "wx/tokenzr.h"
#include "wx/settings.h"
#include "wx/sizer.h"
#include "wx/statusbr.h"
#include "wx/toolbar.h"

#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif

#include "util/MqttLink.H"
#include "util/WindowLayout.H"
#include "wxgui/ToolTabs.H"
#include "wx/splitter.h"

static void allocateChildren(wxWindow *body);
static void fitScrollBody(wxWindow *body);

namespace {

const int ID_OPEN_IN_WINDOW = wxID_HIGHEST + 7301;
const int ID_MOVE_TO_TABS = wxID_HIGHEST + 7302;
const int ID_ALWAYS_WINDOW = wxID_HIGHEST + 7303;

struct Tab {
  wxFrame *tool;          // 0 for the Organizer's own tab
  wxAuiManager *mgr;
  wxMenuBar *menubar;
  wxWindow *page;         // 0 while the tool is in its own window
  wxString title;
  wxColour dot;
  std::string key;        // its WindowLayout row
  ToolTabs::SideFn side;  // fills the page's detail area (Set-up tab)
};

wxFrame *s_host = 0;
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

Tab *activeTab()
{
  if (!s_book || s_book->GetSelection() == wxNOT_FOUND) return 0;
  return findPage(s_book->GetPage(s_book->GetSelection()));
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


// Placement of the active tab's menus.  SwapMenuBar: the main window's
// menu bar becomes the tab's.
void placeMenus(Tab *t)
{
  if (!t || !s_host) return;
  if (ToolTabs::menuPlacement() == ToolTabs::SwapMenuBar) {
    Tab& org = tabs().front();
    wxMenuBar *mb = t->menubar ? t->menubar : org.menubar;
    if (s_host->wxFrame::GetMenuBar() != mb) s_host->SetMenuBar(mb);
  }
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


// Menu commands from the main window's menu bar while a tool's menus are
// on it go to the tool's frame, as from its own menu bar.  Bound on the
// Organizer's frame, so they run before its event table (which handles the
// same wxID_SAVE, wxID_EXIT, ...).
wxFrame *menuTarget(int id)
{
  Tab *t = activeTab();
  if (!t || !t->tool || !t->menubar) return 0;
  return t->menubar->FindItem(id) ? t->tool : 0;
}

void routeMenus(wxFrame *host)
{
  host->Bind(wxEVT_MENU, [](wxCommandEvent& ev) {
    if (wxFrame *f = menuTarget(ev.GetId())) f->GetEventHandler()->ProcessEvent(ev);
    else ev.Skip();
  });
  // GTK sends the key hook to the top-level window, not up from the focused
  // window, so the tool's frame never sees it from a tab.
  host->Bind(wxEVT_CHAR_HOOK, [](wxKeyEvent& ev) {
    Tab *t = activeTab();
    if (t && t->tool && t->tool->GetEventHandler()->ProcessEvent(ev)) return;
    ev.Skip();
  });
  host->Bind(wxEVT_UPDATE_UI, [](wxUpdateUIEvent& ev) {
    if (wxFrame *f = menuTarget(ev.GetId())) f->GetEventHandler()->ProcessEvent(ev);
    else ev.Skip();
  });
  auto menu = [](wxMenuEvent& ev) {
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

  wxSize GetTabSize(wxDC& dc, wxWindow*, const wxString& caption,
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
    bool closable = t && t->tool;
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
      *outButton = b;
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


void onPageChanged(wxAuiNotebookEvent& ev)
{
  Tab *t = activeTab();
  placeMenus(t);
  activateTool(t);
  ev.Skip();
}


void onPageClose(wxAuiNotebookEvent& ev)
{
  // The notebook would delete the page under a live tool: the tool closes
  // itself (its save question may say Cancel), and its Destroy takes the
  // tab away.
  ev.Veto();
  Tab *t = s_book ? findPage(s_book->GetPage(ev.GetSelection())) : 0;
  if (t && t->tool) {
    wxFrame *tool = t->tool;
    s_host->CallAfter([tool]() { tool->Close(); });
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
  if (!t || !t->tool) return;
  wxFrame *tool = t->tool;
  if (id == ID_ALWAYS_WINDOW && !t->key.empty())
    WindowLayout::setRow(t->key, false);
  if (id == ID_OPEN_IN_WINDOW || id == ID_ALWAYS_WINDOW)
    ToolTabs::release(tool);
}

void onTabRightUp(wxAuiNotebookEvent& ev)
{
  Tab *t = s_book ? findPage(s_book->GetPage(ev.GetSelection())) : 0;
  if (!t || !t->tool) return;
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


void ToolTabs::setMainWindow(wxFrame *organizer)
{
  if (s_host || !organizer) return;
  s_host = organizer;
  s_book = new wxAuiNotebook(organizer, wxID_ANY, wxDefaultPosition,
                             wxDefaultSize,
                             wxAUI_NB_TOP | wxAUI_NB_TAB_MOVE |
                             wxAUI_NB_SCROLL_BUTTONS |
                             wxAUI_NB_CLOSE_ON_ALL_TABS | wxNO_BORDER);
  s_book->SetArtProvider(new ChromeTabArt);
  wxPanel *page = new wxPanel(s_book, wxID_ANY);
  moveClient(organizer, page, 0, s_book);
  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(s_book, 1, wxEXPAND);
  organizer->SetSizer(sizer);
  tabs().push_back(Tab{0, 0, organizer->wxFrame::GetMenuBar(), page, _("Organizer"),
                       dotFor("Organizer"), ""});
  s_book->AddPage(page, _("Organizer"), true);
  routeMenus(organizer);
  s_book->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, onPageChanged);
  s_book->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, onPageClose);
  s_book->Bind(wxEVT_AUINOTEBOOK_TAB_RIGHT_UP, onTabRightUp);
  // After the Organizer's own activate handler, which acts as Organizer.
  organizer->Bind(wxEVT_ACTIVATE, [organizer](wxActivateEvent& ev) {
    if (ev.GetActive())
      organizer->CallAfter([]() { activateTool(activeTab()); });
    ev.Skip();
  });
  organizer->Layout();
}


bool ToolTabs::open(wxFrame *tool, wxAuiManager *mgr, const wxString& title,
                    const char *key, SideFn side)
{
  if (!wantsTab(key)) return false;
  if (side && !findTool(tool)) {
    // adopt makes the page with a detail area when the Tab has a SideFn.
    tabs().push_back(Tab{tool, mgr, 0, 0, title, dotFor(title), key, side});
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
    tabs().push_back(Tab{tool, mgr, 0, 0, title, dotFor(title), "", {}});
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
  s_book->AddPage(page, tabCaption(caption), true);
  // AddPage selected it; make sure the menus follow even if it was the
  // only change.
  placeMenus(activeTab());
  return true;
}


wxMenuBar *ToolTabs::menuBar(const wxTopLevelWindow *tool, wxMenuBar *own)
{
  if (own) return own;
  Tab *t = findTool(tool);
  return t && t->page ? t->menubar : 0;
}


wxMenuBar *ToolTabs::mainMenuBar(const wxTopLevelWindow *host, wxMenuBar *own)
{
  if (!s_host || host != s_host || tabs().empty()) return own;
  return tabs().front().menubar;
}


bool ToolTabs::hosts(const wxTopLevelWindow *tool)
{
  Tab *t = findTool(tool);
  return t && t->page;
}


wxSizer *ToolTabs::sizer(const wxTopLevelWindow *tool, wxSizer *own)
{
  if (own) return own;
  Tab *t = findTool(tool);
  return t && t->page ? static_cast<ToolPage*>(t->page)->body()->GetSizer() : 0;
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
  if (s_host->IsIconized()) s_host->Iconize(false);
  s_host->Raise();
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
  int idx = s_book->GetPageIndex(t->page);
  if (idx != wxNOT_FOUND) s_book->SetPageText(idx, tabCaption(title));
}


void ToolTabs::release(wxTopLevelWindow *tlw, bool showFrame)
{
  Tab *t = findTool(tlw);
  if (!t || !t->page) return;
  wxFrame *tool = t->tool;
  wxWindow *page = t->page;
  wxSize size = page->GetSize();
  int idx = s_book->GetPageIndex(page);
  // The tab's menu bar leaves the main window first.
  if (s_host->wxFrame::GetMenuBar() == t->menubar)
    s_host->SetMenuBar(tabs().front().menubar);
  t->page = 0;
  ToolPage *tp = static_cast<ToolPage*>(page);
  moveClient(tp->body(), tool, t->mgr);
  tp->giveStatusBar(tool);
  if (idx != wxNOT_FOUND) s_book->DeletePage(idx);
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
wxWindowRef s_focusBefore;
wxString s_dialogResult = "none";
// Dialogs moved over the main window.  The tool's frame can be gone by the
// time the dialog closes (its own "save changes?" question), so closing
// must not look at the dialog's parent again.
std::set<wxDialog*> s_moved;

bool hostedParent(wxDialog *dlg)
{
  wxWindow *parent = dlg->GetParent();
  wxWindow *top = parent ? wxGetTopLevelParent(parent) : 0;
  wxTopLevelWindow *tlw = wxDynamicCast(top, wxTopLevelWindow);
  return s_host && tlw && ToolTabs::hosts(tlw);
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
  GtkWidget *h = (GtkWidget*)s_host->GetHandle();
  if (d && h) {
    gtk_window_set_transient_for(GTK_WINDOW(d), GTK_WINDOW(h));
    gtk_window_set_position(GTK_WINDOW(d), GTK_WIN_POS_CENTER_ON_PARENT);
  }
#endif
  // Centre now from the best size, and again once shown at its real size.
  wxRect r = s_host->GetScreenRect();
  wxSize sz = dlg->GetBestSize();
  // A native GTK message box has no wx size; GTK centres it on its parent.
  if (sz.x > 1 && sz.y > 1)
    dlg->Move(r.x + (r.width - sz.x) / 2, r.y + (r.height - sz.y) / 2);
  wxWeakRef<wxDialog> ref(dlg);
  wxWindowRef host(s_host);
  dlg->CallAfter([ref, host]() {
    wxDialog *d = ref.get();
    if (!d || !host) return;
    wxRect r = s_host->GetScreenRect();
    wxSize sz = d->GetSize();
    if (sz.x > 1 && sz.y > 1)
      d->Move(r.x + (r.width - sz.x) / 2, r.y + (r.height - sz.y) / 2);
  });
}


void ToolTabs::dialogClosed(wxDialog *dlg)
{
  if (!s_moved.erase(dlg)) return;
  wxWindowRef was = s_focusBefore;
  s_focusBefore = 0;
  // The Organizer itself may be the thing closing (a quit question).
  wxWindowRef host(s_host);
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
  if (wxMenuBar *mb = s_host->wxFrame::GetMenuBar())
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
      if (!t || !t->tool) { outcome = "not a tool tab"; return true; }
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
  } else if (cmd == "charhook") {
    // What GTK does for a key press: the hook goes to the main window.
    long key = 0;
    arg.ToLong(&key);
    wxKeyEvent ev(wxEVT_CHAR_HOOK);
    ev.m_keyCode = key;
    ev.SetEventObject(s_host);
    outcome = s_host->GetEventHandler()->ProcessEvent(ev) ? "handled"
                                                          : "not handled";
  } else if (cmd == "dialog") {
    // Opens a message dialog over the active tool's hidden frame, as its
    // own code does, measures it 1.5 s later and closes it; the
    // measurement is read with "dialoginfo".
    Tab *t = activeTab();
    if (!t || !t->tool) { outcome = "no tool tab"; return true; }
    wxFrame *tool = t->tool;
    s_dialogResult = "pending";
    s_host->CallAfter([tool]() {
      wxMessageDialog dlg(tool, "Test message", "Test dialog", wxOK);
      wxTimer timer;
      timer.Bind(wxEVT_TIMER, [&dlg, &timer](wxTimerEvent&) {
        wxRect hr = s_host->GetScreenRect();
        wxRect dr = dlg.GetScreenRect();
        bool transient = false, mapped = false;
#ifdef __WXGTK__
        GtkWidget *d = (GtkWidget*)dlg.GetHandle();
        GtkWindow *tf = d ? gtk_window_get_transient_for(GTK_WINDOW(d)) : 0;
        transient = tf && (GtkWidget*)tf == (GtkWidget*)s_host->GetHandle();
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
    if (!t || !t->tool) { outcome = "not a tool tab"; return true; }
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
