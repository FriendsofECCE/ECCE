#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/dcbuffer.h"
#include "wx/dcgraph.h"
#include "wx/tooltip.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include "wx/statline.h"
#include "wx/spinctrl.h"
#include <map>
#include <sstream>

#include "util/ResourceUtils.H"
#include "util/Preferences.H"
#include "wxgui/ewxBitmap.H"
#include "wx/toolbar.h"

#include "dsm/PropertyTask.H"
#include "dsm/ResourceType.H"
#include "dsm/ScanPoint.H"
#include "dsm/VDoc.H"
#include "dsm/Session.H"

#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WxResourceTreeItemData.H"
#include "wxgui/WxStateImageList.H"

#include "CalcMgr.H"
#include "RecipePanel.H"
#include "WorkflowLayout.H"
#include "WorkflowPanel.H"

using std::string;
using std::vector;

namespace {

const int kMargin = kWfMargin;

wxColour codeColour(const WorkflowSteps::Code *c)
{
  return c ? wxColour(c->r, c->g, c->b) : wxColour(0x86, 0x8e, 0x96);
}

/// The Organizer tree's run-state icon: the image list is indexed by the
/// state itself, so a box and the tree can never disagree.
int stateIcon(ResourceDescriptor::RUNSTATE s)
{
  return (int)s;
}

struct Seg { wxPoint a, b; };

}  // namespace


/// One box of the drawing.
struct WfBox {
  Resource *step;
  Resource *source;
  wxRect rect;
  double row;
  bool hasIn = false;
};

typedef WfLayoutLink WfLink;


class WorkflowCanvas : public wxScrolledWindow
{
public:
  WorkflowCanvas(WorkflowPanel *panel)
    : wxScrolledWindow(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                       wxHSCROLL | wxVSCROLL | wxBORDER_SUNKEN),
      p_panel(panel)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetScrollRate(10, 10);
    Bind(wxEVT_PAINT, &WorkflowCanvas::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &WorkflowCanvas::OnClick, this);
    Bind(wxEVT_LEFT_DCLICK, &WorkflowCanvas::OnDClick, this);
    Bind(wxEVT_RIGHT_DOWN, &WorkflowCanvas::OnRightDown, this);
    Bind(wxEVT_CONTEXT_MENU, &WorkflowCanvas::OnContextMenu, this);
  }

  void relayout()
  {
    p_boxes.clear();
    p_links.clear();
    Session *wf = p_panel->p_session;
    vector<WorkflowSteps::Node> nodes = WorkflowSteps::tree(wf);

    std::map<const Resource*, int> index;
    for (size_t i = 0; i < nodes.size(); i++) index[nodes[i].step] = (int)i;
    vector<WfLayoutNode> ln(nodes.size());
    for (size_t i = 0; i < nodes.size(); i++) {
      ln[i].depth = nodes[i].depth;
      ln[i].source = nodes[i].source ? index[nodes[i].source] : -1;
      for (size_t k = 0; k < nodes[i].in.size(); k++)
        ln[i].in.push_back(std::make_pair(index[nodes[i].in[k].source],
            WorkflowSteps::linkLabel(nodes[i].in[k].name)));
    }
    wxClientDC dc(this);
    dc.SetFont(GetFont());
    WfLayoutResult res = layoutWorkflow(dc, ln, p_panel->p_vertical);
    for (size_t i = 0; i < nodes.size(); i++) {
      WfBox b;
      b.step = nodes[i].step; b.source = nodes[i].source;
      b.row = res.rows[i]; b.rect = res.boxes[i]; b.hasIn = !nodes[i].in.empty();
      p_boxes.push_back(b);
    }
    p_links = res.links;
    int maxX = res.maxX, maxY = res.maxY;

    // A recipe workflow ends with its key-data box, right of the last column.
    p_keyRect = wxRect();
    p_key = KeyDataResult();
    p_hasKey = !WorkflowSteps::recipeId(wf).empty() && !nodes.empty();
    if (p_hasKey) {
      p_key = keyData(WorkflowSteps::keySteps(wf));
      const int lh = dc.GetTextExtent("Ag").y + 4;
      const int kw = 540;
      p_keyLines.clear();
      if (!p_key.level.empty())
        p_keyLines.push_back(std::make_pair(wxString::FromUTF8(p_key.level.c_str()), wxString()));
      for (size_t i = 0; i < p_key.rows.size(); i++)
        p_keyLines.push_back(std::make_pair(
            wxString::FromUTF8(p_key.rows[i].first.c_str()),
            wxString::FromUTF8(p_key.rows[i].second.c_str())));
      // The note wraps at the card's width.
      wxString line;
      wxArrayString words = wxSplit(wxString::FromUTF8(p_key.note.c_str()), ' ');
      for (size_t i = 0; i < words.size(); i++) {
        wxString next = line.empty() ? words[i] : line + " " + words[i];
        if (dc.GetTextExtent(next).x > kw - 28 && !line.empty()) {
          p_keyLines.push_back(std::make_pair(line, wxString()));
          next = words[i];
        }
        line = next;
      }
      if (!line.empty()) p_keyLines.push_back(std::make_pair(line, wxString()));
      p_keyNote = line.empty() ? 0 : 1;
      p_keyRect = wxRect(maxX + 60, kMargin, kw, ((int)p_keyLines.size() + 2) * lh + 20);
      if (p_panel->p_vertical)
        p_keyRect.SetPosition(wxPoint(kMargin, maxY + 60));
      maxX = std::max(maxX, p_keyRect.GetRight());
      maxY = std::max(maxY, p_keyRect.GetBottom());
    }
    SetVirtualSize(maxX + kMargin, maxY + kMargin);
  }

  const WfBox *boxAt(const wxPoint &unscrolled) const
  {
    for (size_t i = 0; i < p_boxes.size(); i++)
      if (p_boxes[i].rect.Contains(unscrolled)) return &p_boxes[i];
    return 0;
  }

  /// Labels that touch a box, a line, an arrow or another label.
  int overlaps() const
  {
    WfLayoutResult res;
    for (const WfBox &b : p_boxes) res.boxes.push_back(b.rect);
    res.links = p_links;
    int n = layoutOverlaps(res);
    if (p_hasKey)
      for (const WfLink &l : p_links)
        if (l.label.Intersects(p_keyRect)) n++;
    return n;
  }

  string report() const
  {
    int boxOverlaps = 0, rankErrors = 0;
    for (size_t i = 0; i < p_boxes.size(); i++)
      for (size_t j = i + 1; j < p_boxes.size(); j++)
        if (p_boxes[i].rect.Intersects(p_boxes[j].rect)) boxOverlaps++;
    for (const WfLink &l : p_links)
      if (p_panel->p_vertical ? l.p3.y <= l.p0.y : l.p3.x <= l.p0.x)
        rankErrors++;
    std::ostringstream o;
    o << "boxes=" << p_boxes.size() << " links=" << p_links.size()
      << " label-overlaps=" << overlaps() << " keybox=" << (p_hasKey ? 1 : 0)
      << " keyshown=" << (p_hasKey && p_key.shown ? 1 : 0) << " vertical=" << p_panel->p_vertical
      << " box-overlaps=" << boxOverlaps << " rank-errors=" << rankErrors
      << " canvas-height=" << GetSize().y << " icons=";
    for (size_t i = 0; i < p_boxes.size(); i++)
      o << (i ? "," : "") << p_boxes[i].step->getName() << ":"
        << stateIcon(WorkflowSteps::stateOf(p_boxes[i].step));
    return o.str();
  }

  /// "name: line" per box, for the test hook.
  string resultLines()
  {
    string out;
    for (const WfBox &b : p_boxes)
      out += (out.empty() ? "" : "|") + b.step->getName() + ": " +
             string(resultText(b.step).utf8_str());
    return out;
  }

  const WfBox *boxOf(const EcceURL &url) const
  {
    for (size_t i = 0; i < p_boxes.size(); i++)
      if (p_boxes[i].step->getURL() == url) return &p_boxes[i];
    return 0;
  }

  /// What a right-click on the box does, without the popup when popup is
  /// false (the test hook): the menu's labels, or "error: ...".
  string rightClick(const WfBox *b, bool popup)
  {
    if (!b) return "error: no such box";
    string error = CalcMgr::guarded([&]() { select(b, true); });
    if (!error.empty()) return "error: " + error;
    return p_panel->contextMenu(popup);
  }

