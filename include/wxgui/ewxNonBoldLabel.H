#ifndef _EWXNONBOLDLABEL_H
#define _EWXNONBOLDLABEL_H

#include "wxgui/ewxStaticText.H"

class ewxNonBoldLabel : public ewxStaticText
{
  public:
    // constructors
    ewxNonBoldLabel();
    ewxNonBoldLabel(wxWindow* parent, wxWindowID id=wxID_ANY,
            const wxString& label="",
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=0, const wxString& name="ewxNonBoldLabel");
    bool Create(wxWindow* parent, wxWindowID id=wxID_ANY,
            const wxString& label="",
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=0, const wxString& name="ewxNonBoldLabel");
    virtual ~ewxNonBoldLabel();
};

#endif

