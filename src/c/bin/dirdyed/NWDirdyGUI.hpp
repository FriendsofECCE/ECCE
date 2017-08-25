/////////////////////////////////////////////////////////////////////////////
// Name:        NWDirdyGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NWDIRDYGUI_H_
#define _NWDIRDYGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "NWDirdyGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wx/statline.h"
#include "wxgui/WxFeedback.H"
#include "wxgui/ewxFrame.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class ewxFrame;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_NWDIRDYGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_NWDIRDYGUI_TITLE _("ECCE NWChem DirDyVTST Editor")
#define SYMBOL_NWDIRDYGUI_IDNAME ID_NWDIRDY_FRAME
#define SYMBOL_NWDIRDYGUI_SIZE wxDefaultSize
#define SYMBOL_NWDIRDYGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * NWDirdyGUI class declaration
 */

class NWDirdyGUI: public ewxFrame
{    
    DECLARE_CLASS( NWDirdyGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NWDirdyGUI( );
    NWDirdyGUI( wxWindow* parent, wxWindowID id = SYMBOL_NWDIRDYGUI_IDNAME, const wxString& caption = SYMBOL_NWDIRDYGUI_TITLE, const wxPoint& pos = SYMBOL_NWDIRDYGUI_POSITION, const wxSize& size = SYMBOL_NWDIRDYGUI_SIZE, long style = SYMBOL_NWDIRDYGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_NWDIRDYGUI_IDNAME, const wxString& caption = SYMBOL_NWDIRDYGUI_TITLE, const wxPoint& pos = SYMBOL_NWDIRDYGUI_POSITION, const wxSize& size = SYMBOL_NWDIRDYGUI_SIZE, long style = SYMBOL_NWDIRDYGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NWDirdyGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_NWDIRDY_FRAME
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_R1
    virtual    void OnBitmapbuttonDirdyBuildR1Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_R1
    virtual    void OnComboboxDirdyChargeR1Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_R1
    virtual    void OnComboboxDirdyChargeR1Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_R1
    virtual    void OnChoiceDirdySpinR1Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_R1
    virtual    void OnButtonDirdyCalcR1Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_R2
    virtual    void OnBitmapbuttonDirdyBuildR2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_R2
    virtual    void OnComboboxDirdyChargeR2Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_R2
    virtual    void OnComboboxDirdyChargeR2Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_R2
    virtual    void OnChoiceDirdySpinR2Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_R2
    virtual    void OnButtonDirdyCalcR2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_TS
    virtual    void OnBitmapbuttonDirdyBuildTsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_TS
    virtual    void OnComboboxDirdyChargeTsSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_TS
    virtual    void OnComboboxDirdyChargeTsEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_TS
    virtual    void OnChoiceDirdySpinTsSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_TS
    virtual    void OnButtonDirdyCalcTsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CREATE_REACT
    virtual    void OnButtonDirdyCreateReactClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CREATE_PROD
    virtual    void OnButtonDirdyCreateProdClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_P1
    virtual    void OnBitmapbuttonDirdyBuildP1Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_P1
    virtual    void OnComboboxDirdyChargeP1Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_P1
    virtual    void OnComboboxDirdyChargeP1Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_P1
    virtual    void OnChoiceDirdySpinP1Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_P1
    virtual    void OnButtonDirdyCalcP1Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_P2
    virtual    void OnBitmapbuttonDirdyBuildP2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_P2
    virtual    void OnComboboxDirdyChargeP2Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_P2
    virtual    void OnComboboxDirdyChargeP2Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_P2
    virtual    void OnChoiceDirdySpinP2Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_P2
    virtual    void OnButtonDirdyCalcP2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDIRDY_SETUP
    virtual    void OnButtonNwdirdySetupClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDIRDY_FINAL_EDIT
    virtual    void OnButtonNwdirdyFinalEditClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDIRDY_LAUNCH
    virtual    void OnButtonNwdirdyLaunchClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
    virtual    void OnSaveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
    virtual    void OnExitClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK
    virtual    void OnMenuFeedbackClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_MDPREP_HELP
    virtual    void OnMenuMdprepHelpClick( wxCommandEvent& event );

////@end NWDirdyGUI event handler declarations

////@begin NWDirdyGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end NWDirdyGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NWDirdyGUI member variables
    wxBoxSizer* p_codeSizerR1;
    wxBoxSizer* p_calcedSizerR1;
    wxBoxSizer* p_codeSizerR2;
    wxBoxSizer* p_calcedSizerR2;
    wxBoxSizer* p_codeSizerTS;
    wxBoxSizer* p_calcedSizerTS;
    wxBoxSizer* p_codeSizerP1;
    wxBoxSizer* p_calcedSizerP1;
    wxBoxSizer* p_codeSizerP2;
    wxBoxSizer* p_calcedSizerP2;
    wxBoxSizer* p_feedbackSizer;
    static const wxWindowID ID_COMBOBOX_DIRDY_CHARGE_R2;
    static const wxWindowID ID_BITMAPBUTTON_DIRDY_BUILD_P2;
    static const wxWindowID ID_COMBOBOX_DIRDY_CHARGE_TS;
    static const wxWindowID ID_CHOICE_DIRDY_SPIN_R1;
    static const wxWindowID ID_BITMAPBUTTON_DIRDY_BUILD_R2;
    static const wxWindowID ID_STATIC_DIRDY_SPIN_R1;
    static const wxWindowID ID_BUTTON_DIRDY_CALC_P1;
    static const wxWindowID ID_STATIC_DIRDY_CODE_CURRENT_R1;
    static const wxWindowID ID_STATIC_DIRDY_FORMULA_R1;
    static const wxWindowID ID_BUTTON_DIRDY_CREATE_PROD;
    static const wxWindowID ID_STATIC_DIRDY_FORMULA_LABEL_TS;
    static const wxWindowID ID_STATIC_DIRDY_CODE_CURRENT_R2;
    static const wxWindowID ID_STATIC_DIRDY_FORMULA_R2;
    static const wxWindowID ID_BUTTON_NWDIRDY_SETUP;
    static const wxWindowID ID_BITMAPBUTTON_DIRDY_BUILD_P1;
    static const wxWindowID ID_COMBOBOX_DIRDY_CHARGE_R1;
    static const wxWindowID ID_STATIC_DIRDY_CODE_NONE_R1;
    static const wxWindowID ID_STATIC_DIRDY_CODE_NONE_TS;
    static const wxWindowID ID_STATIC_DIRDY_SPIN_R2;
    static const wxWindowID ID_CHOICE_DIRDY_SPIN_TS;
    static const wxWindowID ID_BUTTON_NWDIRDY_LAUNCH;
    static const wxWindowID ID_STATIC_DIRDY_CODE_CURRENT_P2;
    static const wxWindowID ID_STATIC_DIRDY_CHARGE_TS;
    static const wxWindowID ID_STATIC_DIRDY_FORMULA_TS;
    static const wxWindowID ID_BUTTON_DIRDY_CALC_TS;
    static const wxWindowID ID_STATIC_DIRDY_CHARGE_R2;
    static const wxWindowID ID_STATIC_DIRDY_CHARGE_P1;
    static const wxWindowID ID_STATIC_DIRDY_FORMULA_P1;
    static const wxWindowID ID_BUTTON_DIRDY_CALC_R2;
    static const wxWindowID ID_COMBOBOX_DIRDY_CHARGE_P2;
    static const wxWindowID ID_CHOICE_DIRDY_SPIN_P1;
    static const wxWindowID ID_BUTTON_NWDIRDY_FINAL_EDIT;
    static const wxWindowID ID_STATIC_DIRDY_CODE_CURRENT_P1;
    static const wxWindowID ID_CHOICE_DIRDY_SPIN_R2;
    static const wxWindowID ID_STATIC_DIRDY_CODE_NONE_R2;
    static const wxWindowID ID_STATIC_DIRDY_SPIN_P1;
    static const wxWindowID ID_BITMAPBUTTON_DIRDY_BUILD_TS;
    static const wxWindowID ID_MENU_MDPREP_HELP;
    static const wxWindowID ID_STATIC_DIRDY_FORMULA_LABEL_R1;
    static const wxWindowID ID_STATIC_DIRDY_SPIN_P2;
    static const wxWindowID ID_STATIC_DIRDY_BARRIER_TS;
    static const wxWindowID ID_CHOICE_DIRDY_SPIN_P2;
    static const wxWindowID ID_NWDIRDY_FRAME;
    static const wxWindowID ID_STATIC_DIRDY_CODE_NONE_P2;
    static const wxWindowID ID_BUTTON_DIRDY_CREATE_REACT;
    static const wxWindowID ID_STATIC_DIRDY_CHARGE_R1;
    static const wxWindowID ID_STATIC_DIRDY_CODE_CURRENT_TS;
    static const wxWindowID ID_STATIC_DIRDY_CHARGE_P2;
    static const wxWindowID ID_STATIC_DIRDY_CODE_NONE_P1;
    static const wxWindowID ID_STATIC_DIRDY_SPIN_TS;
    static const wxWindowID ID_STATIC_DIRDY_FORMULA_LABEL_P2;
    static const wxWindowID ID_MENU_FEEDBACK;
    static const wxWindowID ID_FEEDBACK;
    static const wxWindowID ID_BUTTON_DIRDY_CALC_P2;
    static const wxWindowID ID_STATIC_DIRDY_FORMULA_LABEL_R2;
    static const wxWindowID ID_STATIC_DIRDY_FORMULA_P2;
    static const wxWindowID ID_COMBOBOX_DIRDY_CHARGE_P1;
    static const wxWindowID ID_BUTTON_DIRDY_CALC_R1;
    static const wxWindowID ID_STATIC_DIRDY_BARRIER_LABEL_TS;
    static const wxWindowID ID_STATIC_DIRDY_FORMULA_LABEL_P1;
    static const wxWindowID ID_BITMAPBUTTON_DIRDY_BUILD_R1;
////@end NWDirdyGUI member variables
};

#endif
    // _NWDIRDYGUI_H_
