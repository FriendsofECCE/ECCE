/////////////////////////////////////////////////////////////////////////////
// Name:        InteractionPanelGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "InteractionPanelGUI.H"
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
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxOpenCloseToggle.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxgui/InteractionPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID InteractionPanelGUI::ID_TEXTCTRL_INTRCT_SCUT = wxNewId();
const wxWindowID InteractionPanelGUI::ID_PANEL_INTRCT_PARAMETER = wxNewId();
const wxWindowID InteractionPanelGUI::ID_CHECKBOX_INTRCT_FSTORD = wxNewId();
const wxWindowID InteractionPanelGUI::ID_TOGGLE_INTRCT_ADVFFT = wxNewId();
const wxWindowID InteractionPanelGUI::ID_CHOICE_INTRCT_FFT_SRC = wxNewId();
const wxWindowID InteractionPanelGUI::ID_CHOICE_INTRCT_ES_OPT = wxNewId();
const wxWindowID InteractionPanelGUI::ID_DIALOG_INTRCT = wxNewId();
const wxWindowID InteractionPanelGUI::ID_CHECKBOX_INTRCT_PERT2 = wxNewId();
const wxWindowID InteractionPanelGUI::ID_RADIOBUTTON_INTRCT_SET2 = wxNewId();
const wxWindowID InteractionPanelGUI::ID_RADIOBUTTON_INTRCT_SET1 = wxNewId();
const wxWindowID InteractionPanelGUI::wxID_STATIC_INTRCT_ULCUT = wxNewId();
const wxWindowID InteractionPanelGUI::wxID_STATIC_INTRCT_ESPLINE = wxNewId();
const wxWindowID InteractionPanelGUI::ID_PANEL_INTRCT_FFTOPT = wxNewId();
const wxWindowID InteractionPanelGUI::ID_CHECKBOX_INTRCT_QGRID = wxNewId();
const wxWindowID InteractionPanelGUI::ID_TEXTCTRL_INTRCT_GRID = wxNewId();
const wxWindowID InteractionPanelGUI::ID_STATICTEXT1 = wxNewId();
const wxWindowID InteractionPanelGUI::ID_TEXTCTRL_INTRCT_LCUT = wxNewId();
const wxWindowID InteractionPanelGUI::wxID_STATIC_INTRCT_NFFT = wxNewId();
const wxWindowID InteractionPanelGUI::ID_TEXTCTRL_INTRCT_EWTOL = wxNewId();
const wxWindowID InteractionPanelGUI::ID_TEXTCTRL_INTRCT_ESPLINE = wxNewId();
const wxWindowID InteractionPanelGUI::wxID_STATIC_INTRCT_LFREQ = wxNewId();
const wxWindowID InteractionPanelGUI::ID_TEXTCTRL_INTRCT_NFFT = wxNewId();
const wxWindowID InteractionPanelGUI::ID_CHECKBOX_INTRCT_PERT1 = wxNewId();
const wxWindowID InteractionPanelGUI::ID_RADIOBUTTON_INTRCT_SROPT = wxNewId();
const wxWindowID InteractionPanelGUI::ID_TEXTCTRL_INTRCT_LFREQ = wxNewId();
const wxWindowID InteractionPanelGUI::ID_PANEL_INTRCT_CUTOFF = wxNewId();
const wxWindowID InteractionPanelGUI::ID_RADIOBUTTON_INTRCT_DROPT = wxNewId();
const wxWindowID InteractionPanelGUI::wxID_STATIC_INTRCT_ECNV = wxNewId();
const wxWindowID InteractionPanelGUI::ID_PANEL_INTRCT_POLARIZATION = wxNewId();
const wxWindowID InteractionPanelGUI::wxID_STATIC_INTRCT_LCUT = wxNewId();
const wxWindowID InteractionPanelGUI::ID_PANEL_INTRCT_EWALD = wxNewId();
const wxWindowID InteractionPanelGUI::ID_TEXTCTRL_INTRCT_PTOL = wxNewId();
const wxWindowID InteractionPanelGUI::wxID_STATIC_INTRCT_SCUT = wxNewId();
const wxWindowID InteractionPanelGUI::ID_TEXTCTRL_INTRCT_SFREQ = wxNewId();
const wxWindowID InteractionPanelGUI::ID_PANEL_INTRCT_LCUT = wxNewId();
const wxWindowID InteractionPanelGUI::ID_RADIOBUTTON_INTRCT_SET3 = wxNewId();
const wxWindowID InteractionPanelGUI::ID_TEXTCTRL_INTRCT_MAXIT = wxNewId();
const wxWindowID InteractionPanelGUI::wxID_STATIC_INTRCT_GRID = wxNewId();
const wxWindowID InteractionPanelGUI::wxID_STATIC_INTRCT_SFREQ = wxNewId();

