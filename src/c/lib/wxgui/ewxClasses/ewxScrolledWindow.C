#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif


#include "wxgui/ewxScrolledWindow.H"
#include "wxgui/ewxHelpHandler.H"


ewxScrolledWindow::ewxScrolledWindow()
  : wxScrolledWindow(),
    ewxStyledWindow()
{

}


ewxScrolledWindow::ewxScrolledWindow(wxWindow* parent, wxWindowID id,
                         const wxPoint& pos, const wxSize& size, long style,
                         const wxString& name)
  : wxScrolledWindow(),
    ewxStyledWindow()
{
   Create(parent, id, pos, size, style|wxWANTS_CHARS, name);
}


bool ewxScrolledWindow::Create(wxWindow* parent, wxWindowID id,
                               const wxPoint& pos, const wxSize& size,
                               long style, const wxString& name)
{
   if (!wxScrolledWindow::Create(parent, id, pos, size, style, name)) {
      wxFAIL_MSG( wxT("ewxScrolledWindow creation failed") );
      return false;
   }

   PushEventHandler(new ewxHelpHandler(this));
   setStyles(this);

   return true;
}


ewxScrolledWindow::~ewxScrolledWindow()
{
  PopEventHandler(true);
}

