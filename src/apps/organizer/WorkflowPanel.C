#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/dcbuffer.h"
#include "wx/dcgraph.h"
#include "wx/tooltip.h"

#include <algorithm>
#include <functional>
#include "wx/statline.h"
#include "wx/spinctrl.h"
#include <map>
#include <sstream>

#include "util/ResourceUtils.H"

#include "dsm/ResourceType.H"
#include "dsm/ScanPoint.H"
#include "dsm/Session.H"

#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WxResourceTreeItemData.H"

#include "CalcMgr.H"
#include "WorkflowPanel.H"

using std::string;
using std::vector;

namespace {

const int kBoxW = 230, kBoxH = 62, kGapX = 170, kGapY = 44, kMargin = 24;
const int kDrop = 40;      // the vertical segment sits this far right of a box

wxColour codeColour(const WorkflowSteps::Code *c)
{
  return c ? wxColour(c->r, c->g, c->b) : wxColour(0x86, 0x8e, 0x96);
}

wxColour stateColour(ResourceDescriptor::RUNSTATE s)
{
  switch (s) {
    case ResourceDescriptor::STATE_COMPLETED:
    case ResourceDescriptor::STATE_LOADED:      return wxColour(0x2b, 0x8a, 0x3e);
    case ResourceDescriptor::STATE_RUNNING:
    case ResourceDescriptor::STATE_SUBMITTED:   return wxColour(0x1f, 0x6f, 0xb2);
    case ResourceDescriptor::STATE_WAITING:     return wxColour(0xe8, 0xa4, 0x00);
    case ResourceDescriptor::STATE_READY:       return wxColour(0x86, 0x8e, 0x96);
    case ResourceDescriptor::STATE_CREATED:     return wxColour(0xce, 0xd4, 0xda);
    default:                                    return wxColour(0xc9, 0x2a, 0x2a);
  }
}

struct Seg { wxPoint a, b; };

}  // namespace


/// One box of the drawing.
struct WfBox {
  Resource *step;
  Resource *source;
  wxRect rect;
  double row;
};