/*!
 * InteractionPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( InteractionPanelGUI, ewxPanel )

/*!
 * InteractionPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( InteractionPanelGUI, ewxPanel )

////@begin InteractionPanelGUI event table entries
    EVT_RADIOBUTTON( ID_RADIOBUTTON_INTRCT_SET1, InteractionPanelGUI::OnRadiobuttonIntrctSet1Selected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_INTRCT_SET2, InteractionPanelGUI::OnRadiobuttonIntrctSet2Selected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_INTRCT_SET3, InteractionPanelGUI::OnRadiobuttonIntrctSet3Selected )

    EVT_CHECKBOX( ID_CHECKBOX_INTRCT_PERT1, InteractionPanelGUI::OnCheckboxIntrctPert1Click )

    EVT_CHECKBOX( ID_CHECKBOX_INTRCT_PERT2, InteractionPanelGUI::OnCheckboxIntrctPert2Click )

    EVT_CHOICE( ID_CHOICE_INTRCT_ES_OPT, InteractionPanelGUI::OnChoiceIntrctEsOptSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_INTRCT_GRID, InteractionPanelGUI::OnTextctrlIntrctGridEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_INTRCT_EWTOL, InteractionPanelGUI::OnTextctrlIntrctEwtolEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_INTRCT_ESPLINE, InteractionPanelGUI::OnTextctrlIntrctEsplineEnter )

    EVT_CHECKBOX( ID_CHECKBOX_INTRCT_QGRID, InteractionPanelGUI::OnCheckboxIntrctQgridClick )

    EVT_BUTTON( ID_TOGGLE_INTRCT_ADVFFT, InteractionPanelGUI::OnToggleIntrctAdvfftClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_INTRCT_NFFT, InteractionPanelGUI::OnTextctrlIntrctNfftEnter )

    EVT_CHOICE( ID_CHOICE_INTRCT_FFT_SRC, InteractionPanelGUI::OnChoiceIntrctFftSrcSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_INTRCT_MAXIT, InteractionPanelGUI::OnTextctrlIntrctMaxitEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_INTRCT_PTOL, InteractionPanelGUI::OnTextctrlIntrctPtolEnter )

    EVT_CHECKBOX( ID_CHECKBOX_INTRCT_FSTORD, InteractionPanelGUI::OnCheckboxIntrctFstordClick )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_INTRCT_SROPT, InteractionPanelGUI::OnRadiobuttonIntrctSroptSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_INTRCT_DROPT, InteractionPanelGUI::OnRadiobuttonIntrctDroptSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_INTRCT_SCUT, InteractionPanelGUI::OnTextctrlIntrctScutEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_INTRCT_SFREQ, InteractionPanelGUI::OnTextctrlIntrctSfreqEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_INTRCT_LCUT, InteractionPanelGUI::OnTextctrlIntrctLcutEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_INTRCT_LFREQ, InteractionPanelGUI::OnTextctrlIntrctLfreqEnter )

////@end InteractionPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * InteractionPanelGUI constructors
 */

InteractionPanelGUI::InteractionPanelGUI( )
{
}

InteractionPanelGUI::InteractionPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * InteractionPanelGUI creator
 */

bool InteractionPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin InteractionPanelGUI member initialisation
    p_mainVSizer = NULL;
    p_parameterPanel = NULL;
    p_ewaldPanel = NULL;
    p_EwaldSizer = NULL;
    p_FFTVSizer = NULL;
    p_fftPanel = NULL;
    p_polarizationPanel = NULL;
    p_cutoffPanel = NULL;
    p_cutoffVSizer = NULL;
    p_cutoffHSizer = NULL;
    p_lcutoffPanel = NULL;
////@end InteractionPanelGUI member initialisation

