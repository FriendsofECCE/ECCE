#ifndef _FLATBITMAPBUTTON_H
#define _FLATBITMAPBUTTON_H

#include "wx/sizer.h"

#include "wxgui/ewxPanel.H"

class wxStaticBitmap;


/**
 * This class provides an alternative to the stock BitmapButton class.
 * It consists of a panel overlayed with a bitmap.  The reason to use
 * it is to get the look we want (thin border) which seemed not to be
 * possible with our combination of wx and gtk.
 * 
 * This button can have a 3D look or not (see showBorder()).
 *
 * Since this is a panel and not truely a button, there is no such thing
 * as a button pushed event.  Its all mouse events.  The events are
 * set to propagate up so you should be able to get these in your panel.
 * 
 */
class FlatBitmapButton: public ewxPanel 
{
  DECLARE_DYNAMIC_CLASS(FlatBitmapButton)
  DECLARE_EVENT_TABLE()

  public:

    FlatBitmapButton();
    FlatBitmapButton(wxWindow* parent, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxTAB_TRAVERSAL,
            const wxString& name = "FlatBitmapButton");
    ~FlatBitmapButton();

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxTAB_TRAVERSAL,
            const wxString& name = "FlatBitmapButton");

    void OnMouseLeftDown( wxMouseEvent& event );
    void OnMouseLeftUp( wxMouseEvent& event );
    void OnMouseRightDown( wxMouseEvent& event );
    void OnMouseEnterWindow( wxMouseEvent& event );
    void OnMouseLeaveWindow( wxMouseEvent& event );
    void OnPaint( wxPaintEvent& event );
    virtual void OnMenuClick( wxCommandEvent& event );

    void SetBitmap(wxBitmap bitmap);

    void showBorder(bool flag);

    void showArmed(bool isArmed);
    bool isArmed() const;

  protected:
    wxBoxSizer * p_buttonSizer;

    wxStaticBitmap * p_bitmap;

    bool p_isArmed;
    bool p_showBorder;
};

#endif // _FLATBITMAPBUTTON_H
