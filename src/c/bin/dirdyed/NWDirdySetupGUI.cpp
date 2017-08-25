/////////////////////////////////////////////////////////////////////////////
// Name:        NWDirdySetupGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "NWDirdySetupGUI.H"
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
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxButton.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxFrame.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "NWDirdySetupGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID NWDirdySetupGUI::ID_CHOICE_NWDRDYSET_TH = wxNewId();
const wxWindowID NWDirdySetupGUI::wxID_STATIC_NWDRDYSET_TOL = wxNewId();
const wxWindowID NWDirdySetupGUI::wxID_STATIC_NWDRDYSET_BASIS_LABEL = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_TEXTCTRL_NWDRDYSET_TOL = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_NWDRDYSET_FRAME = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_TEXTCTRL_NWDRDYSET_DISK = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_CHECKBOX_NWDRDYSET_SGN = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_BUTTON_NWDRDYSET_SPBASIS = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_CHECKBOX_NWDRDYSET_SYM = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_BUTTON_NWDRDYSET_SPDTLS = wxNewId();
const wxWindowID NWDirdySetupGUI::wxID_STATIC_NWDRDYSET_SLM = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_CHOICE_NWDRDYSET_SPTH = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_TEXTCTRL_NWDRDYSET_SLP = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_TEXTCTRL_NWDRDYSET_SLM = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_CHECKBOX_NWDRDYSET_PRT = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_TEXTCTRL_NWDRDYSET_MEMORY = wxNewId();
const wxWindowID NWDirdySetupGUI::wxID_STATIC_NWDRDYSET_SSV = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_BUTTON_NWDRDYSET_CLOSE = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_TEXTCTRL_NWDRDYSET_SMSS = wxNewId();
const wxWindowID NWDirdySetupGUI::wxID_STATIC_NWDRDYSET_SLP = wxNewId();
const wxWindowID NWDirdySetupGUI::wxID_STATIC_NWDRDYSET_SSTEP = wxNewId();
const wxWindowID NWDirdySetupGUI::wxID_STATIC_NWDRDYSET_SHSS = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_STATIC_NWDRDYSET_SPBASIS_NAME = wxNewId();
const wxWindowID NWDirdySetupGUI::wxID_STATICTEXT_NWDRDYSET_MEMORY = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_STATIC_NWDRDYSET_BASIS_NAME = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_FEEDBACK_NWDRDYSET = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_TEXTCTRL_NWDRDYSET_SSTEP = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_CHECKBOX_NWDRDYSET_EXPCOEFF = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_BUTTON_NWDRDYSET_BASIS = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_BUTTON_NWDRDYSET_DTLS = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_CHECKBOX_NWDRDYSET_DISK = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_CHECKBOX_NWDRDYSET_TST = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_CHOICE_NWDRDYSET_INTALG = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_CHECKBOX_NWDRDYSET_MEMORY = wxNewId();
const wxWindowID NWDirdySetupGUI::wxID_STATIC_NWDRDYSET_SEP = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_BUTTON_NWDRDYSET_RESET = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_CHECKBOX_NWDRDYSET_AUTOZ = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF = wxNewId();
const wxWindowID NWDirdySetupGUI::wxID_STATIC_NWDRDYSET_SMSS = wxNewId();
const wxWindowID NWDirdySetupGUI::wxID_STATICTEXT_NWDRDYSET_DISK = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_TEXTCTRL_NWDRDYSET_SEP = wxNewId();
const wxWindowID NWDirdySetupGUI::wxID_STATIC_NWDRDYSET_SPBASIS_LABEL = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_TEXTCTRL_NWDRDYSET_SHSS = wxNewId();
const wxWindowID NWDirdySetupGUI::ID_TEXTCTRL_NWDRDYSET_SSV = wxNewId();

/*!
 * NWDirdySetupGUI type definition
 */

IMPLEMENT_CLASS( NWDirdySetupGUI, ewxFrame )

/*!
 * NWDirdySetupGUI event table definition
 */

BEGIN_EVENT_TABLE( NWDirdySetupGUI, ewxFrame )

