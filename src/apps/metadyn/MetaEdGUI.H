/////////////////////////////////////////////////////////////////////////////
// Name:        MetaEdGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _METAEDGUI_H_
#define _METAEDGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "MetaEdGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wx/statline.h"
#include "wxgui/ewxFrame.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxFlexGridSizer;
class ewxFrame;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_METAEDGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_METAEDGUI_TITLE _("ECCE Metadynamics Editor")
#define SYMBOL_METAEDGUI_IDNAME ID_METAED_FRAME
#define SYMBOL_METAEDGUI_SIZE wxDefaultSize
#define SYMBOL_METAEDGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * MetaEdGUI class declaration
 */

class MetaEdGUI: public ewxFrame
{    
    DECLARE_CLASS( MetaEdGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    MetaEdGUI( );
    MetaEdGUI( wxWindow* parent, wxWindowID id = SYMBOL_METAEDGUI_IDNAME, const wxString& caption = SYMBOL_METAEDGUI_TITLE, const wxPoint& pos = SYMBOL_METAEDGUI_POSITION, const wxSize& size = SYMBOL_METAEDGUI_SIZE, long style = SYMBOL_METAEDGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_METAEDGUI_IDNAME, const wxString& caption = SYMBOL_METAEDGUI_TITLE, const wxPoint& pos = SYMBOL_METAEDGUI_POSITION, const wxSize& size = SYMBOL_METAEDGUI_SIZE, long style = SYMBOL_METAEDGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin MetaEdGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_METAED_FRAME
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
    virtual    void OnSaveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
    virtual    void OnExitClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
    virtual    void OnHelpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK
    virtual    void OnMenuFeedbackClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_METAED_CHARGE
    virtual    void OnComboboxMetaedChargeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_METAED_CHARGE
    virtual    void OnComboboxMetaedChargeEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_METAED_SPIN_MULT
    virtual    void OnComboboxMetaedSpinMultSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_METAED_SPIN_MULT
    virtual    void OnComboboxMetaedSpinMultEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_METAED_IRREDUCIBLE
    virtual    void OnCheckboxMetaedIrreducibleClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_THEORY
    virtual    void OnButtonMetaedTheoryClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_RUNTYPE
    virtual    void OnButtonMetaedRuntypeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_CONSTRAINT
    virtual    void OnButtonMetaedPotentialClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_QMMM
    virtual    void OnButtonMetaedQmmmClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_FINAL_EDIT
    virtual    void OnButtonMetaedFinalEditClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_LAUNCH
    virtual    void OnButtonMetaedLaunchClick( wxCommandEvent& event );

////@end MetaEdGUI event handler declarations

////@begin MetaEdGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end MetaEdGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin MetaEdGUI member variables
    wxStaticBox* p_detailsBox;
    wxFlexGridSizer* p_theoryDetailsSizer;
    wxFlexGridSizer* p_runtypeDetailsSizer;
    static const wxWindowID ID_BUTTON_METAED_QMMM;
    static const wxWindowID ID_STATICTEXT_METAED_ELECTRONS;
    static const wxWindowID ID_BUTTON_METAED_THEORY;
    static const wxWindowID ID_COMBOBOX_METAED_CHARGE;
    static const wxWindowID ID_COMBOBOX_METAED_SPIN_MULT;
    static const wxWindowID ID_STATICTEXT_METAED_ATOMS;
    static const wxWindowID ID_METAED_FRAME;
    static const wxWindowID ID_BUTTON_METAED_RUNTYPE;
    static const wxWindowID ID_CHECKBOX_METAED_IRREDUCIBLE;
    static const wxWindowID ID_BUTTON_METAED_BUILDER;
    static const wxWindowID ID_STATIC_METAED_SPIN_MULT;
    static const wxWindowID ID_STATIC_METAED_CHARGE;
    static const wxWindowID ID_MENU_FEEDBACK;
    static const wxWindowID ID_BUTTON_METAED_FINAL_EDIT;
    static const wxWindowID ID_STATICTEXT_METAED_OPEN_SHELLS;
    static const wxWindowID ID_BUTTON_METAED_LAUNCH;
    static const wxWindowID ID_BUTTON_METAED_CONSTRAINT;
    static const wxWindowID ID_STATICTEXT_METAED_SYMMETRY;
////@end MetaEdGUI member variables
};

#endif
    // _METAEDGUI_H_
