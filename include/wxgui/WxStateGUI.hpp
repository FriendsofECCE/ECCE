/////////////////////////////////////////////////////////////////////////////
// Name:        WxStateGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXSTATEGUI_H_
#define _WXSTATEGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxStateGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wxgui/ewxPanel.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxPanel;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXSTATEGUI_STYLE wxNO_BORDER
#define SYMBOL_WXSTATEGUI_TITLE _("State")
#define SYMBOL_WXSTATEGUI_IDNAME ID_STATE
#define SYMBOL_WXSTATEGUI_SIZE wxSize(15, 15)
#define SYMBOL_WXSTATEGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxStateGUI class declaration
 */

class WxStateGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( WxStateGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxStateGUI( );
    WxStateGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXSTATEGUI_IDNAME, const wxPoint& pos = SYMBOL_WXSTATEGUI_POSITION, const wxSize& size = SYMBOL_WXSTATEGUI_SIZE, long style = SYMBOL_WXSTATEGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXSTATEGUI_IDNAME, const wxPoint& pos = SYMBOL_WXSTATEGUI_POSITION, const wxSize& size = SYMBOL_WXSTATEGUI_SIZE, long style = SYMBOL_WXSTATEGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxStateGUI event handler declarations
    /// wxEVT_PAINT event handler for ID_STATE
    virtual    void OnPaint( wxPaintEvent& event );

////@end WxStateGUI event handler declarations

////@begin WxStateGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxStateGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxStateGUI member variables
    static const wxWindowID ID_STATE;
////@end WxStateGUI member variables
};

#endif
    // _WXSTATEGUI_H_
