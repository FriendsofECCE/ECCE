#include "wxgui/ewxListView.H"


IMPLEMENT_DYNAMIC_CLASS(ewxListView, wxListView)


ewxListView::ewxListView()
  : wxListView()
  , ewxStyledWindow()
{
}


ewxListView::ewxListView(wxWindow* parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size,
        long style, const wxValidator& validator, const wxString& name)
  : wxListView()
  , ewxStyledWindow()
{
  Create(parent, id, pos, size, style, validator, name);
}


ewxListView::~ewxListView()
{
}


bool ewxListView::Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size,
        long style, const wxValidator& validator, const wxString& name)
{
  if (!wxListView::Create(parent, id, pos, size, style, validator, name)) {
    wxFAIL_MSG( wxT("ewxListView creation failed") );
    return false;
  }

  setStyles(this, false);

  return true;
}

