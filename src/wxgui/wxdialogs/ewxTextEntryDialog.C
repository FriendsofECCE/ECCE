/**
 * @file
 *
 *
 */

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxTextEntryDialog.H"


/**
 * Constructor.
 * Override styles with ecce styles.
 */
ewxTextEntryDialog::ewxTextEntryDialog(wxWindow* parent,
                                       const wxString& message,
                                       const wxString& caption,
                                       const wxString& defaultValue,
                                       long style, const wxPoint& pos )
  : wxTextEntryDialog(parent, message, caption, defaultValue, style, pos),
    ewxStyledWindow()
{
  setStyles(this, true);
}


/**
 * Destructor.
 */
ewxTextEntryDialog::~ewxTextEntryDialog()
{
}
