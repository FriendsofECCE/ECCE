/////////////////////////////////////////////////////////////////////////////
// Name:        NWDirdyGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "NWDirdyGUI.H"
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
#include "wxgui/ewxBitmapButton.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxButton.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/ewxFrame.H"
////@end includes

#include "NWDirdyGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID NWDirdyGUI::ID_COMBOBOX_DIRDY_CHARGE_R2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_BITMAPBUTTON_DIRDY_BUILD_P2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_COMBOBOX_DIRDY_CHARGE_TS = wxNewId();
const wxWindowID NWDirdyGUI::ID_CHOICE_DIRDY_SPIN_R1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_BITMAPBUTTON_DIRDY_BUILD_R2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_SPIN_R1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_BUTTON_DIRDY_CALC_P1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CODE_CURRENT_R1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_FORMULA_R1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_BUTTON_DIRDY_CREATE_PROD = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_FORMULA_LABEL_TS = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CODE_CURRENT_R2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_FORMULA_R2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_BUTTON_NWDIRDY_SETUP = wxNewId();
const wxWindowID NWDirdyGUI::ID_BITMAPBUTTON_DIRDY_BUILD_P1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_COMBOBOX_DIRDY_CHARGE_R1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CODE_NONE_R1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CODE_NONE_TS = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_SPIN_R2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_CHOICE_DIRDY_SPIN_TS = wxNewId();
const wxWindowID NWDirdyGUI::ID_BUTTON_NWDIRDY_LAUNCH = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CODE_CURRENT_P2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CHARGE_TS = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_FORMULA_TS = wxNewId();
const wxWindowID NWDirdyGUI::ID_BUTTON_DIRDY_CALC_TS = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CHARGE_R2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CHARGE_P1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_FORMULA_P1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_BUTTON_DIRDY_CALC_R2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_COMBOBOX_DIRDY_CHARGE_P2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_CHOICE_DIRDY_SPIN_P1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_BUTTON_NWDIRDY_FINAL_EDIT = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CODE_CURRENT_P1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_CHOICE_DIRDY_SPIN_R2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CODE_NONE_R2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_SPIN_P1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_BITMAPBUTTON_DIRDY_BUILD_TS = wxNewId();
const wxWindowID NWDirdyGUI::ID_MENU_MDPREP_HELP = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_FORMULA_LABEL_R1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_SPIN_P2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_BARRIER_TS = wxNewId();
const wxWindowID NWDirdyGUI::ID_CHOICE_DIRDY_SPIN_P2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_NWDIRDY_FRAME = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CODE_NONE_P2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_BUTTON_DIRDY_CREATE_REACT = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CHARGE_R1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CODE_CURRENT_TS = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CHARGE_P2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_CODE_NONE_P1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_SPIN_TS = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_FORMULA_LABEL_P2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_MENU_FEEDBACK = wxNewId();
const wxWindowID NWDirdyGUI::ID_FEEDBACK = wxNewId();
const wxWindowID NWDirdyGUI::ID_BUTTON_DIRDY_CALC_P2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_FORMULA_LABEL_R2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_FORMULA_P2 = wxNewId();
const wxWindowID NWDirdyGUI::ID_COMBOBOX_DIRDY_CHARGE_P1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_BUTTON_DIRDY_CALC_R1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_BARRIER_LABEL_TS = wxNewId();
const wxWindowID NWDirdyGUI::ID_STATIC_DIRDY_FORMULA_LABEL_P1 = wxNewId();
const wxWindowID NWDirdyGUI::ID_BITMAPBUTTON_DIRDY_BUILD_R1 = wxNewId();

/*!
 * NWDirdyGUI type definition
 */

IMPLEMENT_CLASS( NWDirdyGUI, ewxFrame )

/*!
 * NWDirdyGUI event table definition
 */

BEGIN_EVENT_TABLE( NWDirdyGUI, ewxFrame )

