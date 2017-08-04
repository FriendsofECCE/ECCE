#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxSmallLabel.H"


ewxSmallLabel::ewxSmallLabel()
  : ewxStaticText()
{

}


ewxSmallLabel::ewxSmallLabel(wxWindow* parent, wxWindowID id,
                             const wxString& label, const wxPoint& pos,
                             const wxSize& size, long style,
                             const wxString& name)
  : ewxStaticText()
{
  Create(parent, id, label, pos, size, style, name);
}


bool ewxSmallLabel::Create(wxWindow* parent, wxWindowID id,
                           const wxString& label, const wxPoint& pos,
                           const wxSize& size, long style,
                           const wxString& name)
{
  if (!ewxStaticText::Create(parent, id, label, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ewxSmallLabel creation failed") );
    return false;
  }

  setStyles(this, false);

  return true;
}


ewxSmallLabel::~ewxSmallLabel()
{

}

