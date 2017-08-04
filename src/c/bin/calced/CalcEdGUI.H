/////////////////////////////////////////////////////////////////////////////
// Name:        CalcEdGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CALCEDGUI_H_
#define _CALCEDGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "CalcEdGUI.C"
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
class wxBoxSizer;
class wxFlexGridSizer;
class ewxFrame;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_CALCEDGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_CALCEDGUI_TITLE _("ECCE Electronic Structure Editor")
#define SYMBOL_CALCEDGUI_IDNAME ID_CALCED_FRAME
#define SYMBOL_CALCEDGUI_SIZE wxDefaultSize
#define SYMBOL_CALCEDGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * CalcEdGUI class declaration
 */

class CalcEdGUI: public ewxFrame
{    
    DECLARE_CLASS( CalcEdGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CalcEdGUI( );
    CalcEdGUI( wxWindow* parent, wxWindowID id = SYMBOL_CALCEDGUI_IDNAME, const wxString& caption = SYMBOL_CALCEDGUI_TITLE, const wxPoint& pos = SYMBOL_CALCEDGUI_POSITION, const wxSize& size = SYMBOL_CALCEDGUI_SIZE, long style = SYMBOL_CALCEDGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_CALCEDGUI_IDNAME, const wxString& caption = SYMBOL_CALCEDGUI_TITLE, const wxPoint& pos = SYMBOL_CALCEDGUI_POSITION, const wxSize& size = SYMBOL_CALCEDGUI_SIZE, long style = SYMBOL_CALCEDGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CalcEdGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_CALCED_FRAME
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
    virtual    void OnSaveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_CALCED_SAVE_PREF
    virtual    void OnMenuCalcedSavePrefClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
    virtual    void OnExitClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
    virtual    void OnHelpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK
    virtual    void OnMenuFeedbackClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_CODE
    virtual    void OnButtonCalcedCodeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_CALCED_CHARGE
    virtual    void OnComboboxCalcedChargeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_CALCED_CHARGE
    virtual    void OnComboboxCalcedChargeEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_CALCED_SPIN_MULT
    virtual    void OnComboboxCalcedSpinMultSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_CALCED_SPIN_MULT
    virtual    void OnComboboxCalcedSpinMultEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CALCED_IRREDUCIBLE
    virtual    void OnCheckboxCalcedIrreducibleClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CALCED_NAME
    virtual    void OnTextctrlCalcedNameEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_BASIS_QUICK
    virtual    void OnButtonCalcedBasisQuickClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CALCED_USE_EXPONENTS
    virtual    void OnCheckboxCalcedUseExponentsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CALCED_THEORY
    virtual    void OnChoiceCalcedTheorySelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_THEORY
    virtual    void OnButtonCalcedTheoryClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CALCED_RUNTYPE
    virtual    void OnChoiceCalcedRuntypeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_RUNTYPE
    virtual    void OnButtonCalcedRuntypeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_PARTIAL
    virtual    void OnButtonCalcedPartialClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_CONSTRAINT
    virtual    void OnButtonCalcedConstraintClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_FINAL_EDIT
    virtual    void OnButtonCalcedFinalEditClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_LAUNCH
    virtual    void OnButtonCalcedLaunchClick( wxCommandEvent& event );

////@end CalcEdGUI event handler declarations

////@begin CalcEdGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CalcEdGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CalcEdGUI member variables
    wxBoxSizer* p_codeSizer;
    wxStaticBox* p_detailsBox;
    wxFlexGridSizer* p_theoryDetailsSizer;
    wxFlexGridSizer* p_runtypeDetailsSizer;
    static const wxWindowID ID_BUTTON_CALCED_LAUNCH;
    static const wxWindowID ID_STATICTEXT_CALCED_ECP;
    static const wxWindowID ID_BUTTON_CALCED_RUNTYPE;
    static const wxWindowID ID_STATICTEXT_CALCED_ATOMS;
    static const wxWindowID ID_TEXTCTRL_CALCED_NAME;
    static const wxWindowID ID_COMBOBOX_CALCED_SPIN_MULT;
    static const wxWindowID ID_CHOICE_CALCED_THEORY;
    static const wxWindowID ID_MENU_CALCED_SAVE_PREF;
    static const wxWindowID ID_BUTTON_CALCED_BUILDER;
    static const wxWindowID ID_LABEL_CALCED_XC;
    static const wxWindowID ID_COMBOBOX_CALCED_CHARGE;
    static const wxWindowID ID_LABEL_CALCED_ECP;
    static const wxWindowID ID_STATICTEXT_CALCED_BASIS_NAME;
    static const wxWindowID ID_STATICTEXT_CALCED_PRIMITIVES;
    static const wxWindowID ID_STATIC_CALCED_CHARGE;
    static const wxWindowID ID_BUTTON_CALCED_CODE;
    static const wxWindowID ID_STATICTEXT_CALCED_ELECTRONS;
    static const wxWindowID ID_STATIC_CALCED_SPIN_MULT;
    static const wxWindowID ID_BUTTON_CALCED_FINAL_EDIT;
    static const wxWindowID ID_STATICTEXT_CALCED_OPEN_SHELLS;
    static const wxWindowID ID_STATICTEXT_CALCED_FUNCTIONS;
    static const wxWindowID ID_STATICTEXT_CALCED_FORMULA;
    static const wxWindowID ID_STATICTEXT_CALCED_SYMMETRY;
    static const wxWindowID ID_STATICTEXT_CALCED_POLARIZATION;
    static const wxWindowID ID_BUTTON_CALCED_BASIS_SET;
    static const wxWindowID ID_CHECKBOX_CALCED_IRREDUCIBLE;
    static const wxWindowID ID_BUTTON_CALCED_THEORY;
    static const wxWindowID ID_STATICTEXT_CALCED_CD;
    static const wxWindowID ID_CHECKBOX_CALCED_USE_EXPONENTS;
    static const wxWindowID ID_BUTTON_CALCED_PARTIAL;
    static const wxWindowID ID_BUTTON_CALCED_BASIS_QUICK;
    static const wxWindowID ID_MENU_FEEDBACK;
    static const wxWindowID ID_CHOICE_CALCED_RUNTYPE;
    static const wxWindowID ID_BUTTON_CALCED_CONSTRAINT;
    static const wxWindowID ID_LABEL_CALCED_CD;
    static const wxWindowID ID_CALCED_FRAME;
    static const wxWindowID ID_STATICTEXT_CALCED_XC;
////@end CalcEdGUI member variables
};

#endif
    // _CALCEDGUI_H_
