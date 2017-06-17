#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxDisabler.H"
#include "wxgui/ewxHelpHandler.H"


ewxCheckBox::ewxCheckBox()
  : wxCheckBox(),
    ewxStyledWindow()
{

}


ewxCheckBox::ewxCheckBox(wxWindow* parent, wxWindowID id,
                         const wxString& label, const wxPoint& pos,
                         const wxSize& size, long style,
                         const wxValidator& validator, const wxString& name)
  : wxCheckBox(),
    ewxStyledWindow()
{
  Create(parent, id, label, pos, size, style, validator, name);
}


bool ewxCheckBox::Create(wxWindow* parent, wxWindowID id,
                         const wxString& label, const wxPoint& pos,
                         const wxSize& size, long style,
                         const wxValidator& validator, const wxString& name)
{
  if (!wxCheckBox::Create(parent, id, label, pos, size, style, validator,
                          name)) {
    wxFAIL_MSG( wxT("ewxCheckBox creation failed") );
    return false;
  }

  PushEventHandler(new ewxHelpHandler(this));
  p_disabler = new ewxDisabler();
  PushEventHandler(p_disabler);
 
  p_editable = true;
  setStyles(this);

  return true;
}


ewxCheckBox::~ewxCheckBox()
{
  PopEventHandler(true);
  PopEventHandler(true);
}


/**
 * Set widget to our custom disabled style.
 * @param enabled true to enable, false to disable
 */
void ewxCheckBox::setCustomDisabledStyle(bool enabled)
{
   if (p_disabler) p_disabler->setEnabled(enabled);
   SetEditable(enabled);
}


/**
 * Set widget to editable or not.  This is NOT
 * a function provided by wxwidgets but we use their method name
 * style since it should be and is less confusing.
 */
void ewxCheckBox::SetEditable(bool editable)
{
   p_editable = editable;
   setStyles(this,false);
}

bool ewxCheckBox::IsEditable()
{
   return p_editable;
}