private:
  // A right-click selects synchronously: the menu is built from the
  // selection, and a pending event would leave it on the previous one.
  void select(const WfBox *b, bool now = false, bool add = false)
  {
    if (!b) return;
    if (!now)
      p_panel->pick(b->step->getURL(), add);
    else if (std::find(p_panel->p_picked.begin(), p_panel->p_picked.end(),
                       b->step->getURL()) == p_panel->p_picked.end())
      p_panel->pick(b->step->getURL(), false);
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, wxID_PANEL_SELECTION);
    evt.SetString(b->step->getURL().toString());
    evt.SetInt(1);
    p_panel->p_selected = b->step->getURL();
    if (now) p_panel->GetEventHandler()->ProcessEvent(evt);
    else p_panel->AddPendingEvent(evt);
    Refresh();
  }

  void OnClick(wxMouseEvent &e)
  {
    SetFocus();
    select(boxAt(CalcUnscrolledPosition(e.GetPosition())), false, e.CmdDown());
  }

  void OnDClick(wxMouseEvent &e)
  {
    const WfBox *b = boxAt(CalcUnscrolledPosition(e.GetPosition()));
    if (b) p_panel->p_calcMgr->findNode(b->step->getURL(), true);
  }

  // The menu comes from the context-menu event, not the button: handled
  // here it never reaches CalcMgr::OnContextMenu, which would show the
  // tree's menu a second time.
  void OnRightDown(wxMouseEvent &e)
  {
    SetFocus();
    e.Skip();
  }

  void OnContextMenu(wxContextMenuEvent &e)
  {
    const WfBox *b = 0;
    if (e.GetPosition() == wxDefaultPosition)      // keyboard
      b = boxOf(p_panel->p_selected);
    else
      b = boxAt(CalcUnscrolledPosition(ScreenToClient(e.GetPosition())));
    if (b) rightClick(b, true);
  }

  void OnPaint(wxPaintEvent &)
  {
    wxAutoBufferedPaintDC pdc(this);
    DoPrepareDC(pdc);
    pdc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
    pdc.Clear();
    wxGCDC dc(pdc);
    const wxColour text = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    const wxColour dim = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
    wxFont base = GetFont();
    wxFont bold = base; bold.SetWeight(wxFONTWEIGHT_BOLD);

    // Lines first, so a box never has one across it.
    dc.SetPen(wxPen(dim, 2));
    dc.SetBrush(wxBrush(dim));
    for (const WfLink &l : p_links) {
      dc.DrawLine(l.p0, l.p1);
      dc.DrawLine(l.p1, l.p2);
      dc.DrawLine(l.p2, p_panel->p_vertical ? wxPoint(l.p3.x, l.p3.y - 8)
                                          : wxPoint(l.p3.x - 8, l.p3.y));
      wxPoint head[3] = {l.p3, wxPoint(l.p3.x - 9, l.p3.y - 5),
                         wxPoint(l.p3.x - 9, l.p3.y + 5)};
      if (p_panel->p_vertical) {
        head[1] = wxPoint(l.p3.x - 5, l.p3.y - 9);
        head[2] = wxPoint(l.p3.x + 5, l.p3.y - 9);
      }
      dc.SetPen(*wxTRANSPARENT_PEN);
      dc.DrawPolygon(3, head);
      dc.SetPen(wxPen(dim, 2));
      dc.SetFont(base);
      dc.SetTextForeground(dim);
      dc.DrawText(l.text, l.label.GetPosition());
    }

    const EcceURL sel = p_panel->p_selected;
    for (const WfBox &b : p_boxes) {
      const WorkflowSteps::Code *code = WorkflowSteps::codeOf(b.step);
      const bool isSel = (b.step->getURL() == sel) ||
          std::find(p_panel->p_picked.begin(), p_panel->p_picked.end(),
                    b.step->getURL()) != p_panel->p_picked.end();
      const wxRect &r = b.rect;
      const bool needs = WorkflowSteps::needsStructure(b.step, b.hasIn);
      dc.SetBrush(wxBrush(needs ? wxColour(0xff, 0xf3, 0xcf)
                                : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
      dc.SetPen(isSel ? wxPen(wxColour(0x1c, 0x7e, 0xd6), 4) : wxPen(dim, 2));
      dc.DrawRoundedRectangle(r, 8);
      dc.SetPen(*wxTRANSPARENT_PEN);
      dc.SetBrush(wxBrush(codeColour(code)));
      dc.DrawRoundedRectangle(wxRect(r.x + 1, r.y + 1, 8, r.height - 2), 4);

      dc.SetFont(bold);
      dc.SetTextForeground(codeColour(code));
      dc.DrawText(code ? code->label : "?", r.x + 18, r.y + 8);

      ResourceDescriptor::RUNSTATE st = WorkflowSteps::stateOf(b.step);
      string word = needs ? "needs structure" : ResourceUtils::stateToString(st);
      dc.SetFont(base);
      dc.SetTextForeground(needs ? wxColour(0x9c, 0x5a, 0x00) : dim);
      wxSize ws = dc.GetTextExtent(word);
      WxStateImageList *icons = WxStateImageList::getImageList();
      wxBitmap icon = icons->GetBitmap(stateIcon(st));
      if (!needs)
        dc.DrawBitmap(icon, r.GetRight() - 10 - icon.GetWidth(),
                      r.y + 16 - icon.GetHeight() / 2, true);
      dc.DrawText(word, r.GetRight() - (needs ? 12 : 16 + icon.GetWidth()) - ws.x,
                  r.y + 16 - ws.y / 2);

      dc.SetTextForeground(text);
      wxString name = wxControl::Ellipsize(wxString::FromUTF8(b.step->getName().c_str()),
                                           dc, wxELLIPSIZE_END, r.width - 32);
      const int lh = dc.GetTextExtent("Ag").y;
      dc.DrawText(name, r.x + 18, r.y + r.height - 8 - 2 * lh);
      const wxString res = resultText(b.step);
      if (!res.empty()) {
        dc.SetTextForeground(dim);
        dc.DrawText(wxControl::Ellipsize(res, dc, wxELLIPSIZE_END, r.width - 32),
                    r.x + 18, r.y + r.height - 8 - lh);
      }
    }

    if (p_hasKey) {
      const wxRect &k = p_keyRect;
      const int lh = dc.GetTextExtent("Ag").y + 4;
      dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
      dc.SetPen(wxPen(dim, 2));
      dc.DrawRoundedRectangle(k, 6);
      dc.SetFont(bold);
      dc.SetTextForeground(text);
      dc.DrawText("Key data", k.x + 14, k.y + 10);
      dc.SetFont(base);
      int y = k.y + 10 + lh + 6;
      for (size_t i = 0; i < p_keyLines.size(); i++, y += lh) {
        const bool isRow = !p_keyLines[i].second.empty();
        dc.SetTextForeground(isRow || (int)i < (p_key.level.empty() ? 0 : 1) ? text : dim);
        if (!isRow) {
          dc.DrawText(p_keyLines[i].first, k.x + 14, y);
          continue;
        }
        const int vw = dc.GetTextExtent(p_keyLines[i].second).x;
        dc.DrawText(wxControl::Ellipsize(p_keyLines[i].first, dc, wxELLIPSIZE_END,
                                         k.width - 40 - vw),
                    k.x + 14, y);
        dc.DrawText(p_keyLines[i].second, k.GetRight() - 14 - vw, y);
      }
    }
  }

public:
  /// A progress message or state change for a step: drop its cached line.
  void resultsChanged(const EcceURL &url)
  {
    p_results.erase(url.toString());
    for (const WfBox &b : p_boxes)
      if (b.step->getURL() == url) { Refresh(); return; }
  }

private:
  // The result line of a step, kept until its state or its snapshot changes,
  // so a paint reads no files.
  wxString resultText(Resource *step)
  {
    CalcMgr *mgr = p_panel->p_calcMgr;
    const string key = step->getURL().toString();
    const ResourceDescriptor::RUNSTATE st = WorkflowSteps::stateOf(step);
    const string ecce = VDoc::getEcceNamespace();
    string progress;
    if (st == ResourceDescriptor::STATE_RUNNING) {
      double at;
      if (!mgr || !mgr->liveProgress(step->getURL(), progress, at))
        progress = step->getProp(ecce + ":progress");
    }
    const string stamp = progress + "|" + step->getProp(ecce + ":completiondate") +
                         "|" + step->getProp(ecce + ":runStatusReason");
    auto it = p_results.find(key);
    if (it == p_results.end() || it->second.state != st || it->second.stamp != stamp) {
      CachedResult c;
      c.state = st;
      c.stamp = stamp;
      string error = CalcMgr::guarded([&]() {
        c.text = wxString::FromUTF8(WorkflowSteps::resultLine(step, progress).c_str());
      });
      if (!error.empty()) c.text.clear();
      it = p_results.insert(std::make_pair(key, c)).first;
      it->second = c;
    }
    return it->second.text;
  }

  struct CachedResult {
    ResourceDescriptor::RUNSTATE state = ResourceDescriptor::STATE_ILLEGAL;
    string stamp;
    wxString text;
  };
  std::map<string, CachedResult> p_results;

  WorkflowPanel *p_panel;
  vector<WfBox> p_boxes;
  vector<WfLink> p_links;
  KeyDataResult p_key;
  wxRect p_keyRect;
  bool p_hasKey = false;
  int p_keyNote = 0;
  vector<std::pair<wxString, wxString> > p_keyLines;
};


namespace {

/// A code of the bar, drawn in its colour.
class CodeButton : public wxControl
{
public:
  CodeButton(wxWindow *parent, const WorkflowSteps::Code &code,
             std::function<void()> onClick)
    : wxControl(parent, wxID_ANY, wxDefaultPosition, wxSize(70, 28),
                wxBORDER_NONE), p_code(code), p_click(onClick)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(70, 28));
    SetToolTip(wxString("New ") + code.label + " step from the selected box");
    Bind(wxEVT_PAINT, [this](wxPaintEvent &) {
      wxAutoBufferedPaintDC pdc(this);
      pdc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
      pdc.Clear();
      wxGCDC dc(pdc);
      dc.SetPen(*wxTRANSPARENT_PEN);
      dc.SetBrush(wxBrush(codeColour(&p_code)));
      dc.DrawRoundedRectangle(wxRect(GetClientSize()).Deflate(1), 6);
      dc.SetTextForeground(*wxWHITE);
      wxSize ts = dc.GetTextExtent(p_code.label);
      dc.DrawText(p_code.label, (GetClientSize().x - ts.x) / 2,
                  (GetClientSize().y - ts.y) / 2);
    });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent &) { p_click(); });
  }

