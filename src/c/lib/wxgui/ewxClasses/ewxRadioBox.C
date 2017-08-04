#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxRadioBox.H"
#include "wxgui/ewxHelpHandler.H"


ewxRadioBox::ewxRadioBox()
  : wxRadioBox(),
    ewxStyledWindow()
{

}


ewxRadioBox::ewxRadioBox(wxWindow* parent, wxWindowID id,
                         const wxString& label,
                         const wxPoint& pos, const wxSize& size, int n,
                         const wxString choices[], int majorDimension,
                         long style, const wxValidator& validator,
                         const wxString& name)
  : wxRadioBox(),
    ewxStyledWindow()
{
  Create(parent, id, label, pos, size, n, choices, majorDimension,
         style|wxWANTS_CHARS, validator, name);
}


bool ewxRadioBox::Create(wxWindow* parent, wxWindowID id,
                         const wxString& label,
                         const wxPoint& pos, const wxSize& size, int n,
                         const wxString choices[], int majorDimension,
                         long style, const wxValidator& validator,
                         const wxString& name)
{
  if (!wxRadioBox::Create(parent, id, label, pos, size, n, choices,
                          majorDimension, style, validator, name)) {
    wxFAIL_MSG( wxT("ewxRadioBox creation failed") );
    return false;
  }

  PushEventHandler(new ewxHelpHandler(this));
  setStyles(this);

  return true;
}


ewxRadioBox::~ewxRadioBox()
{
  PopEventHandler(true);
}

