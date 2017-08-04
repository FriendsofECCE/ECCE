#include "wx/wxprec.h"
#include <iostream>
  using namespace std;


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxDisabler.H"
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxHelpHandler.H"


ewxRadioButton::ewxRadioButton()
  : wxRadioButton(),
    ewxStyledWindow()
{

}


ewxRadioButton::ewxRadioButton( wxWindow *parent, wxWindowID id,
				                        const wxString& label, const wxPoint& pos,
				                        const wxSize& size, long style,
				                        const wxValidator& validator,
				                        const wxString& name)
  : wxRadioButton(),
    ewxStyledWindow()
{
   Create(parent, id, label, pos, size, style|wxWANTS_CHARS, validator, name);
}


bool ewxRadioButton::Create( wxWindow *parent, wxWindowID id,
				                     const wxString& label, const wxPoint& pos,
				                     const wxSize& size, long style,
				                     const wxValidator& validator,
				                     const wxString& name)
{
   if (!wxRadioButton::Create(parent, id, label, pos, size, style,
                              validator, name)) {
      wxFAIL_MSG( wxT("ewxRadioButton creation failed") );
      return false;
   }

   PushEventHandler(new ewxHelpHandler(this));
   p_disabler = new ewxDisabler();
   PushEventHandler(p_disabler);

   setStyles(this);

   return true;
}


ewxRadioButton::~ewxRadioButton()
{
   PopEventHandler(true);
   PopEventHandler(true);
}


/**
 * Set widget to our custom disabled style.
 * @param enabled true to enable, false to disable
 */
void ewxRadioButton::setCustomDisabledStyle(bool enabled)
{
   if (p_disabler) p_disabler->setEnabled(enabled);
}

