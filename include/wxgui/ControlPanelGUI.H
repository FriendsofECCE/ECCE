/////////////////////////////////////////////////////////////////////////////
// Name:        ControlPanelGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CONTROLPANELGUI_H_
#define _CONTROLPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "ControlPanelGUI.C"
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
#define SYMBOL_CONTROLPANELGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxSUNKEN_BORDER
#define SYMBOL_CONTROLPANELGUI_TITLE _("ControlPanelGUI")
#define SYMBOL_CONTROLPANELGUI_IDNAME ID_DIALOG_CNTRL
#define SYMBOL_CONTROLPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_CONTROLPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ControlPanelGUI class declaration
 */

class ControlPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( ControlPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ControlPanelGUI( );
    ControlPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_CONTROLPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_CONTROLPANELGUI_POSITION, const wxSize& size = SYMBOL_CONTROLPANELGUI_SIZE, long style = SYMBOL_CONTROLPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_CONTROLPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_CONTROLPANELGUI_POSITION, const wxSize& size = SYMBOL_CONTROLPANELGUI_SIZE, long style = SYMBOL_CONTROLPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ControlPanelGUI event handler declarations
    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_USELOAD
    virtual    void OnCheckboxCntrlUseloadClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_CELLSZ
    virtual    void OnCheckboxCntrlCellszClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_STRNGTH
    virtual    void OnTextctrlCntrlStrngthEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_ZONLY
    virtual    void OnCheckboxCntrlZonlyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_RDSTRB
    virtual    void OnCheckboxCntrlRdstrbClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CFREQ
    virtual    void OnTextctrlCntrlCfreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CNTRL_TSYNCH
    virtual    void OnChoiceCntrlTsynchSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TOGGLE_CNTRL_CNFG
    virtual    void OnToggleCntrlCnfgClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_PCNFG
    virtual    void OnCheckboxCntrlPcnfgClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_PX
    virtual    void OnTextctrlCntrlPxEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_PY
    virtual    void OnTextctrlCntrlPyEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_PZ
    virtual    void OnTextctrlCntrlPzEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_CCNFG
    virtual    void OnCheckboxCntrlCcnfgClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CX
    virtual    void OnTextctrlCntrlCxEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CY
    virtual    void OnTextctrlCntrlCyEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CZ
    virtual    void OnTextctrlCntrlCzEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_MAXSLV
    virtual    void OnCheckboxCntrlMaxslvClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_MAXSLV
    virtual    void OnTextctrlCntrlMaxslvEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_MAXSLU
    virtual    void OnCheckboxCntrlMaxsluClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_MAXSLU
    virtual    void OnTextctrlCntrlMaxsluEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_PACK
    virtual    void OnCheckboxCntrlPackClick( wxCommandEvent& event );

////@end ControlPanelGUI event handler declarations

////@begin ControlPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ControlPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ControlPanelGUI member variables
    wxBoxSizer* p_controlVSizer;
    wxPanel* p_balancePanel;
    wxBoxSizer* p_configVSizer;
    wxPanel* p_configPanel;
    static const wxWindowID ID_CHECKBOX_CNTRL_CCNFG;
    static const wxWindowID ID_TEXTCTRL_CNTRL_MAXSLU;
    static const wxWindowID ID_CHECKBOX_CNTRL_PACK;
    static const wxWindowID ID_CHECKBOX_CNTRL_MAXSLU;
    static const wxWindowID ID_CHOICE_CNTRL_TSYNCH;
    static const wxWindowID ID_TEXTCTRL_CNTRL_CY;
    static const wxWindowID wxID_STATIC_CNTRL_CFREQ;
    static const wxWindowID ID_TEXTCTRL_CNTRL_CX;
    static const wxWindowID ID_TEXTCTRL_CNTRL_STRNGTH;
    static const wxWindowID ID_CHECKBOX_CNTRL_PCNFG;
    static const wxWindowID ID_PANEL_CNTRL_RDPAR;
    static const wxWindowID ID_PANEL_CNTRL_NCELL;
    static const wxWindowID ID_PANEL_CNTRL_STRNGTH;
    static const wxWindowID ID_CHECKBOX_CNTRL_USELOAD;
    static const wxWindowID ID_DIALOG_CNTRL;
    static const wxWindowID ID_PANEL_CNTRL_ADJUST;
    static const wxWindowID wxID_STATIC_CNTRL_TSYNCH;
    static const wxWindowID ID_PANEL_CNTRL_NPROC;
    static const wxWindowID ID_CHECKBOX_CNTRL_ZONLY;
    static const wxWindowID ID_CHECKBOX_CNTRL_RDSTRB;
    static const wxWindowID ID_TEXTCTRL_CNTRL_PX;
    static const wxWindowID ID_TEXTCTRL_CNTRL_MAXSLV;
    static const wxWindowID ID_TEXTCTRL_CNTRL_PZ;
    static const wxWindowID ID_CHECKBOX_CNTRL_CELLSZ;
    static const wxWindowID ID_CHECKBOX_CNTRL_MAXSLV;
    static const wxWindowID ID_TEXTCTRL_CNTRL_PY;
    static const wxWindowID ID_TEXTCTRL_CNTRL_CFREQ;
    static const wxWindowID ID_PANEL_CNTRL_CNFG;
    static const wxWindowID ID_TEXTCTRL_CNTRL_CZ;
    static const wxWindowID ID_TOGGLE_CNTRL_CNFG;
////@end ControlPanelGUI member variables
};

#endif
    // _CONTROLPANELGUI_H_
