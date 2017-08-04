/////////////////////////////////////////////////////////////////////////////
// Name:        NModesGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NMODESGUI_H_
#define _NMODESGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "NModesGUI.C"
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
#define SYMBOL_NMODESGUI_STYLE wxRESIZE_BORDER|wxSUNKEN_BORDER
#define SYMBOL_NMODESGUI_TITLE _("NMode Panel")
#define SYMBOL_NMODESGUI_IDNAME ID_PANEL_NMODE
#define SYMBOL_NMODESGUI_SIZE wxDefaultSize
#define SYMBOL_NMODESGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * NModesGUI class declaration
 */

class NModesGUI: public VizPropertyPanel
{    
    DECLARE_DYNAMIC_CLASS( NModesGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NModesGUI( );
    NModesGUI( IPropCalculation* calculation, wxWindow* parent, wxWindowID id = SYMBOL_NMODESGUI_IDNAME, const wxPoint& pos = SYMBOL_NMODESGUI_POSITION, const wxSize& size = SYMBOL_NMODESGUI_SIZE, long style = SYMBOL_NMODESGUI_STYLE );

    /// Creation
    bool Create( IPropCalculation* calculation, wxWindow* parent, wxWindowID id = SYMBOL_NMODESGUI_IDNAME, const wxPoint& pos = SYMBOL_NMODESGUI_POSITION, const wxSize& size = SYMBOL_NMODESGUI_SIZE, long style = SYMBOL_NMODESGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NModesGUI event handler declarations
    /// wxEVT_GRID_SELECT_CELL event handler for ID_GRID_NMODE
    virtual    void OnSelectCell( wxGridEvent& event );

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_RADIOBOX_NMODE_VIZTYPE
    virtual    void OnRadioboxNmodeViztypeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NMODE_DELAY
    virtual    void OnTextctrlNmodeDelayEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_START
    virtual    void OnBitmapbuttonStartClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_STOP
    virtual    void OnBitmapbuttonStopClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NMODE_VECCOLOR
    virtual    void OnButtonNmodeVeccolorClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NMODE_VECSIGN
    virtual    void OnCheckboxNmodeVecsignClick( wxCommandEvent& event );

////@end NModesGUI event handler declarations

////@begin NModesGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end NModesGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NModesGUI member variables
    wxBoxSizer* p_gridPlotSizer;
    wxBoxSizer* p_;
    wxBoxSizer* p_sliderSizer;
    wxBoxSizer* p_styleSizer;
    wxBoxSizer* p_animateSizer;
    wxBoxSizer* p_vectorSizer;
    static const wxWindowID ID_BITMAPBUTTON_START;
    static const wxWindowID ID_BITMAPBUTTON_STOP;
    static const wxWindowID ID_RADIOBOX_NMODE_VIZTYPE;
    static const wxWindowID ID_GRID_NMODE;
    static const wxWindowID ID_PANEL_NMODE;
    static const wxWindowID ID_CHECKBOX_NMODE_VECSIGN;
    static const wxWindowID ID_TEXTCTRL_NMODE_DELAY;
    static const wxWindowID ID_BUTTON_NMODE_VECCOLOR;
////@end NModesGUI member variables
};

#endif
    // _NMODESGUI_H_
