/////////////////////////////////////////////////////////////////////////////
// Name:        ThermodynamicsPanelGUI.H
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _THERMODYNAMICSPANELGUI_H_
#define _THERMODYNAMICSPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "ThermodynamicsPanelGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/spinctrl.h"
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
#define SYMBOL_THERMODYNAMICSPANELGUI_STYLE 0
#define SYMBOL_THERMODYNAMICSPANELGUI_TITLE _("ThermodynamicsPanelGUI")
#define SYMBOL_THERMODYNAMICSPANELGUI_IDNAME ID_DIALOG_THERM
#define SYMBOL_THERMODYNAMICSPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_THERMODYNAMICSPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ThermodynamicsPanelGUI class declaration
 */

class ThermodynamicsPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( ThermodynamicsPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ThermodynamicsPanelGUI( );
    ThermodynamicsPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_THERMODYNAMICSPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_THERMODYNAMICSPANELGUI_POSITION, const wxSize& size = SYMBOL_THERMODYNAMICSPANELGUI_SIZE, long style = SYMBOL_THERMODYNAMICSPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_THERMODYNAMICSPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_THERMODYNAMICSPANELGUI_POSITION, const wxSize& size = SYMBOL_THERMODYNAMICSPANELGUI_SIZE, long style = SYMBOL_THERMODYNAMICSPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ThermodynamicsPanelGUI event handler declarations
    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_THERM_PMF
    virtual    void OnCheckboxThermPmfClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_PMF_STANDARD
    virtual    void OnRadiobuttonThermPmfStandardSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_PMF_SCALED
    virtual    void OnRadiobuttonThermPmfScaledSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_PMF_SCALED
    virtual    void OnTextctrlThermPmfScaledEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_PMF_HARMONIC
    virtual    void OnRadiobuttonThermPmfHarmonicSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_PMF_HARMONIC
    virtual    void OnTextctrlThermPmfHarmonicEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_THERM_INTEG
    virtual    void OnCheckboxThermIntegClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_INTEG_NEW
    virtual    void OnRadiobuttonThermIntegNewSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_INTEG_RENEW
    virtual    void OnRadiobuttonThermIntegRenewSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_INTEG_EXTEND
    virtual    void OnRadiobuttonThermIntegExtendSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_THERM_INTEG_DIRECTION
    virtual    void OnChoiceThermIntegDirectionSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_THERM_INTEG_ENSEMBLES_NUM
    virtual    void OnSpinctrlThermIntegEnsemblesNumUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_THERM_INTEG_ENSEMBLES_MAX
    virtual    void OnSpinctrlThermIntegEnsemblesMaxUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_INTEG_ERROR
    virtual    void OnTextctrlThermIntegErrorEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_INTEG_DRIFT
    virtual    void OnTextctrlThermIntegDriftEnter( wxCommandEvent& event );

////@end ThermodynamicsPanelGUI event handler declarations

////@begin ThermodynamicsPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ThermodynamicsPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ThermodynamicsPanelGUI member variables
    static const wxWindowID ID_CHECKBOX_THERM_INTEG;
    static const wxWindowID ID_TEXTCTRL_THERM_PMF_HARMONIC;
    static const wxWindowID ID_SPINCTRL_THERM_INTEG_ENSEMBLES_NUM;
    static const wxWindowID ID_TEXTCTRL_THERM_INTEG_ERROR;
    static const wxWindowID ID_TEXTCTRL_THERM_PMF_SCALED;
    static const wxWindowID ID_DIALOG_THERM;
    static const wxWindowID ID_RADIOBUTTON_THERM_PMF_SCALED;
    static const wxWindowID ID_PANEL_THERM_INTEG;
    static const wxWindowID ID_PANEL_THERM_PMF;
    static const wxWindowID ID_RADIOBUTTON_THERM_PMF_HARMONIC;
    static const wxWindowID wxID_STATIC_THERM_INTEG_ERROR;
    static const wxWindowID ID_SPINCTRL_THERM_INTEG_ENSEMBLES_MAX;
    static const wxWindowID ID_RADIOBUTTON_THERM_PMF_STANDARD;
    static const wxWindowID ID_RADIOBUTTON_THERM_INTEG_NEW;
    static const wxWindowID ID_TEXTCTRL_THERM_INTEG_DRIFT;
    static const wxWindowID ID_RADIOBUTTON_THERM_INTEG_EXTEND;
    static const wxWindowID wxID_STATIC_THERM_INTEG_DRIFT;
    static const wxWindowID ID_CHECKBOX_THERM_PMF;
    static const wxWindowID ID_RADIOBUTTON_THERM_INTEG_RENEW;
    static const wxWindowID ID_CHOICE_THERM_INTEG_DIRECTION;
////@end ThermodynamicsPanelGUI member variables
};

#endif
    // _THERMODYNAMICSPANELGUI_H_
