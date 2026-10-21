#include "wxgui/ewxSelectableText.H"
#include "wxgui/ewxStaticText.H"

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
