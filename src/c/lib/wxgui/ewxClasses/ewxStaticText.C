#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxStaticText.H"


IMPLEMENT_DYNAMIC_CLASS(ewxStaticText, wxStaticText)

ewxStaticText::ewxStaticText()
  : wxStaticText(),
    ewxStyledWindow()
{

}


ewxStaticText::ewxStaticText(wxWindow* parent, wxWindowID id,
                             const wxString& label, const wxPoint& pos,
                             const wxSize& size, long style,
                             const wxString& name)
  : wxStaticText(),
    ewxStyledWindow()
{
  Create(parent, id, label, pos, size, style, name);
}


bool ewxStaticText::Create(wxWindow* parent, wxWindowID id,
                           const wxString& label, const wxPoint& pos,
                           const wxSize& size, long style,
                           const wxString& name)
{
  if (!wxStaticText::Create(parent, id, label, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ewxStaticText creation failed") );
    return false;
  }

  setStyles(this, false);

  return true;
}


ewxStaticText::~ewxStaticText()
{

}

