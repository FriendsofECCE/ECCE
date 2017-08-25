#include "wx/wxprec.h"
#include <iostream>
using namespace std;


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxDisabler.H"
#include "wxgui/ewxBitmapButton.H"
#include "wxgui/ewxHelpHandler.H"


ewxBitmapButton::ewxBitmapButton()
  : wxBitmapButton(),
    ewxStyledWindow()
{
}


ewxBitmapButton::ewxBitmapButton(wxWindow* parent, wxWindowID id,
        const wxBitmap& bitmap, const wxPoint& pos, const wxSize& size,
        long style, const wxValidator& validator, const wxString& name)
  : wxBitmapButton(),
    ewxStyledWindow()
{
  Create(parent, id, bitmap, pos, size, style, validator, name);
}


bool ewxBitmapButton::Create(wxWindow* parent, wxWindowID id,
        const wxBitmap& bitmap, const wxPoint& pos, const wxSize& size,
        long style, const wxValidator& validator, const wxString& name)
{
  if (!wxBitmapButton::Create(parent, id, bitmap, pos, size, style,
                              validator, name)) {
    wxFAIL_MSG( wxT("ewxBitmapButton creation failed") );
    return false;
  }

  PushEventHandler(new ewxHelpHandler(this));
  p_disabler = new ewxDisabler();
  PushEventHandler(p_disabler);
  p_editable = true;

  setStyles(this);

  return true;
}


ewxBitmapButton::~ewxBitmapButton()
{
  PopEventHandler(true);
  PopEventHandler(true);
}


/**
 * Set widget to our custom disabled style.
 * @param enabled true to enable, false to disable
 */
void ewxBitmapButton::setCustomDisabledStyle(bool enabled)
{
   if (p_disabler) p_disabler->setEnabled(enabled);
   SetEditable(enabled);
}


/**
 * Set widget to editable or not.  This is NOT
 * a function provided by wxwidgets but we use their method name
 * style since it should be and is less confusing.
 * No style currently implemented.  We have debated using a gray font
 * or disabling the button but decided against it.  So currently
 * this method mearly sets a flag.
 */
void ewxBitmapButton::SetEditable(bool editable)
{
   p_editable = editable;
   setStyles(this,false);
}

bool ewxBitmapButton::IsEditable()
{
   return p_editable;
}
