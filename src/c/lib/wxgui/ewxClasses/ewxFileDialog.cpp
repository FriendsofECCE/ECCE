/**
 * @file
 *
 *
 */

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include "wx/list.h"
#include "wx/spinctrl.h"
#include "wx/notebook.h"

#include "wxgui/ewxFileDialog.H"


/**
 * Constructor.
 * Override styles with ecce styles.
 */
ewxFileDialog::ewxFileDialog(wxWindow* parent, const wxString& message,
                             const wxString& defaultDir,
                             const wxString& defaultFile,
                             const wxString& wildcard, long style,
                             const wxPoint& pos )
  : wxFileDialog(parent, message, defaultDir, defaultFile, wildcard,
                 style, pos),
    ewxStyledWindow()
{
  setStyles(this,true);
}


/**
 * Destructor.
 */
ewxFileDialog::~ewxFileDialog()
{
}