////@begin NWDirdyGUI event table entries
    EVT_CLOSE( NWDirdyGUI::OnCloseWindow )

    EVT_BUTTON( ID_BITMAPBUTTON_DIRDY_BUILD_R1, NWDirdyGUI::OnBitmapbuttonDirdyBuildR1Click )

    EVT_COMBOBOX( ID_COMBOBOX_DIRDY_CHARGE_R1, NWDirdyGUI::OnComboboxDirdyChargeR1Selected )
    EVT_TEXT_ENTER( ID_COMBOBOX_DIRDY_CHARGE_R1, NWDirdyGUI::OnComboboxDirdyChargeR1Enter )

    EVT_CHOICE( ID_CHOICE_DIRDY_SPIN_R1, NWDirdyGUI::OnChoiceDirdySpinR1Selected )

    EVT_BUTTON( ID_BUTTON_DIRDY_CALC_R1, NWDirdyGUI::OnButtonDirdyCalcR1Click )

    EVT_BUTTON( ID_BITMAPBUTTON_DIRDY_BUILD_R2, NWDirdyGUI::OnBitmapbuttonDirdyBuildR2Click )

    EVT_COMBOBOX( ID_COMBOBOX_DIRDY_CHARGE_R2, NWDirdyGUI::OnComboboxDirdyChargeR2Selected )
    EVT_TEXT_ENTER( ID_COMBOBOX_DIRDY_CHARGE_R2, NWDirdyGUI::OnComboboxDirdyChargeR2Enter )

    EVT_CHOICE( ID_CHOICE_DIRDY_SPIN_R2, NWDirdyGUI::OnChoiceDirdySpinR2Selected )

    EVT_BUTTON( ID_BUTTON_DIRDY_CALC_R2, NWDirdyGUI::OnButtonDirdyCalcR2Click )

    EVT_BUTTON( ID_BITMAPBUTTON_DIRDY_BUILD_TS, NWDirdyGUI::OnBitmapbuttonDirdyBuildTsClick )

    EVT_COMBOBOX( ID_COMBOBOX_DIRDY_CHARGE_TS, NWDirdyGUI::OnComboboxDirdyChargeTsSelected )
    EVT_TEXT_ENTER( ID_COMBOBOX_DIRDY_CHARGE_TS, NWDirdyGUI::OnComboboxDirdyChargeTsEnter )

    EVT_CHOICE( ID_CHOICE_DIRDY_SPIN_TS, NWDirdyGUI::OnChoiceDirdySpinTsSelected )

    EVT_BUTTON( ID_BUTTON_DIRDY_CALC_TS, NWDirdyGUI::OnButtonDirdyCalcTsClick )

    EVT_BUTTON( ID_BUTTON_DIRDY_CREATE_REACT, NWDirdyGUI::OnButtonDirdyCreateReactClick )

    EVT_BUTTON( ID_BUTTON_DIRDY_CREATE_PROD, NWDirdyGUI::OnButtonDirdyCreateProdClick )

    EVT_BUTTON( ID_BITMAPBUTTON_DIRDY_BUILD_P1, NWDirdyGUI::OnBitmapbuttonDirdyBuildP1Click )

    EVT_COMBOBOX( ID_COMBOBOX_DIRDY_CHARGE_P1, NWDirdyGUI::OnComboboxDirdyChargeP1Selected )
    EVT_TEXT_ENTER( ID_COMBOBOX_DIRDY_CHARGE_P1, NWDirdyGUI::OnComboboxDirdyChargeP1Enter )

    EVT_CHOICE( ID_CHOICE_DIRDY_SPIN_P1, NWDirdyGUI::OnChoiceDirdySpinP1Selected )

    EVT_BUTTON( ID_BUTTON_DIRDY_CALC_P1, NWDirdyGUI::OnButtonDirdyCalcP1Click )

    EVT_BUTTON( ID_BITMAPBUTTON_DIRDY_BUILD_P2, NWDirdyGUI::OnBitmapbuttonDirdyBuildP2Click )

    EVT_COMBOBOX( ID_COMBOBOX_DIRDY_CHARGE_P2, NWDirdyGUI::OnComboboxDirdyChargeP2Selected )
    EVT_TEXT_ENTER( ID_COMBOBOX_DIRDY_CHARGE_P2, NWDirdyGUI::OnComboboxDirdyChargeP2Enter )

    EVT_CHOICE( ID_CHOICE_DIRDY_SPIN_P2, NWDirdyGUI::OnChoiceDirdySpinP2Selected )

    EVT_BUTTON( ID_BUTTON_DIRDY_CALC_P2, NWDirdyGUI::OnButtonDirdyCalcP2Click )

    EVT_BUTTON( ID_BUTTON_NWDIRDY_SETUP, NWDirdyGUI::OnButtonNwdirdySetupClick )

    EVT_BUTTON( ID_BUTTON_NWDIRDY_FINAL_EDIT, NWDirdyGUI::OnButtonNwdirdyFinalEditClick )

    EVT_BUTTON( ID_BUTTON_NWDIRDY_LAUNCH, NWDirdyGUI::OnButtonNwdirdyLaunchClick )

    EVT_MENU( wxID_SAVE, NWDirdyGUI::OnSaveClick )

    EVT_MENU( wxID_EXIT, NWDirdyGUI::OnExitClick )

    EVT_MENU( ID_MENU_FEEDBACK, NWDirdyGUI::OnMenuFeedbackClick )

    EVT_MENU( ID_MENU_MDPREP_HELP, NWDirdyGUI::OnMenuMdprepHelpClick )

