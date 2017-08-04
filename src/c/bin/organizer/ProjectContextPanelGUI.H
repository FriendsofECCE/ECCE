/////////////////////////////////////////////////////////////////////////////
// Name:        ProjectContextPanelGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PROJECTCONTEXTPANELGUI_H_
#define _PROJECTCONTEXTPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "ProjectContextPanelGUI.C"
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
#define SYMBOL_PROJECTCONTEXTPANELGUI_STYLE 0
#define SYMBOL_PROJECTCONTEXTPANELGUI_TITLE _("Dialog")
#define SYMBOL_PROJECTCONTEXTPANELGUI_IDNAME ID_DIALOG
#define SYMBOL_PROJECTCONTEXTPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_PROJECTCONTEXTPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ProjectContextPanelGUI class declaration
 */

class ProjectContextPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( ProjectContextPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ProjectContextPanelGUI( );
    ProjectContextPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_PROJECTCONTEXTPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_PROJECTCONTEXTPANELGUI_POSITION, const wxSize& size = SYMBOL_PROJECTCONTEXTPANELGUI_SIZE, long style = SYMBOL_PROJECTCONTEXTPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_PROJECTCONTEXTPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_PROJECTCONTEXTPANELGUI_POSITION, const wxSize& size = SYMBOL_PROJECTCONTEXTPANELGUI_SIZE, long style = SYMBOL_PROJECTCONTEXTPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ProjectContextPanelGUI event handler declarations
////@end ProjectContextPanelGUI event handler declarations

////@begin ProjectContextPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ProjectContextPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ProjectContextPanelGUI member variables
    static const wxWindowID ID_DIALOG;
////@end ProjectContextPanelGUI member variables
};

#endif
    // _PROJECTCONTEXTPANELGUI_H_