/// One link: its elbow, arrow and label, all in unscrolled coordinates.
struct WfLink {
  wxPoint p0, p1, p2, p3;    // p0 source edge, p3 target edge
  wxRect label;
  string text;
};


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

    std::map<const Resource*, size_t> index;
    for (size_t i = 0; i < nodes.size(); i++) {
      WfBox b;
      b.step = nodes[i].step; b.source = nodes[i].source; b.row = 0;
      index[b.step] = p_boxes.size();
      p_boxes.push_back(b);
    }
    // Rows by a walk in tree order so siblings stay in creation order.
    double nextRow = 0;
    std::vector<double> row(p_boxes.size(), -1);
    struct R {
      vector<WfBox> &b; vector<double> &row; double &next;
      double go(size_t i) {
        vector<double> kr;
        for (size_t k = 0; k < b.size(); k++)
          if (b[k].source == b[i].step) kr.push_back(go(k));
        if (kr.empty()) return row[i] = next++;
        return row[i] = (kr.front() + kr.back()) / 2;
      }
    } r = {p_boxes, row, nextRow};
    for (size_t i = 0; i < p_boxes.size(); i++)
      if (!p_boxes[i].source) r.go(i);

    wxClientDC dc(this);
    wxFont small = GetFont();
    dc.SetFont(small);
    wxSize th = dc.GetTextExtent("geometry");
    // The gap between columns holds the longest label and the arrow.
    int maxLabel = 0;
    for (size_t i = 0; i < nodes.size(); i++)
      for (size_t k = 0; k < nodes[i].in.size(); k++)
        maxLabel = std::max(maxLabel, dc.GetTextExtent(wxString::FromUTF8(
            WorkflowSteps::linkLabel(nodes[i].in[k].name).c_str())).x);
    const int gapX = std::max(kGapX, maxLabel + kDrop + 50);

    // A join's box is tall enough for an arrow and its label per source.
    std::vector<int> height(nodes.size(), kBoxH);
    int maxH = kBoxH;
    for (size_t i = 0; i < nodes.size(); i++) {
      if (nodes[i].in.size() > 1)
        height[i] = std::max(kBoxH, (int)(nodes[i].in.size() + 1) * (th.y + 10));
      maxH = std::max(maxH, height[i]);
    }
    int maxX = 0, maxY = 0;
    for (size_t i = 0; i < nodes.size(); i++) {
      p_boxes[i].row = row[i];
      p_boxes[i].rect = wxRect(kMargin + nodes[i].depth * (kBoxW + gapX),
                               kMargin + (int)(row[i] * (maxH + kGapY)) +
                                   (maxH - height[i]) / 2,
                               kBoxW, height[i]);
      maxX = std::max(maxX, p_boxes[i].rect.GetRight());
      maxY = std::max(maxY, p_boxes[i].rect.GetBottom());
    }
    for (size_t i = 0; i < p_boxes.size(); i++) {
      const vector<WorkflowSteps::Incoming> &in = nodes[i].in;
      // Top to bottom by where the source is, so the arrows do not cross.
      vector<size_t> order(in.size());
      for (size_t k = 0; k < order.size(); k++) order[k] = k;
      std::stable_sort(order.begin(), order.end(), [&](size_t x, size_t y) {
        return p_boxes[index[in[x].source]].rect.y <
               p_boxes[index[in[y].source]].rect.y;
      });
      const wxRect &t = p_boxes[i].rect;
      for (size_t slot = 0; slot < order.size(); slot++) {
        const WorkflowSteps::Incoming &inc = in[order[slot]];
        const wxRect &s = p_boxes[index[inc.source]].rect;
        WfLink l;
        l.p0 = wxPoint(s.GetRight() + 1, s.y + s.height / 2);
        l.p1 = wxPoint(l.p0.x + kDrop, l.p0.y);
        l.p2 = wxPoint(l.p1.x, order.size() == 1
                                   ? t.y + t.height / 2
                                   : t.y + (int)(slot + 1) * t.height /
                                               (int)(order.size() + 1));
        l.p3 = wxPoint(t.x, l.p2.y);
        l.text = WorkflowSteps::linkLabel(inc.name);
        // Above the segment that enters the target, just before the arrow.
        const int w = dc.GetTextExtent(wxString::FromUTF8(l.text.c_str())).x;
        l.label = wxRect(l.p3.x - 14 - w, l.p3.y - 5 - th.y, w, th.y);
        p_links.push_back(l);
      }
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
    int n = 0;
    vector<wxRect> lines;
    for (const WfLink &l : p_links) {
      wxPoint pts[4] = {l.p0, l.p1, l.p2, l.p3};
      for (int k = 0; k < 3; k++) {
        wxRect r(pts[k], pts[k + 1]);
        r.Inflate(2, 2);
        lines.push_back(r);
      }
      lines.push_back(wxRect(l.p3.x - 10, l.p3.y - 5, 10, 10));   // arrow head
    }
    for (size_t i = 0; i < p_links.size(); i++) {
      wxRect lab = p_links[i].label;
      for (const WfBox &b : p_boxes) if (lab.Intersects(b.rect)) n++;
      for (const wxRect &r : lines) if (lab.Intersects(r)) n++;
      for (size_t k = 0; k < p_links.size(); k++)
        if (k != i && lab.Intersects(p_links[k].label)) n++;
    }
    return n;
  }

  string report() const
  {
    std::ostringstream o;
    o << "boxes=" << p_boxes.size() << " links=" << p_links.size()
      << " label-overlaps=" << overlaps();
    return o.str();
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
      dc.DrawLine(l.p2, wxPoint(l.p3.x - 8, l.p3.y));
      wxPoint head[3] = {l.p3, wxPoint(l.p3.x - 9, l.p3.y - 5),
                         wxPoint(l.p3.x - 9, l.p3.y + 5)};
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
      dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
      dc.SetPen(isSel ? wxPen(wxColour(0x1c, 0x7e, 0xd6), 4) : wxPen(dim, 2));
      dc.DrawRoundedRectangle(r, 8);
      dc.SetPen(*wxTRANSPARENT_PEN);
      dc.SetBrush(wxBrush(codeColour(code)));
      dc.DrawRoundedRectangle(wxRect(r.x + 1, r.y + 1, 8, r.height - 2), 4);

      dc.SetFont(bold);
      dc.SetTextForeground(codeColour(code));
      dc.DrawText(code ? code->label : "?", r.x + 18, r.y + 8);

      ResourceDescriptor::RUNSTATE st = WorkflowSteps::stateOf(b.step);
      string word = ResourceUtils::stateToString(st);
      dc.SetFont(base);
      dc.SetTextForeground(dim);
      wxSize ws = dc.GetTextExtent(word);
      dc.SetBrush(wxBrush(stateColour(st)));
      dc.SetPen(wxPen(dim, 1));
      dc.DrawCircle(r.GetRight() - 17, r.y + 16, 6);
      dc.DrawText(word, r.GetRight() - 30 - ws.x, r.y + 16 - ws.y / 2);

      dc.SetTextForeground(text);
      wxString name = wxControl::Ellipsize(wxString::FromUTF8(b.step->getName().c_str()),
                                           dc, wxELLIPSIZE_END, r.width - 32);
      dc.DrawText(name, r.x + 18, r.y + r.height - 8 - dc.GetTextExtent(name).y);
    }
  }

  WorkflowPanel *p_panel;
  vector<WfBox> p_boxes;
  vector<WfLink> p_links;
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
    p_canvas(0), p_help(0), p_status(0)
{
  createControls();
}


