#include <iostream>
using namespace std;

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxOpenCloseToggle.H"

/**
 * Private event helper for ewxOpenCloseToggle.
 * This class is responsible for swapping between open/close state
 * representations as the user left clicks.
 */
class ewxOpenCloseHelper : public wxEvtHandler
{
   public:
       ewxOpenCloseHelper(ewxOpenCloseToggle *win)
       { 
          p_Toggle = win; 
       }

       ~ewxOpenCloseHelper()  { p_Toggle = 0;}

       void OnLeftUp( wxMouseEvent& event)
       {

          if (p_Toggle->isOpen()) {
             p_Toggle->close();
          } else {
             p_Toggle->open();
          }
          event.Skip();
       }


   private:
       ewxOpenCloseHelper() {;}
       ewxOpenCloseToggle *p_Toggle;

       DECLARE_EVENT_TABLE()

};

BEGIN_EVENT_TABLE(ewxOpenCloseHelper, wxEvtHandler)
    EVT_LEFT_UP(ewxOpenCloseHelper::OnLeftUp)
END_EVENT_TABLE()

/**
 * Constructor. 
 * This constructor is used if we do change Class property name using
 * a wxBitmapButton object in dialogblocks.
 */
ewxOpenCloseToggle::ewxOpenCloseToggle(wxWindow* parent, wxWindowID id, 
      const wxBitmap& bitmap,
      const wxPoint& pos, 
      const wxSize& size, long style, 
      const wxValidator& validator, const wxString& name) 
: wxBitmapButton(parent, id, bitmap, pos, size, style, validator, name)
{
   PushEventHandler(new ewxOpenCloseHelper(this));
   p_isOpen = false;
   close();
}

/**
 * Constructor. 
 * This constructor is used if we do custom controls in dialog blocks.
 */
ewxOpenCloseToggle::ewxOpenCloseToggle(wxWindow* parent, wxWindowID id, 
      const wxPoint& pos, 
      const wxSize& size, long style, 
      const wxValidator& validator, const wxString& name) 
: wxBitmapButton(parent, id, ewxBitmap("closed.xpm"), pos, size, style, validator, name)
{
   PushEventHandler(new ewxOpenCloseHelper(this));
   p_isOpen = false;
   close();
}

/**
 * Constructor.
 */
ewxOpenCloseToggle::ewxOpenCloseToggle() : wxBitmapButton()
{
   PushEventHandler(new ewxOpenCloseHelper(this));
   p_isOpen = false;
   close();
}

/**
 * Destructor.
 */
ewxOpenCloseToggle::~ewxOpenCloseToggle()
{
  PopEventHandler(true);
}

bool ewxOpenCloseToggle::isOpen() const
{
   return p_isOpen;
}

void ewxOpenCloseToggle::open()
{
   SetBitmapLabel(ewxBitmap("opened.xpm", wxBITMAP_TYPE_XPM));
   p_isOpen = true;
}


void ewxOpenCloseToggle::close()
{
   SetBitmapLabel(ewxBitmap("closed.xpm", wxBITMAP_TYPE_XPM));
   p_isOpen = false;
}

void ewxOpenCloseToggle::setState(bool openit)
{
   if (openit) {
      open();
   } else {
      close();
   }
}