////@end NWDirdyGUI event table entries

END_EVENT_TABLE()

/*!
 * NWDirdyGUI constructors
 */

NWDirdyGUI::NWDirdyGUI( )
{
}

NWDirdyGUI::NWDirdyGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * NWDirdyGUI creator
 */

bool NWDirdyGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin NWDirdyGUI member initialisation
    p_codeSizerR1 = NULL;
    p_calcedSizerR1 = NULL;
    p_codeSizerR2 = NULL;
    p_calcedSizerR2 = NULL;
    p_codeSizerTS = NULL;
    p_calcedSizerTS = NULL;
    p_codeSizerP1 = NULL;
    p_calcedSizerP1 = NULL;
    p_codeSizerP2 = NULL;
    p_calcedSizerP2 = NULL;
    p_feedbackSizer = NULL;
////@end NWDirdyGUI member initialisation

////@begin NWDirdyGUI creation
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
////@end NWDirdyGUI creation
    return true;
}

/*!
 * Control creation for NWDirdyGUI
 */

void NWDirdyGUI::CreateControls()
{    
////@begin NWDirdyGUI content construction
    NWDirdyGUI* itemFrame1 = this;

    ewxMenuBar* menuBar = new ewxMenuBar;
    wxMenu* itemMenu110 = new wxMenu;
    itemMenu110->Append(wxID_SAVE, _("&Save\tCtrl+s"), _T(""), wxITEM_NORMAL);
    itemMenu110->AppendSeparator();
    itemMenu110->Append(wxID_EXIT, _("&Quit\tCtrl+q"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu110, _("File"));
    wxMenu* itemMenu114 = new wxMenu;
    itemMenu114->Append(ID_MENU_FEEDBACK, _("Support..."), _T(""), wxITEM_NORMAL);
    itemMenu114->Append(ID_MENU_MDPREP_HELP, _("on NWChem DirDyVTST Editor..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu114, _("Help"));
    itemFrame1->SetMenuBar(menuBar);

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW|wxRIGHT|wxTOP|wxBOTTOM, 0);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer4, 0, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText5 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Reactant 1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemStaticText5, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 3);

    wxBitmap itemBitmapButton6Bitmap(itemFrame1->GetBitmapResource(wxT("../../../data/client/pixmaps/gwbuilder2.xpm")));
    ewxBitmapButton* itemBitmapButton6 = new ewxBitmapButton( itemFrame1, ID_BITMAPBUTTON_DIRDY_BUILD_R1, itemBitmapButton6Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer4->Add(itemBitmapButton6, 0, wxALIGN_LEFT|wxALL, 3);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer7, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 3);

    ewxNonBoldLabel* itemStaticText8 = new ewxNonBoldLabel( itemFrame1, ID_STATIC_DIRDY_FORMULA_LABEL_R1, _("Formula:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemStaticText8, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_FORMULA_R1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemStaticText9, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer10, 0, wxALIGN_LEFT|wxALL, 3);

    ewxStaticText* itemStaticText11 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CHARGE_R1, _("Charge:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemStaticText11, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString itemComboBox12Strings[] = {
        _("-2"),
        _("-1"),
        _("0"),
        _("1"),
        _("2")
    };
    ewxComboBox* itemComboBox12 = new ewxComboBox( itemFrame1, ID_COMBOBOX_DIRDY_CHARGE_R1, _("0"), wxDefaultPosition, wxSize(50, -1), 5, itemComboBox12Strings, wxCB_DROPDOWN );
    itemComboBox12->SetStringSelection(_("0"));
    itemBoxSizer10->Add(itemComboBox12, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer13, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 3);

    ewxStaticText* itemStaticText14 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_SPIN_R1, _("Spin:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemStaticText14, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString* itemChoice15Strings = NULL;
    ewxChoice* itemChoice15 = new ewxChoice( itemFrame1, ID_CHOICE_DIRDY_SPIN_R1, wxDefaultPosition, wxDefaultSize, 0, itemChoice15Strings, 0 );
    itemBoxSizer13->Add(itemChoice15, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    p_codeSizerR1 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(p_codeSizerR1, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP, 3);

    ewxStaticText* itemStaticText17 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CODE_CURRENT_R1, _("Current Code:"), wxDefaultPosition, wxDefaultSize, 0 );
    p_codeSizerR1->Add(itemStaticText17, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText18 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CODE_NONE_R1, _("None"), wxDefaultPosition, wxDefaultSize, 0 );
    p_codeSizerR1->Add(itemStaticText18, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    p_calcedSizerR1 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(p_calcedSizerR1, 0, wxALIGN_LEFT|wxLEFT|wxTOP, 3);

    ewxButton* itemButton20 = new ewxButton( itemFrame1, ID_BUTTON_DIRDY_CALC_R1, _("Calculate..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton20->SetDefault();
    itemButton20->Show(false);
    p_calcedSizerR1->Add(itemButton20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer4->Add(5, 5, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxStaticText* itemStaticText22 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Reactant 2"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemStaticText22, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 3);

    wxBitmap itemBitmapButton23Bitmap(itemFrame1->GetBitmapResource(wxT("../../../data/client/pixmaps/gwbuilder2.xpm")));
    ewxBitmapButton* itemBitmapButton23 = new ewxBitmapButton( itemFrame1, ID_BITMAPBUTTON_DIRDY_BUILD_R2, itemBitmapButton23Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer4->Add(itemBitmapButton23, 0, wxALIGN_LEFT|wxALL, 3);

    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer24, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 3);

    ewxNonBoldLabel* itemStaticText25 = new ewxNonBoldLabel( itemFrame1, ID_STATIC_DIRDY_FORMULA_LABEL_R2, _("Formula:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemStaticText25, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText26 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_FORMULA_R2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemStaticText26, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer27 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer27, 0, wxALIGN_LEFT|wxALL, 3);

    ewxStaticText* itemStaticText28 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CHARGE_R2, _("Charge:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer27->Add(itemStaticText28, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString itemComboBox29Strings[] = {
        _("-2"),
        _("-1"),
        _("0"),
        _("1"),
        _("2")
    };
    ewxComboBox* itemComboBox29 = new ewxComboBox( itemFrame1, ID_COMBOBOX_DIRDY_CHARGE_R2, _("0"), wxDefaultPosition, wxSize(50, -1), 5, itemComboBox29Strings, wxCB_DROPDOWN );
    itemComboBox29->SetStringSelection(_("0"));
    itemBoxSizer27->Add(itemComboBox29, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    wxBoxSizer* itemBoxSizer30 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer30, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 3);

    ewxStaticText* itemStaticText31 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_SPIN_R2, _("Spin:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer30->Add(itemStaticText31, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString* itemChoice32Strings = NULL;
    ewxChoice* itemChoice32 = new ewxChoice( itemFrame1, ID_CHOICE_DIRDY_SPIN_R2, wxDefaultPosition, wxDefaultSize, 0, itemChoice32Strings, 0 );
    itemBoxSizer30->Add(itemChoice32, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    p_codeSizerR2 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(p_codeSizerR2, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP, 3);

    ewxStaticText* itemStaticText34 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CODE_CURRENT_R2, _("Current Code:"), wxDefaultPosition, wxDefaultSize, 0 );
    p_codeSizerR2->Add(itemStaticText34, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText35 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CODE_NONE_R2, _("None"), wxDefaultPosition, wxDefaultSize, 0 );
    p_codeSizerR2->Add(itemStaticText35, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    p_calcedSizerR2 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(p_calcedSizerR2, 0, wxALIGN_LEFT|wxLEFT|wxTOP, 3);

    ewxButton* itemButton37 = new ewxButton( itemFrame1, ID_BUTTON_DIRDY_CALC_R2, _("Calculate..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton37->SetDefault();
    itemButton37->Show(false);
    p_calcedSizerR2->Add(itemButton37, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer3->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer39 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer39, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText40 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Transition State"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer39->Add(itemStaticText40, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 3);

    wxBitmap itemBitmapButton41Bitmap(itemFrame1->GetBitmapResource(wxT("../../../data/client/pixmaps/gwbuilder2.xpm")));
    ewxBitmapButton* itemBitmapButton41 = new ewxBitmapButton( itemFrame1, ID_BITMAPBUTTON_DIRDY_BUILD_TS, itemBitmapButton41Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer39->Add(itemBitmapButton41, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer42 = new wxFlexGridSizer(3, 1, 0, 0);
    itemBoxSizer39->Add(itemFlexGridSizer42, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer43 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer42->Add(itemBoxSizer43, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3);

    ewxNonBoldLabel* itemStaticText44 = new ewxNonBoldLabel( itemFrame1, ID_STATIC_DIRDY_FORMULA_LABEL_TS, _("Formula:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer43->Add(itemStaticText44, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText45 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_FORMULA_TS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer43->Add(itemStaticText45, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer46 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer42->Add(itemBoxSizer46, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 3);

    ewxNonBoldLabel* itemStaticText47 = new ewxNonBoldLabel( itemFrame1, ID_STATIC_DIRDY_BARRIER_LABEL_TS, _("Barrier:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer46->Add(itemStaticText47, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText48 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_BARRIER_TS, _("Unknown"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer46->Add(itemStaticText48, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer49 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer42->Add(itemBoxSizer49, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText50 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CHARGE_TS, _("Charge:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer49->Add(itemStaticText50, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString itemComboBox51Strings[] = {
        _("-2"),
        _("-1"),
        _("0"),
        _("1"),
        _("2")
    };
    ewxComboBox* itemComboBox51 = new ewxComboBox( itemFrame1, ID_COMBOBOX_DIRDY_CHARGE_TS, _("0"), wxDefaultPosition, wxSize(50, -1), 5, itemComboBox51Strings, wxCB_DROPDOWN );
    itemComboBox51->SetStringSelection(_("0"));
    itemBoxSizer49->Add(itemComboBox51, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    wxBoxSizer* itemBoxSizer52 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer42->Add(itemBoxSizer52, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    ewxStaticText* itemStaticText53 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_SPIN_TS, _("Spin:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer52->Add(itemStaticText53, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString* itemChoice54Strings = NULL;
    ewxChoice* itemChoice54 = new ewxChoice( itemFrame1, ID_CHOICE_DIRDY_SPIN_TS, wxDefaultPosition, wxDefaultSize, 0, itemChoice54Strings, 0 );
    itemBoxSizer52->Add(itemChoice54, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    p_codeSizerTS = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer42->Add(p_codeSizerTS, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 3);

    ewxStaticText* itemStaticText56 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CODE_CURRENT_TS, _("Current Code:"), wxDefaultPosition, wxDefaultSize, 0 );
    p_codeSizerTS->Add(itemStaticText56, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText57 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CODE_NONE_TS, _("None"), wxDefaultPosition, wxDefaultSize, 0 );
    p_codeSizerTS->Add(itemStaticText57, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    p_calcedSizerTS = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer42->Add(p_calcedSizerTS, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 3);

    ewxButton* itemButton59 = new ewxButton( itemFrame1, ID_BUTTON_DIRDY_CALC_TS, _("Calculate..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton59->Show(false);
    p_calcedSizerTS->Add(itemButton59, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton60 = new ewxButton( itemFrame1, ID_BUTTON_DIRDY_CREATE_REACT, _("Create Reactants"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemButton60->SetToolTip(_("Clicking this button will take the selected atoms in the builder and turn them into one reactant, the remaining atoms will become the other reactant."));
    itemFlexGridSizer42->Add(itemButton60, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton61 = new ewxButton( itemFrame1, ID_BUTTON_DIRDY_CREATE_PROD, _("Create Products"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemButton61->SetToolTip(_("Clicking this button will take the selected atoms in the builder and turn them into one product, the remaining atoms will become the other product."));
    itemFlexGridSizer42->Add(itemButton61, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer3->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer63 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer63, 0, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText64 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Product 1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer63->Add(itemStaticText64, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 3);

    wxBitmap itemBitmapButton65Bitmap(itemFrame1->GetBitmapResource(wxT("../../../data/client/pixmaps/gwbuilder2.xpm")));
    ewxBitmapButton* itemBitmapButton65 = new ewxBitmapButton( itemFrame1, ID_BITMAPBUTTON_DIRDY_BUILD_P1, itemBitmapButton65Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer63->Add(itemBitmapButton65, 0, wxALIGN_LEFT|wxALL, 3);

    wxBoxSizer* itemBoxSizer66 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer63->Add(itemBoxSizer66, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 3);

    ewxNonBoldLabel* itemStaticText67 = new ewxNonBoldLabel( itemFrame1, ID_STATIC_DIRDY_FORMULA_LABEL_P1, _("Formula:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer66->Add(itemStaticText67, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText68 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_FORMULA_P1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer66->Add(itemStaticText68, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer69 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer63->Add(itemBoxSizer69, 0, wxALIGN_LEFT|wxALL, 3);

    ewxStaticText* itemStaticText70 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CHARGE_P1, _("Charge:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer69->Add(itemStaticText70, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString itemComboBox71Strings[] = {
        _("-2"),
        _("-1"),
        _("0"),
        _("1"),
        _("2")
    };
    ewxComboBox* itemComboBox71 = new ewxComboBox( itemFrame1, ID_COMBOBOX_DIRDY_CHARGE_P1, _("0"), wxDefaultPosition, wxSize(50, -1), 5, itemComboBox71Strings, wxCB_DROPDOWN );
    itemComboBox71->SetStringSelection(_("0"));
    itemBoxSizer69->Add(itemComboBox71, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    wxBoxSizer* itemBoxSizer72 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer63->Add(itemBoxSizer72, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 3);

    ewxStaticText* itemStaticText73 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_SPIN_P1, _("Spin:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer72->Add(itemStaticText73, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString* itemChoice74Strings = NULL;
    ewxChoice* itemChoice74 = new ewxChoice( itemFrame1, ID_CHOICE_DIRDY_SPIN_P1, wxDefaultPosition, wxDefaultSize, 0, itemChoice74Strings, 0 );
    itemBoxSizer72->Add(itemChoice74, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    p_codeSizerP1 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer63->Add(p_codeSizerP1, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP, 3);

    ewxStaticText* itemStaticText76 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CODE_CURRENT_P1, _("Current Code:"), wxDefaultPosition, wxDefaultSize, 0 );
    p_codeSizerP1->Add(itemStaticText76, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText77 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CODE_NONE_P1, _("None"), wxDefaultPosition, wxDefaultSize, 0 );
    p_codeSizerP1->Add(itemStaticText77, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    p_calcedSizerP1 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer63->Add(p_calcedSizerP1, 0, wxALIGN_LEFT|wxLEFT|wxTOP, 3);

    ewxButton* itemButton79 = new ewxButton( itemFrame1, ID_BUTTON_DIRDY_CALC_P1, _("Calculate..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton79->SetDefault();
    itemButton79->Show(false);
    p_calcedSizerP1->Add(itemButton79, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer63->Add(5, 5, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxStaticText* itemStaticText81 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Product 2"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer63->Add(itemStaticText81, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 3);

    wxBitmap itemBitmapButton82Bitmap(itemFrame1->GetBitmapResource(wxT("../../../data/client/pixmaps/gwbuilder2.xpm")));
    ewxBitmapButton* itemBitmapButton82 = new ewxBitmapButton( itemFrame1, ID_BITMAPBUTTON_DIRDY_BUILD_P2, itemBitmapButton82Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer63->Add(itemBitmapButton82, 0, wxALIGN_LEFT|wxALL, 3);

    wxBoxSizer* itemBoxSizer83 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer63->Add(itemBoxSizer83, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT, 3);

    ewxNonBoldLabel* itemStaticText84 = new ewxNonBoldLabel( itemFrame1, ID_STATIC_DIRDY_FORMULA_LABEL_P2, _("Formula:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer83->Add(itemStaticText84, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText85 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_FORMULA_P2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer83->Add(itemStaticText85, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer86 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer63->Add(itemBoxSizer86, 0, wxALIGN_LEFT|wxALL, 3);

    ewxStaticText* itemStaticText87 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CHARGE_P2, _("Charge:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer86->Add(itemStaticText87, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString itemComboBox88Strings[] = {
        _("-2"),
        _("-1"),
        _("0"),
        _("1"),
        _("2")
    };
    ewxComboBox* itemComboBox88 = new ewxComboBox( itemFrame1, ID_COMBOBOX_DIRDY_CHARGE_P2, _("0"), wxDefaultPosition, wxSize(50, -1), 5, itemComboBox88Strings, wxCB_DROPDOWN );
    itemComboBox88->SetStringSelection(_("0"));
    itemBoxSizer86->Add(itemComboBox88, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    wxBoxSizer* itemBoxSizer89 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer63->Add(itemBoxSizer89, 0, wxALIGN_LEFT|wxLEFT, 3);

    ewxStaticText* itemStaticText90 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_SPIN_P2, _("Spin:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer89->Add(itemStaticText90, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString* itemChoice91Strings = NULL;
    ewxChoice* itemChoice91 = new ewxChoice( itemFrame1, ID_CHOICE_DIRDY_SPIN_P2, wxDefaultPosition, wxDefaultSize, 0, itemChoice91Strings, 0 );
    itemBoxSizer89->Add(itemChoice91, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    p_codeSizerP2 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer63->Add(p_codeSizerP2, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP, 3);

    ewxStaticText* itemStaticText93 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CODE_CURRENT_P2, _("Current Code:"), wxDefaultPosition, wxDefaultSize, 0 );
    p_codeSizerP2->Add(itemStaticText93, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText94 = new ewxStaticText( itemFrame1, ID_STATIC_DIRDY_CODE_NONE_P2, _("None"), wxDefaultPosition, wxDefaultSize, 0 );
    p_codeSizerP2->Add(itemStaticText94, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    p_calcedSizerP2 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer63->Add(p_calcedSizerP2, 0, wxALIGN_LEFT|wxLEFT|wxTOP, 3);

    ewxButton* itemButton96 = new ewxButton( itemFrame1, ID_BUTTON_DIRDY_CALC_P2, _("Calculate..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton96->Show(false);
    p_calcedSizerP2->Add(itemButton96, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer97 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer97, 0, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine98 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer97->Add(itemStaticLine98, 1, wxGROW|wxTOP|wxBOTTOM, 3);

    wxBoxSizer* itemBoxSizer99 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer99, 0, wxALIGN_LEFT|wxALL, 3);

    ewxButton* itemButton100 = new ewxButton( itemFrame1, ID_BUTTON_NWDIRDY_SETUP, _("Setup..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer99->Add(itemButton100, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer99->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton102 = new ewxButton( itemFrame1, ID_BUTTON_NWDIRDY_FINAL_EDIT, _("Final Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer99->Add(itemButton102, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer99->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton104 = new ewxButton( itemFrame1, ID_BUTTON_NWDIRDY_LAUNCH, _("Launch..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer99->Add(itemButton104, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer105 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer105, 0, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine106 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer105->Add(itemStaticLine106, 1, wxGROW|wxTOP|wxBOTTOM, 3);

    p_feedbackSizer = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(p_feedbackSizer, 0, wxGROW|wxALL, 0);

    WxFeedback* itemWxFeedback108 = new WxFeedback( itemFrame1, ID_FEEDBACK, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxCLIP_CHILDREN  );
    p_feedbackSizer->Add(itemWxFeedback108, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end NWDirdyGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_NWDIRDY_FRAME
 */

void NWDirdyGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_NWDIRDY_FRAME in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_NWDIRDY_FRAME in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_R1
 */

void NWDirdyGUI::OnBitmapbuttonDirdyBuildR1Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_R1 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_R1 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_R1
 */

void NWDirdyGUI::OnComboboxDirdyChargeR1Selected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_R1 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_R1 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_R1
 */

void NWDirdyGUI::OnComboboxDirdyChargeR1Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_R1 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_R1 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_R1
 */

void NWDirdyGUI::OnChoiceDirdySpinR1Selected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_R1 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_R1 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_R1
 */

void NWDirdyGUI::OnButtonDirdyCalcR1Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_R1 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_R1 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_R2
 */

void NWDirdyGUI::OnBitmapbuttonDirdyBuildR2Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_R2 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_R2 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_R2
 */

void NWDirdyGUI::OnComboboxDirdyChargeR2Selected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_R2 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_R2 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_R2
 */

void NWDirdyGUI::OnComboboxDirdyChargeR2Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_R2 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_R2 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_R2
 */

void NWDirdyGUI::OnChoiceDirdySpinR2Selected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_R2 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_R2 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_R2
 */

void NWDirdyGUI::OnButtonDirdyCalcR2Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_R2 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_R2 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_TS
 */

void NWDirdyGUI::OnBitmapbuttonDirdyBuildTsClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_TS in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_TS in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_TS
 */

void NWDirdyGUI::OnComboboxDirdyChargeTsSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_TS in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_TS in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_TS
 */

void NWDirdyGUI::OnComboboxDirdyChargeTsEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_TS in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_TS in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_TS
 */

void NWDirdyGUI::OnChoiceDirdySpinTsSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_TS in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_TS in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_TS
 */

void NWDirdyGUI::OnButtonDirdyCalcTsClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_TS in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_TS in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CREATE_REACT
 */

void NWDirdyGUI::OnButtonDirdyCreateReactClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CREATE_REACT in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CREATE_REACT in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CREATE_PROD
 */

void NWDirdyGUI::OnButtonDirdyCreateProdClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CREATE_PROD in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CREATE_PROD in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_P1
 */

void NWDirdyGUI::OnBitmapbuttonDirdyBuildP1Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_P1 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_P1 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_P1
 */

void NWDirdyGUI::OnComboboxDirdyChargeP1Selected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_P1 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_P1 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_P1
 */

void NWDirdyGUI::OnComboboxDirdyChargeP1Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_P1 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_P1 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_P1
 */

void NWDirdyGUI::OnChoiceDirdySpinP1Selected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_P1 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_P1 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_P1
 */

void NWDirdyGUI::OnButtonDirdyCalcP1Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_P1 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_P1 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_P2
 */

void NWDirdyGUI::OnBitmapbuttonDirdyBuildP2Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_P2 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_DIRDY_BUILD_P2 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_P2
 */

void NWDirdyGUI::OnComboboxDirdyChargeP2Selected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_P2 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_DIRDY_CHARGE_P2 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_P2
 */

void NWDirdyGUI::OnComboboxDirdyChargeP2Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_P2 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_DIRDY_CHARGE_P2 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_P2
 */

void NWDirdyGUI::OnChoiceDirdySpinP2Selected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_P2 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DIRDY_SPIN_P2 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_P2
 */

void NWDirdyGUI::OnButtonDirdyCalcP2Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_P2 in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIRDY_CALC_P2 in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDIRDY_SETUP
 */

void NWDirdyGUI::OnButtonNwdirdySetupClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDIRDY_SETUP in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDIRDY_SETUP in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDIRDY_FINAL_EDIT
 */

void NWDirdyGUI::OnButtonNwdirdyFinalEditClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDIRDY_FINAL_EDIT in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDIRDY_FINAL_EDIT in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDIRDY_LAUNCH
 */

void NWDirdyGUI::OnButtonNwdirdyLaunchClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDIRDY_LAUNCH in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDIRDY_LAUNCH in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
 */

void NWDirdyGUI::OnSaveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 */

void NWDirdyGUI::OnExitClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK
 */

void NWDirdyGUI::OnMenuFeedbackClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK in NWDirdyGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_MDPREP_HELP
 */

void NWDirdyGUI::OnMenuMdprepHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_MDPREP_HELP in NWDirdyGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_MDPREP_HELP in NWDirdyGUI. 
}

/*!
 * Should we show tooltips?
 */

bool NWDirdyGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap NWDirdyGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin NWDirdyGUI bitmap retrieval
    wxUnusedVar(name);
    if (name == wxT("../../../data/client/pixmaps/gwbuilder2.xpm"))
    {
        ewxBitmap bitmap(_T("../../../data/client/pixmaps/gwbuilder2.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    return wxNullBitmap;
////@end NWDirdyGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon NWDirdyGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin NWDirdyGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end NWDirdyGUI icon retrieval
}
