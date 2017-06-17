/////////////////////////////////////////////////////////////////////////////
// Name:        ConstraintPanelGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CONSTRAINTPANELGUI_H_
#define _CONSTRAINTPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "ConstraintPanelGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/statline.h"
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
#define SYMBOL_CONSTRAINTPANELGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxSUNKEN_BORDER
#define SYMBOL_CONSTRAINTPANELGUI_TITLE _("ConstraintPanelGUI")
#define SYMBOL_CONSTRAINTPANELGUI_IDNAME ID_DIALOG_CONST
#define SYMBOL_CONSTRAINTPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_CONSTRAINTPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ConstraintPanelGUI class declaration
 */

class ConstraintPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( ConstraintPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ConstraintPanelGUI( );
    ConstraintPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_CONSTRAINTPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_CONSTRAINTPANELGUI_POSITION, const wxSize& size = SYMBOL_CONSTRAINTPANELGUI_SIZE, long style = SYMBOL_CONSTRAINTPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_CONSTRAINTPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_CONSTRAINTPANELGUI_POSITION, const wxSize& size = SYMBOL_CONSTRAINTPANELGUI_SIZE, long style = SYMBOL_CONSTRAINTPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ConstraintPanelGUI event handler declarations
    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_USESHK
    virtual    void OnCheckboxConstUseshkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLVITER
    virtual    void OnTextctrlConstSlviterEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLVTOL
    virtual    void OnTextctrlConstSlvtolEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLUITER
    virtual    void OnTextctrlConstSluiterEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLUTOL
    virtual    void OnTextctrlConstSlutolEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_SHKOPT
    virtual    void OnChoiceConstShkoptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_FIXOPT
    virtual    void OnChoiceConstFixoptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_HONLY
    virtual    void OnCheckboxConstHonlyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_PERM
    virtual    void OnCheckboxConstPermClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_SELOPT
    virtual    void OnChoiceConstSeloptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_GETSEL
    virtual    void OnButtonConstGetselClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_CLEAR
    virtual    void OnButtonConstClearClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_FIXADD
    virtual    void OnButtonConstFixaddClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_FIXDEL
    virtual    void OnButtonConstFixdelClick( wxCommandEvent& event );

////@end ConstraintPanelGUI event handler declarations

////@begin ConstraintPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ConstraintPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ConstraintPanelGUI member variables
    wxBoxSizer* p_constraintVSizer;
    wxPanel* p_constrainResidue;
    wxPanel* p_constrainRadius;
    static const wxWindowID ID_TEXTCTRL_CONST_SLUTOL;
    static const wxWindowID wxID_STATIC_CONST_SLVTOL_UNITS;
    static const wxWindowID wxID_STATIC_CONST_RADIUS;
    static const wxWindowID ID_CHECKBOX_CONST_PERM;
    static const wxWindowID wxID_STATIC_CONST_MAXRES;
    static const wxWindowID ID_TEXTCTRL_CONST_ATCENT;
    static const wxWindowID ID_CHOICE_CONST_FIXOPT;
    static const wxWindowID wxID_STATIC_CONST_RADIUS_UNITS;
    static const wxWindowID ID_PANEL_CONST_SHAKE;
    static const wxWindowID ID_PANEL_CONST_RADIUS;
    static const wxWindowID wxID_STATIC_CONST_SLVITER;
    static const wxWindowID ID_BUTTON_CONST_FIXDEL;
    static const wxWindowID ID_TEXTCTRL_CONST_SLUITER;
    static const wxWindowID wxID_STATIC_CONST_SLUTOL_UNITS;
    static const wxWindowID ID_TEXTCTRL_CONST_SLVTOL;
    static const wxWindowID ID_BUTTON_CONST_CLEAR;
    static const wxWindowID ID_RADIOBUTTON_CONST_RADIN;
    static const wxWindowID ID_TEXTCTRL_CONST_MINRES;
    static const wxWindowID ID_CHECKBOX_CONST_USESHK;
    static const wxWindowID wxID_STATIC_CONST_SLUITER;
    static const wxWindowID ID_BUTTON_CONST_FIXADD;
    static const wxWindowID ID_TEXTCTRL_CONST_SLVITER;
    static const wxWindowID ID_LISTBOX_CONST_FIXAT;
    static const wxWindowID wxID_STATIC_CONST_MINRES;
    static const wxWindowID ID_BUTTON_CONST_GETSEL;
    static const wxWindowID ID_CHECKBOX_CONST_HONLY;
    static const wxWindowID ID_PANEL_CONST_RSDUE;
    static const wxWindowID wxID_STATIC_CONST_SLVTOL;
    static const wxWindowID ID_RADIOBUTTON_CONST_RADOUT;
    static const wxWindowID ID_DIALOG_CONST;
    static const wxWindowID ID_CHOICE_CONST_SELOPT;
    static const wxWindowID ID_CHOICE_CONST_SHKOPT;
    static const wxWindowID ID_TEXTCTRL_CONST_MAXRES;
    static const wxWindowID ID_TEXTCTRL_CONST_RADIUS;
    static const wxWindowID wxID_STATIC_CONST_ATCENT;
    static const wxWindowID wxID_STATIC_CONST_SLUTOL;
////@end ConstraintPanelGUI member variables
};

#endif
    // _CONSTRAINTPANELGUI_H_
