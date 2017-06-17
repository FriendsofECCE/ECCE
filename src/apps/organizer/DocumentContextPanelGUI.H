/////////////////////////////////////////////////////////////////////////////
// Name:        DocumentContextPanelGUI.H
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _DOCUMENTCONTEXTPANELGUI_H_
#define _DOCUMENTCONTEXTPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "DocumentContextPanelGUI.C"
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
class ewxTextCtrl;
class ewxPanel;
class ewxTextCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_DOCUMENTCONTEXTPANELGUI_STYLE 0
#define SYMBOL_DOCUMENTCONTEXTPANELGUI_TITLE _("DocumentContextPanelGUI")
#define SYMBOL_DOCUMENTCONTEXTPANELGUI_IDNAME ID_PANEL_DOCUMENT_CONTEXT_TOP
#define SYMBOL_DOCUMENTCONTEXTPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_DOCUMENTCONTEXTPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * DocumentContextPanelGUI class declaration
 */

class DocumentContextPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( DocumentContextPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    DocumentContextPanelGUI( );
    DocumentContextPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_DOCUMENTCONTEXTPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_DOCUMENTCONTEXTPANELGUI_POSITION, const wxSize& size = SYMBOL_DOCUMENTCONTEXTPANELGUI_SIZE, long style = SYMBOL_DOCUMENTCONTEXTPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_DOCUMENTCONTEXTPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_DOCUMENTCONTEXTPANELGUI_POSITION, const wxSize& size = SYMBOL_DOCUMENTCONTEXTPANELGUI_SIZE, long style = SYMBOL_DOCUMENTCONTEXTPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin DocumentContextPanelGUI event handler declarations
////@end DocumentContextPanelGUI event handler declarations

////@begin DocumentContextPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end DocumentContextPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin DocumentContextPanelGUI member variables
    wxBoxSizer* p_topSizer;
    ewxTextCtrl* p_textDisplay;
    static const wxWindowID ID_PANEL_DOCUMENT_CONTEXT_TOP;
    static const wxWindowID ID_TEXTCTRL;
////@end DocumentContextPanelGUI member variables
};

#endif
    // _DOCUMENTCONTEXTPANELGUI_H_
