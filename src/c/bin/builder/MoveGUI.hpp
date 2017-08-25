/////////////////////////////////////////////////////////////////////////////
// Name:        MoveGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _MOVEGUI_H_
#define _MOVEGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "MoveGUI.C"
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
#define SYMBOL_MOVEGUI_STYLE wxSUNKEN_BORDER
#define SYMBOL_MOVEGUI_TITLE _("Dialog")
#define SYMBOL_MOVEGUI_IDNAME ID_DIALOG
#define SYMBOL_MOVEGUI_SIZE wxDefaultSize
#define SYMBOL_MOVEGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * MoveGUI class declaration
 */

class MoveGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( MoveGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    MoveGUI( );
    MoveGUI( wxWindow* parent, wxWindowID id = SYMBOL_MOVEGUI_IDNAME, const wxPoint& pos = SYMBOL_MOVEGUI_POSITION, const wxSize& size = SYMBOL_MOVEGUI_SIZE, long style = SYMBOL_MOVEGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_MOVEGUI_IDNAME, const wxPoint& pos = SYMBOL_MOVEGUI_POSITION, const wxSize& size = SYMBOL_MOVEGUI_SIZE, long style = SYMBOL_MOVEGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin MoveGUI event handler declarations
    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_OPTS
    virtual    void OnChoiceMoveOptsSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_ACTION
    virtual    void OnButtonMoveActionClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_XYZ
    virtual    void OnButtonMoveCntrXyzClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_X
    virtual    void OnButtonMoveCntrXClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_Y
    virtual    void OnButtonMoveCntrYClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_Z
    virtual    void OnButtonMoveCntrZClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_TRANSL_X
    virtual    void OnTextctrlMoveTranslXEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_TRANSL_Y
    virtual    void OnTextctrlMoveTranslYEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_TRANSL_Z
    virtual    void OnTextctrlMoveTranslZEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_ROT_AXS
    virtual    void OnChoiceMoveRotAxsSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ROT_ANGL
    virtual    void OnTextctrlMoveRotAnglEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_ATOM
    virtual    void OnTextctrlMoveAlgnAtomEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_ALGN_LN
    virtual    void OnChoiceMoveAlgnLnSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_X
    virtual    void OnTextctrlMoveAlgnXEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_Y
    virtual    void OnTextctrlMoveAlgnYEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_Z
    virtual    void OnTextctrlMoveAlgnZEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_ATOM1
    virtual    void OnTextctrlMoveAlgnAtom1Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_ATOM2
    virtual    void OnTextctrlMoveAlgnAtom2Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_ALGN_OPT
    virtual    void OnChoiceMoveAlgnOptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_NX
    virtual    void OnTextctrlMoveAlgnNxEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_NY
    virtual    void OnTextctrlMoveAlgnNyEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_NZ
    virtual    void OnTextctrlMoveAlgnNzEnter( wxCommandEvent& event );

////@end MoveGUI event handler declarations

////@begin MoveGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end MoveGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin MoveGUI member variables
    wxBoxSizer* p_gridSizer;
    static const wxWindowID ID_TEXTCTRL_MOVE_TRANSL_Z;
    static const wxWindowID ID_TEXTCTRL_MOVE_ALGN_X;
    static const wxWindowID ID_TEXTCTRL_MOVE_ALGN_ATOM;
    static const wxWindowID ID_DIALOG;
    static const wxWindowID ID_TEXTCTRL_MOVE_ALGN_NX;
    static const wxWindowID ID_BUTTON_MOVE_ACTION;
    static const wxWindowID ID_TEXTCTRL_MOVE_ALGN_NY;
    static const wxWindowID ID_PANEL_MOVE_ALGN_LN;
    static const wxWindowID ID_TEXTCTRL_MOVE_ALGN_Z;
    static const wxWindowID ID_TEXTCTRL_MOVE_ROT_ANGL;
    static const wxWindowID ID_BUTTON_MOVE_CNTR_X;
    static const wxWindowID ID_TEXTCTRL_MOVE_ALGN_ATOM1;
    static const wxWindowID ID_PANEL_MOVE_ALGN_PLN;
    static const wxWindowID ID_CHOICE_MOVE_ALGN_LN;
    static const wxWindowID ID_TEXTCTRL_MOVE_ALGN_Y;
    static const wxWindowID ID_BUTTON_MOVE_CNTR_XYZ;
    static const wxWindowID ID_TEXTCTRL_MOVE_TRANSL_X;
    static const wxWindowID ID_BUTTON_MOVE_CNTR_Z;
    static const wxWindowID ID_CHOICE_MOVE_ALGN_OPT;
    static const wxWindowID ID_BUTTON_MOVE_CNTR_Y;
    static const wxWindowID ID_TEXTCTRL_MOVE_TRANSL_Y;
    static const wxWindowID ID_PANEL_MOVE_TRANSLATE;
    static const wxWindowID ID_TEXTCTRL_MOVE_ALGN_ATOM2;
    static const wxWindowID ID_CHOICE_MOVE_ROT_AXS;
    static const wxWindowID ID_PANEL_MOVE_ROTATE;
    static const wxWindowID ID_TEXTCTRL_MOVE_ALGN_NZ;
    static const wxWindowID ID_PANEL_MOVE_CENTER;
    static const wxWindowID ID_CHOICE_MOVE_OPTS;
////@end MoveGUI member variables
};

#endif
    // _MOVEGUI_H_
