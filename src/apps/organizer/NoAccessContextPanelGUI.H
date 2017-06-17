/////////////////////////////////////////////////////////////////////////////
// Name:        NoAccessContextPanelGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NOACCESSCONTEXTPANELGUI_H_
#define _NOACCESSCONTEXTPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "NoAccessContextPanelGUI.C"
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
#define SYMBOL_NOACCESSCONTEXTPANELGUI_STYLE 0
#define SYMBOL_NOACCESSCONTEXTPANELGUI_TITLE _("Dialog")
#define SYMBOL_NOACCESSCONTEXTPANELGUI_IDNAME ID_DIALOG
#define SYMBOL_NOACCESSCONTEXTPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_NOACCESSCONTEXTPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * NoAccessContextPanelGUI class declaration
 */

class NoAccessContextPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( NoAccessContextPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NoAccessContextPanelGUI( );
    NoAccessContextPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_NOACCESSCONTEXTPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_NOACCESSCONTEXTPANELGUI_POSITION, const wxSize& size = SYMBOL_NOACCESSCONTEXTPANELGUI_SIZE, long style = SYMBOL_NOACCESSCONTEXTPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_NOACCESSCONTEXTPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_NOACCESSCONTEXTPANELGUI_POSITION, const wxSize& size = SYMBOL_NOACCESSCONTEXTPANELGUI_SIZE, long style = SYMBOL_NOACCESSCONTEXTPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NoAccessContextPanelGUI event handler declarations
////@end NoAccessContextPanelGUI event handler declarations

////@begin NoAccessContextPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end NoAccessContextPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NoAccessContextPanelGUI member variables
    static const wxWindowID ID_DIALOG;
////@end NoAccessContextPanelGUI member variables
};

#endif
    // _NOACCESSCONTEXTPANELGUI_H_