private:
  WorkflowSteps::Code p_code;
  std::function<void()> p_click;
};

}  // namespace


WorkflowPanel::WorkflowPanel(CalcMgr *calcMgr, WxResourceTreeItemData *node,
                             wxWindow *parent)
  : ewxPanel(parent, wxID_ANY), p_calcMgr(calcMgr),
    p_session(dynamic_cast<Session*>(node->getResource())),
    p_canvas(0), p_help(0), p_status(0), p_launch(0), p_apply(0), p_empty(0),
    p_left(0), p_top(0), p_recipe(0), p_vertical(false),
    p_toolbar(0), p_orientationId(wxWindow::NewControlId())
{
  Preferences prefs("Organizer");
  prefs.getBool("Organizer.Workflow.Vertical", p_vertical);
  createControls();
  Bind(wxEVT_MENU, [this](wxCommandEvent &) { recipeFromHere(); }, recipeHereId());
}


void WorkflowPanel::createControls()
{
  wxBoxSizer *top = new wxBoxSizer(wxHORIZONTAL);
  SetSizer(top);
  p_top = top;

  wxBoxSizer *left = new wxBoxSizer(wxVERTICAL);
  p_left = left;
  p_toolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxTB_HORIZONTAL | wxTB_NODIVIDER);
  p_toolbar->AddTool(p_orientationId, "Orientation",
      ewxBitmap(p_vertical ? "vtree16.xpm" : "htree16.xpm"),
      "Change tree orientation");
  p_toolbar->Realize();
  p_toolbar->Bind(wxEVT_TOOL, [this](wxCommandEvent &) {
    setVertical(!p_vertical);
  }, p_orientationId);
  left->Add(p_toolbar, 0, wxEXPAND);
  p_canvas = new WorkflowCanvas(this);
  left->Add(p_canvas, 1, wxEXPAND);

  // An empty workflow offers its two ways to start.
  p_empty = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN);
  wxBoxSizer *es = new wxBoxSizer(wxVERTICAL);
  wxStaticText *et = new wxStaticText(p_empty, wxID_ANY, "This workflow is empty.");
  wxFont ef = et->GetFont(); ef.SetWeight(wxFONTWEIGHT_BOLD); ef.SetPointSize(ef.GetPointSize() + 3);
  et->SetFont(ef);
  wxButton *eApply = new wxButton(p_empty, wxID_ANY, "Apply Recipe...");
  wxButton *eStep = new wxButton(p_empty, wxID_ANY, "New Step...");
  eApply->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { openRecipe(); });
  eStep->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) {
    say("Pick a code on the right to add the first step.");
  });
  wxBoxSizer *eb = new wxBoxSizer(wxHORIZONTAL);
  eb->Add(eApply, 0, wxRIGHT, 10);
  eb->Add(eStep, 0);
  es->AddStretchSpacer(1);
  es->Add(et, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 14);
  es->Add(eb, 0, wxALIGN_CENTER_HORIZONTAL);
  es->AddStretchSpacer(1);
  p_empty->SetSizer(es);
  left->Add(p_empty, 1, wxEXPAND);

  // The instructions have a panel of their own, never among the controls.
  wxPanel *notes = new wxPanel(this, wxID_ANY, wxDefaultPosition,
                               wxDefaultSize, wxBORDER_SIMPLE);
  wxBoxSizer *noteSizer = new wxBoxSizer(wxHORIZONTAL);
  p_help = new wxStaticText(notes, wxID_ANY, "");
  p_status = new wxStaticText(notes, wxID_ANY, "");
  wxBoxSizer *texts = new wxBoxSizer(wxVERTICAL);
  texts->Add(p_help, 0, wxALL, 6);
  texts->Add(p_status, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  // Beside the instructions, where the text that names it is read.
  p_launch = new wxButton(notes, wxID_ANY, "Launch Ready Steps");
  p_launch->SetToolTip("Launch every step that has not been launched and whose "
                       "source step is Complete");
  p_launch->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { launchReadyClicked(); });
  p_apply = new wxButton(notes, wxID_ANY, "Apply Recipe...");
  p_apply->SetToolTip("Add the steps of a recipe, after the selected box if there is one");
  p_apply->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { openRecipe(); });
  wxButton *saveRecipe = new wxButton(notes, wxID_ANY, "Save as Recipe...");
  saveRecipe->SetToolTip("Save this workflow's steps, settings and links as a recipe");
  saveRecipe->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) {
    wxTextEntryDialog dlg(this, "Title of the recipe:", "Save Workflow as Recipe",
                          wxString::FromUTF8(p_session->getName().c_str()));
    if (dlg.ShowModal() != wxID_OK) return;
    const string title = string(dlg.GetValue().ToUTF8());
    string id;
    for (size_t i = 0; i < title.size(); i++)
      id += isalnum((unsigned char)title[i]) ? (char)tolower(title[i]) : '-';
    const string got = saveAsRecipe(id, title);
    say(got.compare(0, 3, "ok ") == 0 ? "Saved as a recipe: " + got.substr(3) : got);
  });
  noteSizer->Add(texts, 1, wxEXPAND);
  noteSizer->Add(saveRecipe, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM | wxLEFT, 8);
  noteSizer->Add(p_apply, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM | wxLEFT, 8);
  noteSizer->Add(p_launch, 0, wxALIGN_CENTER_VERTICAL | wxALL, 8);
  notes->SetSizer(noteSizer);
  left->Add(notes, 0, wxEXPAND | wxTOP, 4);
  top->Add(left, 1, wxEXPAND);

  wxBoxSizer *bar = new wxBoxSizer(wxVERTICAL);
  const vector<WorkflowSteps::Code> &codes = WorkflowSteps::codes();
  for (size_t i = 0; i < codes.size(); i++) {
    bar->Add(new CodeButton(this, codes[i], [this, i]() { codeClicked(i); }),
             0, wxALL, 2);
  }
  bar->Add(new wxStaticText(this, wxID_ANY, "new step"), 0,
           wxALIGN_CENTER_HORIZONTAL | wxTOP, 4);
  top->Add(bar, 0, wxEXPAND | wxALL, 4);

  p_canvas->SetMinSize(wxSize(1, 120));
  p_canvas->relayout();
  updateTexts();
  // Keep the code bar and instructions visible even at the smallest canvas.
  SetMinSize(wxSize(1, top->GetMinSize().y));
  Layout();
}


