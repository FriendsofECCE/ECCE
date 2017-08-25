/**
 * @file 
 *
 *
 */
#ifndef EWXREGEXPVALIDATOR_H
#define EWXREGEXPVALIDATOR_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ewxRegexpValidator.H"
#endif

#include "wx/defs.h"

#if wxUSE_VALIDATORS

#include "wx/textctrl.h"
#include "wx/validate.h"

#include "wxgui/ewxGenericValidator.H"

/**
 * ewxRegexpValidator the validates against regular expression.
 * This code copied from wxTextValidator and then modified.  The
 * wxWidget coding style is mostly maintained.
 *
 * Example
 *  For atom residue strings such as 9:_C
 *  text->SetValidator( ewxRegexpValidator("[0-9]+:_[a-z,A-Z]", ""));
 *
 * This validator does not currently validate as you type but only after
 * loseFocus or <return> assuming you have a ewxTextCtrl.
 */
class ewxRegexpValidator: public ewxGenericValidator
{
DECLARE_DYNAMIC_CLASS(ewxRegexpValidator)
public:

    ewxRegexpValidator(const wxString& exp=".+", const wxString& val = "");
    ewxRegexpValidator(const ewxRegexpValidator& val);

    ~ewxRegexpValidator();

    // Make a clone of this validator (or return NULL) - currently necessary
    // if you're passing a reference to a validator.
    // Another possibility is to always pass a pointer to a new validator
    // (so the calling code can use a copy constructor of the relevant class).
    virtual wxObject *Clone() const { return new ewxRegexpValidator(*this); }
    bool Copy(const ewxRegexpValidator& val);

    virtual wxString getExpression();
    virtual void setExpression(const wxString& exp);

    virtual wxString getValue() const;

    // Called when the value in the window must be validated.
    // This function can pop up an error message.
    virtual bool Validate(wxWindow *parent);

DECLARE_EVENT_TABLE()
protected:
    // Filter keystrokes
    void onChar(wxKeyEvent& event);

    bool isValidKey(int keyCode) const;

    bool CheckValidator() const;

    wxString    p_value;
    wxString    p_expression;

private:
// Cannot use
//  DECLARE_NO_COPY_CLASS(wxTextValidator)
// because copy constructor is explicitly declared above;
// but no copy assignment operator is defined, so declare
// it private to prevent the compiler from defining it:
    ewxRegexpValidator& operator=(const ewxRegexpValidator&);
};

#endif
  // wxUSE_VALIDATORS

#endif
  // EWXREGEXPVALIDATOR_H
