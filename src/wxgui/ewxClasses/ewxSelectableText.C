#include "wxgui/ewxSelectableText.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxNonBoldLabel.H"
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <algorithm>
#include <vector>

#if defined(__WXGTK__)
extern "C" {
  #include <gtk/gtk.h>
}
#endif


ewxSelectableText::ewxSelectableText(wxWindow* parent, const wxString& text,
                                     long align)
  : wxTextCtrl(parent, wxID_ANY, text, wxDefaultPosition, wxDefaultSize,
               wxTE_READONLY | wxBORDER_NONE | align)
{
  //  The colour the panel is drawn in: the nearest window that sets one.
  wxWindow *bg = parent;
  while (bg && !bg->UseBgCol() && bg->GetParent()) bg = bg->GetParent();
  SetBackgroundColour((bg && bg->UseBgCol() ? bg : parent)->GetBackgroundColour());
  SetForegroundColour(parent->GetForegroundColour());
  SetFont(parent->GetFont());
  fit();
}


void ewxSelectableText::SetLabel(const wxString& text)
{
  ChangeValue(text);
  fit();
}


//  As wide as the text plus the field's own inner margin, never wider.
void ewxSelectableText::fit()
{
  int w = 0, h = 0;
  GetTextExtent(GetValue().empty() ? wxString("0") : GetValue(), &w, &h);
  const wxSize best = GetSizeFromTextSize(w);
  SetMinSize(wxSize(best.x, -1));
  SetInitialSize(wxSize(best.x, -1));
}


wxWindow *ewxSelectableValue(wxWindow* parent, const wxString& text, long align)
{
#if defined(__WXGTK__)
  ewxStaticText *label = new ewxStaticText(parent, wxID_ANY, text,
      wxDefaultPosition, wxDefaultSize, align == wxTE_RIGHT ? wxALIGN_RIGHT : 0);
  if (GTK_IS_LABEL(label->GetHandle()))
    gtk_label_set_selectable(GTK_LABEL(label->GetHandle()), TRUE);
  return label;
#else
  return new ewxSelectableText(parent, text, align);
#endif
}


bool ewxIsSelectable(wxWindow *w)
{
  if (dynamic_cast<ewxSelectableText*>(w)) return true;
#if defined(__WXGTK__)
  if (w && GTK_IS_LABEL(w->GetHandle()))
    return gtk_label_get_selectable(GTK_LABEL(w->GetHandle()));
#endif
  return false;
}


void ewxMakeSelectable(wxWindow *label)
{
#if defined(__WXGTK__)
  if (label && GTK_IS_LABEL(label->GetHandle()))
    gtk_label_set_selectable(GTK_LABEL(label->GetHandle()), TRUE);
#else
  (void)label;
#endif
}


#if defined(__WXGTK__)
namespace {

//  The label's own wrap does the breaking; the width must be given as a
//  size request and the matching height as the minimum, or the sizer
//  would still lay the label out one line high.
void wrapToWidth(wxWindow *label, int w)
{
  GtkWidget *g = label->GetHandle();
  gtk_widget_set_size_request(g, w, -1);
  gint minH = 0, natH = 0;
  gtk_widget_get_preferred_height_for_width(g, w, &minH, &natH);
  label->SetMinSize(wxSize(w, natH));
}

//  The body's sizer is the grid itself, or a box of grids and Launch Info blocks.
std::vector<wxFlexGridSizer*> valueGrids(wxWindow *body)
{
  std::vector<wxFlexGridSizer*> grids;
  wxSizer *sizer = body->GetSizer();
  if (!sizer) return grids;
  if (wxFlexGridSizer *fg = dynamic_cast<wxFlexGridSizer*>(sizer)) {
    grids.push_back(fg);
  } else {
    for (wxSizerItem *it : sizer->GetChildren())
      if (it->IsSizer())
        if (wxFlexGridSizer *fg = dynamic_cast<wxFlexGridSizer*>(it->GetSizer()))
          grids.push_back(fg);
  }
  return grids;
}

void fitColumn(wxWindow *body)
{
  wxSizer *sizer = body->GetSizer();
  if (!sizer) return;
  for (wxFlexGridSizer *grid : valueGrids(body)) {
    //  The column as laid out, which includes any common minimum (#657).
    grid->CalcMin();
    const int labelW = grid->GetColWidths().empty() ? 0 : grid->GetColWidths()[0];
    int col = 0;
    const int avail = std::max(60, body->GetClientSize().x - labelW -
                               grid->GetHGap());
    col = 0;
    for (wxSizerItem *it : grid->GetChildren()) {
      if (col == 1 && it->IsWindow() && GTK_IS_LABEL(it->GetWindow()->GetHandle()))
        wrapToWidth(it->GetWindow(), avail);
      col = 1 - col;
    }
  }
  ewxFitLaunchInfo(body);
  sizer->Layout();
  if (wxScrolledWindow *sw = dynamic_cast<wxScrolledWindow*>(body))
    sw->FitInside();
}

void onBodySize(wxSizeEvent& e)
{
  e.Skip();
  fitColumn(static_cast<wxWindow*>(e.GetEventObject()));
}

}  // namespace
#endif


