#ifndef _EWXSLIDER_H
#define _EWXSLIDER_H

#include "wx/slider.h"
#include "wxgui/ewxGenericValidator.H" // for ewxDefaultValidator
#include "wxgui/ewxStyledWindow.H"

class ewxSlider : public wxSlider, public ewxStyledWindow
{
    public:
    // constructors
    ewxSlider();
    ewxSlider(wxWindow* parent, wxWindowID id,
              int value, int minValue, int maxValue,
              const wxPoint& postn = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxSL_HORIZONTAL,
              const wxValidator& validator = ewxDefaultValidator,
              const wxString& name = "ewxSlider");
    bool Create(wxWindow* parent, wxWindowID id,
                int value, int minValue, int maxValue,
                const wxPoint& postn = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxSL_HORIZONTAL,
                const wxValidator& validator = ewxDefaultValidator,
                const wxString& name = "ewxSlider");
    virtual ~ewxSlider();
};

#endif
