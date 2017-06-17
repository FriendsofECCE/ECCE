#ifndef _EWXSMALLLABEL_H
#define _EWXSMALLLABEL_H

#include "wxgui/ewxStaticText.H"

class ewxSmallLabel : public ewxStaticText
{
    public:
        // constructors
        ewxSmallLabel();
        ewxSmallLabel(wxWindow* parent, wxWindowID id, const wxString& label,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      long style = 0, const wxString& name = "ewxSmallLabel");
        bool Create(wxWindow* parent, wxWindowID id, const wxString& label,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    long style = 0, const wxString& name = "ewxSmallLabel");
        virtual ~ewxSmallLabel();
};

#endif  // _EWXSMALLLABEL_H
