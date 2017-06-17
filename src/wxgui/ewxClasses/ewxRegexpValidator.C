/**
 * @file
 *
 *
 */

#include <iostream>
  using std::cout;
  using std::endl;
#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ewxRegexpValidator.H"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#if wxUSE_VALIDATORS && wxUSE_TEXTCTRL

#ifndef WX_PRECOMP
  #include <stdio.h>
  #include "wx/textctrl.h"
  #include "wx/utils.h"
  #include "wx/msgdlg.h"
  #include "wx/intl.h"
#endif


#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <string>

#ifdef __SALFORDC__
    #include <clib.h>
#endif

#include "util/EcceRegexp.H"
#include "wxgui/ewxRegexpValidator.H"


IMPLEMENT_DYNAMIC_CLASS(ewxRegexpValidator, ewxGenericValidator)


BEGIN_EVENT_TABLE(ewxRegexpValidator, ewxGenericValidator)
    EVT_CHAR(ewxRegexpValidator::onChar)
END_EVENT_TABLE()


/**
 * Constructor taking the validation expression and the default value.
 */
ewxRegexpValidator::ewxRegexpValidator(const wxString& exp, const wxString& val)
  :ewxGenericValidator(val)
{
   p_expression = exp;
}


/**
 * Copy constructor.
 */
ewxRegexpValidator::ewxRegexpValidator(const ewxRegexpValidator& val)
    : ewxGenericValidator(val)
{
    Copy(val);
}


/**
 * Explicit copy.
 */
bool ewxRegexpValidator::Copy(const ewxRegexpValidator& val)
{
    bool ret = true;
    
    ret &= ewxGenericValidator::Copy(val);

    p_expression = val.p_expression;

    return ret;
}


/**
 * Destructor does nothing.
 */
ewxRegexpValidator::~ewxRegexpValidator()
{
}


/**
 * Return the last known good value for this validator.
 */
wxString ewxRegexpValidator::getValue() const
{
   return *m_pString;
}


/**
 * Return the regexp validation expression.
 */
wxString ewxRegexpValidator::getExpression()
{
   return p_expression;
}


/**
 * Set the regexp validation expression.
 */
void ewxRegexpValidator::setExpression(const wxString& exp)
{
   p_expression = exp;
}


/**
 *
 */
bool ewxRegexpValidator::CheckValidator() const
{
   wxCHECK_MSG( m_validatorWindow, FALSE,
         _T("No window associated with validator") );
   /* This validator should be able to be used on any control with a text 
    * field.
   wxCHECK_MSG( m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)), FALSE,
         _T("wxTextValidator is only for wxTextCtrl's") );
   */
   /* empty field is ok 
   wxCHECK_MSG( p_value, FALSE,
         _T("No variable storage for validator") );
   */

   return TRUE;
}





/**
 * Called when the value in the window must be validated.
 */
bool ewxRegexpValidator::Validate(wxWindow *parent)
{

   if( !CheckValidator() )
        return FALSE;

    // If window is disabled, simply return
    if ( !m_validatorWindow->IsEnabled() )
        return TRUE;

    // get the value in the window, return if failed
    wxString val = getControlValue();

    bool ok = TRUE;

    // NB: this format string should contian exactly one '%s'
    wxString errormsg;

    if (val == "") {
         ok = FALSE;
    } else {
       //NOTE:  The expression is constructed each time - could compile
       // it just once when set to improve performance
       EcceRegexp re(p_expression.c_str());
       int foundIdx = 0;
       int substrLength = 0;
       string result;
       result = re.match(val.c_str(), foundIdx, substrLength);
       if (foundIdx == -1) {
         errormsg = _("'%s' is invalid");
         ok = FALSE;
       }
    }

    if ( !ok )
    {
        // This is the default TextValidator behavior - yucky!
        /*
        wxASSERT_MSG( !errormsg.empty(), _T("you forgot to set errormsg") );

        m_validatorWindow->SetFocus();

        wxString buf;
        buf.Printf(errormsg, val.c_str());

        wxMessageBox(buf, _("Validation conflict"),
                     wxOK | wxICON_EXCLAMATION, parent);
        */
        wxBell();
    }

    return ok;
}



/**
 * Event handler for a user key stroke.
 * This keystroke validator lets most keys pass through.
 */
void ewxRegexpValidator::onChar(wxKeyEvent& event)
{
  int keyCode = event.GetKeyCode();

  if (isValidKey(keyCode)) {
    // do something with it?
  } else { 
     if ( !wxValidator::IsSilent() )
        wxBell();

     // eat message
     return;
  }

  event.Skip();
}


/**
 * Returns true for all but obscure control keys.
 */
bool ewxRegexpValidator::isValidKey(int keyCode) const
{
  bool ret = false;

  // if the key is a control key, just let it fall through
  if (keyCode == WXK_BACK || keyCode == WXK_TAB || keyCode == WXK_RETURN ||
      keyCode == WXK_ESCAPE || keyCode == WXK_SPACE || keyCode == WXK_DELETE)
  {
    ret = true;
  } else {
    // the usual alphanum ascii range
    if (keyCode >= WXK_SPACE && keyCode <= WXK_DELETE) {
      ret = true;
    }
  }

  return ret;
}


#endif
  // wxUSE_VALIDATORS && wxUSE_TEXTCTRL
