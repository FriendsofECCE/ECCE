/**
 * @file
 *
 *
 */
#ifndef _EWXSINGLECHOICEDIALOG_H
#define _EWXSINGLECHOICEDIALOG_H

#include "wx/choicdlg.h"
#include "wxgui/ewxStyledWindow.H"

/**
 * Subclass wxSingleChoiceDialog to provide style changes.
 */
class ewxSingleChoiceDialog : public wxSingleChoiceDialog, 
                              public ewxStyledWindow
{
public:
  ewxSingleChoiceDialog(wxWindow* parent, 
                        const wxString& message, 
                        const wxString& caption,
                        const wxArrayString& choices,
                        void **clientData = 0,
		                    long style = wxCHOICEDLG_STYLE, 
                        const wxPoint& pos = wxDefaultPosition);
};

#endif
