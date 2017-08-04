#include "wx/wxprec.h"

#include <iostream>
  using std::cout;
  using std::endl;


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxNonBoldLabel.H"


ewxNonBoldLabel::ewxNonBoldLabel()
  : ewxStaticText()
{

}


ewxNonBoldLabel::ewxNonBoldLabel(wxWindow* parent, wxWindowID id,
                                 const wxString& label, const wxPoint&pos,
                                 const wxSize& size, long style,
                                 const wxString& name)
  : ewxStaticText()
{
  Create(parent, id, label, pos, size, style, name);
}


bool ewxNonBoldLabel::Create(wxWindow* parent, wxWindowID id,
                             const wxString& label, const wxPoint&pos,
                             const wxSize& size, long style,
                             const wxString& name)
{
  if (!ewxStaticText::Create(parent, id, label, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ewxNonBoldLabel creation failed") );
    return false;
  }

  setStyles(this, false);

  return true;
}


ewxNonBoldLabel::~ewxNonBoldLabel()
{

}

