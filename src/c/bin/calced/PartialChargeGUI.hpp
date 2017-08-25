/////////////////////////////////////////////////////////////////////////////
// Name:        PartialChargeGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PARTIALCHARGEGUI_H_
#define _PARTIALCHARGEGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "PartialChargeGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/statline.h"
#include "wxgui/ewxDialog.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxDialog;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_PARTIALCHARGEGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_PARTIALCHARGEGUI_TITLE _("Partial Charge Editor")
#define SYMBOL_PARTIALCHARGEGUI_IDNAME ID_DIALOG
#define SYMBOL_PARTIALCHARGEGUI_SIZE wxSize(400, 300)
#define SYMBOL_PARTIALCHARGEGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * PartialChargeGUI class declaration
 */

class PartialChargeGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( PartialChargeGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    PartialChargeGUI( );
    PartialChargeGUI( wxWindow* parent, wxWindowID id = SYMBOL_PARTIALCHARGEGUI_IDNAME, const wxString& caption = SYMBOL_PARTIALCHARGEGUI_TITLE, const wxPoint& pos = SYMBOL_PARTIALCHARGEGUI_POSITION, const wxSize& size = SYMBOL_PARTIALCHARGEGUI_SIZE, long style = SYMBOL_PARTIALCHARGEGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_PARTIALCHARGEGUI_IDNAME, const wxString& caption = SYMBOL_PARTIALCHARGEGUI_TITLE, const wxPoint& pos = SYMBOL_PARTIALCHARGEGUI_POSITION, const wxSize& size = SYMBOL_PARTIALCHARGEGUI_SIZE, long style = SYMBOL_PARTIALCHARGEGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin PartialChargeGUI event handler declarations
    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONSTRAIN_ALL_METHYL_GROUPS
    virtual    void OnButtonConstrainAllMethylGroupsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONSTRAIN_ALL_HYDROGENS
    virtual    void OnButtonConstrainAllHydrogensClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONSTRAINTS
    virtual    void OnChoiceConstraintsSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_GROUP1
    virtual    void OnButtonGroup1Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_GROUP2
    virtual    void OnButtonGroup2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ADD_CMD
    virtual    void OnButtonAddCmdClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DELETE_CMD
    virtual    void OnButtonDeleteCmdClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_CMDLIST
    virtual    void OnListboxCmdlistSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SHOW_UNCONSTRAINED_ATOMS
    virtual    void OnButtonShowUnconstrainedAtomsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SHOW_MULTIPLY_CONSTRAINED_ATOMS
    virtual    void OnButtonShowMultiplyConstrainedAtomsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
    virtual    void OnCloseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    virtual    void OnHelpClick( wxCommandEvent& event );

////@end PartialChargeGUI event handler declarations

////@begin PartialChargeGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end PartialChargeGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin PartialChargeGUI member variables
    static const wxWindowID ID_TEXTCTRL_GROUP2;
    static const wxWindowID ID_BUTTON_CONSTRAIN_ALL_METHYL_GROUPS;
    static const wxWindowID ID_BUTTON_SHOW_MULTIPLY_CONSTRAINED_ATOMS;
    static const wxWindowID ID_BUTTON_GROUP1;
    static const wxWindowID ID_DIALOG;
    static const wxWindowID ID_LISTBOX_CMDLIST;
    static const wxWindowID ID_GROUP2;
    static const wxWindowID ID_TEXTCTRL_FIX_VALUE;
    static const wxWindowID ID_BUTTON_DELETE_CMD;
    static const wxWindowID ID_TOTAL_CHARGE;
    static const wxWindowID ID_BUTTON_CONSTRAIN_ALL_HYDROGENS;
    static const wxWindowID ID_BUTTON_SHOW_UNCONSTRAINED_ATOMS;
    static const wxWindowID ID_CHOICE_CONSTRAINTS;
    static const wxWindowID ID_GROUP1;
    static const wxWindowID ID_FIXED_CHARGE;
    static const wxWindowID ID_TEXTCTRL_GROUP1;
    static const wxWindowID ID_BUTTON_ADD_CMD;
    static const wxWindowID ID_BUTTON_GROUP2;
    static const wxWindowID ID_TEXTCTRL_TOTAL_CHRG;
////@end PartialChargeGUI member variables
};

#endif
    // _PARTIALCHARGEGUI_H_
