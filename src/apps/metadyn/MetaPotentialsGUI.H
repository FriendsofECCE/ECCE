/////////////////////////////////////////////////////////////////////////////
// Name:        MetaPotentialsGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _METAPOTENTIALSGUI_H_
#define _METAPOTENTIALSGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "MetaPotentialsGUI.C"
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
#define SYMBOL_METAPOTENTIALSGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_METAPOTENTIALSGUI_TITLE _("ECCE Metadynamics Potential Toolkit")
#define SYMBOL_METAPOTENTIALSGUI_IDNAME ID_DIALOG
#define SYMBOL_METAPOTENTIALSGUI_SIZE wxSize(400, 300)
#define SYMBOL_METAPOTENTIALSGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * MetaPotentialsGUI class declaration
 */

class MetaPotentialsGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( MetaPotentialsGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    MetaPotentialsGUI( );
    MetaPotentialsGUI( wxWindow* parent, wxWindowID id = SYMBOL_METAPOTENTIALSGUI_IDNAME, const wxString& caption = SYMBOL_METAPOTENTIALSGUI_TITLE, const wxPoint& pos = SYMBOL_METAPOTENTIALSGUI_POSITION, const wxSize& size = SYMBOL_METAPOTENTIALSGUI_SIZE, long style = SYMBOL_METAPOTENTIALSGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_METAPOTENTIALSGUI_IDNAME, const wxString& caption = SYMBOL_METAPOTENTIALSGUI_TITLE, const wxPoint& pos = SYMBOL_METAPOTENTIALSGUI_POSITION, const wxSize& size = SYMBOL_METAPOTENTIALSGUI_SIZE, long style = SYMBOL_METAPOTENTIALSGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin MetaPotentialsGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_METADYNAMICS_POTENTIAL_BUTTON
    virtual    void OnPotentialButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_REMOVE_BUTTON
    virtual    void OnRemoveButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLEAR_ALL_BUTTON
    virtual    void OnClearAllButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
    virtual    void OnCloseButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    virtual    void OnHelpButtonClick( wxCommandEvent& event );

////@end MetaPotentialsGUI event handler declarations

////@begin MetaPotentialsGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end MetaPotentialsGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin MetaPotentialsGUI member variables
    wxBoxSizer* p_allSizers;
    wxStaticBoxSizer* p_bondPotentialSizer;
    wxStaticBoxSizer* p_anglePotentialSizer;
    static const wxWindowID ID_DIALOG;
    static const wxWindowID ID_GEOM_REMOVE_BUTTON;
    static const wxWindowID ID_STATICBOXSIZER2;
    static const wxWindowID ID_METADYNAMICS_POTENTIAL_BUTTON;
    static const wxWindowID ID_GEOM_CLEAR_ALL_BUTTON;
    static const wxWindowID ID_STATICBOXSIZER3;
////@end MetaPotentialsGUI member variables
};

#endif
    // _METAPOTENTIALSGUI_H_