void WorkflowPanel::setVertical(bool vertical)
{
  p_vertical = vertical;
  p_toolbar->SetToolNormalBitmap(p_orientationId,
      ewxBitmap(p_vertical ? "vtree16.xpm" : "htree16.xpm"));
  Preferences prefs("Organizer");
  prefs.setBool("Organizer.Workflow.Vertical", p_vertical);
  prefs.saveFile();
  p_canvas->Scroll(0, 0);
  p_canvas->relayout();
  p_canvas->Refresh();
}


void WorkflowPanel::updateTexts()
{
  if (p_left && !p_recipe) {
    const bool empty = p_session->numMembers() == 0;
    p_left->Show(p_canvas, !empty);
    p_left->Show(p_empty, empty);
  }
  if (p_session->numMembers() == 0)
    p_help->SetLabel("Pick a code on the right to add the first step.  "
                     "Its structure is set in the Builder.");
  else
    p_help->SetLabel(
      "Select a box, then a code on the right: the new step starts from that "
      "box's final geometry, charge and multiplicity.\n"
      "Several boxes from one source = branches.  Ctrl-click several boxes, "
      "then G16 or ORCA, joins them (QST2/QST3, NEB-TS).  Launch Ready Steps "
      "runs every step whose source is Complete.");
}


void WorkflowPanel::say(const string &text)
{
  if (p_status) p_status->SetLabel(wxString::FromUTF8(text.c_str()));
  Layout();
}


