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
#include "wx/control.h"
#include "wx/dcclient.h"
#include "wx/frame.h"
#include "wx/menu.h"
#include "wx/panel.h"
#include "wx/settings.h"
#include "wx/sizer.h"
#include "wx/statusbr.h"
#include "wx/toolbar.h"

#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif

#include "util/MqttLink.H"
#include "wxgui/ToolTabs.H"

namespace {

const int ID_OPEN_IN_WINDOW = wxID_HIGHEST + 7301;
const int ID_MOVE_TO_TABS = wxID_HIGHEST + 7302;

struct Tab {
  wxFrame *tool;          // 0 for the Organizer's own tab
  wxAuiManager *mgr;
  wxMenuBar *menubar;
  wxWindow *page;         // 0 while the tool is in its own window
  wxString title;
  wxColour dot;
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
  ToolPage(wxWindow *parent, wxFrame *tool)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxTAB_TRAVERSAL | wxNO_BORDER), p_tool(tool)
  {
    // The body is the tool's managed window; the status bar sits below it
    // (AUI would otherwise lay its panes over the bar).
    p_body = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                         wxTAB_TRAVERSAL | wxNO_BORDER);
    wxBoxSizer *sz = new wxBoxSizer(wxVERTICAL);
    sz->Add(p_body, 1, wxEXPAND);
    SetSizer(sz);
  }
  wxWindow *body() const { return p_body; }
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


void onTabRightUp(wxAuiNotebookEvent& ev)
{
  Tab *t = s_book ? findPage(s_book->GetPage(ev.GetSelection())) : 0;
  if (!t || !t->tool) return;
  wxMenu menu;
  menu.Append(ID_OPEN_IN_WINDOW, _("Open in Window"),
              _("Move this tool into a window of its own"));
  wxFrame *tool = t->tool;
  if (s_book->GetPopupMenuSelectionFromUser(menu) == ID_OPEN_IN_WINDOW)
    ToolTabs::release(tool);
}

}


bool ToolTabs::enabled()
{
  if (!MqttLink::localDelivery()) return false;
  const char *layout = getenv("ECCE_LAYOUT");
  return layout && wxString(layout).Lower() == "small";
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
                       dotFor("Organizer")});
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


bool ToolTabs::adopt(wxFrame *tool, wxAuiManager *mgr, const wxString& title)
{
  if (!s_book || !tool) return false;
  Tab *t = findTool(tool);
  if (t && t->page) return true;
  if (!t) {
    tabs().push_back(Tab{tool, mgr, 0, 0, title, dotFor(title)});
    t = &tabs().back();
  }
  if (wxMenuBar *mb = tool->wxFrame::GetMenuBar()) {
    tool->SetMenuBar(0);
    t->menubar = mb;
  }
  setMoveToTabsItem(tool, t->menubar, false);
  ToolPage *page = new ToolPage(s_book, tool);
  moveClient(tool, page->body(), mgr);
  page->takeStatusBar(tool);
  t->page = page;
  wxString caption = tool->GetTitle().empty() ? title : tool->GetTitle();
  s_book->AddPage(page, caption, true);
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


bool ToolTabs::layout(wxTopLevelWindow *tool)
{
  Tab *t = findTool(tool);
  if (!t || !t->page) return false;
  static_cast<ToolPage*>(t->page)->body()->Layout();
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
}


void ToolTabs::setTitle(wxTopLevelWindow *tool, const wxString& title)
{
  Tab *t = findTool(tool);
  if (!t || !t->page) return;
  int idx = s_book->GetPageIndex(t->page);
  if (idx != wxNOT_FOUND) s_book->SetPageText(idx, title);
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
