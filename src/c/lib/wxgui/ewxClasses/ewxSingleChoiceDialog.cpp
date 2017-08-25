/**
 * @file
 *
 *
 */

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxStyledWindow.H"
#include "wxgui/ewxSingleChoiceDialog.H"


/**
 * Constructor.
 * Override styles with ecce styles.
 * Only this one constructor is implmented.
 * I had trouble with the clientdata arg so set it to 0 because I don't think
 * we'll use it anyway.
 */
ewxSingleChoiceDialog::ewxSingleChoiceDialog( wxWindow* parent,
        const wxString& message, const wxString& caption,
        const wxArrayString& choices, void **clientData, long style,
        const wxPoint& pos)
  : wxSingleChoiceDialog(parent, message, caption, choices, /*clientData*/0,
                         style, pos),
    ewxStyledWindow()
{
  Centre();
  setStyles(this, true);
}

