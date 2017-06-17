/////////////////////////////////////////////////////////////////////////////
// Name:        PropertyContextPanelGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PROPERTYCONTEXTPANELGUI_H_
#define _PROPERTYCONTEXTPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "PropertyContextPanelGUI.C"
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
class wxBoxSizer;
class ewxPanel;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_PROPERTYCONTEXTPANELGUI_STYLE wxSUNKEN_BORDER
#define SYMBOL_PROPERTYCONTEXTPANELGUI_TITLE _("Dialog")
#define SYMBOL_PROPERTYCONTEXTPANELGUI_IDNAME ID_DIALOG
#define SYMBOL_PROPERTYCONTEXTPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_PROPERTYCONTEXTPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * PropertyContextPanelGUI class declaration
 */

class PropertyContextPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( PropertyContextPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    PropertyContextPanelGUI( );
    PropertyContextPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_PROPERTYCONTEXTPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_PROPERTYCONTEXTPANELGUI_POSITION, const wxSize& size = SYMBOL_PROPERTYCONTEXTPANELGUI_SIZE, long style = SYMBOL_PROPERTYCONTEXTPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_PROPERTYCONTEXTPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_PROPERTYCONTEXTPANELGUI_POSITION, const wxSize& size = SYMBOL_PROPERTYCONTEXTPANELGUI_SIZE, long style = SYMBOL_PROPERTYCONTEXTPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin PropertyContextPanelGUI event handler declarations
////@end PropertyContextPanelGUI event handler declarations

////@begin PropertyContextPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end PropertyContextPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin PropertyContextPanelGUI member variables
    wxBoxSizer* p_gridSizer;
    static const wxWindowID ID_DIALOG;
////@end PropertyContextPanelGUI member variables
};

#endif
    // _PROPERTYCONTEXTPANELGUI_H_
