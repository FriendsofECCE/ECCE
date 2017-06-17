/////////////////////////////////////////////////////////////////////////////
// Name:        PBCGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PBCGUI_H_
#define _PBCGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "PBCGUI.C"
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
class ewxButton;
class wxBoxSizer;
class ewxPanel;
class ewxButton;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_PBCGUI_STYLE wxSUNKEN_BORDER
#define SYMBOL_PBCGUI_TITLE _("Dialog")
#define SYMBOL_PBCGUI_IDNAME ID_PANEL_PBC
#define SYMBOL_PBCGUI_SIZE wxDefaultSize
#define SYMBOL_PBCGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * PBCGUI class declaration
 */

class PBCGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( PBCGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    PBCGUI( );
    PBCGUI( wxWindow* parent, wxWindowID id = SYMBOL_PBCGUI_IDNAME, const wxPoint& pos = SYMBOL_PBCGUI_POSITION, const wxSize& size = SYMBOL_PBCGUI_SIZE, long style = SYMBOL_PBCGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_PBCGUI_IDNAME, const wxPoint& pos = SYMBOL_PBCGUI_POSITION, const wxSize& size = SYMBOL_PBCGUI_SIZE, long style = SYMBOL_PBCGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin PBCGUI event handler declarations
    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_CREATE
    virtual    void OnButtonPbcCreateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PBC_TYPE
    virtual    void OnChoicePbcTypeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A1X
    virtual    void OnTextctrlPbcA1xEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A1Y
    virtual    void OnTextctrlPbcA1yEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A1Z
    virtual    void OnTextctrlPbcA1zEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A2X
    virtual    void OnTextctrlPbcA2xEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A2Y
    virtual    void OnTextctrlPbcA2yEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A2Z
    virtual    void OnTextctrlPbcA2zEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A3X
    virtual    void OnTextctrlPbcA3xEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A3Y
    virtual    void OnTextctrlPbcA3yEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A3Z
    virtual    void OnTextctrlPbcA3zEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A
    virtual    void OnTextctrlPbcAEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_ALPHA
    virtual    void OnTextctrlPbcAlphaEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_B
    virtual    void OnTextctrlPbcBEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_BETA
    virtual    void OnTextctrlPbcBetaEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_C
    virtual    void OnTextctrlPbcCEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_GAMMA
    virtual    void OnTextctrlPbcGammaEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PBC_CENTER
    virtual    void OnCheckboxPbcCenterClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PBC_SHOWLATTICE
    virtual    void OnCheckboxPbcShowlatticeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_FOLD
    virtual    void OnButtonPbcFoldClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_DELETE
    virtual    void OnButtonPbcDeleteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_EQUIVRECT
    virtual    void OnButtonPbcEquivrectClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PBC_GENERATE
    virtual    void OnPbcGenerateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PBC_XREP
    virtual    void OnSpinctrlPbcXrepUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_PBC_XREP
    virtual    void OnSpinctrlPbcXrepTextUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PBC_YREP
    virtual    void OnSpinctrlPbcYrepUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_PBC_YREP
    virtual    void OnSpinctrlPbcYrepTextUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PBC_ZREP
    virtual    void OnSpinctrlPbcZrepUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_PBC_ZREP
    virtual    void OnSpinctrlPbcZrepTextUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PBC_UNIFORM
    virtual    void OnCheckboxPbcUniformClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_REPLICATE
    virtual    void OnButtonPbcReplicateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_RESTORE
    virtual    void OnButtonPbcRestoreClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_SUPER
    virtual    void OnButtonPbcSuperClick( wxCommandEvent& event );

////@end PBCGUI event handler declarations

////@begin PBCGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end PBCGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin PBCGUI member variables
    ewxButton* p_createSizer;
    wxPanel* p_latticePanel;
    wxBoxSizer* p_operationsSizer;
    wxBoxSizer* p_symSizer;
    static const wxWindowID ID_TEXTCTRL_PBC_A3Y;
    static const wxWindowID ID_SPINCTRL_PBC_ZREP;
    static const wxWindowID ID_CHOICE_PBC_TYPE;
    static const wxWindowID ID_TEXTCTRL_PBC_A1X;
    static const wxWindowID ID_CHECKBOX_PBC_SHOWLATTICE;
    static const wxWindowID ID_TEXTCTRL_PBC_A2Z;
    static const wxWindowID ID_CHECKBOX_PBC_LOCK;
    static const wxWindowID ID_BUTTON_PBC_EQUIVRECT;
    static const wxWindowID ID_CHECKBOX_PBC_CENTER;
    static const wxWindowID ID_TEXTCTRL_PBC_B;
    static const wxWindowID ID_TEXTCTRL_PBC_A2X;
    static const wxWindowID ID_PANEL;
    static const wxWindowID ID_SPINCTRL_PBC_YREP;
    static const wxWindowID ID_CHECKBOX_PBC_UNIFORM;
    static const wxWindowID ID_TEXTCTRL_PBC_BETA;
    static const wxWindowID ID_TEXTCTRL_PBC_A2Y;
    static const wxWindowID ID_BUTTON_PBC_RESTORE;
    static const wxWindowID ID_BUTTON_PBC_REPLICATE;
    static const wxWindowID ID_PANEL_PBC;
    static const wxWindowID ID_TEXTCTRL_PBC_A3X;
    static const wxWindowID ID_TEXTCTRL_PBC_A1Z;
    static const wxWindowID ID_TEXTCTRL_PBC_ALPHA;
    static const wxWindowID ID_BUTTON_PBC_FOLD;
    static const wxWindowID ID_TEXTCTRL_PBC_A3Z;
    static const wxWindowID ID_TEXTCTRL_PBC_C;
    static const wxWindowID ID_BUTTON_PBC_SUPER;
    static const wxWindowID ID_PBC_GENERATE;
    static const wxWindowID ID_BUTTON_PBC_DELETE;
    static const wxWindowID ID_SPINCTRL_PBC_XREP;
    static const wxWindowID ID_TEXTCTRL_PBC_A;
    static const wxWindowID ID_TEXTCTRL_PBC_A1Y;
    static const wxWindowID ID_TEXTCTRL_PBC_GAMMA;
    static const wxWindowID ID_BUTTON_PBC_CREATE;
////@end PBCGUI member variables
};

#endif
    // _PBCGUI_H_
