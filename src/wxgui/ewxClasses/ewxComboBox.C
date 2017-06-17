#include "wx/wxprec.h"
#include <iostream>
using namespace std;


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxDisabler.H"
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxHelpHandler.H"
#include "wxgui/ewxGenericValidator.H"


ewxComboBox::ewxComboBox()
  : wxComboBox(),
    ewxStyledWindow()
{

}


ewxComboBox::ewxComboBox(wxWindow* parent, wxWindowID id,
                         const wxString& value,
                         const wxPoint& pos, const wxSize& size,
                         int n, const wxString choices[], long style,
                         const wxValidator& validator, const wxString& name)
  : wxComboBox(),
    ewxStyledWindow()
{
  Create(parent, id, value, pos, size, n, choices, style, validator, name);
}


bool ewxComboBox::Create(wxWindow* parent, wxWindowID id,
                         const wxString& value,
                         const wxPoint& pos, const wxSize& size,
                         int n, const wxString choices[], long style,
                         const wxValidator& validator, const wxString& name)
{
  if (!wxComboBox::Create(parent, id, value, pos, size, n, choices, style,
                          validator, name)) {
    wxFAIL_MSG( wxT("ewxComboBox creation failed") );
    return false;
  }

  SetEditable(true);

  PushEventHandler(new ewxHelpHandler(this));
  p_disabler = new ewxDisabler();
  PushEventHandler(p_disabler);

  setStyles(this);

  return true;
}


/**
 *
 */
ewxComboBox::~ewxComboBox()
{
  PopEventHandler(true);
  PopEventHandler(true);
}


/**
 * Set widget to editable or not.  This is NOT
 * a function provided by wxwidgets but we use their method name
 * style since it should be and is less confusing.
 */
void ewxComboBox::SetEditable(bool editable)
{
   p_editable = editable;
   setStyles(this,false);
}


/**
 *
 */
bool ewxComboBox::IsEditable()
{
   return p_editable;
}


/**
 * Set widget to our custom disabled style.
 * @param enabled true to enable, false to disable
 */
void ewxComboBox::setCustomDisabledStyle(bool enabled)
{
  if (p_disabler) p_disabler->setEnabled(enabled);
  SetEditable(enabled);
}


/**
 * Set whether mouse leave events are mapped to enter key events.
 */
void ewxComboBox::setLeaveAsEnter(bool value)
{
  ewxGenericValidator* validator = 0;
  validator = dynamic_cast<ewxGenericValidator*>(GetValidator());
  if (validator) {
    validator->setLeaveAsEnter(value);
  }
}
