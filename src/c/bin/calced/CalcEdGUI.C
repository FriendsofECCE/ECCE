/////////////////////////////////////////////////////////////////////////////
// Name:        CalcEdGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "CalcEdGUI.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxFrame.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "CalcEdGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID CalcEdGUI::ID_BUTTON_CALCED_LAUNCH = wxNewId();
const wxWindowID CalcEdGUI::ID_STATICTEXT_CALCED_ECP = wxNewId();
const wxWindowID CalcEdGUI::ID_BUTTON_CALCED_RUNTYPE = wxNewId();
const wxWindowID CalcEdGUI::ID_STATICTEXT_CALCED_ATOMS = wxNewId();
const wxWindowID CalcEdGUI::ID_TEXTCTRL_CALCED_NAME = wxNewId();
const wxWindowID CalcEdGUI::ID_COMBOBOX_CALCED_SPIN_MULT = wxNewId();
const wxWindowID CalcEdGUI::ID_CHOICE_CALCED_THEORY = wxNewId();
const wxWindowID CalcEdGUI::ID_MENU_CALCED_SAVE_PREF = wxNewId();
const wxWindowID CalcEdGUI::ID_BUTTON_CALCED_BUILDER = wxNewId();
const wxWindowID CalcEdGUI::ID_LABEL_CALCED_XC = wxNewId();
const wxWindowID CalcEdGUI::ID_COMBOBOX_CALCED_CHARGE = wxNewId();
const wxWindowID CalcEdGUI::ID_LABEL_CALCED_ECP = wxNewId();
const wxWindowID CalcEdGUI::ID_STATICTEXT_CALCED_BASIS_NAME = wxNewId();
const wxWindowID CalcEdGUI::ID_STATICTEXT_CALCED_PRIMITIVES = wxNewId();
const wxWindowID CalcEdGUI::ID_STATIC_CALCED_CHARGE = wxNewId();
const wxWindowID CalcEdGUI::ID_BUTTON_CALCED_CODE = wxNewId();
const wxWindowID CalcEdGUI::ID_STATICTEXT_CALCED_ELECTRONS = wxNewId();
const wxWindowID CalcEdGUI::ID_STATIC_CALCED_SPIN_MULT = wxNewId();
const wxWindowID CalcEdGUI::ID_BUTTON_CALCED_FINAL_EDIT = wxNewId();
const wxWindowID CalcEdGUI::ID_STATICTEXT_CALCED_OPEN_SHELLS = wxNewId();
const wxWindowID CalcEdGUI::ID_STATICTEXT_CALCED_FUNCTIONS = wxNewId();
const wxWindowID CalcEdGUI::ID_STATICTEXT_CALCED_FORMULA = wxNewId();
const wxWindowID CalcEdGUI::ID_STATICTEXT_CALCED_SYMMETRY = wxNewId();
const wxWindowID CalcEdGUI::ID_STATICTEXT_CALCED_POLARIZATION = wxNewId();
const wxWindowID CalcEdGUI::ID_BUTTON_CALCED_BASIS_SET = wxNewId();
const wxWindowID CalcEdGUI::ID_CHECKBOX_CALCED_IRREDUCIBLE = wxNewId();
const wxWindowID CalcEdGUI::ID_BUTTON_CALCED_THEORY = wxNewId();
const wxWindowID CalcEdGUI::ID_STATICTEXT_CALCED_CD = wxNewId();
const wxWindowID CalcEdGUI::ID_CHECKBOX_CALCED_USE_EXPONENTS = wxNewId();
const wxWindowID CalcEdGUI::ID_BUTTON_CALCED_PARTIAL = wxNewId();
const wxWindowID CalcEdGUI::ID_BUTTON_CALCED_BASIS_QUICK = wxNewId();
const wxWindowID CalcEdGUI::ID_MENU_FEEDBACK = wxNewId();
const wxWindowID CalcEdGUI::ID_CHOICE_CALCED_RUNTYPE = wxNewId();
const wxWindowID CalcEdGUI::ID_BUTTON_CALCED_CONSTRAINT = wxNewId();
const wxWindowID CalcEdGUI::ID_LABEL_CALCED_CD = wxNewId();
const wxWindowID CalcEdGUI::ID_CALCED_FRAME = wxNewId();
const wxWindowID CalcEdGUI::ID_STATICTEXT_CALCED_XC = wxNewId();

