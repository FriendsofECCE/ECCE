#ifndef _EWXSTATICLINE_H
#define _EWXSTATICLINE_H

#include "wx/statline.h"
#include "wxgui/ewxStyledWindow.H"

class ewxStaticLine : public wxStaticLine, public ewxStyledWindow
{
  public:
    // constructors
    ewxStaticLine();
    ewxStaticLine(wxWindow* parent, wxWindowID id, 
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = wxLI_HORIZONTAL, 
                  const wxString& name = "ewxStaticLine");
    bool Create(wxWindow* parent, wxWindowID id, 
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxLI_HORIZONTAL, 
                const wxString& name = "ewxStaticLine");
    virtual ~ewxStaticLine();
};

#endif
