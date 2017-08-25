#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxPanel.H"


IMPLEMENT_DYNAMIC_CLASS(ewxPanel, wxPanel)


ewxPanel::ewxPanel()
{

}


ewxPanel::ewxPanel(wxWindow* parent,
                   wxWindowID id,
                   const wxPoint& pos,
                   const wxSize& size,
                   long style,
                   const wxString& name)
  : wxPanel(),
    ewxStyledWindow()
{
  Create(parent, id, pos, size, style, name);
}


ewxPanel::~ewxPanel()
{

}


bool ewxPanel::Create(wxWindow* parent,
                      wxWindowID id,
                      const wxPoint& pos,
                      const wxSize& size,
                      long style,
                      const wxString& name)
{
  if (!wxPanel::Create(parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ewxPanel creation failed") );
    return false;
  }

  setStyles(this, false);

  return true;
}