void WorkflowPanel::refresh()
{
  p_canvas->relayout();
  updateTexts();
  Layout();
  p_canvas->Refresh();
}


RecipePanel *WorkflowPanel::openRecipe()
{
  if (p_recipe) return 0;
  Resource *after = 0;
  vector<Resource*> picked = pickedSteps();
  if (picked.size() == 1) after = picked[0];
  p_recipe = new RecipePanel(this, this, p_session, after);
  p_left->Show(false);
  p_top->Insert(0, p_recipe, 1, wxEXPAND);
  Layout();
  return p_recipe;
}


void WorkflowPanel::closeRecipe()
{
  if (!p_recipe) return;
  p_top->Detach(p_recipe);
  p_recipe->Destroy();
  p_recipe = 0;
  p_left->Show(true);
  updateTexts();
  Layout();
}


string WorkflowPanel::recipeCreate(const Recipe &recipe, const RecipeVariant &variant,
                                   const WorkflowSteps::Levels &levels, Resource *after)
{
  string outcome;
  vector<Resource*> made = p_calcMgr->newRecipeSteps(p_session, recipe, variant, levels,
                                                     after, &outcome);
  if (made.empty()) return outcome.empty() ? "Nothing was created." : outcome;
  closeRecipe();
  refresh();
  return "";
}


string WorkflowPanel::saveAsRecipe(const string &id, const string &title)
{
  Recipe r;
  string why;
  if (!WorkflowSteps::toRecipe(p_session, id, title, r, why)) return "error: " + why;
  const string dir = WorkflowSteps::userRecipeDir();
  string mk = "mkdir -p '" + dir + "'";
  if (system(mk.c_str()) != 0) return "error: cannot make " + dir;
  const string file = dir + "/" + id + ".recipe";
  std::ofstream f(file.c_str());
  f << recipeText(r);
  f.close();
  return f ? "ok " + file : "error: cannot write " + file;
}


string WorkflowPanel::keyDataText()
{
  if (WorkflowSteps::recipeId(p_session).empty()) return "";
  return keyData(WorkflowSteps::keySteps(p_session)).text();
}


void WorkflowPanel::resultsChanged(const EcceURL &url)
{
  p_canvas->resultsChanged(url);
}


string WorkflowPanel::resultLines()
{
  return p_canvas->resultLines();
}


void WorkflowPanel::selectResource(const EcceURL &url)
{
  if (url != p_selected) p_picked.clear();
  p_selected = url;
  p_canvas->Refresh();
}


void WorkflowPanel::loseFocus()
{
  p_canvas->Refresh();
}


void WorkflowPanel::pick(const EcceURL &step, bool add)
{
  if (!add) {
    p_picked.clear();
    return;
  }
  if (p_picked.empty() && p_selected != EcceURL()) p_picked.push_back(p_selected);
  vector<EcceURL>::iterator it = std::find(p_picked.begin(), p_picked.end(), step);
  if (it == p_picked.end()) p_picked.push_back(step);
  else p_picked.erase(it);
}


vector<Resource*> WorkflowPanel::pickedSteps() const
{
  vector<Resource*> out;
  vector<Resource*> members = p_session->getMembers();
  const vector<EcceURL> urls = p_picked.size() > 1
      ? p_picked : vector<EcceURL>(1, p_selected);
  for (size_t k = 0; k < urls.size(); k++)
    for (size_t i = 0; i < members.size(); i++)
      if (members[i]->getURL() == urls[k]) out.push_back(members[i]);
  return out;
}


