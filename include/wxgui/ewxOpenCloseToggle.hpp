/**
 * @file
 *
 *
 */
#ifndef _EWXOPENCLOSETOGGLE_H
#define _EWXOPENCLOSETOGGLE_H

#include <wx/bmpbuttn.h>

#include "wxgui/ewxGenericValidator.H" // for ewxDefaultValidator

/**
 * A class that acts as a toggle to hide and show panels.
 * To use from dialog blocks, create a horizontal sizer,
 * add a wxBitmapButton then set the Class property in the property panel
 * to ewxOpenCloseToggle.  That is the only special thing you have to
 * do.  Add your callback processing as normal.
 * This widget mainly provides style (switching icons to represent open
 * or closed state).  But it also has open/close methods.  However you
 * are responsible for the call to hide or show the panel as it does not
 * know anything about your panel.
 * An example callback:
 * <pre>
 *   ewxOpenCloseToggle *tgl = 0;
 *   tgl = ((ewxOpenCloseToggle*)FindWindow(ID_TOGGLE_MDPREP_SETSLV));
 *   showMyPanel(tgl->isOpen());
 *   event.Skip();
 * <pre>
 */
class ewxOpenCloseToggle : public wxBitmapButton
{
   public:
      ewxOpenCloseToggle();
      ewxOpenCloseToggle(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxBU_AUTODRAW, const wxValidator& validator = ewxDefaultValidator, const wxString& name = "Toggle");
      ewxOpenCloseToggle(wxWindow* parent, wxWindowID id, const wxBitmap& bitmap, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxBU_AUTODRAW, const wxValidator& validator = ewxDefaultValidator, const wxString& name = "Toggle");

      virtual ~ewxOpenCloseToggle();

      bool isOpen() const;
      void open();
      void close();
      void setState(bool open);

   protected:
      bool p_isOpen;
};

#endif