void WorkflowPanel::createControls()
{
  wxBoxSizer *top = new wxBoxSizer(wxHORIZONTAL);
  SetSizer(top);

  wxBoxSizer *left = new wxBoxSizer(wxVERTICAL);
  p_canvas = new WorkflowCanvas(this);
  left->Add(p_canvas, 1, wxEXPAND);

  // The instructions have a panel of their own, never among the controls.
  wxPanel *notes = new wxPanel(this, wxID_ANY, wxDefaultPosition,
                               wxDefaultSize, wxBORDER_SIMPLE);
  wxBoxSizer *noteSizer = new wxBoxSizer(wxVERTICAL);
  p_help = new wxStaticText(notes, wxID_ANY, "");
  p_status = new wxStaticText(notes, wxID_ANY, "");
  noteSizer->Add(p_help, 0, wxALL, 6);
  noteSizer->Add(p_status, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  notes->SetSizer(noteSizer);
  left->Add(notes, 0, wxEXPAND | wxTOP, 4);
  top->Add(left, 1, wxEXPAND);

  wxBoxSizer *bar = new wxBoxSizer(wxVERTICAL);
  wxButton *launch = new wxButton(this, wxID_ANY, "Launch\nReady Steps");
  launch->SetToolTip("Launch every step that has not been launched and whose "
                     "source step is Complete");
  launch->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { launchReadyClicked(); });
  bar->Add(launch, 0, wxEXPAND | wxALL, 4);
  bar->Add(new wxStaticLine(this), 0, wxEXPAND | wxALL, 4);
  const vector<WorkflowSteps::Code> &codes = WorkflowSteps::codes();
  for (size_t i = 0; i < codes.size(); i++) {
    bar->Add(new CodeButton(this, codes[i], [this, i]() { codeClicked(i); }),
             0, wxALL, 2);
  }
  bar->Add(new wxStaticText(this, wxID_ANY, "new step"), 0,
           wxALIGN_CENTER_HORIZONTAL | wxTOP, 4);
  top->Add(bar, 0, wxEXPAND | wxALL, 4);

  p_canvas->relayout();
  updateTexts();
  Layout();
}


void WorkflowPanel::updateTexts()
{
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


string WorkflowPanel::contextMenu(bool popup)
{
  wxMenu menu;
  string error = CalcMgr::guarded([&]() {
    p_calcMgr->getContextMenu(menu);
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

  wxStaticText *carryLabel = new wxStaticText(this, wxID_ANY, "Carry over:");
  wxFont bold = carryLabel->GetFont(); bold.SetWeight(wxFONTWEIGHT_BOLD);
  carryLabel->SetFont(bold);
  top->Add(carryLabel, 0, wxLEFT | wxRIGHT, 14);

  p_point = 0;
  if (source && WorkflowSteps::isScan(source)) {
    top->Add(new wxStaticText(this, wxID_ANY, "Structure of the scan:"), 0,
             wxLEFT | wxRIGHT, 14);
    p_point = new PointChooser(this, "last");
    top->Add(p_point, 0, wxLEFT | wxTOP | wxBOTTOM, 28);
  }
  p_geometry = new wxCheckBox(this, wxID_ANY,
                              p_point ? "Geometry of that point" : "Final geometry");
  p_charge = new wxCheckBox(this, wxID_ANY, "Charge and multiplicity");
  p_geometry->SetValue(source != 0);
  p_charge->SetValue(source != 0);
  p_geometry->Enable(source != 0);
  p_charge->Enable(source != 0);
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
  c.geometry = p_geometry->GetValue();
  c.chargeMult = p_charge->GetValue();
  if (p_point) c.point = p_point->spec();
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