void WorkflowPanel::codeClicked(size_t code)
{
  const WorkflowSteps::Code &c = WorkflowSteps::codes()[code];
  ResourceDescriptor &rd = ResourceDescriptor::getResourceDescriptor();
  ResourceType *rt = 0;
  vector<ResourceType*> types = rd.getResourceTypes();
  for (size_t i = 0; i < types.size() && !rt; i++)
    if (types[i]->getName() == c.typeName) rt = types[i];
  if (!rt) { say(string("This installation has no ") + c.label + "."); return; }

  Resource *source = 0;
  if (p_session->numMembers() > 0) {
    vector<Resource*> picked = pickedSteps();
    if (picked.size() > 1) {
      if (!WorkflowSteps::canJoin(c)) {
        say(string("A join is made with G16 (QST2 or QST3) or ORCA (NEB-TS), "
                   "not ") + c.label + ".");
        return;
      }
      say("");
      joinClicked(c, rt, picked);
      return;
    }
    if (!picked.empty()) source = picked[0];
    if (!source) { say("Select the box the new step starts from first."); return; }
  }
  say("");
  p_calcMgr->newWorkflowStep(p_session, rt, source, true, 0);
}


void WorkflowPanel::launchReadyClicked()
{
  say("");
  p_calcMgr->launchReadySteps(p_session, true, 0, "");
}


void WorkflowPanel::joinClicked(const WorkflowSteps::Code &code,
                                ResourceType *rt,
                                const vector<Resource*> &sources)
{
  p_calcMgr->newWorkflowJoin(p_session, rt, WorkflowSteps::defaultRoles(sources),
                             true, 0, "");
}


int WorkflowPanel::recipeHereId()
{
  static const int id = wxNewId();
  return id;
}


// The recipe's chain is appended after the box that was right-clicked,
// which the right-click has selected.
RecipePanel *WorkflowPanel::recipeFromHere()
{
  closeRecipe();
  return openRecipe();
}


string WorkflowPanel::contextMenu(bool popup)
{
  wxMenu menu;
  string error = CalcMgr::guarded([&]() {
    p_calcMgr->getContextMenu(menu);
    menu.AppendSeparator();
    menu.Append(recipeHereId(), "Apply Recipe from here...");
    if (popup) ewxPopupMenu(this, menu);
  });
  if (!error.empty()) {
    p_calcMgr->setMessage("The menu for this box could not be made: " + error,
                          WxFeedback::ERROR);
    return "error: " + error;
  }
  string labels;
  wxMenuItemList &items = menu.GetMenuItems();
  for (wxMenuItemList::iterator it = items.begin(); it != items.end(); ++it)
    if (!(*it)->IsSeparator())
      labels += (labels.empty() ? "" : "|") +
                (*it)->GetItemLabelText().ToStdString();
  return labels;
}


string WorkflowPanel::rightClick(const EcceURL &step, bool popup)
{
  p_canvas->relayout();
  return p_canvas->rightClick(p_canvas->boxOf(step), popup);
}


string WorkflowPanel::launchButtonReport()
{
  Layout();
  const wxRect r = p_launch->GetScreenRect();
  const wxRect mine = GetScreenRect();
  return string("label=") + p_launch->GetLabel().ToStdString() +
         " shown=" + (p_launch->IsShownOnScreen() ? "1" : "0") +
         " inside=" + (mine.Contains(r) ? "1" : "0") +
         " enabled=" + (p_launch->IsEnabled() ? "1" : "0");
}


string WorkflowPanel::layoutReport()
{
  p_canvas->relayout();
  return p_canvas->report();
}


// ---- which point of a scan ----

/// Last point, the maximum, or a numbered point of a scan or IRC.
class PointChooser : public wxPanel
{
public:
  PointChooser(wxWindow *parent, const string &initial)
    : wxPanel(parent, wxID_ANY)
  {
    wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
    p_choice = new wxChoice(this, wxID_ANY);
    p_choice->Append("Last point");
    p_choice->Append("Maximum (highest energy)");
    p_choice->Append("Point number");
    p_number = new wxSpinCtrl(this, wxID_ANY, "1", wxDefaultPosition,
                              wxSize(70, -1), wxSP_ARROW_KEYS, 1, 9999, 1);
    row->Add(p_choice, 0, wxALIGN_CENTER_VERTICAL);
    row->Add(p_number, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
    SetSizerAndFit(row);
    set(initial);
    p_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent &) { sync(); });
  }

  void set(const string &spec)
  {
    if (spec == "max") p_choice->SetSelection(1);
    else if (spec.empty() || spec == "last") p_choice->SetSelection(0);
    else {
      p_choice->SetSelection(2);
      p_number->SetValue(atoi(spec.c_str()));
    }
    sync();
  }

  string spec() const
  {
    switch (p_choice->GetSelection()) {
      case 1: return "max";
      case 2: return std::to_string(p_number->GetValue());
      default: return "last";
    }
  }

private:
  void sync() { p_number->Enable(p_choice->GetSelection() == 2); }
  wxChoice *p_choice;
  wxSpinCtrl *p_number;
};


// ---- the new-step dialog ----