/*!
 * CalcEdGUI type definition
 */

IMPLEMENT_CLASS( CalcEdGUI, ewxFrame )

/*!
 * CalcEdGUI event table definition
 */

BEGIN_EVENT_TABLE( CalcEdGUI, ewxFrame )

////@begin CalcEdGUI event table entries
    EVT_CLOSE( CalcEdGUI::OnCloseWindow )

    EVT_MENU( wxID_SAVE, CalcEdGUI::OnSaveClick )

    EVT_MENU( ID_MENU_CALCED_SAVE_PREF, CalcEdGUI::OnMenuCalcedSavePrefClick )

    EVT_MENU( wxID_EXIT, CalcEdGUI::OnExitClick )

    EVT_MENU( wxID_HELP, CalcEdGUI::OnHelpClick )

    EVT_MENU( ID_MENU_FEEDBACK, CalcEdGUI::OnMenuFeedbackClick )

    EVT_BUTTON( ID_BUTTON_CALCED_CODE, CalcEdGUI::OnButtonCalcedCodeClick )

    EVT_COMBOBOX( ID_COMBOBOX_CALCED_CHARGE, CalcEdGUI::OnComboboxCalcedChargeSelected )
    EVT_TEXT_ENTER( ID_COMBOBOX_CALCED_CHARGE, CalcEdGUI::OnComboboxCalcedChargeEnter )

    EVT_COMBOBOX( ID_COMBOBOX_CALCED_SPIN_MULT, CalcEdGUI::OnComboboxCalcedSpinMultSelected )
    EVT_TEXT_ENTER( ID_COMBOBOX_CALCED_SPIN_MULT, CalcEdGUI::OnComboboxCalcedSpinMultEnter )

    EVT_CHECKBOX( ID_CHECKBOX_CALCED_IRREDUCIBLE, CalcEdGUI::OnCheckboxCalcedIrreducibleClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CALCED_NAME, CalcEdGUI::OnTextctrlCalcedNameEnter )

    EVT_BUTTON( ID_BUTTON_CALCED_BASIS_QUICK, CalcEdGUI::OnButtonCalcedBasisQuickClick )

    EVT_CHECKBOX( ID_CHECKBOX_CALCED_USE_EXPONENTS, CalcEdGUI::OnCheckboxCalcedUseExponentsClick )

    EVT_CHOICE( ID_CHOICE_CALCED_THEORY, CalcEdGUI::OnChoiceCalcedTheorySelected )

    EVT_BUTTON( ID_BUTTON_CALCED_THEORY, CalcEdGUI::OnButtonCalcedTheoryClick )

    EVT_CHOICE( ID_CHOICE_CALCED_RUNTYPE, CalcEdGUI::OnChoiceCalcedRuntypeSelected )

    EVT_BUTTON( ID_BUTTON_CALCED_RUNTYPE, CalcEdGUI::OnButtonCalcedRuntypeClick )

    EVT_BUTTON( ID_BUTTON_CALCED_PARTIAL, CalcEdGUI::OnButtonCalcedPartialClick )

    EVT_BUTTON( ID_BUTTON_CALCED_CONSTRAINT, CalcEdGUI::OnButtonCalcedConstraintClick )

    EVT_BUTTON( ID_BUTTON_CALCED_FINAL_EDIT, CalcEdGUI::OnButtonCalcedFinalEditClick )

    EVT_BUTTON( ID_BUTTON_CALCED_LAUNCH, CalcEdGUI::OnButtonCalcedLaunchClick )

