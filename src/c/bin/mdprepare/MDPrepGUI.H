/////////////////////////////////////////////////////////////////////////////
// Name:        MDPrepGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _MDPREPGUI_H_
#define _MDPREPGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "MDPrepGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wx/notebook.h"
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
#define SYMBOL_MDPREPGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_MDPREPGUI_TITLE _("ECCE NWChem MD Prepare")
#define SYMBOL_MDPREPGUI_IDNAME ID_MDPREP_FRAME
#define SYMBOL_MDPREPGUI_SIZE wxDefaultSize
#define SYMBOL_MDPREPGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * MDPrepGUI class declaration
 */

class MDPrepGUI: public ewxFrame
{    
    DECLARE_CLASS( MDPrepGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    MDPrepGUI( );
    MDPrepGUI( wxWindow* parent, wxWindowID id = SYMBOL_MDPREPGUI_IDNAME, const wxString& caption = SYMBOL_MDPREPGUI_TITLE, const wxPoint& pos = SYMBOL_MDPREPGUI_POSITION, const wxSize& size = SYMBOL_MDPREPGUI_SIZE, long style = SYMBOL_MDPREPGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_MDPREPGUI_IDNAME, const wxString& caption = SYMBOL_MDPREPGUI_TITLE, const wxPoint& pos = SYMBOL_MDPREPGUI_POSITION, const wxSize& size = SYMBOL_MDPREPGUI_SIZE, long style = SYMBOL_MDPREPGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin MDPrepGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_MDPREP_FRAME
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CHEMSYS
    virtual    void OnButtonMdprepChemsysClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CONFIGURE
    virtual    void OnButtonMdprepConfigureClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_HID
    virtual    void OnRadiobuttonMdprepHidSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_HIE
    virtual    void OnRadiobuttonMdprepHieSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_HIP
    virtual    void OnRadiobuttonMdprepHipSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_SOLVENTS
    virtual    void OnChoiceMdprepSolventsSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SOL_NAM
    virtual    void OnTextctrlMdprepSolNamEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SOL_MOD
    virtual    void OnTextctrlMdprepSolModEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_MDPREP_CINAME
    virtual    void OnComboboxMdprepCinameSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_MDPREP_CINAME
    virtual    void OnComboboxMdprepCinameEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CINUM
    virtual    void OnTextctrlMdprepCinumEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_GETCHARGE
    virtual    void OnButtonMdprepGetchargeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CISCALE
    virtual    void OnTextctrlMdprepCiscaleEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CIGRIDSZ
    virtual    void OnTextctrlMdprepCigridszEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_CIMINDIS
    virtual    void OnTextctrlMdprepCimindisEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CIADD
    virtual    void OnButtonMdprepCiaddClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CIDEL
    virtual    void OnButtonMdprepCidelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC1
    virtual    void OnTextctrlMdprepFrac1Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC2
    virtual    void OnTextctrlMdprepFrac2Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC3
    virtual    void OnTextctrlMdprepFrac3Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC4
    virtual    void OnTextctrlMdprepFrac4Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC5
    virtual    void OnTextctrlMdprepFrac5Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC6
    virtual    void OnTextctrlMdprepFrac6Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC7
    virtual    void OnTextctrlMdprepFrac7Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC8
    virtual    void OnTextctrlMdprepFrac8Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC9
    virtual    void OnTextctrlMdprepFrac9Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_FRAC10
    virtual    void OnTextctrlMdprepFrac10Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LNK_SELECT
    virtual    void OnButtonMdprepLnkSelectClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LNK_ADD
    virtual    void OnButtonMdprepLnkAddClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LNK_DELETE
    virtual    void OnButtonMdprepLnkDeleteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTER
    virtual    void OnButtonMdprepCenterClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTERX
    virtual    void OnButtonMdprepCenterxClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTERY
    virtual    void OnButtonMdprepCenteryClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_CENTERZ
    virtual    void OnButtonMdprepCenterzClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_TRNS_MOL
    virtual    void OnRadiobuttonMdprepTrnsMolSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_TRNS_SEG
    virtual    void OnRadiobuttonMdprepTrnsSegSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_TRNS_ATOM
    virtual    void OnRadiobuttonMdprepTrnsAtomSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_TRNS_X
    virtual    void OnTextctrlMdprepTrnsXEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_TRNS_Y
    virtual    void OnTextctrlMdprepTrnsYEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_TRNS_Z
    virtual    void OnTextctrlMdprepTrnsZEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_TRNS_SOL
    virtual    void OnButtonMdprepTrnsSolClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ORIENT
    virtual    void OnButtonMdprepOrientClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ALIGN
    virtual    void OnButtonMdprepAlignClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ORNTSL
    virtual    void OnButtonMdprepOrntslClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_ORNT_MOL
    virtual    void OnRadiobuttonMdprepOrntMolSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_ORNT_SEG
    virtual    void OnRadiobuttonMdprepOrntSegSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_MDPREP_ORNT_ATOM
    virtual    void OnRadiobuttonMdprepOrntAtomSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_ANGLE
    virtual    void OnTextctrlMdprepOrntAngleEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_X
    virtual    void OnTextctrlMdprepOrntXEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_Y
    virtual    void OnTextctrlMdprepOrntYEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ORNT_Z
    virtual    void OnTextctrlMdprepOrntZEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_ROTATE
    virtual    void OnButtonMdprepRotateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SETX
    virtual    void OnTextctrlMdprepSetxEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SETY
    virtual    void OnTextctrlMdprepSetyEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SETZ
    virtual    void OnTextctrlMdprepSetzEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SETDIM
    virtual    void OnButtonMdprepSetdimClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SETBC
    virtual    void OnButtonMdprepSetbcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_BCOPT
    virtual    void OnChoiceMdprepBcoptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_SLV_GEOM
    virtual    void OnChoiceMdprepSlvGeomSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SLV_MINDIST
    virtual    void OnTextctrlMdprepSlvMindistEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SOLVATE
    virtual    void OnButtonMdprepSolvateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SET_MINSLV
    virtual    void OnButtonMdprepSetMinslvClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MINSOLV
    virtual    void OnTextctrlMdprepMinsolvEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SET_EXPND
    virtual    void OnButtonMdprepSetExpndClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_EXPND
    virtual    void OnTextctrlMdprepExpndEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SET_SLVTOL
    virtual    void OnButtonMdprepSetSlvtolClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SLVTOL
    virtual    void OnTextctrlMdprepSlvtolEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_RMV_OPT
    virtual    void OnChoiceMdprepRmvOptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_XRANGE
    virtual    void OnCheckboxMdprepXrangeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_YRANGE
    virtual    void OnCheckboxMdprepYrangeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_ZRANGE
    virtual    void OnCheckboxMdprepZrangeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SLV_RMV
    virtual    void OnButtonMdprepSlvRmvClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_REPOPTS
    virtual    void OnChoiceMdprepRepoptsSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_XREP
    virtual    void OnTextctrlMdprepXrepEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_YREP
    virtual    void OnTextctrlMdprepYrepEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_ZREP
    virtual    void OnTextctrlMdprepZrepEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MINRSEP
    virtual    void OnTextctrlMdprepMinrsepEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_RANX
    virtual    void OnCheckboxMdprepRanxClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_RANY
    virtual    void OnCheckboxMdprepRanyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_RANZ
    virtual    void OnCheckboxMdprepRanzClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_INVCPY
    virtual    void OnCheckboxMdprepInvcpyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_USEZBOX
    virtual    void OnCheckboxMdprepUsezboxClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_REPLIC
    virtual    void OnButtonMdprepReplicClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SKIPX
    virtual    void OnTextctrlMdprepSkipxEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SKIPY
    virtual    void OnTextctrlMdprepSkipyEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_SKIPZ
    virtual    void OnTextctrlMdprepSkipzEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_SKIP
    virtual    void OnButtonMdprepSkipClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_UP
    virtual    void OnButtonMdprepUpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_DOWN
    virtual    void OnButtonMdprepDownClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_DELETE
    virtual    void OnButtonMdprepDeleteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PMF_OPTION
    virtual    void OnChoicePmfOptionSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS1
    virtual    void OnTextctrlPmfSelectAtoms1Updated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS1
    virtual    void OnButtonPmfSelectAtoms1Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS1
    virtual    void OnButtonPmfShowAtoms1Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS2
    virtual    void OnTextctrlPmfSelectAtoms2Updated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS2
    virtual    void OnButtonPmfSelectAtoms2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS2
    virtual    void OnButtonPmfShowAtoms2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS3
    virtual    void OnTextctrlPmfSelectAtoms3Updated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS3
    virtual    void OnButtonPmfSelectAtoms3Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS3
    virtual    void OnButtonPmfShowAtoms3Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PMF_SELECT_ATOMS4
    virtual    void OnTextctrlPmfSelectAtoms4Updated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SELECT_ATOMS4
    virtual    void OnButtonPmfSelectAtoms4Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_SHOW_ATOMS4
    virtual    void OnButtonPmfShowAtoms4Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DEFAULTS_DISTANCE
    virtual    void OnButtonPmfDefaultsDistanceClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DEFAULTS_ANGLE
    virtual    void OnButtonPmfDefaultsAngleClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_ADD_DIRECTIVE
    virtual    void OnButtonPmfAddDirectiveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PMF_DIRECTIVES
    virtual    void OnListboxPmfDirectivesSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_UP
    virtual    void OnButtonPmfUpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DOWN
    virtual    void OnButtonPmfDownClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PMF_DELETE
    virtual    void OnButtonPmfDeleteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_MDPREP_SETMEM
    virtual    void OnCheckboxMdprepSetmemClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MEMHEAP
    virtual    void OnTextctrlMdprepMemheapEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MEMSTK
    virtual    void OnTextctrlMdprepMemstkEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MDPREP_MEMGLBL
    virtual    void OnTextctrlMdprepMemglblEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MDPREP_PRTLEV
    virtual    void OnChoiceMdprepPrtlevSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_BUILDER
    virtual    void OnButtonMdprepBuilderClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_FINAL_EDIT
    virtual    void OnButtonMdprepFinalEditClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_LOCAL
    virtual    void OnButtonMdprepLocalClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_REMOTE
    virtual    void OnButtonMdprepRemoteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MDPREP_RESET
    virtual    void OnButtonMdprepResetClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
    virtual    void OnSaveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
    virtual    void OnExitClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_MDPREP_HELP
    virtual    void OnMenuMdprepHelpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK
    virtual    void OnMenuFeedbackClick( wxCommandEvent& event );

////@end MDPrepGUI event handler declarations

////@begin MDPrepGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end MDPrepGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin MDPrepGUI member variables
    wxBoxSizer* p_mainTopoSizer;
    wxPanel* p_solventPanel;
    wxPanel* p_ionPanel;
    wxPanel* p_linkPanel;
    wxBoxSizer* p_translateVSizer;
    wxPanel* p_translateMolID;
    wxPanel* p_translateAtomID;
    wxBoxSizer* p_orientVSizer;
    wxPanel* p_orientMolID;
    wxPanel* p_orientAtomID;
    wxBoxSizer* p_solvateVSizer;
    wxPanel* p_edgePanel;
    wxBoxSizer* p_slvEdgeSizer;
    wxPanel* p_spherePanel;
    wxPanel* p_slvBoxPanel;
    wxBoxSizer* p_modifyVSizer;
    wxPanel* p_invertZPanel;
    wxPanel* p_invertZDis;
    wxBoxSizer* p_feedbackSizer;
    static const wxWindowID wxID_STATIC_MDPREP_CIMINDIS;
    static const wxWindowID ID_TEXTCTRL_PMF_SELECT_ATOMS1;
    static const wxWindowID ID_PANEL_MDPREP_SLV_BOX;
    static const wxWindowID ID_CHECKBOX_MDPREP_SETMEM;
    static const wxWindowID ID_CHECKBOX_MDPREP_RANY;
    static const wxWindowID wxID_STATIC_MDPREP_TRNS_INIT_AT;
    static const wxWindowID ID_STATICTEXT_PMF_SELECT_ATOMS4;
    static const wxWindowID ID_PANEL_THERMODYNAMICS;
    static const wxWindowID wxID_STATIC_MDPREP_ORNT_ID;
    static const wxWindowID ID_PANEL_MDPREP_SLV_SPHERE;
    static const wxWindowID wxID_STATIC_MDPREP_SLV_MINDIST;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SLV_RAD;
    static const wxWindowID ID_BUTTON_MDPREP_CIDEL;
    static const wxWindowID ID_CHECKBOX_PMF_COMBINE;
    static const wxWindowID ID_RADIOBUTTON_MDPREP_TRNS_ATOM;
    static const wxWindowID ID_NOTEBOOK;
    static const wxWindowID wxID_STATIC_MDPREP_XMAX;
    static const wxWindowID wxID_STATIC_MDPREP_FRAG_NAME;
    static const wxWindowID ID_BUTTON_PMF_SHOW_ATOMS1;
    static const wxWindowID ID_BUTTON_MDPREP_SETBC;
    static const wxWindowID ID_TEXTCTRL_PMF_FORCE2;
    static const wxWindowID ID_CHECKBOX_MDPREP_XRANGE;
    static const wxWindowID ID_CHECKBOX_MDPREP_USEZBOX;
    static const wxWindowID ID_TEXTCTRL_PMF_FORCE2_ANGLE;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SETX;
    static const wxWindowID ID_BUTTON_MDPREP_SET_EXPND;
    static const wxWindowID ID_MENU_FEEDBACK;
    static const wxWindowID ID_TEXTCTRL_MDPREP_XREP;
    static const wxWindowID ID_FEEDBACK;
    static const wxWindowID ID_PANEL_MDPREP_TRNS_ID;
    static const wxWindowID ID_PANEL1;
    static const wxWindowID wxID_STATIC_PMF_SELECT_ATOMS1;
    static const wxWindowID ID_CHOICE_MDPREP_PRTLEV;
    static const wxWindowID ID_STATICTEXT_PMF_SELECT_ATOMS3;
    static const wxWindowID ID_PANEL_MDPREP_SLV_EDGE;
    static const wxWindowID ID_BUTTON_MDPREP_CENTERY;
    static const wxWindowID wxID_STATIC_MDPREP_ALT_LOC;
    static const wxWindowID ID_BUTTON_MDPREP_SET_SLVTOL;
    static const wxWindowID ID_PANEL_PMF_FORCE_ANGLE;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SOL_MOD;
    static const wxWindowID ID_BUTTON_MDPREP_CONFIGURE;
    static const wxWindowID ID_BUTTON_PMF_DEFAULTS_ANGLE;
    static const wxWindowID ID_TEXTCTRL_MDPREP_FRAC4;
    static const wxWindowID ID_TEXTCTRL_PMF_DISTANCE2;
    static const wxWindowID ID_NOTEBOOK_MDPREP_MAIN;
    static const wxWindowID ID_BUTTON_MDPREP_GETCHARGE;
    static const wxWindowID ID_NOTEBOOK_MDPREP_COORDS;
    static const wxWindowID ID_PANEL_MDPREP_TRANSLATE;
    static const wxWindowID ID_TEXTCTRL_PMF_FORCE2_TORSION;
    static const wxWindowID wxID_STATIC_MDPREP_SLV_XUNIT;
    static const wxWindowID ID_CHECKBOX_MDPREP_RANZ;
    static const wxWindowID ID_PANEL_MDPREP_ZOPTS;
    static const wxWindowID ID_BUTTON_MDPREP_SETDIM;
    static const wxWindowID ID_RADIOBUTTON_MDPREP_HIP;
    static const wxWindowID ID_TEXTCTRL_MDPREP_CIGRIDSZ;
    static const wxWindowID wxID_STATIC_MDPREP_ORNT_INIT_AT;
    static const wxWindowID ID_TEXTCTRL_MDPREP_FRAC8;
    static const wxWindowID ID_PANEL_PMF_FORCE_TORSION;
    static const wxWindowID ID_TEXTCTRL_MDPREP_ORNT_FNL_AT;
    static const wxWindowID ID_PANEL_PMF_DISTANCE;
    static const wxWindowID ID_TEXTCTRL_MDPREP_ORNT_Y;
    static const wxWindowID ID_TEXTCTRL_MDPREP_ALGNI;
    static const wxWindowID wxID_STATIC_PMF_SELECT_ATOMS3;
    static const wxWindowID wxID_STATIC_MDPREP_SLV_ZUNIT;
    static const wxWindowID ID_TEXTCTRL_MDPREP_TRNS_ID;
    static const wxWindowID ID_BUTTON_PMF_DEFAULTS_DISTANCE;
    static const wxWindowID ID_CHECKBOX_PMF_CONSTRAINT;
    static const wxWindowID ID_TEXTCTRL_MDPREP_MEMGLBL;
    static const wxWindowID ID_BUTTON_PMF_UP;
    static const wxWindowID wxID_STATIC_MDPREP_SLV_X;
    static const wxWindowID wxID_STATIC_PMF_FORCE1_DISTANCE_UNIT;
    static const wxWindowID ID_PANEL_MDPREP_SOLVATE;
    static const wxWindowID ID_TEXTCTRL_MDPREP_ALGNK;
    static const wxWindowID ID_TEXTCTRL_MDPREP_ORNT_ID;
    static const wxWindowID ID_CHOICE_PMF_OPTION;
    static const wxWindowID ID_BUTTON_MDPREP_CHEMSYS;
    static const wxWindowID ID_BUTTON_PMF_SELECT_ATOMS3;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SOL_NAM;
    static const wxWindowID ID_PANEL_PMF_OPTION;
    static const wxWindowID ID_TEXTCTRL_MDPREP_FRAC5;
    static const wxWindowID ID_TEXTCTRL_PMF_FORCE1_ANGLE;
    static const wxWindowID ID_CHECKBOX_MDPREP_RANX;
    static const wxWindowID ID_TEXTCTRL_PMF_ANGLE2;
    static const wxWindowID ID_CHOICE_MDPREP_REPOPTS;
    static const wxWindowID ID_PANEL_MDPREP_BXSZ;
    static const wxWindowID ID_CHECKBOX_PMF_BIAS;
    static const wxWindowID ID_RADIOBUTTON_MDPREP_TRNS_MOL;
    static const wxWindowID ID_TEXTCTRL_MDPREP_TRNS_Z;
    static const wxWindowID ID_CHOICE_MDPREP_BCOPT;
    static const wxWindowID ID_TEXTCTRL_MDPREP_FRAC1;
    static const wxWindowID ID_BUTTON_MDPREP_TRNS_SOL;
    static const wxWindowID ID_CHECKBOX_MDPREP_YRANGE;
    static const wxWindowID ID_BUTTON_MDPREP_LNK_ADD;
    static const wxWindowID ID_PANEL_PMF_FORCE_DISTANCE;
    static const wxWindowID ID_TEXTCTRL_MDPREP_MEMSTK;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SLV_ZMIN;
    static const wxWindowID wxID_STATIC_PMF_FORCE1_TORSION_UNIT;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SLV_YMIN;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SETY;
    static const wxWindowID ID_BUTTON_MDPREP_UP;
    static const wxWindowID ID_PANEL_MDPREP_ORIENT;
    static const wxWindowID ID_TEXTCTRL_MDPREP_MEMHEAP;
    static const wxWindowID wxID_STATIC_PMF_ANGLE1_UNIT;
    static const wxWindowID wxID_STATIC_MDPREP_SETZ;
    static const wxWindowID ID_CHECKBOX_PMF_ALL;
    static const wxWindowID ID_BUTTON_MDPREP_LNK_SELECT;
    static const wxWindowID ID_BUTTON_MDPREP_CIADD;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SLV_BOXZ;
    static const wxWindowID ID_STATICTEXT_PMF_SELECT_ATOMS1;
    static const wxWindowID ID_BUTTON_MDPREP_DOWN;
    static const wxWindowID ID_PANEL;
    static const wxWindowID ID_BUTTON_MDPREP_RESET;
    static const wxWindowID ID_BUTTON_MDPREP_REPLIC;
    static const wxWindowID wxID_STATIC_PMF_DISTANCE2_UNIT;
    static const wxWindowID ID_TEXTCTRL_MDPREP_ORNT_ANGLE;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SLV_XMAX;
    static const wxWindowID ID_TEXTCTRL_MDPREP_LNK_ATOM1;
    static const wxWindowID ID_TEXTCTRL_MDPREP_MINSOLV;
    static const wxWindowID ID_TEXTCTRL_MDPREP_YREP;
    static const wxWindowID ID_STATICTEXT_MDPREP_SLV_SPHERE_UNIT;
    static const wxWindowID wxID_STATIC_MDPREP_XTRNS_UNIT;
    static const wxWindowID ID_PANEL_PMF_SELECT_ATOMS3;
    static const wxWindowID ID_MENU_MDPREP_HELP;
    static const wxWindowID wxID_STATIC_MDPREP_SLV_YUNIT;
    static const wxWindowID ID_BUTTON_MDPREP_CENTERX;
    static const wxWindowID wxID_STATIC_MDPREP_SLV_BOX_UNIT;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SLV_ZMAX;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SLVTOL;
    static const wxWindowID ID_RADIOBUTTON_MDPREP_HIE;
    static const wxWindowID ID_TEXTCTRL_MDPREP_MINRSEP;
    static const wxWindowID ID_BUTTON_MDPREP_ROTATE;
    static const wxWindowID ID_TEXTCTRL_MDPREP_CIMINDIS;
    static const wxWindowID wxID_STATIC_PMF_FORCE2_DISTANCE_UNIT;
    static const wxWindowID ID_BUTTON_PMF_DELETE;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SKIPY;
    static const wxWindowID ID_TEXTCTRL_MDPREP_EXPND;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SLV_EDGE;
    static const wxWindowID ID_TEXTCTRL_MDPREP_ORNT_X;
    static const wxWindowID ID_BUTTON_MDPREP_SET_MINSLV;
    static const wxWindowID ID_TEXTCTRL_MDPREP_FRAC2;
    static const wxWindowID ID_TEXTCTRL_MDPREP_FRAC10;
    static const wxWindowID ID_MDPREP_FRAME;
    static const wxWindowID ID_TEXTCTRL_PMF_SELECT_ATOMS3;
    static const wxWindowID ID_TEXTCTRL_PMF_FORCE1_DISTANCE;
    static const wxWindowID wxID_STATIC_MDPREP_MINRSEP;
    static const wxWindowID ID_PANEL_COORDINATES;
    static const wxWindowID ID_LISTBOX_MDPREP_LNK_LIST;
    static const wxWindowID wxID_STATIC_MDPREP_CHAIN;
    static const wxWindowID ID_CHOICE_MDPREP_SLV_GEOM;
    static const wxWindowID ID_RADIOBUTTON_MDPREP_ORNT_ATOM;
    static const wxWindowID wxID_STATIC_MDPREP_ZMAX;
    static const wxWindowID ID_TEXTCTRL_MDPREP_TRNS_Y;
    static const wxWindowID wxID_STATIC_MDPREP_ZTRNS_UNIT;
    static const wxWindowID ID_BUTTON_MDPREP_SLV_RMV;
    static const wxWindowID ID_TEXTCTRL_MDPREP_IZDIS;
    static const wxWindowID ID_TEXTCTRL_PMF_FORCE1_TORSION;
    static const wxWindowID ID_LISTBOX_PMF_DIRECTIVES;
    static const wxWindowID wxID_STATIC_MDPREP_SLV_EDGE;
    static const wxWindowID ID_CHECKBOX_MDPREP_INVCPY;
    static const wxWindowID wxID_STATIC_PMF_FORCE1_ANGLE_UNIT;
    static const wxWindowID ID_TEXTCTRL_MDPREP_ORNT_INIT_AT;
    static const wxWindowID wxID_STATIC_PMF_ANGLE2_UNIT;
    static const wxWindowID ID_BUTTON_MDPREP_BUILDER;
    static const wxWindowID ID_PANEL_PMF_ANGLE;
    static const wxWindowID ID_BUTTON_PMF_ADD_DIRECTIVE;
    static const wxWindowID ID_BUTTON_MDPREP_ALIGN;
    static const wxWindowID ID_PANEL_TOPOLOGY;
    static const wxWindowID wxID_STATIC_MDPREP_SLV_Y;
    static const wxWindowID ID_BUTTON_PMF_SHOW_ATOMS2;
    static const wxWindowID ID_STATICTEXT_PMF_SELECT_ATOMS2;
    static const wxWindowID wxID_STATIC_MDPREP_MINSOLV;
    static const wxWindowID ID_RADIOBUTTON_MDPREP_ORNT_SEG;
    static const wxWindowID wxID_STATIC_PMF_DISTANCE1_UNIT;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SETZ;
    static const wxWindowID ID_BUTTON_MDPREP_DELETE;
    static const wxWindowID ID_TEXTCTRL_MDPREP_FRAC9;
    static const wxWindowID ID_BUTTON_MDPREP_LOCAL;
    static const wxWindowID ID_PANEL_MDPREP_ORNT_ID;
    static const wxWindowID ID_PANEL_PMF_SELECT_ATOMS2;
    static const wxWindowID wxID_STATIC_MDPREP_YMIN;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SKIPX;
    static const wxWindowID wxID_STATIC_MDPREP_SETY;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SLV_MINDIST;
    static const wxWindowID ID_BUTTON_MDPREP_SOLVATE;
    static const wxWindowID wxID_STATIC_MDPREP_SLCT_MOD;
    static const wxWindowID ID_RADIOBUTTON_MDPREP_HID;
    static const wxWindowID ID_PANEL_MDPREP_DETAILS;
    static const wxWindowID ID_BUTTON_PMF_SELECT_ATOMS1;
    static const wxWindowID ID_TEXTCTRL_PMF_ANGLE1;
    static const wxWindowID ID_TEXTCTRL_MDPREP_CISCALE;
    static const wxWindowID ID_PANEL_MDPREP_SLVNT;
    static const wxWindowID ID_COMBOBOX_MDPREP_CINAME;
    static const wxWindowID ID_TEXTCTRL_MDPREP_TRNS_FNL_AT;
    static const wxWindowID wxID_STATIC_MDPREP_ZREP;
    static const wxWindowID wxID_STATIC_PMF_FORCE2_ANGLE_UNIT;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SLV_BOXY;
    static const wxWindowID ID_BUTTON_MDPREP_ORIENT;
    static const wxWindowID ID_PANEL_MDPREP_REMOVESOLVENT;
    static const wxWindowID ID_BUTTON_PMF_DOWN;
    static const wxWindowID wxID_STATIC_PMF_SELECT_ATOMS4;
    static const wxWindowID ID_TEXTCTRL_MDPREP_ZREP;
    static const wxWindowID ID_TEXTCTRL_PMF_FORCE2_DISTANCE;
    static const wxWindowID ID_PANEL_MDPREP_IZDIS;
    static const wxWindowID wxID_STATIC_MDPREP_ORNT_ANGLE_UNIT;
    static const wxWindowID wxID_STATIC_MDPREP_XMIN;
    static const wxWindowID ID_RADIOBUTTON_MDPREP_ORNT_MOL;
    static const wxWindowID wxID_STATIC_PMF_FORCE2_UNIT;
    static const wxWindowID ID_BUTTON_MDPREP_SKIP;
    static const wxWindowID ID_PANEL_PMF_FORCE;
    static const wxWindowID ID_PANEL_PMF_SELECT_ATOMS1;
    static const wxWindowID ID_TEXTCTRL_PMF_SELECT_ATOMS2;
    static const wxWindowID ID_CHOICE_MDPREP_SOLVENTS;
    static const wxWindowID ID_BUTTON_MDPREP_REMOTE;
    static const wxWindowID wxID_STATIC_MDPREP_SLV_Z;
    static const wxWindowID ID_TEXTCTRL_MDPREP_TRNS_X;
    static const wxWindowID ID_BUTTON_PMF_SHOW_ATOMS3;
    static const wxWindowID wxID_STATIC_MDPREP_ORNT_FNL_AT;
    static const wxWindowID wxID_STATIC_MDPREP_SLVTOL;
    static const wxWindowID ID_TEXTCTRL_PMF_SELECT_ATOMS4;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SLV_YMAX;
    static const wxWindowID ID_STATICTEXT_MDPREP_SLV_EDGE_UNIT;
    static const wxWindowID ID_BUTTON_MDPREP_ORNTSL;
    static const wxWindowID ID_CHECKBOX_MDPREP_ZRANGE;
    static const wxWindowID wxID_STATIC_MDPREP_IZDIS;
    static const wxWindowID ID_TEXTCTRL_PMF_FORCE1;
    static const wxWindowID ID_BUTTON_MDPREP_CENTER;
    static const wxWindowID ID_TEXTCTRL_MDPREP_TRNS_INIT_AT;
    static const wxWindowID ID_PANEL_MDPREP_TRNS_AIDX;
    static const wxWindowID ID_CHOICE_MDPREP_RMV_OPT;
    static const wxWindowID ID_RADIOBUTTON_MDPREP_TRNS_SEG;
    static const wxWindowID ID_TEXTCTRL_MDPREP_LNK_ATOM2;
    static const wxWindowID wxID_STATIC_MDPREP_BOXDIM;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SKIPZ;
    static const wxWindowID ID_LISTBOX_MDPREP_CILIST;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SLV_BOXX;
    static const wxWindowID wxID_STATIC_MDPREP_EXPND;
    static const wxWindowID ID_PANEL_PMF_SELECT_ATOMS4;
    static const wxWindowID wxID_STATIC_MDPREP_SETX;
    static const wxWindowID ID_TEXTCTRL_MDPREP_CINUM;
    static const wxWindowID wxID_STATIC_MDPREP_YMAX;
    static const wxWindowID wxID_STATIC_PMF_SELECT_ATOMS_2;
    static const wxWindowID ID_LISTBOX_MDPREP_CMDS;
    static const wxWindowID ID_TEXTCTRL_MDPREP_ALGNJ;
    static const wxWindowID wxID_STATIC_MDPREP_TRNS_FNL_AT;
    static const wxWindowID ID_BUTTON_PMF_SHOW_ATOMS4;
    static const wxWindowID ID_BUTTON_PMF_SELECT_ATOMS2;
    static const wxWindowID wxID_STATIC_MDPREP_ALT_LOC_LABEL;
    static const wxWindowID ID_BUTTON_MDPREP_FINAL_EDIT;
    static const wxWindowID ID_PANEL_MDPREP_CNTION;
    static const wxWindowID ID_TEXTCTRL_MDPREP_ORNT_Z;
    static const wxWindowID wxID_STATIC_MDPREP_ZMIN;
    static const wxWindowID ID_BUTTON_MDPREP_LNK_DELETE;
    static const wxWindowID ID_PANEL_MDPREP_MEMORY;
    static const wxWindowID ID_PANEL_MDPREP_ORNT_AIDX;
    static const wxWindowID wxID_STATIC_MDPREP_YTRNS_UNIT;
    static const wxWindowID wxID_STATIC_MDPREP_TRNS_ID;
    static const wxWindowID ID_PANEL_MDPREP_ADDLNK;
    static const wxWindowID wxID_STATIC_PMF_FORCE1_UNIT;
    static const wxWindowID wxID_STATIC_MDPREP_SLV_RAD;
    static const wxWindowID wxID_STATIC_PMF_FORCE2_TORSION_UNIT;
    static const wxWindowID ID_PANEL_MDPREP_TOPOPT;
    static const wxWindowID ID_TEXTCTRL_MDPREP_FRAC6;
    static const wxWindowID ID_TEXTCTRL_PMF_DISTANCE1;
    static const wxWindowID ID_BUTTON_MDPREP_CENTERZ;
    static const wxWindowID ID_PANEL_MDPREP_MODIFY_CELL;
    static const wxWindowID ID_TEXTCTRL_MDPREP_FRAC3;
    static const wxWindowID ID_TEXTCTRL_MDPREP_SLV_XMIN;
    static const wxWindowID ID_BUTTON_PMF_SELECT_ATOMS4;
    static const wxWindowID ID_TEXTCTRL_MDPREP_FRAC7;
////@end MDPrepGUI member variables
};

#endif
    // _MDPREPGUI_H_
