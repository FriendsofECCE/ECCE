#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/notebook.h"
#include "wx/statline.h"

#include "wxgui/GromacsProfile.H"


void GromacsProfile::show(wxWindow *w, bool show)
{
  if (w != 0) w->Show(show);
}

void GromacsProfile::showRow(wxWindow *w, bool show)
{
  if (w == 0) return;
  wxSizer *s = w->GetContainingSizer();
  if (s != 0) {
    s->ShowItems(show);
  } else {
    w->Show(show);
  }
}

void GromacsProfile::showAllBut(wxWindow *root, wxWindow *keep, bool show)
{
  if (root == 0) return;
  const wxWindowList& kids = root->GetChildren();
  for (wxWindowList::const_iterator it = kids.begin(); it != kids.end(); ++it) {
    if (*it != keep) (*it)->Show(show);
  }
}

void GromacsProfile::label(wxWindow *w, const wxString& text)
{
  if (w != 0) w->SetLabel(text);
}

void GromacsProfile::relabel(wxWindow *root, const wxString& from,
                             const wxString& to)
{
  if (root == 0) return;
  const wxWindowList& kids = root->GetChildren();
  for (wxWindowList::const_iterator it = kids.begin(); it != kids.end(); ++it) {
    if ((*it)->GetLabel() == from && !(*it)->IsKindOf(CLASSINFO(wxPanel))) {
      (*it)->SetLabel(to);
      return;
    }
    relabel(*it, from, to);
  }
}

void GromacsProfile::showByLabel(wxWindow *root, const wxString& text,
                                 bool show)
{
  if (root == 0) return;
  const wxWindowList& kids = root->GetChildren();
  for (wxWindowList::const_iterator it = kids.begin(); it != kids.end(); ++it) {
    wxStaticText *st = dynamic_cast<wxStaticText*>(*it);
    if (st != 0 && st->GetLabel() == text) {
      st->Show(show);
      return;
    }
    if (st == 0 && (*it)->GetChildren().GetCount() > 0) {
      showByLabel(*it, text, show);
    }
  }
}

void GromacsProfile::showTab(wxNotebook *nb, wxWindow *page,
                             const wxString& title, int index, bool show)
{
  if (nb == 0 || page == 0) return;
  int at = nb->FindPage(page);
  if (show && at == wxNOT_FOUND) {
    int n = (int)nb->GetPageCount();
    nb->InsertPage(index > n ? n : index, page, title);
    page->Show();
  } else if (!show && at != wxNOT_FOUND) {
    nb->RemovePage(at);
    page->Hide();
  }
}

void GromacsProfile::relayout(wxWindow *w)
{
  for (; w != 0; w = w->GetParent()) {
    w->Layout();
    if (w->IsTopLevel()) break;
  }
}