////@end CalcEdGUI event table entries

END_EVENT_TABLE()

/*!
 * CalcEdGUI constructors
 */

CalcEdGUI::CalcEdGUI( )
{
}

CalcEdGUI::CalcEdGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * CalcEdGUI creator
 */

bool CalcEdGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CalcEdGUI member initialisation
    p_codeSizer = NULL;
    p_detailsBox = NULL;
    p_theoryDetailsSizer = NULL;
    p_runtypeDetailsSizer = NULL;
////@end CalcEdGUI member initialisation

////@begin CalcEdGUI creation
    SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CalcEdGUI creation
    return true;
}

/*!
 * Control creation for CalcEdGUI
 */

void CalcEdGUI::CreateControls()
{    
////@begin CalcEdGUI content construction
    CalcEdGUI* itemFrame1 = this;

    ewxMenuBar* menuBar = new ewxMenuBar;
    wxMenu* itemMenu3 = new wxMenu;
    itemMenu3->Append(wxID_SAVE, _("Save\tCtrl+s"), _T(""), wxITEM_NORMAL);
    itemMenu3->Append(ID_MENU_CALCED_SAVE_PREF, _("Save Theory/Runtype Preference"), _T(""), wxITEM_NORMAL);
    itemMenu3->AppendSeparator();
    itemMenu3->Append(wxID_EXIT, _("Quit\tCtrl+q"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu3, _("File"));
    wxMenu* itemMenu8 = new wxMenu;
    itemMenu8->Append(wxID_HELP, _("on Electronic Structure Editor..."), _T(""), wxITEM_NORMAL);
    itemMenu8->Append(ID_MENU_FEEDBACK, _("Support..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu8, _("Help"));
    itemFrame1->SetMenuBar(menuBar);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(itemBoxSizer11);

    wxStaticBox* itemStaticBoxSizer12Static = new wxStaticBox(itemFrame1, wxID_ANY, _("Code"));
    wxStaticBoxSizer* itemStaticBoxSizer12 = new wxStaticBoxSizer(itemStaticBoxSizer12Static, wxHORIZONTAL);
    itemBoxSizer11->Add(itemStaticBoxSizer12, 0, wxGROW|wxALL, 5);

    p_codeSizer = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer12->Add(p_codeSizer, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    ewxButton* itemButton14 = new ewxButton( itemFrame1, ID_BUTTON_CALCED_CODE, _("Button"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton14->Show(false);
    itemStaticBoxSizer12->Add(itemButton14, 0, wxALIGN_CENTER_VERTICAL, 5);

    wxStaticBox* itemStaticBoxSizer15Static = new wxStaticBox(itemFrame1, wxID_ANY, _("Chemical System"));
    wxStaticBoxSizer* itemStaticBoxSizer15 = new wxStaticBoxSizer(itemStaticBoxSizer15Static, wxHORIZONTAL);
    itemBoxSizer11->Add(itemStaticBoxSizer15, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    ewxButton* itemButton16 = new ewxButton( itemFrame1, ID_BUTTON_CALCED_BUILDER, _("Builder"), wxDefaultPosition, wxSize(64, 64), 0 );
    itemStaticBoxSizer15->Add(itemButton16, 0, wxALIGN_TOP|wxALL, 5);

    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxVERTICAL);
    itemStaticBoxSizer15->Add(itemBoxSizer17, 0, wxALIGN_CENTER_VERTICAL, 5);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer17->Add(itemBoxSizer18, 0, wxALIGN_LEFT, 5);

    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer18->Add(itemBoxSizer19, 0, wxALIGN_CENTER_VERTICAL, 5);

    ewxStaticText* itemStaticText20 = new ewxStaticText( itemFrame1, ID_STATIC_CALCED_CHARGE, _("Charge:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemStaticText20, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemComboBox21Strings[] = {
        _("-2"),
        _("-1"),
        _("0"),
        _("1"),
        _("2")
    };
    ewxComboBox* itemComboBox21 = new ewxComboBox( itemFrame1, ID_COMBOBOX_CALCED_CHARGE, _("0"), wxDefaultPosition, wxSize(50, -1), 5, itemComboBox21Strings, wxCB_DROPDOWN );
    itemComboBox21->SetStringSelection(_("0"));
    itemBoxSizer19->Add(itemComboBox21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer22 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer18->Add(itemBoxSizer22, 0, wxALIGN_CENTER_VERTICAL, 5);

    ewxStaticText* itemStaticText23 = new ewxStaticText( itemFrame1, ID_STATIC_CALCED_SPIN_MULT, _("Spin Mult.:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer22->Add(itemStaticText23, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemComboBox24Strings[] = {
        _("Singlet"),
        _("Triplet"),
        _("Quintet"),
        _("Septet"),
        _("Nontet")
    };
    ewxComboBox* itemComboBox24 = new ewxComboBox( itemFrame1, ID_COMBOBOX_CALCED_SPIN_MULT, _("Singlet"), wxDefaultPosition, wxSize(75, -1), 5, itemComboBox24Strings, wxCB_DROPDOWN );
    itemComboBox24->SetStringSelection(_("Singlet"));
    itemBoxSizer22->Add(itemComboBox24, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxCheckBox* itemCheckBox25 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_CALCED_IRREDUCIBLE, _("Use As Irreducible\nFragment In Input"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox25->SetValue(false);
    itemBoxSizer18->Add(itemCheckBox25, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer26 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer18->Add(itemBoxSizer26, 0, wxALIGN_CENTER_VERTICAL, 5);

    ewxStaticText* itemStaticText27 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText27->Show(false);
    itemBoxSizer26->Add(itemStaticText27, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl28 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_CALCED_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemTextCtrl28->Show(false);
    itemBoxSizer26->Add(itemTextCtrl28, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer29 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer17->Add(itemBoxSizer29, 0, wxALIGN_LEFT|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer30 = new wxFlexGridSizer(0, 2, 0, 2);
    itemFlexGridSizer30->AddGrowableCol(0);
    itemBoxSizer29->Add(itemFlexGridSizer30, 0, wxALIGN_TOP, 2);

    ewxNonBoldLabel* itemStaticText31 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC, _("Formula:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer30->Add(itemStaticText31, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText32 = new ewxStaticText( itemFrame1, ID_STATICTEXT_CALCED_FORMULA, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer30->Add(itemStaticText32, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    ewxNonBoldLabel* itemStaticText33 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC, _("Atoms:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer30->Add(itemStaticText33, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText34 = new ewxStaticText( itemFrame1, ID_STATICTEXT_CALCED_ATOMS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer30->Add(itemStaticText34, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    ewxNonBoldLabel* itemStaticText35 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC, _("Electrons:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer30->Add(itemStaticText35, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText36 = new ewxStaticText( itemFrame1, ID_STATICTEXT_CALCED_ELECTRONS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer30->Add(itemStaticText36, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    itemBoxSizer29->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer38 = new wxFlexGridSizer(2, 2, 0, 2);
    itemBoxSizer29->Add(itemFlexGridSizer38, 0, wxALIGN_TOP, 5);

    ewxNonBoldLabel* itemStaticText39 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC, _("Symmetry:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer38->Add(itemStaticText39, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText40 = new ewxStaticText( itemFrame1, ID_STATICTEXT_CALCED_SYMMETRY, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer38->Add(itemStaticText40, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    ewxNonBoldLabel* itemStaticText41 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC, _("Open Shells:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer38->Add(itemStaticText41, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText42 = new ewxStaticText( itemFrame1, ID_STATICTEXT_CALCED_OPEN_SHELLS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer38->Add(itemStaticText42, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    wxStaticBox* itemStaticBoxSizer43Static = new wxStaticBox(itemFrame1, wxID_ANY, _("Basis Set"));
    wxStaticBoxSizer* itemStaticBoxSizer43 = new wxStaticBoxSizer(itemStaticBoxSizer43Static, wxHORIZONTAL);
    itemBoxSizer11->Add(itemStaticBoxSizer43, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    ewxButton* itemButton44 = new ewxButton( itemFrame1, ID_BUTTON_CALCED_BASIS_SET, _("Basis"), wxDefaultPosition, wxSize(64, 64), 0 );
    itemStaticBoxSizer43->Add(itemButton44, 0, wxALIGN_TOP|wxALL, 5);

    wxBoxSizer* itemBoxSizer45 = new wxBoxSizer(wxVERTICAL);
    itemStaticBoxSizer43->Add(itemBoxSizer45, 0, wxALIGN_CENTER_VERTICAL, 5);

    wxBoxSizer* itemBoxSizer46 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer45->Add(itemBoxSizer46, 0, wxALIGN_LEFT, 5);

    ewxButton* itemButton47 = new ewxButton( itemFrame1, ID_BUTTON_CALCED_BASIS_QUICK, _("Quick Basis Menu  V "), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer46->Add(itemButton47, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxCheckBox* itemCheckBox48 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_CALCED_USE_EXPONENTS, _("Use Exponents\n&& Coefficients"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox48->SetValue(false);
    itemBoxSizer46->Add(itemCheckBox48, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    wxBoxSizer* itemBoxSizer49 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer45->Add(itemBoxSizer49, 0, wxALIGN_LEFT|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer50 = new wxFlexGridSizer(0, 2, 0, 2);
    itemFlexGridSizer50->AddGrowableCol(0);
    itemBoxSizer49->Add(itemFlexGridSizer50, 0, wxALIGN_TOP, 2);

    ewxNonBoldLabel* itemStaticText51 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer50->Add(itemStaticText51, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText52 = new ewxStaticText( itemFrame1, ID_STATICTEXT_CALCED_BASIS_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer50->Add(itemStaticText52, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    ewxNonBoldLabel* itemStaticText53 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC, _("Polarization:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer50->Add(itemStaticText53, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText54 = new ewxStaticText( itemFrame1, ID_STATICTEXT_CALCED_POLARIZATION, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer50->Add(itemStaticText54, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    ewxNonBoldLabel* itemStaticText55 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC, _("Functions:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer50->Add(itemStaticText55, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText56 = new ewxStaticText( itemFrame1, ID_STATICTEXT_CALCED_FUNCTIONS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer50->Add(itemStaticText56, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    ewxNonBoldLabel* itemStaticText57 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC, _("Primitives:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer50->Add(itemStaticText57, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText58 = new ewxStaticText( itemFrame1, ID_STATICTEXT_CALCED_PRIMITIVES, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer50->Add(itemStaticText58, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    itemBoxSizer49->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer60 = new wxFlexGridSizer(2, 2, 0, 2);
    itemBoxSizer49->Add(itemFlexGridSizer60, 0, wxALIGN_TOP, 5);

    ewxNonBoldLabel* itemStaticText61 = new ewxNonBoldLabel( itemFrame1, ID_LABEL_CALCED_ECP, _("ECP Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer60->Add(itemStaticText61, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText62 = new ewxStaticText( itemFrame1, ID_STATICTEXT_CALCED_ECP, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer60->Add(itemStaticText62, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    ewxNonBoldLabel* itemStaticText63 = new ewxNonBoldLabel( itemFrame1, ID_LABEL_CALCED_CD, _("CD Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer60->Add(itemStaticText63, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText64 = new ewxStaticText( itemFrame1, ID_STATICTEXT_CALCED_CD, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer60->Add(itemStaticText64, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    ewxNonBoldLabel* itemStaticText65 = new ewxNonBoldLabel( itemFrame1, ID_LABEL_CALCED_XC, _("XC Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer60->Add(itemStaticText65, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText66 = new ewxStaticText( itemFrame1, ID_STATICTEXT_CALCED_XC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer60->Add(itemStaticText66, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    p_detailsBox = new wxStaticBox(itemFrame1, wxID_ANY, _("Settings"));
    wxStaticBoxSizer* itemStaticBoxSizer67 = new wxStaticBoxSizer(p_detailsBox, wxHORIZONTAL);
    itemBoxSizer11->Add(itemStaticBoxSizer67, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer68 = new wxBoxSizer(wxVERTICAL);
    itemStaticBoxSizer67->Add(itemBoxSizer68, 0, wxALIGN_TOP, 5);

    wxBoxSizer* itemBoxSizer69 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer68->Add(itemBoxSizer69, 0, wxALIGN_LEFT, 5);

    wxBoxSizer* itemBoxSizer70 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer69->Add(itemBoxSizer70, 0, wxALIGN_LEFT|wxLEFT, 5);

    ewxStaticText* itemStaticText71 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Theory:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer70->Add(itemStaticText71, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* itemChoice72Strings = NULL;
    ewxChoice* itemChoice72 = new ewxChoice( itemFrame1, ID_CHOICE_CALCED_THEORY, wxDefaultPosition, wxDefaultSize, 0, itemChoice72Strings, 0 );
    itemBoxSizer70->Add(itemChoice72, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton73 = new ewxButton( itemFrame1, ID_BUTTON_CALCED_THEORY, _("Theory Details..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer69->Add(itemButton73, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    p_theoryDetailsSizer = new wxFlexGridSizer(0, 2, 0, 0);
    itemBoxSizer68->Add(p_theoryDetailsSizer, 0, wxALIGN_LEFT|wxLEFT, 5);

    itemStaticBoxSizer67->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer76 = new wxBoxSizer(wxVERTICAL);
    itemStaticBoxSizer67->Add(itemBoxSizer76, 0, wxALIGN_TOP, 5);

    wxBoxSizer* itemBoxSizer77 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer76->Add(itemBoxSizer77, 0, wxALIGN_LEFT, 5);

    wxBoxSizer* itemBoxSizer78 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer77->Add(itemBoxSizer78, 0, wxALIGN_LEFT|wxLEFT, 5);

    ewxStaticText* itemStaticText79 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Runtype:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer78->Add(itemStaticText79, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* itemChoice80Strings = NULL;
    ewxChoice* itemChoice80 = new ewxChoice( itemFrame1, ID_CHOICE_CALCED_RUNTYPE, wxDefaultPosition, wxDefaultSize, 0, itemChoice80Strings, 0 );
    itemBoxSizer78->Add(itemChoice80, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton81 = new ewxButton( itemFrame1, ID_BUTTON_CALCED_RUNTYPE, _("Runtype Details..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer77->Add(itemButton81, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    p_runtypeDetailsSizer = new wxFlexGridSizer(0, 2, 0, 0);
    itemBoxSizer76->Add(p_runtypeDetailsSizer, 0, wxALIGN_LEFT|wxLEFT, 5);

    wxBoxSizer* itemBoxSizer83 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer11->Add(itemBoxSizer83, 0, wxGROW|wxRIGHT|wxBOTTOM, 5);

    ewxButton* itemButton84 = new ewxButton( itemFrame1, ID_BUTTON_CALCED_PARTIAL, _("Partial Charge Editor..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton84->Show(false);
    itemBoxSizer83->Add(itemButton84, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    ewxButton* itemButton85 = new ewxButton( itemFrame1, ID_BUTTON_CALCED_CONSTRAINT, _("Constraint Editor..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton85->Show(false);
    itemBoxSizer83->Add(itemButton85, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    ewxButton* itemButton86 = new ewxButton( itemFrame1, ID_BUTTON_CALCED_FINAL_EDIT, _("Final Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer83->Add(itemButton86, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    ewxButton* itemButton87 = new ewxButton( itemFrame1, ID_BUTTON_CALCED_LAUNCH, _("Launch..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer83->Add(itemButton87, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    ewxStaticLine* itemStaticLine88 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer11->Add(itemStaticLine88, 0, wxGROW|wxBOTTOM, 5);

////@end CalcEdGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_CALCED_FRAME
 */

void CalcEdGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_CALCED_FRAME in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_CALCED_FRAME in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
 */

void CalcEdGUI::OnSaveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_CALCED_SAVE_PREF
 */

void CalcEdGUI::OnMenuCalcedSavePrefClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_CALCED_SAVE_PREF in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_CALCED_SAVE_PREF in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 */

void CalcEdGUI::OnExitClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
 */

void CalcEdGUI::OnHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK
 */

void CalcEdGUI::OnMenuFeedbackClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_CODE
 */

void CalcEdGUI::OnButtonCalcedCodeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_CODE in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_CODE in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_CALCED_CHARGE
 */

void CalcEdGUI::OnComboboxCalcedChargeSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_CALCED_CHARGE in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_CALCED_CHARGE in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_CALCED_CHARGE
 */

void CalcEdGUI::OnComboboxCalcedChargeEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_CALCED_CHARGE in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_CALCED_CHARGE in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_CALCED_SPIN_MULT
 */

void CalcEdGUI::OnComboboxCalcedSpinMultSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_CALCED_SPIN_MULT in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_CALCED_SPIN_MULT in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_CALCED_SPIN_MULT
 */

void CalcEdGUI::OnComboboxCalcedSpinMultEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_CALCED_SPIN_MULT in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_CALCED_SPIN_MULT in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CALCED_IRREDUCIBLE
 */

void CalcEdGUI::OnCheckboxCalcedIrreducibleClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CALCED_IRREDUCIBLE in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CALCED_IRREDUCIBLE in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CALCED_NAME
 */

void CalcEdGUI::OnTextctrlCalcedNameEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CALCED_NAME in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CALCED_NAME in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_BASIS_QUICK
 */

void CalcEdGUI::OnButtonCalcedBasisQuickClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_BASIS_QUICK in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_BASIS_QUICK in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CALCED_USE_EXPONENTS
 */

void CalcEdGUI::OnCheckboxCalcedUseExponentsClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CALCED_USE_EXPONENTS in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CALCED_USE_EXPONENTS in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CALCED_THEORY
 */

void CalcEdGUI::OnChoiceCalcedTheorySelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CALCED_THEORY in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CALCED_THEORY in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_THEORY
 */

void CalcEdGUI::OnButtonCalcedTheoryClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_THEORY in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_THEORY in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CALCED_RUNTYPE
 */

void CalcEdGUI::OnChoiceCalcedRuntypeSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CALCED_RUNTYPE in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CALCED_RUNTYPE in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_RUNTYPE
 */

void CalcEdGUI::OnButtonCalcedRuntypeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_RUNTYPE in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_RUNTYPE in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_PARTIAL
 */

void CalcEdGUI::OnButtonCalcedPartialClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_PARTIAL in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_PARTIAL in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_CONSTRAINT
 */

void CalcEdGUI::OnButtonCalcedConstraintClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_CONSTRAINT in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_CONSTRAINT in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_FINAL_EDIT
 */

void CalcEdGUI::OnButtonCalcedFinalEditClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_FINAL_EDIT in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_FINAL_EDIT in CalcEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_LAUNCH
 */

void CalcEdGUI::OnButtonCalcedLaunchClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_LAUNCH in CalcEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CALCED_LAUNCH in CalcEdGUI. 
}

/*!
 * Should we show tooltips?
 */

bool CalcEdGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap CalcEdGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CalcEdGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end CalcEdGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CalcEdGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CalcEdGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end CalcEdGUI icon retrieval
}
