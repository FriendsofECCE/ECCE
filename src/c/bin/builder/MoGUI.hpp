/////////////////////////////////////////////////////////////////////////////
// Name:        MoGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _MOGUI_H_
#define _MOGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "MoGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/grid.h"
#include "VizPropertyPanel.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class IPropCalculation;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_MOGUI_STYLE wxRESIZE_BORDER|wxSUNKEN_BORDER
#define SYMBOL_MOGUI_TITLE _("MO Panel")
#define SYMBOL_MOGUI_IDNAME ID_PANEL_MO
#define SYMBOL_MOGUI_SIZE wxDefaultSize
#define SYMBOL_MOGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * MoGUI class declaration
 */

class MoGUI: public VizPropertyPanel
{    
    DECLARE_DYNAMIC_CLASS( MoGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    MoGUI( );
    MoGUI( IPropCalculation* calculation, wxWindow* parent, wxWindowID id = SYMBOL_MOGUI_IDNAME, const wxPoint& pos = SYMBOL_MOGUI_POSITION, const wxSize& size = SYMBOL_MOGUI_SIZE, long style = SYMBOL_MOGUI_STYLE );

    /// Creation
    bool Create( IPropCalculation* calculation, wxWindow* parent, wxWindowID id = SYMBOL_MOGUI_IDNAME, const wxPoint& pos = SYMBOL_MOGUI_POSITION, const wxSize& size = SYMBOL_MOGUI_SIZE, long style = SYMBOL_MOGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin MoGUI event handler declarations
    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MO_COMPUTE
    virtual    void OnButtonMoComputeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MO_VIEWCOEF
    virtual    void OnButtonMoViewcoefClick( wxCommandEvent& event );

////@end MoGUI event handler declarations

////@begin MoGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end MoGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin MoGUI member variables
    wxBoxSizer* p_sliderSizer;
    static const wxWindowID ID_PANEL;
    static const wxWindowID ID_CHOICE_MO_TYPE;
    static const wxWindowID ID_PANEL_MO;
    static const wxWindowID ID_BUTTON_MO_VIEWCOEF;
    static const wxWindowID ID_GRID_MO;
    static const wxWindowID ID_TEXTCTRL_MO_CUTOFF;
    static const wxWindowID ID_BUTTON_MO_COMPUTE;
////@end MoGUI member variables
};

#endif
    // _MOGUI_H_