WorkflowStepDialog::WorkflowStepDialog(wxWindow *parent, Session *wf,
                                       const WorkflowSteps::Code &code,
                                       ResourceType *type, Resource *source)
  : wxDialog(parent, wxID_ANY, wxString("New ") + code.label + " step"),
    p_session(wf), p_type(type)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);

  wxFlexGridSizer *grid = new wxFlexGridSizer(2, 8, 12);
  grid->AddGrowableCol(1);
  grid->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0,
            wxALIGN_CENTER_VERTICAL);
  p_name = new wxTextCtrl(this, wxID_ANY,
                          WorkflowSteps::defaultName(wf, code),
                          wxDefaultPosition, wxSize(320, -1));
  grid->Add(p_name, 1, wxEXPAND);

  string from = "no earlier step";
  bool haveResult = false;
  if (source) {
    const WorkflowSteps::Code *sc = WorkflowSteps::codeOf(source);
    from = string(sc ? sc->label : "") + "  " + source->getName() + "   (" +
           ResourceUtils::stateToString(WorkflowSteps::stateOf(source)) + ")";
    haveResult = WorkflowSteps::hasResultGeometry(source);
  }
  grid->Add(new wxStaticText(this, wxID_ANY, "Starts from:"), 0,
            wxALIGN_CENTER_VERTICAL);
  grid->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8(from.c_str())),
            0, wxALIGN_CENTER_VERTICAL);
  top->Add(grid, 0, wxEXPAND | wxALL, 14);

  p_point = 0;
  p_geometry = 0;
  p_charge = 0;
  p_best = 0;
  p_bestN = 0;
  p_patterns = 0;
  if (!source) {
    // Nothing to carry over: say so instead of showing dead checkboxes.
    top->Add(new wxStaticText(this, wxID_ANY,
        "This step has no source: it starts from its own structure."),
        0, wxLEFT | wxRIGHT, 14);
  } else {
    wxStaticText *carryLabel = new wxStaticText(this, wxID_ANY, "Carry over:");
    wxFont bold = carryLabel->GetFont(); bold.SetWeight(wxFONTWEIGHT_BOLD);
    carryLabel->SetFont(bold);
    top->Add(carryLabel, 0, wxLEFT | wxRIGHT, 14);

    if (WorkflowSteps::isScan(source)) {
      top->Add(new wxStaticText(this, wxID_ANY, "Structure of the scan:"), 0,
               wxLEFT | wxRIGHT, 14);
      p_point = new PointChooser(this, "last");
      top->Add(p_point, 0, wxLEFT | wxTOP | wxBOTTOM, 28);
    }
    p_geometry = new wxCheckBox(this, wxID_ANY,
                                p_point ? "Geometry of that point" : "Final geometry");
    p_charge = new wxCheckBox(this, wxID_ANY, "Charge and multiplicity");
    p_geometry->SetValue(true);
    p_charge->SetValue(true);
    wxCheckBox *orbitals = new wxCheckBox(this, wxID_ANY,
                             "Orbitals as the starting guess");
    wxCheckBox *hessian = new wxCheckBox(this, wxID_ANY, "Hessian (for a TS search)");
    orbitals->Enable(false);
    hessian->Enable(false);
    const wxString notYet = "not yet: needs orbital converters";
    orbitals->SetToolTip(notYet);
    hessian->SetToolTip(notYet);
    for (wxCheckBox *c : {p_geometry, p_charge, orbitals, hessian})
      top->Add(c, 0, wxLEFT | wxTOP, 28);
    const int conformers = ScanPoint::ensembleSize(dynamic_cast<PropertyTask*>(source));
    if (conformers > 0) {
      // A CREST ensemble: one step per conformer, lowest energy first.
      p_best = new wxCheckBox(this, wxID_ANY, "From the best conformers:");
      p_bestN = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition,
                               wxDefaultSize, wxSP_ARROW_KEYS, 1, conformers,
                               std::min(3, conformers));
      p_bestN->Enable(false);
      std::vector<ScanPoint::Pattern> pats;
      std::string patWhy;
      if (ScanPoint::ensemblePatterns(dynamic_cast<PropertyTask*>(source),
                                      pats, patWhy)) {
        p_patterns = new wxCheckBox(this, wxID_ANY,
            wxString::Format("One per coordination pattern (%d patterns, "
                             "lowest energy of each)", (int)pats.size()));
        p_patterns->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &) {
          const bool on = p_patterns->GetValue();
          if (on) p_best->SetValue(false);
          p_best->Enable(!on);
          p_bestN->Enable(!on && p_best->GetValue());
          p_geometry->Enable(!on);
        });
      }
      p_best->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &) {
        p_bestN->Enable(p_best->GetValue());
        p_geometry->Enable(!p_best->GetValue());
      });
      wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
      row->Add(p_best, 0, wxALIGN_CENTER_VERTICAL);
      row->Add(p_bestN, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 8);
      row->Add(new wxStaticText(this, wxID_ANY,
               wxString::Format("of %d (lowest relative energy first)",
                                conformers)),
               0, wxLEFT | wxALIGN_CENTER_VERTICAL, 8);
      top->Add(row, 0, wxLEFT | wxTOP, 28);
      if (p_patterns) top->Add(p_patterns, 0, wxLEFT | wxTOP, 28);
    }
  }

  if (source && !haveResult) {
    wxStaticText *note = new wxStaticText(this, wxID_ANY,
        "The step has no result geometry yet: its input geometry is used.");
    top->Add(note, 0, wxLEFT | wxRIGHT | wxTOP, 14);
  }
  top->Add(new wxStaticText(this, wxID_ANY,
      wxString("Then the ") + code.label +
      " Calculation Editor opens with these filled in."),
      0, wxLEFT | wxRIGHT | wxTOP, 14);

  p_problem = new wxStaticText(this, wxID_ANY, "");
  p_problem->SetForegroundColour(wxColour(0xc9, 0x2a, 0x2a));
  top->Add(p_problem, 0, wxLEFT | wxRIGHT | wxTOP, 14);

  wxStdDialogButtonSizer *buttons = new wxStdDialogButtonSizer();
  p_create = new wxButton(this, wxID_OK, "Create");
  buttons->AddButton(p_create);
  buttons->AddButton(new wxButton(this, wxID_CANCEL, "Cancel"));
  buttons->Realize();
  p_create->SetDefault();
  top->Add(buttons, 0, wxEXPAND | wxALL, 14);

  SetSizerAndFit(top);
  p_name->Bind(wxEVT_TEXT, &WorkflowStepDialog::OnName, this);
  validate();
}


string WorkflowStepDialog::name() const
{
  return string(p_name->GetValue().ToUTF8());
}


WorkflowSteps::Carry WorkflowStepDialog::carry() const
{
  WorkflowSteps::Carry c;
  if (p_geometry) c.geometry = p_geometry->GetValue();
  if (p_charge) c.chargeMult = p_charge->GetValue();
  if (p_point) c.point = p_point->spec();
  if (p_patterns && p_patterns->GetValue()) {
    c.perPattern = true;
    c.geometry = true;
  } else if (p_best && p_best->GetValue()) {
    c.best = p_bestN->GetValue();
    c.geometry = true;
  }
  return c;
}


