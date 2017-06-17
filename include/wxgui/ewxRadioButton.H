#ifndef _EWXRADIOBUTTON_H
#define _EWXRADIOBUTTON_H

#include "wx/radiobut.h"

#include "wxgui/ewxGenericValidator.H" // for ewxDefaultValidator
#include "wxgui/ewxStyledWindow.H"

class ewxDisabler;

class ewxRadioButton : public wxRadioButton, public ewxStyledWindow
{
  public:
    // constructors
    ewxRadioButton();
    ewxRadioButton( wxWindow *parent, wxWindowID id, const wxString& label,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    long style = 0,
                    const wxValidator& validator = ewxDefaultValidator,
                    const wxString& name = wxRadioButtonNameStr );
    bool Create( wxWindow *parent, wxWindowID id, const wxString& label,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = 0,
                 const wxValidator& validator = ewxDefaultValidator,
                 const wxString& name = wxRadioButtonNameStr );
    virtual ~ewxRadioButton();

    virtual void setCustomDisabledStyle(bool disabled);

  protected:
    ewxDisabler *p_disabler;
};

#endif
