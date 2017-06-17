/////////////////////////////////////////////////////////////////////////////
// Name:        WxPovrayOptionsGUI.H
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXPOVRAYOPTIONSGUI_H_
#define _WXPOVRAYOPTIONSGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxPovrayOptionsGUI.C"
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
#define SYMBOL_WXPOVRAYOPTIONSGUI_STYLE 0
#define SYMBOL_WXPOVRAYOPTIONSGUI_TITLE _("WxPovrayOptionsGUI")
#define SYMBOL_WXPOVRAYOPTIONSGUI_IDNAME ID_POVRAY_PANEL
#define SYMBOL_WXPOVRAYOPTIONSGUI_SIZE wxSize(400, 300)
#define SYMBOL_WXPOVRAYOPTIONSGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxPovrayOptionsGUI class declaration
 */

class WxPovrayOptionsGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( WxPovrayOptionsGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxPovrayOptionsGUI( );
    WxPovrayOptionsGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXPOVRAYOPTIONSGUI_IDNAME, const wxPoint& pos = SYMBOL_WXPOVRAYOPTIONSGUI_POSITION, const wxSize& size = SYMBOL_WXPOVRAYOPTIONSGUI_SIZE, long style = SYMBOL_WXPOVRAYOPTIONSGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXPOVRAYOPTIONSGUI_IDNAME, const wxPoint& pos = SYMBOL_WXPOVRAYOPTIONSGUI_POSITION, const wxSize& size = SYMBOL_WXPOVRAYOPTIONSGUI_SIZE, long style = SYMBOL_WXPOVRAYOPTIONSGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxPovrayOptionsGUI event handler declarations
////@end WxPovrayOptionsGUI event handler declarations

////@begin WxPovrayOptionsGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxPovrayOptionsGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxPovrayOptionsGUI member variables
    static const wxWindowID ID_POVRAY_PANEL;
    static const wxWindowID ID_POVRAY_HEIGHT;
    static const wxWindowID ID_POVRAY_WIDTH;
    static const wxWindowID ID_POVRAY_DISPLAY;
    static const wxWindowID ID_POVRAY_ISOSURFACES;
    static const wxWindowID ID_POVRAY_BONDS;
    static const wxWindowID ID_POVRAY_FINISH;
////@end WxPovrayOptionsGUI member variables
};

#endif
    // _WXPOVRAYOPTIONSGUI_H_
