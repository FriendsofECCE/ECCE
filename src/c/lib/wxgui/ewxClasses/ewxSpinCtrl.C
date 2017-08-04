#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxDisabler.H"
#include "wxgui/ewxGenericValidator.H" // for ewxDefaultValidator
#include "wxgui/ewxHelpHandler.H"
#include "wxgui/ewxSpinCtrl.H"


/**
 * Default ctor.
 */
ewxSpinCtrl::ewxSpinCtrl()
  : wxSpinCtrl(),
    ewxStyledWindow()
{
  Init();
}


/**
 * Ctor.
 */
ewxSpinCtrl::ewxSpinCtrl(wxWindow* parent, wxWindowID id, const wxString& value,
                         const wxPoint& pos, const wxSize& size, long style,
                         int min, int max, int initial, const wxString& name)
  : wxSpinCtrl(),
    ewxStyledWindow()
{
  Init();
  Create(parent, id, value, pos, size, style, min, max, initial, name);
}


/**
 * Two-part ctor.
 */
bool ewxSpinCtrl::Create(wxWindow* parent, wxWindowID id,
                         const wxString& value, const wxPoint& pos,
                         const wxSize& size, long style,
                         int min, int max, int initial, const wxString& name)
{
   if (!wxSpinCtrl::Create(parent, id, value, pos, size, style|wxWANTS_CHARS,
                           min, max, initial, name)) {
     wxFAIL_MSG( wxT("ewxSpinCtrl creation failed") );
     return false;
   }

   PushEventHandler(new ewxHelpHandler(this));

   p_disabler = new ewxDisabler();
   PushEventHandler(p_disabler);

   SetValidator(ewxDefaultValidator);

   setStyles(this, true);

   return true;
}


void ewxSpinCtrl::Init()
{

}


ewxSpinCtrl::~ewxSpinCtrl()
{
  PopEventHandler(true);
  PopEventHandler(true);
}


void ewxSpinCtrl::setCustomDisabledStyle(bool enabled)
{
    if (p_disabler)
        p_disabler->setEnabled(enabled);

    Enable(enabled);
    setStyles(this, false);
}

