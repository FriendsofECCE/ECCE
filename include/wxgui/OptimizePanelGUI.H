/////////////////////////////////////////////////////////////////////////////
// Name:        OptimizePanelGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _OPTIMIZEPANELGUI_H_
#define _OPTIMIZEPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "OptimizePanelGUI.C"
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
#define SYMBOL_OPTIMIZEPANELGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxSUNKEN_BORDER
#define SYMBOL_OPTIMIZEPANELGUI_TITLE _("OptimizePanelGUI")
#define SYMBOL_OPTIMIZEPANELGUI_IDNAME ID_DIALOG
#define SYMBOL_OPTIMIZEPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_OPTIMIZEPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * OptimizePanelGUI class declaration
 */

class OptimizePanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( OptimizePanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    OptimizePanelGUI( );
    OptimizePanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_OPTIMIZEPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_OPTIMIZEPANELGUI_POSITION, const wxSize& size = SYMBOL_OPTIMIZEPANELGUI_SIZE, long style = SYMBOL_OPTIMIZEPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_OPTIMIZEPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_OPTIMIZEPANELGUI_POSITION, const wxSize& size = SYMBOL_OPTIMIZEPANELGUI_SIZE, long style = SYMBOL_OPTIMIZEPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin OptimizePanelGUI event handler declarations
    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_OPT_USESD
    virtual    void OnCheckboxOptUsesdClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_SD_MAXSTP
    virtual    void OnTextctrlOptSdMaxstpEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_ISTEP
    virtual    void OnTextctrlOptIstepEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_TOL
    virtual    void OnTextctrlOptTolEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_MAXSZ
    virtual    void OnTextctrlOptMaxszEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_OPT_USECG
    virtual    void OnCheckboxOptUsecgClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_CG_MAXSTP
    virtual    void OnTextctrlOptCgMaxstpEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_ISRCH
    virtual    void OnTextctrlOptIsrchEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_THRESH
    virtual    void OnTextctrlOptThreshEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_REFRESH
    virtual    void OnTextctrlOptRefreshEnter( wxCommandEvent& event );

////@end OptimizePanelGUI event handler declarations

////@begin OptimizePanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end OptimizePanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin OptimizePanelGUI member variables
    wxBoxSizer* p_optimizeVSizer;
    wxPanel* p_SDPanel;
    wxPanel* p_CGPanel;
    static const wxWindowID ID_TEXTCTRL_OPT_MAXSZ;
    static const wxWindowID ID_TEXTCTRL_OPT_SD_MAXSTP;
    static const wxWindowID ID_PANEL_OPT_CGPAR;
    static const wxWindowID ID_TEXTCTRL_OPT_TOL;
    static const wxWindowID ID_TEXTCTRL_OPT_ISTEP;
    static const wxWindowID ID_DIALOG;
    static const wxWindowID wxID_STATIC_OPT_THRESH;
    static const wxWindowID ID_TEXTCTRL_OPT_THRESH;
    static const wxWindowID ID_TEXTCTRL_OPT_ISRCH;
    static const wxWindowID ID_PANEL_OPT_SDPAR;
    static const wxWindowID wxID_STATIC_OPT_MAXSZ;
    static const wxWindowID wxID_STATIC_OPT_ISRCH;
    static const wxWindowID wxID_STATIC_OPT_ISTEP;
    static const wxWindowID ID_TEXTCTRL_OPT_CG_MAXSTP;
    static const wxWindowID wxID_STATIC_OPT_TOL;
    static const wxWindowID ID_TEXTCTRL_OPT_REFRESH;
    static const wxWindowID ID_CHECKBOX_OPT_USESD;
    static const wxWindowID ID_CHECKBOX_OPT_USECG;
////@end OptimizePanelGUI member variables
};

#endif
    // _OPTIMIZEPANELGUI_H_
