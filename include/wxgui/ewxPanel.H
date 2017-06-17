#ifndef _EWXPANEL_H
#define _EWXPANEL_H

#include "wx/panel.h"

#include "wxgui/ewxStyledWindow.H"

class ewxPanel : public wxPanel, public ewxStyledWindow
{
  DECLARE_DYNAMIC_CLASS(ewxPanel)

  public:
    // constructors
    ewxPanel();
  
    ewxPanel(wxWindow* parent, wxWindowID id = -1,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxTAB_TRAVERSAL,
             const wxString& name = "ewxPanel");

    virtual ~ewxPanel();
  
    virtual bool Create(wxWindow* parent, wxWindowID id = -1,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = wxTAB_TRAVERSAL,
                        const wxString& name = "ewxPanel");
};

#endif
