#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxSlider.H"
#include "wxgui/ewxHelpHandler.H"


ewxSlider::ewxSlider()
  : wxSlider(),
    ewxStyledWindow()
{

}


ewxSlider::ewxSlider(wxWindow* parent, wxWindowID id,
                    int value, int minValue, int maxValue,
                    const wxPoint& postn, const wxSize& size,
                    long style, const wxValidator& validator,
                    const wxString& name)
  : wxSlider(),
    ewxStyledWindow()
{
   Create(parent, id, value, minValue, maxValue, postn, size,
          style|wxWANTS_CHARS, validator, name);
}


bool ewxSlider::Create(wxWindow* parent, wxWindowID id,
                       int value, int minValue, int maxValue,
                       const wxPoint& postn, const wxSize& size,
                       long style, const wxValidator& validator,
                       const wxString& name)
{
   if (!wxSlider::Create(parent, id, value, minValue, maxValue, postn, size,
                         style, validator, name)) {
      wxFAIL_MSG( wxT("ewxSlider creation failed") );
      return false;
   }

   PushEventHandler(new ewxHelpHandler(this));
   setStyles(this);

   return true;
}


ewxSlider::~ewxSlider()
{
  PopEventHandler(true);
}