////@begin InteractionPanelGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_VALIDATE_RECURSIVELY|wxWS_EX_BLOCK_EVENTS);
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end InteractionPanelGUI creation
    return true;
}

/*!
 * Control creation for InteractionPanelGUI
 */

void InteractionPanelGUI::CreateControls()
{    
////@begin InteractionPanelGUI content construction
    InteractionPanelGUI* itemPanel1 = this;

    p_mainVSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(p_mainVSizer);

    p_parameterPanel = new ewxPanel( itemPanel1, ID_PANEL_INTRCT_PARAMETER, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    p_parameterPanel->Show(false);
    p_mainVSizer->Add(p_parameterPanel, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    p_parameterPanel->SetSizer(itemBoxSizer4);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer5, 0, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine6 = new ewxStaticLine( p_parameterPanel, wxID_STATIC, wxDefaultPosition, wxSize(10, -1), wxLI_HORIZONTAL );
    itemBoxSizer5->Add(itemStaticLine6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText7 = new ewxStaticText( p_parameterPanel, wxID_STATIC, _("Parameter Set"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine8 = new ewxStaticLine( p_parameterPanel, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer5->Add(itemStaticLine8, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer9, 0, wxALIGN_LEFT|wxALL, 0);

    ewxRadioButton* itemRadioButton10 = new ewxRadioButton( p_parameterPanel, ID_RADIOBUTTON_INTRCT_SET1, _("Current Parameter Set"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton10->SetValue(false);
    itemBoxSizer9->Add(itemRadioButton10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton11 = new ewxRadioButton( p_parameterPanel, ID_RADIOBUTTON_INTRCT_SET2, _("Initial Parameter Set"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton11->SetValue(false);
    itemBoxSizer9->Add(itemRadioButton11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton12 = new ewxRadioButton( p_parameterPanel, ID_RADIOBUTTON_INTRCT_SET3, _("Final Parameter Set"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton12->SetValue(false);
    itemBoxSizer9->Add(itemRadioButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer13, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox14 = new ewxCheckBox( p_parameterPanel, ID_CHECKBOX_INTRCT_PERT1, _("Use First Perturbation Parameter Set"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox14->SetValue(false);
    itemBoxSizer13->Add(itemCheckBox14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox15 = new ewxCheckBox( p_parameterPanel, ID_CHECKBOX_INTRCT_PERT2, _("Use Second Perturbation Parameter Set"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox15->SetValue(false);
    itemBoxSizer13->Add(itemCheckBox15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    p_mainVSizer->Add(itemBoxSizer16, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText17 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Electrostatic Interaction Model:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add(itemStaticText17, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice18Strings[] = {
        _("Particle-Mesh Ewald"),
        _("Cutoff only")
    };
    ewxChoice* itemChoice18 = new ewxChoice( itemPanel1, ID_CHOICE_INTRCT_ES_OPT, wxDefaultPosition, wxDefaultSize, 2, itemChoice18Strings, 0 );
    itemChoice18->SetStringSelection(_("Particle-Mesh Ewald"));
    itemBoxSizer16->Add(itemChoice18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_ewaldPanel = new ewxPanel( itemPanel1, ID_PANEL_INTRCT_EWALD, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    p_mainVSizer->Add(p_ewaldPanel, 0, wxGROW|wxALL, 5);

    p_EwaldSizer = new wxBoxSizer(wxVERTICAL);
    p_ewaldPanel->SetSizer(p_EwaldSizer);

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxHORIZONTAL);
    p_EwaldSizer->Add(itemBoxSizer21, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText22 = new ewxStaticText( p_ewaldPanel, wxID_STATIC_INTRCT_GRID, _("Grid Points per Dimension:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(itemStaticText22, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl23 = new ewxTextCtrl( p_ewaldPanel, ID_TEXTCTRL_INTRCT_GRID, _("8"), wxDefaultPosition, wxSize(50, -1), 0 );
    itemBoxSizer21->Add(itemTextCtrl23, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxHORIZONTAL);
    p_EwaldSizer->Add(itemBoxSizer24, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText25 = new ewxStaticText( p_ewaldPanel, wxID_STATIC_INTRCT_ECNV, _("Ewald Convergence Coefficient:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemStaticText25, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl26 = new ewxTextCtrl( p_ewaldPanel, ID_TEXTCTRL_INTRCT_EWTOL, _("0.0001"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemTextCtrl26, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer24->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText28 = new ewxStaticText( p_ewaldPanel, wxID_STATIC_INTRCT_ESPLINE, _("Cardinal B-Spline Order:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemStaticText28, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl29 = new ewxTextCtrl( p_ewaldPanel, ID_TEXTCTRL_INTRCT_ESPLINE, _("4"), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer24->Add(itemTextCtrl29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer30 = new wxBoxSizer(wxHORIZONTAL);
    p_EwaldSizer->Add(itemBoxSizer30, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox31 = new ewxCheckBox( p_ewaldPanel, ID_CHECKBOX_INTRCT_QGRID, _("Charge Grid from Solvent Only"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox31->SetValue(false);
    itemBoxSizer30->Add(itemCheckBox31, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15);

    p_FFTVSizer = new wxBoxSizer(wxVERTICAL);
    p_EwaldSizer->Add(p_FFTVSizer, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer33 = new wxBoxSizer(wxHORIZONTAL);
    p_FFTVSizer->Add(itemBoxSizer33, 0, wxALIGN_LEFT|wxALL, 0);

    itemBoxSizer33->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxBitmap itemBitmapButton35Bitmap(wxNullBitmap);
    ewxOpenCloseToggle* itemBitmapButton35 = new ewxOpenCloseToggle( p_ewaldPanel, ID_TOGGLE_INTRCT_ADVFFT, itemBitmapButton35Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer33->Add(itemBitmapButton35, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText36 = new ewxStaticText( p_ewaldPanel, wxID_STATIC, _("Advanced FFT Options"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer33->Add(itemStaticText36, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    p_fftPanel = new ewxPanel( p_ewaldPanel, ID_PANEL_INTRCT_FFTOPT, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_FFTVSizer->Add(p_fftPanel, 0, wxALIGN_LEFT|wxLEFT, 30);

    wxBoxSizer* itemBoxSizer38 = new wxBoxSizer(wxVERTICAL);
    p_fftPanel->SetSizer(itemBoxSizer38);

    wxBoxSizer* itemBoxSizer39 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer38->Add(itemBoxSizer39, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText40 = new ewxStaticText( p_fftPanel, wxID_STATIC_INTRCT_NFFT, _("Number of Processors:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer39->Add(itemStaticText40, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl41 = new ewxTextCtrl( p_fftPanel, ID_TEXTCTRL_INTRCT_NFFT, _("2"), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer39->Add(itemTextCtrl41, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer39->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText43 = new ewxStaticText( p_fftPanel, ID_STATICTEXT1, _("FFT Source:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer39->Add(itemStaticText43, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice44Strings[] = {
        _("NWChem"),
        _("Platform Specific")
    };
    ewxChoice* itemChoice44 = new ewxChoice( p_fftPanel, ID_CHOICE_INTRCT_FFT_SRC, wxDefaultPosition, wxDefaultSize, 2, itemChoice44Strings, 0 );
    itemChoice44->SetStringSelection(_("NWChem"));
    itemBoxSizer39->Add(itemChoice44, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_polarizationPanel = new ewxPanel( itemPanel1, ID_PANEL_INTRCT_POLARIZATION, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    p_polarizationPanel->Show(false);
    p_mainVSizer->Add(p_polarizationPanel, 0, wxGROW|wxLEFT, 3);

    wxBoxSizer* itemBoxSizer46 = new wxBoxSizer(wxVERTICAL);
    p_polarizationPanel->SetSizer(itemBoxSizer46);

    wxBoxSizer* itemBoxSizer47 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer46->Add(itemBoxSizer47, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText48 = new ewxStaticText( p_polarizationPanel, wxID_STATIC, _("Maximum Iterations:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer47->Add(itemStaticText48, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl49 = new ewxTextCtrl( p_polarizationPanel, ID_TEXTCTRL_INTRCT_MAXIT, _("14"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer47->Add(itemTextCtrl49, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer47->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText51 = new ewxStaticText( p_polarizationPanel, wxID_STATIC, _("Convergence Tolerance:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer47->Add(itemStaticText51, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl52 = new ewxTextCtrl( p_polarizationPanel, ID_TEXTCTRL_INTRCT_PTOL, _("0.001"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer47->Add(itemTextCtrl52, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer53 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer46->Add(itemBoxSizer53, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox54 = new ewxCheckBox( p_polarizationPanel, ID_CHECKBOX_INTRCT_FSTORD, _("Use First Order Polarization Only"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox54->SetValue(false);
    itemBoxSizer53->Add(itemCheckBox54, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 15);

    p_cutoffPanel = new ewxPanel( itemPanel1, ID_PANEL_INTRCT_CUTOFF, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    p_mainVSizer->Add(p_cutoffPanel, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 3);

    p_cutoffVSizer = new wxBoxSizer(wxVERTICAL);
    p_cutoffPanel->SetSizer(p_cutoffVSizer);

    wxBoxSizer* itemBoxSizer57 = new wxBoxSizer(wxHORIZONTAL);
    p_cutoffVSizer->Add(itemBoxSizer57, 0, wxGROW|wxLEFT|wxRIGHT, 3);

    ewxStaticLine* itemStaticLine58 = new ewxStaticLine( p_cutoffPanel, wxID_STATIC, wxDefaultPosition, wxSize(10, -1), wxLI_HORIZONTAL );
    itemBoxSizer57->Add(itemStaticLine58, 0, wxGROW|wxALL, 3);

    ewxStaticText* itemStaticText59 = new ewxStaticText( p_cutoffPanel, wxID_STATIC, _("Cutoff Options"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer57->Add(itemStaticText59, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine60 = new ewxStaticLine( p_cutoffPanel, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer57->Add(itemStaticLine60, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer61 = new wxBoxSizer(wxHORIZONTAL);
    p_cutoffVSizer->Add(itemBoxSizer61, 0, wxALIGN_LEFT|wxALL, 0);

    ewxRadioButton* itemRadioButton62 = new ewxRadioButton( p_cutoffPanel, ID_RADIOBUTTON_INTRCT_SROPT, _("Single Ranged"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton62->SetValue(false);
    itemBoxSizer61->Add(itemRadioButton62, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton63 = new ewxRadioButton( p_cutoffPanel, ID_RADIOBUTTON_INTRCT_DROPT, _("Dual Ranged"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton63->SetValue(false);
    itemBoxSizer61->Add(itemRadioButton63, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_cutoffHSizer = new wxBoxSizer(wxHORIZONTAL);
    p_cutoffVSizer->Add(p_cutoffHSizer, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText65 = new ewxStaticText( p_cutoffPanel, wxID_STATIC, _("Short-Ranged Cutoff:"), wxDefaultPosition, wxDefaultSize, 0 );
    p_cutoffHSizer->Add(itemStaticText65, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl66 = new ewxTextCtrl( p_cutoffPanel, ID_TEXTCTRL_INTRCT_SCUT, _("0.9"), wxDefaultPosition, wxDefaultSize, 0 );
    p_cutoffHSizer->Add(itemTextCtrl66, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText67 = new ewxStaticText( p_cutoffPanel, wxID_STATIC_INTRCT_SCUT, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    p_cutoffHSizer->Add(itemStaticText67, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer68 = new wxBoxSizer(wxHORIZONTAL);
    p_cutoffVSizer->Add(itemBoxSizer68, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText69 = new ewxStaticText( p_cutoffPanel, wxID_STATIC_INTRCT_SFREQ, _("Frequency to Update Short-Ranged Pair List:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer68->Add(itemStaticText69, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl70 = new ewxTextCtrl( p_cutoffPanel, ID_TEXTCTRL_INTRCT_SFREQ, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer68->Add(itemTextCtrl70, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_lcutoffPanel = new ewxPanel( p_cutoffPanel, ID_PANEL_INTRCT_LCUT, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_cutoffVSizer->Add(p_lcutoffPanel, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer72 = new wxBoxSizer(wxVERTICAL);
    p_lcutoffPanel->SetSizer(itemBoxSizer72);

    wxBoxSizer* itemBoxSizer73 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer72->Add(itemBoxSizer73, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText74 = new ewxStaticText( p_lcutoffPanel, wxID_STATIC_INTRCT_LCUT, _("Long-Ranged Cutoff:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer73->Add(itemStaticText74, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl75 = new ewxTextCtrl( p_lcutoffPanel, ID_TEXTCTRL_INTRCT_LCUT, _("0.9"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer73->Add(itemTextCtrl75, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText76 = new ewxStaticText( p_lcutoffPanel, wxID_STATIC_INTRCT_ULCUT, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer73->Add(itemStaticText76, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer77 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer72->Add(itemBoxSizer77, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    ewxStaticText* itemStaticText78 = new ewxStaticText( p_lcutoffPanel, wxID_STATIC_INTRCT_LFREQ, _("Frequency to Update Long-Ranged Pair List:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer77->Add(itemStaticText78, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl79 = new ewxTextCtrl( p_lcutoffPanel, ID_TEXTCTRL_INTRCT_LFREQ, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer77->Add(itemTextCtrl79, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

////@end InteractionPanelGUI content construction
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SET1
 */

void InteractionPanelGUI::OnRadiobuttonIntrctSet1Selected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SET1 in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SET1 in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SET2
 */

void InteractionPanelGUI::OnRadiobuttonIntrctSet2Selected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SET2 in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SET2 in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SET3
 */

void InteractionPanelGUI::OnRadiobuttonIntrctSet3Selected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SET3 in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SET3 in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_PERT1
 */

void InteractionPanelGUI::OnCheckboxIntrctPert1Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_PERT1 in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_PERT1 in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_PERT2
 */

void InteractionPanelGUI::OnCheckboxIntrctPert2Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_PERT2 in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_PERT2 in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_INTRCT_ES_OPT
 */

void InteractionPanelGUI::OnChoiceIntrctEsOptSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_INTRCT_ES_OPT in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_INTRCT_ES_OPT in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_GRID
 */

void InteractionPanelGUI::OnTextctrlIntrctGridEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_GRID in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_GRID in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_EWTOL
 */

void InteractionPanelGUI::OnTextctrlIntrctEwtolEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_EWTOL in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_EWTOL in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_ESPLINE
 */

void InteractionPanelGUI::OnTextctrlIntrctEsplineEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_ESPLINE in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_ESPLINE in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_QGRID
 */

void InteractionPanelGUI::OnCheckboxIntrctQgridClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_QGRID in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_QGRID in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TOGGLE_INTRCT_ADVFFT
 */

void InteractionPanelGUI::OnToggleIntrctAdvfftClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TOGGLE_INTRCT_ADVFFT in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TOGGLE_INTRCT_ADVFFT in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_NFFT
 */

void InteractionPanelGUI::OnTextctrlIntrctNfftEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_NFFT in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_NFFT in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_INTRCT_FFT_SRC
 */

void InteractionPanelGUI::OnChoiceIntrctFftSrcSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_INTRCT_FFT_SRC in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_INTRCT_FFT_SRC in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_MAXIT
 */

void InteractionPanelGUI::OnTextctrlIntrctMaxitEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_MAXIT in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_MAXIT in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_PTOL
 */

void InteractionPanelGUI::OnTextctrlIntrctPtolEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_PTOL in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_PTOL in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_FSTORD
 */

void InteractionPanelGUI::OnCheckboxIntrctFstordClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_FSTORD in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_FSTORD in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SROPT
 */

void InteractionPanelGUI::OnRadiobuttonIntrctSroptSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SROPT in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SROPT in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_DROPT
 */

void InteractionPanelGUI::OnRadiobuttonIntrctDroptSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_DROPT in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_DROPT in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_SCUT
 */

void InteractionPanelGUI::OnTextctrlIntrctScutEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_SCUT in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_SCUT in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_SFREQ
 */

void InteractionPanelGUI::OnTextctrlIntrctSfreqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_SFREQ in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_SFREQ in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_LCUT
 */

void InteractionPanelGUI::OnTextctrlIntrctLcutEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_LCUT in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_LCUT in InteractionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_LFREQ
 */

void InteractionPanelGUI::OnTextctrlIntrctLfreqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_LFREQ in InteractionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_LFREQ in InteractionPanelGUI. 
}

/*!
 * Should we show tooltips?
 */

bool InteractionPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap InteractionPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin InteractionPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end InteractionPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon InteractionPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin InteractionPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end InteractionPanelGUI icon retrieval
}