////@begin NWDirdySetupGUI event table entries
    EVT_CLOSE( NWDirdySetupGUI::OnCloseWindow )

    EVT_CHECKBOX( ID_CHECKBOX_NWDRDYSET_SYM, NWDirdySetupGUI::OnCheckboxNwdrdysetSymClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_NWDRDYSET_TOL, NWDirdySetupGUI::OnTextctrlNwdrdysetTolEnter )

    EVT_CHECKBOX( ID_CHECKBOX_NWDRDYSET_AUTOZ, NWDirdySetupGUI::OnCheckboxNwdrdysetAutozClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_NWDRDYSET_SEP, NWDirdySetupGUI::OnTextctrlNwdrdysetSepEnter )

    EVT_CHECKBOX( ID_CHECKBOX_NWDRDYSET_MEMORY, NWDirdySetupGUI::OnCheckboxNwdrdysetMemoryClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_NWDRDYSET_MEMORY, NWDirdySetupGUI::OnTextctrlNwdrdysetMemoryEnter )

    EVT_CHECKBOX( ID_CHECKBOX_NWDRDYSET_DISK, NWDirdySetupGUI::OnCheckboxNwdrdysetDiskClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_NWDRDYSET_DISK, NWDirdySetupGUI::OnTextctrlNwdrdysetDiskEnter )

    EVT_CHOICE( ID_CHOICE_NWDRDYSET_TH, NWDirdySetupGUI::OnChoiceNwdrdysetThSelected )

    EVT_CHOICE( ID_CHOICE_NWDRDYSET_SPTH, NWDirdySetupGUI::OnChoiceNwdrdysetSpthSelected )

    EVT_BUTTON( ID_BUTTON_NWDRDYSET_BASIS, NWDirdySetupGUI::OnButtonNwdrdysetBasisClick )

    EVT_BUTTON( ID_BUTTON_NWDRDYSET_SPBASIS, NWDirdySetupGUI::OnButtonNwdrdysetSpbasisClick )

    EVT_CHECKBOX( ID_CHECKBOX_NWDRDYSET_EXPCOEFF, NWDirdySetupGUI::OnCheckboxNwdrdysetExpcoeffClick )

    EVT_CHECKBOX( ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF, NWDirdySetupGUI::OnCheckboxNwdrdysetSpexpcoeffClick )

    EVT_BUTTON( ID_BUTTON_NWDRDYSET_DTLS, NWDirdySetupGUI::OnButtonNwdrdysetDtlsClick )

    EVT_BUTTON( ID_BUTTON_NWDRDYSET_SPDTLS, NWDirdySetupGUI::OnButtonNwdrdysetSpdtlsClick )

    EVT_CHECKBOX( ID_CHECKBOX_NWDRDYSET_TST, NWDirdySetupGUI::OnCheckboxNwdrdysetTstClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_NWDRDYSET_SSTEP, NWDirdySetupGUI::OnTextctrlNwdrdysetSstepEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_NWDRDYSET_SSV, NWDirdySetupGUI::OnTextctrlNwdrdysetSsvEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_NWDRDYSET_SHSS, NWDirdySetupGUI::OnTextctrlNwdrdysetShssEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_NWDRDYSET_SMSS, NWDirdySetupGUI::OnTextctrlNwdrdysetSmssEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_NWDRDYSET_SLM, NWDirdySetupGUI::OnTextctrlNwdrdysetSlmEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_NWDRDYSET_SLP, NWDirdySetupGUI::OnTextctrlNwdrdysetSlpEnter )

    EVT_CHECKBOX( ID_CHECKBOX_NWDRDYSET_SGN, NWDirdySetupGUI::OnCheckboxNwdrdysetSgnClick )

    EVT_CHOICE( ID_CHOICE_NWDRDYSET_INTALG, NWDirdySetupGUI::OnChoiceNwdrdysetIntalgSelected )

    EVT_CHECKBOX( ID_CHECKBOX_NWDRDYSET_PRT, NWDirdySetupGUI::OnCheckboxNwdrdysetPrtClick )

    EVT_BUTTON( ID_BUTTON_NWDRDYSET_CLOSE, NWDirdySetupGUI::OnButtonNwdrdysetCloseClick )

    EVT_BUTTON( ID_BUTTON_NWDRDYSET_RESET, NWDirdySetupGUI::OnButtonNwdrdysetResetClick )

