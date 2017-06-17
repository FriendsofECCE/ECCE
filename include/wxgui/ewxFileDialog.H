/**
 * @file
 *
 *
 */
#ifndef _EWXFILEDIALOG_H  
#define _EWXFILEDIALOG_H  

#include "wx/textdlg.h"
#include "wxgui/ewxStyledWindow.H"

/**
 * Subclass wxFileDialog to provide style changes.
 */
class ewxFileDialog : public wxFileDialog, public ewxStyledWindow
{
  public:
    ewxFileDialog(wxWindow* parent, const wxString& message="Choose a file",
                  const wxString& defaultDir="",
                  const wxString& defaultFile="",
                  const wxString& wildcard="*.*",
                  long style=wxOK|wxCANCEL|wxCENTRE,
                  const wxPoint& pos = wxDefaultPosition);
    virtual ~ewxFileDialog();
};

#endif
