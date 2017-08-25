#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxStaticLine.H"


ewxStaticLine::ewxStaticLine()
  : wxStaticLine(),
    ewxStyledWindow()
{

}


ewxStaticLine::ewxStaticLine(wxWindow* parent, wxWindowID id,
                             const wxPoint& pos, const wxSize& size,
                             long style, const wxString& name)
  : wxStaticLine(),
    ewxStyledWindow()
{
  Create(parent, id, pos, size, style, name);
}


bool ewxStaticLine::Create(wxWindow* parent, wxWindowID id,
                           const wxPoint& pos, const wxSize& size,
                           long style, const wxString& name)
{
  if (!wxStaticLine::Create(parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ewxStaticLine creation failed") );
    return false;
  }

  setStyles(this, false);

  return true;
}


ewxStaticLine::~ewxStaticLine()
{

}