////@end NWDirdySetupGUI event table entries

END_EVENT_TABLE()

/*!
 * NWDirdySetupGUI constructors
 */

NWDirdySetupGUI::NWDirdySetupGUI( )
{
}

NWDirdySetupGUI::NWDirdySetupGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * NWDirdySetupGUI creator
 */

bool NWDirdySetupGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin NWDirdySetupGUI member initialisation
////@end NWDirdySetupGUI member initialisation

////@begin NWDirdySetupGUI creation
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
////@end NWDirdySetupGUI creation
    return true;
}

/*!
 * Control creation for NWDirdySetupGUI
 */

void NWDirdySetupGUI::CreateControls()
{    
////@begin NWDirdySetupGUI content construction
    NWDirdySetupGUI* itemFrame1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(2, 2, 0, 20);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer3->Add(itemBoxSizer4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 0);

    ewxCheckBox* itemCheckBox5 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_NWDRDYSET_SYM, _("Use Available Symmetry"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox5->SetValue(false);
    itemBoxSizer4->Add(itemCheckBox5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer3->Add(itemBoxSizer6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Sym. Tolerance:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemStaticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl8 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_NWDRDYSET_TOL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemTextCtrl8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemFrame1, wxID_STATIC_NWDRDYSET_TOL, _("Bohr"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemStaticText9, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer3->Add(itemBoxSizer10, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 0);

    ewxCheckBox* itemCheckBox11 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_NWDRDYSET_AUTOZ, _("Use Automatic Z-matrix"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox11->SetValue(false);
    itemBoxSizer10->Add(itemCheckBox11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer3->Add(itemBoxSizer12, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText13 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Frag. Separation:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemStaticText13, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl14 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_NWDRDYSET_SEP, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemTextCtrl14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText15 = new ewxStaticText( itemFrame1, wxID_STATIC_NWDRDYSET_SEP, _("Bohr"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemStaticText15, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer16, 0, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine17 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxSize(20, -1), wxLI_HORIZONTAL );
    itemBoxSizer16->Add(itemStaticLine17, 0, wxGROW|wxALL, 3);

    ewxStaticText* itemStaticText18 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Memory/Disk Limits"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add(itemStaticText18, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticLine* itemStaticLine19 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer16->Add(itemStaticLine19, 1, wxGROW|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer20 = new wxFlexGridSizer(2, 2, 0, 20);
    itemBoxSizer2->Add(itemFlexGridSizer20, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer20->Add(itemBoxSizer21, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxCheckBox* itemCheckBox22 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_NWDRDYSET_MEMORY, _("Memory:"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox22->SetValue(false);
    itemBoxSizer21->Add(itemCheckBox22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxTextCtrl* itemTextCtrl23 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_NWDRDYSET_MEMORY, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(itemTextCtrl23, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText24 = new ewxStaticText( itemFrame1, wxID_STATICTEXT_NWDRDYSET_MEMORY, _("MWords"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText24->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemBoxSizer21->Add(itemStaticText24, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer20->Add(itemBoxSizer25, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxCheckBox* itemCheckBox26 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_NWDRDYSET_DISK, _("Disk:"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox26->SetValue(false);
    itemBoxSizer25->Add(itemCheckBox26, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxTextCtrl* itemTextCtrl27 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_NWDRDYSET_DISK, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer25->Add(itemTextCtrl27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText28 = new ewxStaticText( itemFrame1, wxID_STATICTEXT_NWDRDYSET_DISK, _("MWords"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText28->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemBoxSizer25->Add(itemStaticText28, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer29 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer29, 0, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine30 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxSize(20, -1), wxLI_HORIZONTAL );
    itemBoxSizer29->Add(itemStaticLine30, 0, wxGROW|wxALL, 3);

    ewxStaticText* itemStaticText31 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Theory"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer29->Add(itemStaticText31, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticLine* itemStaticLine32 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer29->Add(itemStaticLine32, 1, wxGROW|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer33 = new wxFlexGridSizer(3, 2, 0, 40);
    itemBoxSizer2->Add(itemFlexGridSizer33, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer34 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer33->Add(itemBoxSizer34, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText35 = new ewxStaticText( itemFrame1, wxID_STATIC, _("General Theory:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer34->Add(itemStaticText35, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemChoice36Strings = NULL;
    ewxChoice* itemChoice36 = new ewxChoice( itemFrame1, ID_CHOICE_NWDRDYSET_TH, wxDefaultPosition, wxDefaultSize, 0, itemChoice36Strings, 0 );
    itemBoxSizer34->Add(itemChoice36, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer37 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer33->Add(itemBoxSizer37, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText38 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Single Point:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer37->Add(itemStaticText38, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemChoice39Strings = NULL;
    ewxChoice* itemChoice39 = new ewxChoice( itemFrame1, ID_CHOICE_NWDRDYSET_SPTH, wxDefaultPosition, wxDefaultSize, 0, itemChoice39Strings, 0 );
    itemBoxSizer37->Add(itemChoice39, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer40 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer33->Add(itemBoxSizer40, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton41 = new ewxButton( itemFrame1, ID_BUTTON_NWDRDYSET_BASIS, _("General Theory Basis Set..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemButton41, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer42 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer33->Add(itemBoxSizer42, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton43 = new ewxButton( itemFrame1, ID_BUTTON_NWDRDYSET_SPBASIS, _("Single Point Basis Set..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer42->Add(itemButton43, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer44 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer33->Add(itemBoxSizer44, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxNonBoldLabel* itemStaticText45 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC_NWDRDYSET_BASIS_LABEL, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer44->Add(itemStaticText45, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxBOTTOM|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText46 = new ewxStaticText( itemFrame1, ID_STATIC_NWDRDYSET_BASIS_NAME, _("None"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer44->Add(itemStaticText46, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxBOTTOM|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer47 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer33->Add(itemBoxSizer47, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxNonBoldLabel* itemStaticText48 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC_NWDRDYSET_SPBASIS_LABEL, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer47->Add(itemStaticText48, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxBOTTOM|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText49 = new ewxStaticText( itemFrame1, ID_STATIC_NWDRDYSET_SPBASIS_NAME, _("None"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer47->Add(itemStaticText49, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxBOTTOM|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer50 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer33->Add(itemBoxSizer50, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxCheckBox* itemCheckBox51 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_NWDRDYSET_EXPCOEFF, _("Use Basis Exponents\nand Coefficients"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox51->SetValue(false);
    itemBoxSizer50->Add(itemCheckBox51, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer52 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer33->Add(itemBoxSizer52, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxCheckBox* itemCheckBox53 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF, _("Use Basis Exponents\nand Coefficients"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox53->SetValue(false);
    itemBoxSizer52->Add(itemCheckBox53, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer54 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer33->Add(itemBoxSizer54, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton55 = new ewxButton( itemFrame1, ID_BUTTON_NWDRDYSET_DTLS, _("General Theory Details..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer54->Add(itemButton55, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer56 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer33->Add(itemBoxSizer56, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton57 = new ewxButton( itemFrame1, ID_BUTTON_NWDRDYSET_SPDTLS, _("Single Point Details..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer56->Add(itemButton57, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer58 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer58, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox59 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_NWDRDYSET_TST, _("Transition State Theory Only Calculation"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox59->SetValue(true);
    itemBoxSizer58->Add(itemCheckBox59, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer60 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer60, 0, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine61 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxSize(20, -1), wxLI_HORIZONTAL );
    itemBoxSizer60->Add(itemStaticLine61, 0, wxGROW|wxALL, 3);

    ewxStaticText* itemStaticText62 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Path Parameters"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer60->Add(itemStaticText62, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticLine* itemStaticLine63 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer60->Add(itemStaticLine63, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer64 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer64, 0, wxALIGN_LEFT|wxRIGHT|wxTOP|wxBOTTOM, 0);

    ewxStaticText* itemStaticText65 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Minimum Energy Path Step Increment:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer64->Add(itemStaticText65, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl66 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_NWDRDYSET_SSTEP, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer64->Add(itemTextCtrl66, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText67 = new ewxStaticText( itemFrame1, wxID_STATIC_NWDRDYSET_SSTEP, _("Bohr"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer64->Add(itemStaticText67, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer68 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer68, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText69 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Increment for Saving Potential and Gradients:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer68->Add(itemStaticText69, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl70 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_NWDRDYSET_SSV, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer68->Add(itemTextCtrl70, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText71 = new ewxStaticText( itemFrame1, wxID_STATIC_NWDRDYSET_SSV, _("Bohr"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer68->Add(itemStaticText71, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer72 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer72, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText73 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Increment for Recalculating Hessian:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer72->Add(itemStaticText73, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl74 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_NWDRDYSET_SHSS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer72->Add(itemTextCtrl74, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText75 = new ewxStaticText( itemFrame1, wxID_STATIC_NWDRDYSET_SHSS, _("Bohr"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer72->Add(itemStaticText75, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer76 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer76, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText77 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Reduced Bimolecular Mass:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer76->Add(itemStaticText77, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl78 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_NWDRDYSET_SMSS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer76->Add(itemTextCtrl78, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText79 = new ewxStaticText( itemFrame1, wxID_STATIC_NWDRDYSET_SMSS, _("amu"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer76->Add(itemStaticText79, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer80 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer80, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText81 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Lower Limit of Reaction Coordinate:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer80->Add(itemStaticText81, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl82 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_NWDRDYSET_SLM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer80->Add(itemTextCtrl82, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText83 = new ewxStaticText( itemFrame1, wxID_STATIC_NWDRDYSET_SLM, _("Bohr"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer80->Add(itemStaticText83, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer84 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer84, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText85 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Upper Limit of Reaction Coordinate:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer84->Add(itemStaticText85, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl86 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_NWDRDYSET_SLP, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer84->Add(itemTextCtrl86, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText87 = new ewxStaticText( itemFrame1, wxID_STATIC_NWDRDYSET_SLP, _("Bohr"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer84->Add(itemStaticText87, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer88 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer88, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox89 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_NWDRDYSET_SGN, _("Switch Sign Along Reaction Coordinate"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox89->SetValue(false);
    itemBoxSizer88->Add(itemCheckBox89, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer90 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer90, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText91 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Integration Algorithm:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer90->Add(itemStaticText91, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice92Strings[] = {
        _("Euler"),
        _("Local Quadratic Approximation"),
        _("Corrected LQA"),
        _("Cubic")
    };
    ewxChoice* itemChoice92 = new ewxChoice( itemFrame1, ID_CHOICE_NWDRDYSET_INTALG, wxDefaultPosition, wxDefaultSize, 4, itemChoice92Strings, 0 );
    itemChoice92->SetStringSelection(_("Euler"));
    itemBoxSizer90->Add(itemChoice92, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer93 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer93, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox94 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_NWDRDYSET_PRT, _("Print Projected Frequencies and Eigenvectors"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox94->SetValue(false);
    itemBoxSizer93->Add(itemCheckBox94, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer95 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer95, 0, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine96 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer95->Add(itemStaticLine96, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer97 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer97, 0, wxGROW|wxALL, 0);

    WxFeedback* itemWxFeedback98 = new WxFeedback( itemFrame1, ID_FEEDBACK_NWDRDYSET, wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
    itemBoxSizer97->Add(itemWxFeedback98, 1, wxGROW|wxRIGHT|wxTOP|wxBOTTOM, 0);

    wxBoxSizer* itemBoxSizer99 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer99, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    ewxButton* itemButton100 = new ewxButton( itemFrame1, ID_BUTTON_NWDRDYSET_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer99->Add(itemButton100, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer99->Add(70, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton102 = new ewxButton( itemFrame1, ID_BUTTON_NWDRDYSET_RESET, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer99->Add(itemButton102, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

////@end NWDirdySetupGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_NWDRDYSET_FRAME
 */

void NWDirdySetupGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_NWDRDYSET_FRAME in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_NWDRDYSET_FRAME in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_SYM
 */

void NWDirdySetupGUI::OnCheckboxNwdrdysetSymClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_SYM in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_SYM in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_TOL
 */

void NWDirdySetupGUI::OnTextctrlNwdrdysetTolEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_TOL in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_TOL in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_AUTOZ
 */

void NWDirdySetupGUI::OnCheckboxNwdrdysetAutozClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_AUTOZ in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_AUTOZ in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SEP
 */

void NWDirdySetupGUI::OnTextctrlNwdrdysetSepEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SEP in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SEP in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_MEMORY
 */

void NWDirdySetupGUI::OnCheckboxNwdrdysetMemoryClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_MEMORY in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_MEMORY in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_MEMORY
 */

void NWDirdySetupGUI::OnTextctrlNwdrdysetMemoryEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_MEMORY in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_MEMORY in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_DISK
 */

void NWDirdySetupGUI::OnCheckboxNwdrdysetDiskClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_DISK in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_DISK in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_DISK
 */

void NWDirdySetupGUI::OnTextctrlNwdrdysetDiskEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_DISK in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_DISK in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_NWDRDYSET_TH
 */

void NWDirdySetupGUI::OnChoiceNwdrdysetThSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_NWDRDYSET_TH in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_NWDRDYSET_TH in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_NWDRDYSET_SPTH
 */

void NWDirdySetupGUI::OnChoiceNwdrdysetSpthSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_NWDRDYSET_SPTH in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_NWDRDYSET_SPTH in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_BASIS
 */

void NWDirdySetupGUI::OnButtonNwdrdysetBasisClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_BASIS in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_BASIS in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_SPBASIS
 */

void NWDirdySetupGUI::OnButtonNwdrdysetSpbasisClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_SPBASIS in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_SPBASIS in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_EXPCOEFF
 */

void NWDirdySetupGUI::OnCheckboxNwdrdysetExpcoeffClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_EXPCOEFF in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_EXPCOEFF in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF
 */

void NWDirdySetupGUI::OnCheckboxNwdrdysetSpexpcoeffClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_DTLS
 */

void NWDirdySetupGUI::OnButtonNwdrdysetDtlsClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_DTLS in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_DTLS in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_SPDTLS
 */

void NWDirdySetupGUI::OnButtonNwdrdysetSpdtlsClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_SPDTLS in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_SPDTLS in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_TST
 */

void NWDirdySetupGUI::OnCheckboxNwdrdysetTstClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_TST in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_TST in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SSTEP
 */

void NWDirdySetupGUI::OnTextctrlNwdrdysetSstepEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SSTEP in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SSTEP in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SSV
 */

void NWDirdySetupGUI::OnTextctrlNwdrdysetSsvEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SSV in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SSV in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SHSS
 */

void NWDirdySetupGUI::OnTextctrlNwdrdysetShssEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SHSS in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SHSS in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SMSS
 */

void NWDirdySetupGUI::OnTextctrlNwdrdysetSmssEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SMSS in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SMSS in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SLM
 */

void NWDirdySetupGUI::OnTextctrlNwdrdysetSlmEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SLM in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SLM in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SLP
 */

void NWDirdySetupGUI::OnTextctrlNwdrdysetSlpEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SLP in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SLP in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_SGN
 */

void NWDirdySetupGUI::OnCheckboxNwdrdysetSgnClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_SGN in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_SGN in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_NWDRDYSET_INTALG
 */

void NWDirdySetupGUI::OnChoiceNwdrdysetIntalgSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_NWDRDYSET_INTALG in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_NWDRDYSET_INTALG in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_PRT
 */

void NWDirdySetupGUI::OnCheckboxNwdrdysetPrtClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_PRT in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_PRT in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_CLOSE
 */

void NWDirdySetupGUI::OnButtonNwdrdysetCloseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_CLOSE in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_CLOSE in NWDirdySetupGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_RESET
 */

void NWDirdySetupGUI::OnButtonNwdrdysetResetClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_RESET in NWDirdySetupGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_RESET in NWDirdySetupGUI. 
}

/*!
 * Should we show tooltips?
 */

bool NWDirdySetupGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap NWDirdySetupGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin NWDirdySetupGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end NWDirdySetupGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon NWDirdySetupGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin NWDirdySetupGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end NWDirdySetupGUI icon retrieval
}
