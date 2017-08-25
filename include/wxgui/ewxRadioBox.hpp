#ifndef _EWXRADIOBOX_H
#define _EWXRADIOBOX_H

#include "wx/radiobox.h"

#include "wxgui/ewxGenericValidator.H" // for ewxDefaultValidator
#include "wxgui/ewxStyledWindow.H"

class ewxRadioBox : public wxRadioBox, public ewxStyledWindow
{
  public:
    // constructors
    ewxRadioBox();
    ewxRadioBox(wxWindow* parent, wxWindowID id, const wxString& label,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize, int n = 0,
                const wxString choices[] = NULL, int majorDimension = 0,
                long style = wxRA_SPECIFY_COLS,
                const wxValidator& validator = ewxDefaultValidator,
                const wxString& name = "ewxRadioBox");
    bool Create(wxWindow* parent, wxWindowID id, const wxString& label,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize, int n = 0,
                const wxString choices[] = NULL, int majorDimension = 0,
                long style = wxRA_SPECIFY_COLS,
                const wxValidator& validator = ewxDefaultValidator,
                const wxString& name = "ewxRadioBox");

    virtual ~ewxRadioBox();
};

#endif
