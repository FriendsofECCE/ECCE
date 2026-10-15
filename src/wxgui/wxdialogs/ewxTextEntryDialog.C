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


// Names become folder names on the data server and in job scripts, where
// a space breaks paths; turning it into '_' as typed beats rejecting it.
void ewxTextEntryDialog::spacesToUnderscores()
{
  if (!m_textctrl) return;
  m_textctrl->Bind(wxEVT_TEXT, [this](wxCommandEvent& ev) {
    wxString v = m_textctrl->GetValue();
    if (v.Find(' ') != wxNOT_FOUND) {
      long at = m_textctrl->GetInsertionPoint();
      v.Replace(" ", "_");
      m_textctrl->ChangeValue(v);
      m_textctrl->SetInsertionPoint(at);
    }
    ev.Skip();
  });
}


/**
 * Destructor.
 */
ewxTextEntryDialog::~ewxTextEntryDialog()
{
}