void ewxWrapValueColumn(wxWindow *body)
{
#if defined(__WXGTK__)
  if (!body || !body->GetSizer()) return;
  for (wxFlexGridSizer *grid : valueGrids(body)) {
  int col = 0;
  for (wxSizerItem *it : grid->GetChildren()) {
    if (col == 1 && it->IsWindow() && GTK_IS_LABEL(it->GetWindow()->GetHandle())) {
      GtkLabel *l = GTK_LABEL(it->GetWindow()->GetHandle());
      gtk_label_set_line_wrap(l, TRUE);
      gtk_label_set_line_wrap_mode(l, PANGO_WRAP_WORD_CHAR);
      //  Narrow until the first size event, so the panel's minimum width
      //  does not come from the longest value.
      wrapToWidth(it->GetWindow(), 100);
    }
    col = 1 - col;
  }
  }
  body->Unbind(wxEVT_SIZE, &onBodySize);
  body->Bind(wxEVT_SIZE, &onBodySize);
#else
  (void)body;
#endif
}


namespace {
//  Two value columns side by side and, below both, the rows that need the
//  whole width (a queue state can be a hundred characters).
struct LaunchBlock : public wxBoxSizer {
  wxFlexGridSizer *col[2];
  wxFlexGridSizer *full;
  LaunchBlock() : wxBoxSizer(wxVERTICAL), full(0) { col[0] = col[1] = 0; }
};

void wrapLaunchValue(wxWindow *w, int width)
{
#if defined(__WXGTK__)
  if (!GTK_IS_LABEL(w->GetHandle())) return;
  GtkLabel *l = GTK_LABEL(w->GetHandle());
  gtk_label_set_line_wrap(l, TRUE);
  gtk_label_set_line_wrap_mode(l, PANGO_WRAP_WORD_CHAR);
  wrapToWidth(w, width);
#else
  (void)w; (void)width;
#endif
}

int itemBorders(wxSizerItem *it)
{
  return it->GetBorder() * ((it->GetFlag() & wxLEFT ? 1 : 0) +
                            (it->GetFlag() & wxRIGHT ? 1 : 0));
}

//  What the first column would measure with no minimum forced on it.
int naturalLabelWidth(wxFlexGridSizer *grid)
{
  int w = 0, col = 0;
  for (wxSizerItem *it : grid->GetChildren()) {
    if (col == 0) {
      int x = 0;
      if (it->IsWindow()) x = it->GetWindow()->GetBestSize().x;
      else if (it->IsSizer()) x = it->GetSizer()->CalcMin().x;
      w = std::max(w, x + itemBorders(it));
    }
    col = 1 - col;
  }
  return w;
}

int labelWidthNow(wxFlexGridSizer *grid)
{
  grid->CalcMin();
  const wxArrayInt& cw = grid->GetColWidths();
  return cw.empty() ? 0 : cw[0];
}

//  Widest value text in the grid's second column, unwrapped.
int naturalValueWidth(wxFlexGridSizer *grid)
{
  int w = 0, col = 0;
  for (wxSizerItem *it : grid->GetChildren()) {
    if (col == 1 && it->IsWindow())
      w = std::max(w, it->GetWindow()->GetTextExtent(
                          it->GetWindow()->GetLabel()).x + 8);
    col = 1 - col;
  }
  return w;
}

void wrapValues(wxFlexGridSizer *grid, int w)
{
  int col = 0;
  for (wxSizerItem *it : grid->GetChildren()) {
    if (col == 1 && it->IsWindow()) wrapLaunchValue(it->GetWindow(), w);
    col = 1 - col;
  }
}

//  Each column takes the width its text needs; only when both together are
//  wider than the panel is the wider one squeezed, so the right column sits
//  a normal gap after the left instead of at the panel's right edge.
void fitLaunchBlock(LaunchBlock *lb, wxWindow *body)
{
  const int gap = 20;
  const int total = std::max(120, body->GetClientSize().x -
                             lb->GetPosition().x - 10);
  wxFlexGridSizer *g[2] = {lb->col[0], lb->col[1]};
  int labelW[2] = {0, 0}, nat[2] = {0, 0}, alloc[2] = {0, 0};
  for (int c = 0; c < 2; c++) {
    if (!g[c]) continue;
    labelW[c] = labelWidthNow(g[c]) + g[c]->GetHGap();
    nat[c] = labelW[c] + naturalValueWidth(g[c]);
  }
  const int avail = total - (g[0] && g[1] ? gap : 0);
  if (nat[0] + nat[1] <= avail) {
    alloc[0] = nat[0]; alloc[1] = nat[1];
  } else if (!g[1] || nat[0] <= avail / 2) {
    alloc[0] = nat[0]; alloc[1] = avail - nat[0];
  } else if (!g[0] || nat[1] <= avail / 2) {
    alloc[1] = nat[1]; alloc[0] = avail - nat[1];
  } else {
    alloc[0] = avail / 2; alloc[1] = avail - alloc[0];
  }
  for (int c = 0; c < 2; c++)
    if (g[c]) wrapValues(g[c], std::max(60, alloc[c] - labelW[c]));
  if (lb->full) {
    const int lw = labelWidthNow(lb->full) + lb->full->GetHGap();
    wrapValues(lb->full, std::max(60, std::min(total - lw,
                                  naturalValueWidth(lb->full))));
  }
}

void fitBlocks(wxSizer *sizer, wxWindow *body)
{
  for (wxSizerItem *it : sizer->GetChildren()) {
    if (!it->IsSizer()) continue;
    wxSizer *sub = it->GetSizer();
    if (LaunchBlock *lb = dynamic_cast<LaunchBlock*>(sub))
      fitLaunchBlock(lb, body);
    else
      fitBlocks(sub, body);
  }
}

wxFlexGridSizer *launchRowGrid(wxWindow *parent,
                               const std::vector<LaunchInfoRows::Row>& rows)
{
  wxFlexGridSizer *grid = new wxFlexGridSizer(2, 1, 5);
  for (const LaunchInfoRows::Row& r : rows) {
    grid->Add(new ewxNonBoldLabel(parent, wxID_ANY, wxString::FromUTF8(r.label.c_str()),
                  wxDefaultPosition, wxDefaultSize, 0, "launch:" + r.label),
              0, wxALIGN_LEFT | wxALIGN_TOP);
    ewxStaticText *v = new ewxStaticText(parent, wxID_ANY,
                  wxString::FromUTF8(r.value.c_str()), wxDefaultPosition,
                  wxDefaultSize, 0, "launchvalue:" + r.label);
    ewxMakeSelectable(v);
    //  The property grids put a pixel around every cell; the same here keeps
    //  the value columns of all blocks on one x.
    grid->Add(v, 0, wxALIGN_LEFT | wxALIGN_TOP | wxLEFT, 1);
  }
  return grid;
}
}  // namespace


