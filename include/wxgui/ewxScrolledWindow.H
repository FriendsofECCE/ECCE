#ifndef _EWXSCROLLEDWINDOW_H
#define _EWXSCROLLEDWINDOW_H

#include "wxgui/ewxStyledWindow.H"
#include "wx/wx.h"

/**
 */
class ewxScrolledWindow : public wxScrolledWindow, public ewxStyledWindow
{
   public:
      ewxScrolledWindow();
      ewxScrolledWindow( wxWindow* parent, wxWindowID id=-1, 
                         const wxPoint& pos = wxDefaultPosition,
                         const wxSize& size = wxDefaultSize, 
                         long style = wxHSCROLL|wxVSCROLL,
                         const wxString& name = "ewxScrolledWindow" );
      bool Create( wxWindow* parent, wxWindowID id=-1, 
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize, 
                   long style = wxHSCROLL|wxVSCROLL,
                   const wxString& name = "ewxScrolledWindow" );
      virtual ~ewxScrolledWindow();
};

#endif
