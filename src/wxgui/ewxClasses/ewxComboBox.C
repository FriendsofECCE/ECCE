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
    ewxStyledWindow(),
    p_disabler(NULL),
    p_explicitWidth(-1)
{

}


ewxComboBox::ewxComboBox(wxWindow* parent, wxWindowID id,
                         const wxString& value,
                         const wxPoint& pos, const wxSize& size,
                         int n, const wxString choices[], long style,
                         const wxValidator& validator, const wxString& name)
  : wxComboBox(),
    ewxStyledWindow(),
    p_disabler(NULL),
    p_explicitWidth(-1)
{
  Create(parent, id, value, pos, size, n, choices, style, validator, name);
}


bool ewxComboBox::Create(wxWindow* parent, wxWindowID id,
                         const wxString& value,
                         const wxPoint& pos, const wxSize& size,
                         int n, const wxString choices[], long style,
                         const wxValidator& validator, const wxString& name)
{
  p_explicitWidth = size.x;
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
  fitDropDown(this, p_explicitWidth, value);

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


int ewxComboBox::DoInsertItems(const wxArrayStringsAdapter& items,
                               unsigned int pos, void **clientData,
                               wxClientDataType type)
{
  int ret = wxComboBox::DoInsertItems(items, pos, clientData, type);
  // Not while the base class is still being created, which inserts the
  // initial entries before the control is ready to be measured.
  if (p_disabler) fitDropDown(this, p_explicitWidth, GetValue());
  return ret;
}
