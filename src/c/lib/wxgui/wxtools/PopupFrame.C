#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "PopupFrame.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif


#include "util/EventDispatcher.H"
#include "wxgui/WindowEvent.H"
  using namespace ecce;


#include "wxgui/PopupFrame.H"

IMPLEMENT_CLASS( PopupFrame, wxFrame )

BEGIN_EVENT_TABLE( PopupFrame, wxFrame )

    EVT_CLOSE( PopupFrame::mainWindowCloseCB )

END_EVENT_TABLE()


PopupFrame::PopupFrame( )
  : Listener("PopupFrame")
{
   p_notifyClose = true;
   subscribe();
}

PopupFrame::PopupFrame( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
  : Listener(caption.c_str())
{
   p_notifyClose = true;
   subscribe();
   Create( parent, id, caption, pos, size, style );
}

bool PopupFrame::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{

    ewxFrame::Create( parent, id, caption, pos, size, style);

    CreateControls();
    Centre();
    return true;
}

PopupFrame::~PopupFrame()
{
  unsubscribe();
}

void PopupFrame::CreateControls()
{    
    PopupFrame* itemFrame1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(itemBoxSizer2);

}

void PopupFrame::setNotifyOnClose(bool flag)
{
   p_notifyClose = flag;
}

void PopupFrame::eventMCB(const Event& event)
{
  if (event.getName() == "PopupClose") {
    if (event.getSenderId() == getId()) {
      Destroy();
    }
  }
}

void PopupFrame::mainWindowCloseCB( wxCloseEvent& event )
{
   Destroy();
}

/**
 * Should we show tooltips?
 */
bool PopupFrame::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap PopupFrame::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin PopupFrame bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end PopupFrame bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon PopupFrame::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin PopupFrame icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end PopupFrame icon retrieval
}
