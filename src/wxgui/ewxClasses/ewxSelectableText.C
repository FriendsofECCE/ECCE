#include "wxgui/ewxSelectableText.H"
#include "wxgui/ewxStaticText.H"
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <algorithm>

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

void fitColumn(wxWindow *body)
{
  wxSizer *sizer = body->GetSizer();
  if (!sizer) return;
  int labelW = 0, col = 0;
  for (wxSizerItem *it : sizer->GetChildren()) {
    if (col == 0 && it->IsWindow())
      labelW = std::max(labelW, it->GetWindow()->GetBestSize().x);
    col = 1 - col;
  }
  int gap = 5;
  if (wxFlexGridSizer *fg = dynamic_cast<wxFlexGridSizer*>(sizer))
    gap = fg->GetHGap();
  const int avail = std::max(60, body->GetClientSize().x - labelW - gap);
  col = 0;
  for (wxSizerItem *it : sizer->GetChildren()) {
    if (col == 1 && it->IsWindow() && GTK_IS_LABEL(it->GetWindow()->GetHandle()))
      wrapToWidth(it->GetWindow(), avail);
    col = 1 - col;
  }
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
  int col = 0;
  for (wxSizerItem *it : body->GetSizer()->GetChildren()) {
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
  body->Unbind(wxEVT_SIZE, &onBodySize);
  body->Bind(wxEVT_SIZE, &onBodySize);
#else
  (void)body;
#endif
}