wxSizer *ewxLaunchInfoBlock(wxWindow *parent, const LaunchInfoRows::Columns& cols)
{
  LaunchBlock *block = new LaunchBlock;
  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
  const std::vector<LaunchInfoRows::Row> *columns[2] = {&cols.left, &cols.right};
  for (int c = 0; c < 2; c++) {
    if (columns[c]->empty()) continue;
    block->col[c] = launchRowGrid(parent, *columns[c]);
    if (c == 1 && block->col[0]) row->AddSpacer(20);
    row->Add(block->col[c], 0, wxALIGN_TOP);
  }
  block->Add(row, 0, wxALIGN_LEFT);
  if (!cols.full.empty()) {
    block->full = launchRowGrid(parent, cols.full);
    block->Add(block->full, 0, wxALIGN_LEFT | wxTOP, 2);
  }
  return block;
}

void ewxFitLaunchInfo(wxWindow *body)
{
  if (body && body->GetSizer()) fitBlocks(body->GetSizer(), body);
}

namespace {
int labelColumnWidth(wxSizer *s)
{
  if (LaunchBlock *lb = dynamic_cast<LaunchBlock*>(s)) {
    int w = 0;
    if (lb->col[0]) w = naturalLabelWidth(lb->col[0]);
    if (lb->full) w = std::max(w, naturalLabelWidth(lb->full));
    return w;
  }
  wxFlexGridSizer *g = dynamic_cast<wxFlexGridSizer*>(s);
  return g ? naturalLabelWidth(g) : 0;
}

void setLabelColumnWidth(wxSizer *s, int w)
{
  wxFlexGridSizer *grids[2] = {0, 0};
  if (LaunchBlock *lb = dynamic_cast<LaunchBlock*>(s)) {
    grids[0] = lb->col[0];
    grids[1] = lb->full;
  } else {
    grids[0] = dynamic_cast<wxFlexGridSizer*>(s);
  }
  for (wxFlexGridSizer *g : grids) {
    if (!g || g->GetChildren().empty()) continue;
    wxSizerItem *first = g->GetChildren().front();
    const wxSize min(std::max(0, w - itemBorders(first)), -1);
    //  A sizer item takes its minimum from its own sizer, not from the item.
    if (first->IsSizer()) first->GetSizer()->SetMinSize(min);
    else first->SetMinSize(min);
  }
}
}  // namespace

void ewxAlignLabelColumns(const std::vector<wxSizer*>& blocks)
{
  int w = 0;
  for (wxSizer *s : blocks) w = std::max(w, labelColumnWidth(s));
  for (wxSizer *s : blocks) setLabelColumnWidth(s, w);
}
