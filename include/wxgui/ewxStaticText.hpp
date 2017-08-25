#ifndef _EWXSTATICTEXT_H
#define _EWXSTATICTEXT_H

#include <wx/stattext.h>

#include "wxgui/ewxStyledWindow.H"

class ewxStaticText : public wxStaticText, public ewxStyledWindow
{
  DECLARE_DYNAMIC_CLASS(ewxStaticText)

  public:
    // constructors
    ewxStaticText();
    ewxStaticText(wxWindow* parent, wxWindowID id=wxID_ANY,
            const wxString& label="",
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=0, const wxString& name="ewxStaticText");
    bool Create(wxWindow* parent, wxWindowID id=wxID_ANY,
            const wxString& label="",
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=0, const wxString& name="ewxStaticText");
    virtual ~ewxStaticText();
};

#endif
