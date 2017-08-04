/////////////////////////////////////////////////////////////////////////////
// Name:        GeomConstraintsGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _GEOMCONSTRAINTSGUI_H_
#define _GEOMCONSTRAINTSGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "GeomConstraintsGUI.C"
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
class wxBoxSizer;
class ewxDialog;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_GEOMCONSTRAINTSGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_GEOMCONSTRAINTSGUI_TITLE _("ECCE Geometry Constraint/Restraint Toolkit")
#define SYMBOL_GEOMCONSTRAINTSGUI_IDNAME ID_DIALOG
#define SYMBOL_GEOMCONSTRAINTSGUI_SIZE wxSize(400, 300)
#define SYMBOL_GEOMCONSTRAINTSGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * GeomConstraintsGUI class declaration
 */

class GeomConstraintsGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( GeomConstraintsGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    GeomConstraintsGUI( );
    GeomConstraintsGUI( wxWindow* parent, wxWindowID id = SYMBOL_GEOMCONSTRAINTSGUI_IDNAME, const wxString& caption = SYMBOL_GEOMCONSTRAINTSGUI_TITLE, const wxPoint& pos = SYMBOL_GEOMCONSTRAINTSGUI_POSITION, const wxSize& size = SYMBOL_GEOMCONSTRAINTSGUI_SIZE, long style = SYMBOL_GEOMCONSTRAINTSGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_GEOMCONSTRAINTSGUI_IDNAME, const wxString& caption = SYMBOL_GEOMCONSTRAINTSGUI_TITLE, const wxPoint& pos = SYMBOL_GEOMCONSTRAINTSGUI_POSITION, const wxSize& size = SYMBOL_GEOMCONSTRAINTSGUI_SIZE, long style = SYMBOL_GEOMCONSTRAINTSGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin GeomConstraintsGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CONSTRAIN_BUTTON
    virtual    void OnConstrainButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_RESTRAIN_BUTTON
    virtual    void OnRestrainButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_FIX_ATOM_BUTTON
    virtual    void OnFixAtomsButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_REMOVE_BUTTON
    virtual    void OnRemoveButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLEAR_ALL_BUTTON
    virtual    void OnClearAllButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
    virtual    void OnCloseButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    virtual    void OnHelpButtonClick( wxCommandEvent& event );

////@end GeomConstraintsGUI event handler declarations

////@begin GeomConstraintsGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end GeomConstraintsGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin GeomConstraintsGUI member variables
    wxBoxSizer* p_allSizers;
    wxStaticBoxSizer* p_bondConstraintSizer;
    wxStaticBoxSizer* p_angleConstraintSizer;
    wxStaticBoxSizer* p_torsionConstraintSizer;
    wxStaticBoxSizer* p_bondRestraintSizer;
    wxStaticBoxSizer* p_angleRestraintSizer;
    wxStaticBoxSizer* p_torsionRestraintSizer;
    wxStaticBoxSizer* p_fixedAtomSizer;
    static const wxWindowID ID_STATICBOXSIZER7;
    static const wxWindowID ID_GEOM_FIX_ATOM_BUTTON;
    static const wxWindowID ID_STATICBOXSIZER4;
    static const wxWindowID ID_GEOM_RESTRAIN_BUTTON;
    static const wxWindowID ID_GEOM_CLEAR_ALL_BUTTON;
    static const wxWindowID ID_STATICBOXSIZER3;
    static const wxWindowID ID_STATICBOXSIZER1;
    static const wxWindowID ID_DIALOG;
    static const wxWindowID ID_GEOM_REMOVE_BUTTON;
    static const wxWindowID ID_STATICBOXSIZER2;
    static const wxWindowID ID_STATICBOXSIZER6;
    static const wxWindowID ID_STATICBOXSIZER5;
    static const wxWindowID ID_GEOM_CONSTRAIN_BUTTON;
////@end GeomConstraintsGUI member variables
};

#endif
    // _GEOMCONSTRAINTSGUI_H_
