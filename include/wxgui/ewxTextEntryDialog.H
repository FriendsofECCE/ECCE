/**
 * @file
 *
 *
 */
#ifndef _EWXTEXTENTRYDIALOG_H  
#define _EWXTEXTENTRYDIALOG_H  

#include "wx/textdlg.h"
#include "wxgui/ewxStyledWindow.H"

/**
 * Subclass wxTextEntryDialog to provide style changes.
 */
class ewxTextEntryDialog : public wxTextEntryDialog, public ewxStyledWindow
{
  public:
    ewxTextEntryDialog(wxWindow* parent, const wxString& message,
                       const wxString& caption="Please enter text",
                       const wxString& defaultValue="",
                       long style=wxOK|wxCANCEL|wxCENTRE,
                       const wxPoint& pos = wxDefaultPosition);
    
    virtual ~ewxTextEntryDialog();

};

#endif
