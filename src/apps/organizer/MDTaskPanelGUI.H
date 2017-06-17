/////////////////////////////////////////////////////////////////////////////
// Name:        MDTaskPanelGUI.H
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _MDTASKPANELGUI_H_
#define _MDTASKPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "MDTaskPanelGUI.C"
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
class ewxStaticText;
class ewxPanel;
class ewxStaticText;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_MDTASKPANELGUI_STYLE wxSIMPLE_BORDER
#define SYMBOL_MDTASKPANELGUI_TITLE _("MDTaskPanelGUI")
#define SYMBOL_MDTASKPANELGUI_IDNAME ID_DIALOG
#define SYMBOL_MDTASKPANELGUI_SIZE wxSize(300, 100)
#define SYMBOL_MDTASKPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * MDTaskPanelGUI class declaration
 */

class MDTaskPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( MDTaskPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    MDTaskPanelGUI( );
    MDTaskPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_MDTASKPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_MDTASKPANELGUI_POSITION, const wxSize& size = SYMBOL_MDTASKPANELGUI_SIZE, long style = SYMBOL_MDTASKPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_MDTASKPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_MDTASKPANELGUI_POSITION, const wxSize& size = SYMBOL_MDTASKPANELGUI_SIZE, long style = SYMBOL_MDTASKPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin MDTaskPanelGUI event handler declarations
////@end MDTaskPanelGUI event handler declarations

////@begin MDTaskPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end MDTaskPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin MDTaskPanelGUI member variables
    wxBoxSizer* p_topSizer;
    wxBoxSizer* p_nameSizer;
    ewxStaticText* p_name;
    wxBoxSizer* p_codeSizer;
    wxStaticBitmap* p_codeBitmap;
    ewxStaticText* p_code;
    static const wxWindowID ID_DIALOG;
////@end MDTaskPanelGUI member variables
};

#endif
    // _MDTASKPANELGUI_H_
