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
    int labelW = 0, col = 0;
    for (wxSizerItem *it : grid->GetChildren()) {
      if (col == 0 && it->IsWindow())
        labelW = std::max(labelW, it->GetWindow()->GetBestSize().x);
      col = 1 - col;
    }
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
struct LaunchBlock : public wxBoxSizer {
  LaunchBlock() : wxBoxSizer(wxHORIZONTAL) {}
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

//  Value width of one column: the share of `avail` left after its labels.
void fitColumnGrid(wxFlexGridSizer *grid, int avail)
{
  int labelW = 0, col = 0;
  for (wxSizerItem *it : grid->GetChildren()) {
    if (col == 0 && it->IsWindow())
      labelW = std::max(labelW, it->GetWindow()->GetBestSize().x);
    col = 1 - col;
  }
  const int w = std::max(60, avail - labelW - grid->GetHGap());
  col = 0;
  for (wxSizerItem *it : grid->GetChildren()) {
    if (col == 1 && it->IsWindow()) wrapLaunchValue(it->GetWindow(), w);
    col = 1 - col;
  }
}

void fitBlocks(wxSizer *sizer, wxWindow *body)
{
  for (wxSizerItem *it : sizer->GetChildren()) {
    if (!it->IsSizer()) continue;
    wxSizer *sub = it->GetSizer();
    if (dynamic_cast<LaunchBlock*>(sub)) {
      const int total = std::max(120, body->GetClientSize().x -
                                 sub->GetPosition().x - 10);
      const int each = (total - 20) / 2;
      for (wxSizerItem *g : sub->GetChildren())
        if (wxFlexGridSizer *fg = dynamic_cast<wxFlexGridSizer*>(g->GetSizer()))
          fitColumnGrid(fg, each);
    } else {
      fitBlocks(sub, body);
    }
  }
}
}  // namespace


wxSizer *ewxLaunchInfoBlock(wxWindow *parent, const LaunchInfoRows::Columns& cols)
{
  wxBoxSizer *block = new LaunchBlock;
  const std::vector<LaunchInfoRows::Row> *columns[2] = {&cols.left, &cols.right};
  for (int c = 0; c < 2; c++) {
    if (columns[c]->empty()) continue;
    wxFlexGridSizer *grid = new wxFlexGridSizer(2, 1, 5);
    for (const LaunchInfoRows::Row& r : *columns[c]) {
      grid->Add(new ewxNonBoldLabel(parent, wxID_ANY, wxString::FromUTF8(r.label.c_str()),
                    wxDefaultPosition, wxDefaultSize, 0, "launch:" + r.label),
                0, wxALIGN_LEFT | wxALIGN_TOP);
      ewxStaticText *v = new ewxStaticText(parent, wxID_ANY,
                    wxString::FromUTF8(r.value.c_str()), wxDefaultPosition,
                    wxDefaultSize, 0, "launchvalue:" + r.label);
      ewxMakeSelectable(v);
      grid->Add(v, 0, wxALIGN_LEFT | wxALIGN_TOP);
    }
    if (c == 1 && !columns[0]->empty()) block->AddSpacer(20);
    block->Add(grid, 0, wxALIGN_TOP);
  }
  return block;
}

void ewxFitLaunchInfo(wxWindow *body)
{
  if (body && body->GetSizer()) fitBlocks(body->GetSizer(), body);
}
