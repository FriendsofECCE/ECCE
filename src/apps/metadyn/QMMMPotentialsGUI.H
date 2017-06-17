/////////////////////////////////////////////////////////////////////////////
// Name:        QMMMPotentialsGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _QMMMPOTENTIALSGUI_H_
#define _QMMMPOTENTIALSGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "QMMMPotentialsGUI.C"
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
#define SYMBOL_QMMMPOTENTIALSGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_QMMMPOTENTIALSGUI_TITLE _("ECCE Atomic Parameters Toolkit")
#define SYMBOL_QMMMPOTENTIALSGUI_IDNAME ID_DIALOG
#define SYMBOL_QMMMPOTENTIALSGUI_SIZE wxSize(400, 300)
#define SYMBOL_QMMMPOTENTIALSGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * QMMMPotentialsGUI class declaration
 */

class QMMMPotentialsGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( QMMMPotentialsGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    QMMMPotentialsGUI( );
    QMMMPotentialsGUI( wxWindow* parent, wxWindowID id = SYMBOL_QMMMPOTENTIALSGUI_IDNAME, const wxString& caption = SYMBOL_QMMMPOTENTIALSGUI_TITLE, const wxPoint& pos = SYMBOL_QMMMPOTENTIALSGUI_POSITION, const wxSize& size = SYMBOL_QMMMPOTENTIALSGUI_SIZE, long style = SYMBOL_QMMMPOTENTIALSGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_QMMMPOTENTIALSGUI_IDNAME, const wxString& caption = SYMBOL_QMMMPOTENTIALSGUI_TITLE, const wxPoint& pos = SYMBOL_QMMMPOTENTIALSGUI_POSITION, const wxSize& size = SYMBOL_QMMMPOTENTIALSGUI_SIZE, long style = SYMBOL_QMMMPOTENTIALSGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin QMMMPotentialsGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATMT_CLEAR_ALL_BUTTON
    virtual    void OnClearAllButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_ATMT_USE_SHAKE_CHECKBOX
    virtual    void OnAtmtUseShakeCheckboxClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
    virtual    void OnCloseButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    virtual    void OnHelpButtonClick( wxCommandEvent& event );

////@end QMMMPotentialsGUI event handler declarations

////@begin QMMMPotentialsGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end QMMMPotentialsGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin QMMMPotentialsGUI member variables
    wxBoxSizer* p_allSizers;
    wxStaticBoxSizer* p_soluteAtomsSizer;
    wxStaticBoxSizer* p_solventAtomsSizer;
    wxStaticBoxSizer* p_solventPseudPotSizer;
    wxStaticBoxSizer* p_bondStretchSizer;
    wxStaticBoxSizer* p_angleBendSizer;
    static const wxWindowID ID_ATMT_USE_SHAKE_CHECKBOX;
    static const wxWindowID ID_STATICBOXSIZER3;
    static const wxWindowID ID_STATICBOXSIZER1;
    static const wxWindowID ID_DIALOG;
    static const wxWindowID ID_STATICBOXSIZER2;
    static const wxWindowID ID_ATMT_CLEAR_ALL_BUTTON;
    static const wxWindowID ID_STATICBOXSIZER6;
    static const wxWindowID ID_STATICBOXSIZER5;
////@end QMMMPotentialsGUI member variables
};

#endif
    // _QMMMPOTENTIALSGUI_H_