void WorkflowStepDialog::OnName(wxCommandEvent &)
{
  validate();
}


void WorkflowStepDialog::validate()
{
  string n = name();
  string problem;
  if (!p_type->isValidResourceName(n))
    problem = "A name may contain only letters, digits, '.', '_' and '-'.";
  else {
    vector<Resource*> members = p_session->getMembers();
    for (size_t i = 0; i < members.size(); i++)
      if (members[i]->getName() == n) problem = "A step has this name already.";
  }
  p_problem->SetLabel(wxString::FromUTF8(problem.c_str()));
  p_create->Enable(problem.empty());
  Layout();
}


// ---- the join dialog ----

WorkflowJoinDialog::WorkflowJoinDialog(wxWindow *parent, Session *wf,
                                       const WorkflowSteps::Code &code,
                                       ResourceType *type,
                                       const vector<Resource*> &sources)
  : wxDialog(parent, wxID_ANY, wxString("New ") + code.label + " join"),
    p_session(wf), p_type(type), p_code(code), p_steps(sources)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxFlexGridSizer *grid = new wxFlexGridSizer(2, 8, 12);
  grid->AddGrowableCol(1);
  grid->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
  p_name = new wxTextCtrl(this, wxID_ANY, WorkflowSteps::defaultName(wf, code),
                          wxDefaultPosition, wxSize(320, -1));
  grid->Add(p_name, 1, wxEXPAND);
  top->Add(grid, 0, wxEXPAND | wxALL, 14);

  top->Add(new wxStaticText(this, wxID_ANY, "Sources and their roles:"), 0,
           wxLEFT | wxRIGHT, 14);
  vector<WorkflowSteps::JoinSource> roles = WorkflowSteps::defaultRoles(sources);
  wxFlexGridSizer *rows = new wxFlexGridSizer(3, 8, 12);
  for (size_t i = 0; i < sources.size(); i++) {
    const WorkflowSteps::Code *sc = WorkflowSteps::codeOf(sources[i]);
    rows->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8(
                  ((sc ? sc->label : "") + string("  ") +
                   sources[i]->getName()).c_str())),
              0, wxALIGN_CENTER_VERTICAL);
    wxChoice *role = new wxChoice(this, wxID_ANY);
    role->Append("reactant");
    role->Append("product");
    role->Append("TS guess");
    role->SetSelection(roles[i].role == "reactant" ? 0
                       : roles[i].role == "product" ? 1 : 2);
    role->Bind(wxEVT_CHOICE, [this](wxCommandEvent &) { validate(); });
    rows->Add(role, 0, wxALIGN_CENTER_VERTICAL);
    p_role.push_back(role);
    if (WorkflowSteps::isScan(sources[i])) {
      PointChooser *pc = new PointChooser(this, roles[i].point);
      rows->Add(pc, 0, wxALIGN_CENTER_VERTICAL);
      p_points.push_back(pc);
    } else {
      rows->Add(new wxStaticText(this, wxID_ANY, "final geometry"), 0,
                wxALIGN_CENTER_VERTICAL);
      p_points.push_back(0);
    }
  }
  top->Add(rows, 0, wxALL, 14);

  p_what = new wxStaticText(this, wxID_ANY, "");
  top->Add(p_what, 0, wxLEFT | wxRIGHT, 14);
  top->Add(new wxStaticText(this, wxID_ANY,
      wxString("The structures are given to the ") + code.label +
      " Calculation Editor's TS Search, which matches the atoms."),
      0, wxLEFT | wxRIGHT | wxTOP, 14);
  p_problem = new wxStaticText(this, wxID_ANY, "");
  p_problem->SetForegroundColour(wxColour(0xc9, 0x2a, 0x2a));
  top->Add(p_problem, 0, wxLEFT | wxRIGHT | wxTOP, 14);

  wxStdDialogButtonSizer *buttons = new wxStdDialogButtonSizer();
  p_create = new wxButton(this, wxID_OK, "Create");
  buttons->AddButton(p_create);
  buttons->AddButton(new wxButton(this, wxID_CANCEL, "Cancel"));
  buttons->Realize();
  p_create->SetDefault();
  top->Add(buttons, 0, wxEXPAND | wxALL, 14);
  SetSizerAndFit(top);
  p_name->Bind(wxEVT_TEXT, [this](wxCommandEvent &) { validate(); });
  validate();
}


string WorkflowJoinDialog::name() const
{
  return string(p_name->GetValue().ToUTF8());
}


vector<WorkflowSteps::JoinSource> WorkflowJoinDialog::sources() const
{
  vector<WorkflowSteps::JoinSource> out;
  static const char *const names[3] = {"reactant", "product", "tsguess"};
  for (size_t i = 0; i < p_steps.size(); i++) {
    int sel = p_role[i]->GetSelection();
    WorkflowSteps::JoinSource js = {p_steps[i], names[sel < 0 ? 0 : sel],
                                    p_points[i] ? p_points[i]->spec() : ""};
    out.push_back(js);
  }
  return out;
}


void WorkflowJoinDialog::validate()
{
  string n = name();
  string problem;
  if (!p_type->isValidResourceName(n))
    problem = "A name may contain only letters, digits, '.', '_' and '-'.";
  else {
    vector<Resource*> members = p_session->getMembers();
    for (size_t i = 0; i < members.size(); i++)
      if (members[i]->getName() == n) problem = "A step has this name already.";
  }
  vector<WorkflowSteps::JoinSource> js = sources();
  string roles = WorkflowSteps::checkRoles(js);
  if (problem.empty()) problem = roles;
  bool guess = false;
  for (size_t i = 0; i < js.size(); i++) guess = guess || js[i].role == "tsguess";
  const bool orca = string(p_code.typeName) == "orca_es";
  p_what->SetLabel(orca ? (guess ? "NEB-TS with a TS guess" : "NEB-TS")
                        : (guess ? "QST3 with a TS guess" : "QST2"));
  p_problem->SetLabel(wxString::FromUTF8(problem.c_str()));
  p_create->Enable(problem.empty());
  Layout();
}
