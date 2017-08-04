/////////////////////////////////////////////////////////////////////////////
// Name:        DynamicsPanelGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "DynamicsPanelGUI.H"
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
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxgui/DynamicsPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_FRAC3 = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_CHOICE_DYNMC_VOLALG = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_SLVTIM = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_NPT = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_RCNTR = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_RADIOBUTTON_DYNMC_ZONLY = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_RADIOBUTTON_DYNMC_ALLCRD = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_CHECKBOX_DYNMC_RTHRM = wxNewId();
const wxWindowID DynamicsPanelGUI::wxID_STATIC_DYNMC_PRSTIM = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_ALL = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_DATSTP = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_DIALOG_DYNMC = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_CHECKBOX_DYNMC_RMCM = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_IANL = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_RMCM = wxNewId();
const wxWindowID DynamicsPanelGUI::wxID_STATIC_DYNMC_SLVTIM = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_FANL = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_ITIME = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_TEMP = wxNewId();
const wxWindowID DynamicsPanelGUI::wxID_STATIC_DYNMC_SLUTXT = wxNewId();
const wxWindowID DynamicsPanelGUI::wxID_STATIC_DYNMC_IANL = wxNewId();
const wxWindowID DynamicsPanelGUI::wxID_STATIC_DYNMC_ANLTMP = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_FRAC4 = wxNewId();
const wxWindowID DynamicsPanelGUI::wxID_STATIC_DYNMC_RTHTMP = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_SLUTIM = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_RADIOBUTTON_DYNMC_SPLTRX = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_CNTFRQ = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_RADIOBUTTON_DYNMC_XYONLY = wxNewId();
const wxWindowID DynamicsPanelGUI::wxID_STATIC_DYNMC_ITIME = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_SLUTIM = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_EQSTP = wxNewId();
const wxWindowID DynamicsPanelGUI::wxID_STATIC_DYNMC_NVTTMP = wxNewId();
const wxWindowID DynamicsPanelGUI::wxID_STATIC_DYNMC_CMPRSS = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_CHOICE_DYNMC_INTALG = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_CHECKBOX_DYNMC_USENVT = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_FRACS = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_CHECKBOX_DYNMC_CNTSYS = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_CNTR = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_FRAC5 = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_RTHRM = wxNewId();
const wxWindowID DynamicsPanelGUI::wxID_STATIC_DYNMC_TSTEP = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_RETHRM = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_NVTTMP = wxNewId();
const wxWindowID DynamicsPanelGUI::wxID_STATIC_DYNMC_NPTPRS = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_TSTEP = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_ANLOPT = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_CHECKBOX_DYNMC_USEFRAC = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_NOTEBOOK_DYNMC = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_FRAC2 = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_NPTPRS = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_CMPRSS = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_VELFRQ = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_NVT = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_PRSTIM = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_RTHTMP = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_VFRAC = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_CMFRQ = wxNewId();
const wxWindowID DynamicsPanelGUI::wxID_STATIC_DYNMC_USLUTIM = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_CHECKBOX_DYNMC_USENPT = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_CHECKBOX_DYNMC_USEANL = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_PRSSR = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_RADIOBUTTON_DYNMC_SNGLRX = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_CHOICE_DYNMC_RTHOPT = wxNewId();
const wxWindowID DynamicsPanelGUI::wxID_STATIC_DYNMC_FANL = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_PANEL_DYNMC_ANNEAL = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_ANLTMP = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_CHECKBOX_DYNMC_RESUM = wxNewId();
const wxWindowID DynamicsPanelGUI::ID_TEXTCTRL_DYNMC_FRAC1 = wxNewId();

/*!
 * DynamicsPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( DynamicsPanelGUI, ewxPanel )

/*!
 * DynamicsPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( DynamicsPanelGUI, ewxPanel )

////@begin DynamicsPanelGUI event table entries
    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_DATSTP, DynamicsPanelGUI::OnTextctrlDynmcDatstpEnter )

    EVT_CHECKBOX( ID_CHECKBOX_DYNMC_RESUM, DynamicsPanelGUI::OnCheckboxDynmcResumClick )

    EVT_CHOICE( ID_CHOICE_DYNMC_INTALG, DynamicsPanelGUI::OnChoiceDynmcIntalgSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_EQSTP, DynamicsPanelGUI::OnTextctrlDynmcEqstpEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_TSTEP, DynamicsPanelGUI::OnTextctrlDynmcTstepEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_ITIME, DynamicsPanelGUI::OnTextctrlDynmcItimeEnter )

    EVT_CHECKBOX( ID_CHECKBOX_DYNMC_CNTSYS, DynamicsPanelGUI::OnCheckboxDynmcCntsysClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_CNTFRQ, DynamicsPanelGUI::OnTextctrlDynmcCntfrqEnter )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_DYNMC_ALLCRD, DynamicsPanelGUI::OnRadiobuttonDynmcAllcrdSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_DYNMC_XYONLY, DynamicsPanelGUI::OnRadiobuttonDynmcXyonlySelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_DYNMC_ZONLY, DynamicsPanelGUI::OnRadiobuttonDynmcZonlySelected )

    EVT_CHECKBOX( ID_CHECKBOX_DYNMC_USEFRAC, DynamicsPanelGUI::OnCheckboxDynmcUsefracClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_FRAC1, DynamicsPanelGUI::OnTextctrlDynmcFrac1Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_FRAC2, DynamicsPanelGUI::OnTextctrlDynmcFrac2Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_FRAC3, DynamicsPanelGUI::OnTextctrlDynmcFrac3Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_FRAC4, DynamicsPanelGUI::OnTextctrlDynmcFrac4Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_FRAC5, DynamicsPanelGUI::OnTextctrlDynmcFrac5Enter )

    EVT_CHECKBOX( ID_CHECKBOX_DYNMC_RMCM, DynamicsPanelGUI::OnCheckboxDynmcRmcmClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_CMFRQ, DynamicsPanelGUI::OnTextctrlDynmcCmfrqEnter )

    EVT_CHECKBOX( ID_CHECKBOX_DYNMC_USENVT, DynamicsPanelGUI::OnCheckboxDynmcUsenvtClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_NVTTMP, DynamicsPanelGUI::OnTextctrlDynmcNvttmpEnter )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_DYNMC_SNGLRX, DynamicsPanelGUI::OnRadiobuttonDynmcSnglrxSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_DYNMC_SPLTRX, DynamicsPanelGUI::OnRadiobuttonDynmcSpltrxSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_SLVTIM, DynamicsPanelGUI::OnTextctrlDynmcSlvtimEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_SLUTIM, DynamicsPanelGUI::OnTextctrlDynmcSlutimEnter )

    EVT_CHECKBOX( ID_CHECKBOX_DYNMC_USENPT, DynamicsPanelGUI::OnCheckboxDynmcUsenptClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_NPTPRS, DynamicsPanelGUI::OnTextctrlDynmcNptprsEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_PRSTIM, DynamicsPanelGUI::OnTextctrlDynmcPrstimEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_CMPRSS, DynamicsPanelGUI::OnTextctrlDynmcCmprssEnter )

    EVT_CHOICE( ID_CHOICE_DYNMC_VOLALG, DynamicsPanelGUI::OnChoiceDynmcVolalgSelected )

    EVT_CHECKBOX( ID_CHECKBOX_DYNMC_RTHRM, DynamicsPanelGUI::OnCheckboxDynmcRthrmClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_VELFRQ, DynamicsPanelGUI::OnTextctrlDynmcVelfrqEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_RTHTMP, DynamicsPanelGUI::OnTextctrlDynmcRthtmpEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_VFRAC, DynamicsPanelGUI::OnTextctrlDynmcVfracEnter )

    EVT_CHOICE( ID_CHOICE_DYNMC_RTHOPT, DynamicsPanelGUI::OnChoiceDynmcRthoptSelected )

    EVT_CHECKBOX( ID_CHECKBOX_DYNMC_USEANL, DynamicsPanelGUI::OnCheckboxDynmcUseanlClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_ANLTMP, DynamicsPanelGUI::OnTextctrlDynmcAnltmpEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_IANL, DynamicsPanelGUI::OnTextctrlDynmcIanlEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_DYNMC_FANL, DynamicsPanelGUI::OnTextctrlDynmcFanlEnter )

////@end DynamicsPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * DynamicsPanelGUI constructors
 */

DynamicsPanelGUI::DynamicsPanelGUI( )
{
}

DynamicsPanelGUI::DynamicsPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * DynamicsPanelGUI creator
 */

bool DynamicsPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin DynamicsPanelGUI member initialisation
    p_dynamicsVSizer = NULL;
    p_recenterPanel = NULL;
    p_NVTPanel = NULL;
    p_NPTPanel = NULL;
    p_rethermalizationPanel = NULL;
    p_annealingPanel = NULL;
////@end DynamicsPanelGUI member initialisation

////@begin DynamicsPanelGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_VALIDATE_RECURSIVELY|wxWS_EX_BLOCK_EVENTS);
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end DynamicsPanelGUI creation
    return true;
}

/*!
 * Control creation for DynamicsPanelGUI
 */

void DynamicsPanelGUI::CreateControls()
{    
////@begin DynamicsPanelGUI content construction
    DynamicsPanelGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText4 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Data Collection Steps:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText5 = new ewxStaticText( itemPanel1, wxID_STATIC, _("* "), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText5->SetForegroundColour(wxColour(255, 0, 0));
    itemStaticText5->SetFont(wxFont(14, wxSWISS, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemBoxSizer3->Add(itemStaticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl6 = new ewxTextCtrl( itemPanel1, ID_TEXTCTRL_DYNMC_DATSTP, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemTextCtrl6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer7, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox8 = new ewxCheckBox( itemPanel1, ID_CHECKBOX_DYNMC_RESUM, _("Resume Calculation from Previous Dynamics Task"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox8->SetValue(false);
    itemBoxSizer7->Add(itemCheckBox8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel9 = new ewxPanel( itemPanel1, ID_PANEL_DYNMC_ALL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer2->Add(itemPanel9, 0, wxALIGN_LEFT|wxALL, 0);

    p_dynamicsVSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel9->SetSizer(p_dynamicsVSizer);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxHORIZONTAL);
    p_dynamicsVSizer->Add(itemBoxSizer11, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText12 = new ewxStaticText( itemPanel9, wxID_STATIC, _("Integration Algorithm:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemStaticText12, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice13Strings[] = {
        _("Standard Leapfrog"),
        _(" Brown-Clarke Leapfrog")
    };
    ewxChoice* itemChoice13 = new ewxChoice( itemPanel9, ID_CHOICE_DYNMC_INTALG, wxDefaultPosition, wxDefaultSize, 2, itemChoice13Strings, 0 );
    itemChoice13->SetStringSelection(_("Standard Leapfrog"));
    itemBoxSizer11->Add(itemChoice13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    p_dynamicsVSizer->Add(itemBoxSizer14, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText15 = new ewxStaticText( itemPanel9, wxID_STATIC, _("Equilibration Steps:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemStaticText15, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl16 = new ewxTextCtrl( itemPanel9, ID_TEXTCTRL_DYNMC_EQSTP, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemTextCtrl16, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxHORIZONTAL);
    p_dynamicsVSizer->Add(itemBoxSizer17, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText18 = new ewxStaticText( itemPanel9, wxID_STATIC, _("Time Step:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemStaticText18, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl19 = new ewxTextCtrl( itemPanel9, ID_TEXTCTRL_DYNMC_TSTEP, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemTextCtrl19, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText20 = new ewxStaticText( itemPanel9, wxID_STATIC_DYNMC_TSTEP, _("ps"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemStaticText20, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer17->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText22 = new ewxStaticText( itemPanel9, wxID_STATIC, _("Initial Simulation Time:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemStaticText22, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl23 = new ewxTextCtrl( itemPanel9, ID_TEXTCTRL_DYNMC_ITIME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemTextCtrl23, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText24 = new ewxStaticText( itemPanel9, wxID_STATIC_DYNMC_ITIME, _("ps"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemStaticText24, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxHORIZONTAL);
    p_dynamicsVSizer->Add(itemBoxSizer25, 0, wxALIGN_LEFT|wxALL, 0);

    itemBoxSizer25->Add(5, 15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer27 = new wxBoxSizer(wxHORIZONTAL);
    p_dynamicsVSizer->Add(itemBoxSizer27, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    itemBoxSizer27->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxNotebook* itemNotebook29 = new ewxNotebook( itemPanel9, ID_NOTEBOOK_DYNMC, wxDefaultPosition, wxDefaultSize, wxNB_TOP|wxSUNKEN_BORDER );

    ewxPanel* itemPanel30 = new ewxPanel( itemNotebook29, ID_PANEL_DYNMC_CNTR, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer31 = new wxBoxSizer(wxVERTICAL);
    itemPanel30->SetSizer(itemBoxSizer31);

    wxBoxSizer* itemBoxSizer32 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer31->Add(itemBoxSizer32, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox33 = new ewxCheckBox( itemPanel30, ID_CHECKBOX_DYNMC_CNTSYS, _("Recenter System"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox33->SetValue(false);
    itemBoxSizer32->Add(itemCheckBox33, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_recenterPanel = new ewxPanel( itemPanel30, ID_PANEL_DYNMC_RCNTR, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer31->Add(p_recenterPanel, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer35 = new wxBoxSizer(wxVERTICAL);
    p_recenterPanel->SetSizer(itemBoxSizer35);

    wxBoxSizer* itemBoxSizer36 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer35->Add(itemBoxSizer36, 0, wxALIGN_LEFT|wxALL, 0);
    itemBoxSizer36->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText38 = new ewxStaticText( p_recenterPanel, wxID_STATIC, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer36->Add(itemStaticText38, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl39 = new ewxTextCtrl( p_recenterPanel, ID_TEXTCTRL_DYNMC_CNTFRQ, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer36->Add(itemTextCtrl39, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton40 = new ewxRadioButton( p_recenterPanel, ID_RADIOBUTTON_DYNMC_ALLCRD, _("All Coordinates"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton40->SetValue(false);
    itemBoxSizer36->Add(itemRadioButton40, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton41 = new ewxRadioButton( p_recenterPanel, ID_RADIOBUTTON_DYNMC_XYONLY, _("X and Y Only"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton41->SetValue(false);
    itemBoxSizer36->Add(itemRadioButton41, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton42 = new ewxRadioButton( p_recenterPanel, ID_RADIOBUTTON_DYNMC_ZONLY, _("Z Only"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton42->SetValue(false);
    itemBoxSizer36->Add(itemRadioButton42, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer43 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer35->Add(itemBoxSizer43, 0, wxALIGN_LEFT|wxALL, 0);
    itemBoxSizer43->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox45 = new ewxCheckBox( p_recenterPanel, ID_CHECKBOX_DYNMC_USEFRAC, _("Use System Fractions"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox45->SetValue(false);
    itemBoxSizer43->Add(itemCheckBox45, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer43->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel47 = new ewxPanel( p_recenterPanel, ID_PANEL_DYNMC_FRACS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer43->Add(itemPanel47, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
    wxBoxSizer* itemBoxSizer48 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel47->SetSizer(itemBoxSizer48);

    ewxStaticText* itemStaticText49 = new ewxStaticText( itemPanel47, wxID_STATIC, _("1:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemStaticText49, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl50 = new ewxTextCtrl( itemPanel47, ID_TEXTCTRL_DYNMC_FRAC1, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer48->Add(itemTextCtrl50, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText51 = new ewxStaticText( itemPanel47, wxID_STATIC, _("2:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemStaticText51, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl52 = new ewxTextCtrl( itemPanel47, ID_TEXTCTRL_DYNMC_FRAC2, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer48->Add(itemTextCtrl52, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText53 = new ewxStaticText( itemPanel47, wxID_STATIC, _("3:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemStaticText53, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl54 = new ewxTextCtrl( itemPanel47, ID_TEXTCTRL_DYNMC_FRAC3, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer48->Add(itemTextCtrl54, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText55 = new ewxStaticText( itemPanel47, wxID_STATIC, _("4:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemStaticText55, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl56 = new ewxTextCtrl( itemPanel47, ID_TEXTCTRL_DYNMC_FRAC4, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer48->Add(itemTextCtrl56, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText57 = new ewxStaticText( itemPanel47, wxID_STATIC, _("5:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemStaticText57, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl58 = new ewxTextCtrl( itemPanel47, ID_TEXTCTRL_DYNMC_FRAC5, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer48->Add(itemTextCtrl58, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer59 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer31->Add(itemBoxSizer59, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox60 = new ewxCheckBox( itemPanel30, ID_CHECKBOX_DYNMC_RMCM, _("Remove Center of Mass Motion"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox60->SetValue(false);
    itemBoxSizer59->Add(itemCheckBox60, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer59->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel62 = new ewxPanel( itemPanel30, ID_PANEL_DYNMC_RMCM, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer59->Add(itemPanel62, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
    wxBoxSizer* itemBoxSizer63 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel62->SetSizer(itemBoxSizer63);

    ewxStaticText* itemStaticText64 = new ewxStaticText( itemPanel62, wxID_STATIC, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer63->Add(itemStaticText64, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl65 = new ewxTextCtrl( itemPanel62, ID_TEXTCTRL_DYNMC_CMFRQ, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer63->Add(itemTextCtrl65, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook29->AddPage(itemPanel30, _("Centering"));

    ewxPanel* itemPanel66 = new ewxPanel( itemNotebook29, ID_PANEL_DYNMC_TEMP, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer67 = new wxBoxSizer(wxVERTICAL);
    itemPanel66->SetSizer(itemBoxSizer67);

    wxBoxSizer* itemBoxSizer68 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer67->Add(itemBoxSizer68, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox69 = new ewxCheckBox( itemPanel66, ID_CHECKBOX_DYNMC_USENVT, _("Use Constant Temperature Algorithm"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox69->SetValue(false);
    itemBoxSizer68->Add(itemCheckBox69, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_NVTPanel = new ewxPanel( itemPanel66, ID_PANEL_DYNMC_NVT, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer67->Add(p_NVTPanel, 0, wxGROW|wxALL, 0);
    wxBoxSizer* itemBoxSizer71 = new wxBoxSizer(wxVERTICAL);
    p_NVTPanel->SetSizer(itemBoxSizer71);

    wxBoxSizer* itemBoxSizer72 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer71->Add(itemBoxSizer72, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText73 = new ewxStaticText( p_NVTPanel, wxID_STATIC, _("Temperature:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer72->Add(itemStaticText73, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl74 = new ewxTextCtrl( p_NVTPanel, ID_TEXTCTRL_DYNMC_NVTTMP, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer72->Add(itemTextCtrl74, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText75 = new ewxStaticText( p_NVTPanel, wxID_STATIC_DYNMC_NVTTMP, _("Kelvin"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer72->Add(itemStaticText75, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer72->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton77 = new ewxRadioButton( p_NVTPanel, ID_RADIOBUTTON_DYNMC_SNGLRX, _("Single Relaxation"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton77->SetValue(false);
    itemBoxSizer72->Add(itemRadioButton77, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton78 = new ewxRadioButton( p_NVTPanel, ID_RADIOBUTTON_DYNMC_SPLTRX, _("Split Relaxation"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton78->SetValue(false);
    itemBoxSizer72->Add(itemRadioButton78, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer79 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer71->Add(itemBoxSizer79, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText80 = new ewxStaticText( p_NVTPanel, wxID_STATIC, _("Solvent/System Relaxation Time:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer79->Add(itemStaticText80, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl81 = new ewxTextCtrl( p_NVTPanel, ID_TEXTCTRL_DYNMC_SLVTIM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer79->Add(itemTextCtrl81, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText82 = new ewxStaticText( p_NVTPanel, wxID_STATIC_DYNMC_SLVTIM, _("ps"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer79->Add(itemStaticText82, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxPanel* itemPanel83 = new ewxPanel( p_NVTPanel, ID_PANEL_DYNMC_SLUTIM, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer71->Add(itemPanel83, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer84 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel83->SetSizer(itemBoxSizer84);

    ewxStaticText* itemStaticText85 = new ewxStaticText( itemPanel83, wxID_STATIC_DYNMC_SLUTXT, _("Solute Relaxation Time:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer84->Add(itemStaticText85, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl86 = new ewxTextCtrl( itemPanel83, ID_TEXTCTRL_DYNMC_SLUTIM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer84->Add(itemTextCtrl86, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText87 = new ewxStaticText( itemPanel83, wxID_STATIC_DYNMC_USLUTIM, _("ps"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer84->Add(itemStaticText87, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    itemNotebook29->AddPage(itemPanel66, _("Temperature"));

    ewxPanel* itemPanel88 = new ewxPanel( itemNotebook29, ID_PANEL_DYNMC_PRSSR, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer89 = new wxBoxSizer(wxVERTICAL);
    itemPanel88->SetSizer(itemBoxSizer89);

    wxBoxSizer* itemBoxSizer90 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer89->Add(itemBoxSizer90, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox91 = new ewxCheckBox( itemPanel88, ID_CHECKBOX_DYNMC_USENPT, _("Use Constant Pressure Algorithm"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox91->SetValue(false);
    itemBoxSizer90->Add(itemCheckBox91, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer90->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_NPTPanel = new ewxPanel( itemPanel88, ID_PANEL_DYNMC_NPT, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer89->Add(p_NPTPanel, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer94 = new wxBoxSizer(wxVERTICAL);
    p_NPTPanel->SetSizer(itemBoxSizer94);

    wxBoxSizer* itemBoxSizer95 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer94->Add(itemBoxSizer95, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText96 = new ewxStaticText( p_NPTPanel, wxID_STATIC, _("Pressure:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer95->Add(itemStaticText96, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl97 = new ewxTextCtrl( p_NPTPanel, ID_TEXTCTRL_DYNMC_NPTPRS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer95->Add(itemTextCtrl97, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText98 = new ewxStaticText( p_NPTPanel, wxID_STATIC_DYNMC_NPTPRS, _("N/m**2"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer95->Add(itemStaticText98, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer99 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer94->Add(itemBoxSizer99, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText100 = new ewxStaticText( p_NPTPanel, wxID_STATIC, _("Pressure Relaxation Time:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer99->Add(itemStaticText100, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl101 = new ewxTextCtrl( p_NPTPanel, ID_TEXTCTRL_DYNMC_PRSTIM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer99->Add(itemTextCtrl101, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText102 = new ewxStaticText( p_NPTPanel, wxID_STATIC_DYNMC_PRSTIM, _("ps"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer99->Add(itemStaticText102, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer103 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer94->Add(itemBoxSizer103, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText104 = new ewxStaticText( p_NPTPanel, wxID_STATIC, _("Compressibility:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer103->Add(itemStaticText104, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl105 = new ewxTextCtrl( p_NPTPanel, ID_TEXTCTRL_DYNMC_CMPRSS, _T(""), wxDefaultPosition, wxSize(100, -1), 0 );
    itemBoxSizer103->Add(itemTextCtrl105, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText106 = new ewxStaticText( p_NPTPanel, wxID_STATIC_DYNMC_CMPRSS, _("m**2/N"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer103->Add(itemStaticText106, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer107 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer94->Add(itemBoxSizer107, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText108 = new ewxStaticText( p_NPTPanel, wxID_STATIC, _("Volume Adjustment Algorithm:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer107->Add(itemStaticText108, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    wxString itemChoice109Strings[] = {
        _("Isotropic"),
        _("Isotropic in XY"),
        _("Z Only"),
        _("XY Independent of Z")
    };
    ewxChoice* itemChoice109 = new ewxChoice( p_NPTPanel, ID_CHOICE_DYNMC_VOLALG, wxDefaultPosition, wxDefaultSize, 4, itemChoice109Strings, 0 );
    itemChoice109->SetStringSelection(_("Isotropic"));
    itemBoxSizer107->Add(itemChoice109, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook29->AddPage(itemPanel88, _("Pressure"));

    ewxPanel* itemPanel110 = new ewxPanel( itemNotebook29, ID_PANEL_DYNMC_RETHRM, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer111 = new wxBoxSizer(wxVERTICAL);
    itemPanel110->SetSizer(itemBoxSizer111);

    wxBoxSizer* itemBoxSizer112 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer111->Add(itemBoxSizer112, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox113 = new ewxCheckBox( itemPanel110, ID_CHECKBOX_DYNMC_RTHRM, _("Use Velocity Rethermalization Algorithm"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox113->SetValue(false);
    itemBoxSizer112->Add(itemCheckBox113, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_rethermalizationPanel = new ewxPanel( itemPanel110, ID_PANEL_DYNMC_RTHRM, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer111->Add(p_rethermalizationPanel, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer115 = new wxBoxSizer(wxVERTICAL);
    p_rethermalizationPanel->SetSizer(itemBoxSizer115);

    wxBoxSizer* itemBoxSizer116 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer115->Add(itemBoxSizer116, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText117 = new ewxStaticText( p_rethermalizationPanel, wxID_STATIC, _("Rethermalization Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer116->Add(itemStaticText117, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl118 = new ewxTextCtrl( p_rethermalizationPanel, ID_TEXTCTRL_DYNMC_VELFRQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer116->Add(itemTextCtrl118, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer119 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer115->Add(itemBoxSizer119, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText120 = new ewxStaticText( p_rethermalizationPanel, wxID_STATIC, _("Rethermalization Temperature:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer119->Add(itemStaticText120, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl121 = new ewxTextCtrl( p_rethermalizationPanel, ID_TEXTCTRL_DYNMC_RTHTMP, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer119->Add(itemTextCtrl121, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText122 = new ewxStaticText( p_rethermalizationPanel, wxID_STATIC_DYNMC_RTHTMP, _("Kelvin"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer119->Add(itemStaticText122, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer123 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer115->Add(itemBoxSizer123, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText124 = new ewxStaticText( p_rethermalizationPanel, wxID_STATIC, _("Fraction of Velocities to Rethermalize:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer123->Add(itemStaticText124, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl125 = new ewxTextCtrl( p_rethermalizationPanel, ID_TEXTCTRL_DYNMC_VFRAC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer123->Add(itemTextCtrl125, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer126 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer115->Add(itemBoxSizer126, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText127 = new ewxStaticText( p_rethermalizationPanel, wxID_STATIC, _("Additional Rethermalization Options:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer126->Add(itemStaticText127, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    wxString itemChoice128Strings[] = {
        _("Default"),
        _("First Step Only"),
        _("First Window Only"),
        _("Last Window Only"),
        _("First and Last Window Only")
    };
    ewxChoice* itemChoice128 = new ewxChoice( p_rethermalizationPanel, ID_CHOICE_DYNMC_RTHOPT, wxDefaultPosition, wxDefaultSize, 5, itemChoice128Strings, 0 );
    itemChoice128->SetStringSelection(_("Default"));
    itemBoxSizer126->Add(itemChoice128, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook29->AddPage(itemPanel110, _("Rethermalization"));

    ewxPanel* itemPanel129 = new ewxPanel( itemNotebook29, ID_PANEL_DYNMC_ANLOPT, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer130 = new wxBoxSizer(wxVERTICAL);
    itemPanel129->SetSizer(itemBoxSizer130);

    wxBoxSizer* itemBoxSizer131 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer130->Add(itemBoxSizer131, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox132 = new ewxCheckBox( itemPanel129, ID_CHECKBOX_DYNMC_USEANL, _("Use Temperature Annealing Algorithm"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox132->SetValue(false);
    itemBoxSizer131->Add(itemCheckBox132, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_annealingPanel = new ewxPanel( itemPanel129, ID_PANEL_DYNMC_ANNEAL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer130->Add(p_annealingPanel, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer134 = new wxBoxSizer(wxVERTICAL);
    p_annealingPanel->SetSizer(itemBoxSizer134);

    wxBoxSizer* itemBoxSizer135 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer134->Add(itemBoxSizer135, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText136 = new ewxStaticText( p_annealingPanel, wxID_STATIC, _("Final Temperature:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer135->Add(itemStaticText136, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl137 = new ewxTextCtrl( p_annealingPanel, ID_TEXTCTRL_DYNMC_ANLTMP, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer135->Add(itemTextCtrl137, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText138 = new ewxStaticText( p_annealingPanel, wxID_STATIC_DYNMC_ANLTMP, _("Kelvin"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer135->Add(itemStaticText138, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer139 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer134->Add(itemBoxSizer139, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText140 = new ewxStaticText( p_annealingPanel, wxID_STATIC, _("Starting Time:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer139->Add(itemStaticText140, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 15);

    ewxTextCtrl* itemTextCtrl141 = new ewxTextCtrl( p_annealingPanel, ID_TEXTCTRL_DYNMC_IANL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer139->Add(itemTextCtrl141, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText142 = new ewxStaticText( p_annealingPanel, wxID_STATIC_DYNMC_IANL, _("ps"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer139->Add(itemStaticText142, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer139->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText144 = new ewxStaticText( p_annealingPanel, wxID_STATIC, _("Final Time:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer139->Add(itemStaticText144, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl145 = new ewxTextCtrl( p_annealingPanel, ID_TEXTCTRL_DYNMC_FANL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer139->Add(itemTextCtrl145, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText146 = new ewxStaticText( p_annealingPanel, wxID_STATIC_DYNMC_FANL, _("ps"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer139->Add(itemStaticText146, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    itemNotebook29->AddPage(itemPanel129, _("Annealing"));

    itemBoxSizer27->Add(itemNotebook29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemBoxSizer27->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxBoxSizer* itemBoxSizer148 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer148, 0, wxALIGN_LEFT|wxALL, 0);

    itemBoxSizer148->Add(5, 15, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 0);

    ewxStaticText* itemStaticText150 = new ewxStaticText( itemPanel1, wxID_STATIC, _("*"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText150->SetForegroundColour(wxColour(255, 0, 0));
    itemStaticText150->SetFont(wxFont(14, wxSWISS, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemBoxSizer148->Add(itemStaticText150, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText151 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Required value"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer148->Add(itemStaticText151, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 3);

////@end DynamicsPanelGUI content construction
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_DATSTP
 */

void DynamicsPanelGUI::OnTextctrlDynmcDatstpEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_DATSTP in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_DATSTP in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RESUM
 */

void DynamicsPanelGUI::OnCheckboxDynmcResumClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RESUM in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RESUM in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_INTALG
 */

void DynamicsPanelGUI::OnChoiceDynmcIntalgSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_INTALG in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_INTALG in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_EQSTP
 */

void DynamicsPanelGUI::OnTextctrlDynmcEqstpEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_EQSTP in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_EQSTP in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_TSTEP
 */

void DynamicsPanelGUI::OnTextctrlDynmcTstepEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_TSTEP in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_TSTEP in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_ITIME
 */

void DynamicsPanelGUI::OnTextctrlDynmcItimeEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_ITIME in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_ITIME in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_CNTSYS
 */

void DynamicsPanelGUI::OnCheckboxDynmcCntsysClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_CNTSYS in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_CNTSYS in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CNTFRQ
 */

void DynamicsPanelGUI::OnTextctrlDynmcCntfrqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CNTFRQ in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CNTFRQ in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_ALLCRD
 */

void DynamicsPanelGUI::OnRadiobuttonDynmcAllcrdSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_ALLCRD in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_ALLCRD in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_XYONLY
 */

void DynamicsPanelGUI::OnRadiobuttonDynmcXyonlySelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_XYONLY in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_XYONLY in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_ZONLY
 */

void DynamicsPanelGUI::OnRadiobuttonDynmcZonlySelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_ZONLY in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_ZONLY in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USEFRAC
 */

void DynamicsPanelGUI::OnCheckboxDynmcUsefracClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USEFRAC in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USEFRAC in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC1
 */

void DynamicsPanelGUI::OnTextctrlDynmcFrac1Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC1 in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC1 in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC2
 */

void DynamicsPanelGUI::OnTextctrlDynmcFrac2Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC2 in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC2 in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC3
 */

void DynamicsPanelGUI::OnTextctrlDynmcFrac3Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC3 in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC3 in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC4
 */

void DynamicsPanelGUI::OnTextctrlDynmcFrac4Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC4 in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC4 in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC5
 */

void DynamicsPanelGUI::OnTextctrlDynmcFrac5Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC5 in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC5 in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RMCM
 */

void DynamicsPanelGUI::OnCheckboxDynmcRmcmClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RMCM in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RMCM in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CMFRQ
 */

void DynamicsPanelGUI::OnTextctrlDynmcCmfrqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CMFRQ in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CMFRQ in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USENVT
 */

void DynamicsPanelGUI::OnCheckboxDynmcUsenvtClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USENVT in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USENVT in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_NVTTMP
 */

void DynamicsPanelGUI::OnTextctrlDynmcNvttmpEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_NVTTMP in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_NVTTMP in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_SNGLRX
 */

void DynamicsPanelGUI::OnRadiobuttonDynmcSnglrxSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_SNGLRX in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_SNGLRX in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_SPLTRX
 */

void DynamicsPanelGUI::OnRadiobuttonDynmcSpltrxSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_SPLTRX in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_SPLTRX in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_SLVTIM
 */

void DynamicsPanelGUI::OnTextctrlDynmcSlvtimEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_SLVTIM in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_SLVTIM in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_SLUTIM
 */

void DynamicsPanelGUI::OnTextctrlDynmcSlutimEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_SLUTIM in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_SLUTIM in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USENPT
 */

void DynamicsPanelGUI::OnCheckboxDynmcUsenptClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USENPT in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USENPT in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_NPTPRS
 */

void DynamicsPanelGUI::OnTextctrlDynmcNptprsEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_NPTPRS in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_NPTPRS in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_PRSTIM
 */

void DynamicsPanelGUI::OnTextctrlDynmcPrstimEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_PRSTIM in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_PRSTIM in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CMPRSS
 */

void DynamicsPanelGUI::OnTextctrlDynmcCmprssEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CMPRSS in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CMPRSS in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_VOLALG
 */

void DynamicsPanelGUI::OnChoiceDynmcVolalgSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_VOLALG in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_VOLALG in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RTHRM
 */

void DynamicsPanelGUI::OnCheckboxDynmcRthrmClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RTHRM in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RTHRM in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_VELFRQ
 */

void DynamicsPanelGUI::OnTextctrlDynmcVelfrqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_VELFRQ in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_VELFRQ in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_RTHTMP
 */

void DynamicsPanelGUI::OnTextctrlDynmcRthtmpEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_RTHTMP in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_RTHTMP in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_VFRAC
 */

void DynamicsPanelGUI::OnTextctrlDynmcVfracEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_VFRAC in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_VFRAC in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_RTHOPT
 */

void DynamicsPanelGUI::OnChoiceDynmcRthoptSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_RTHOPT in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_RTHOPT in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USEANL
 */

void DynamicsPanelGUI::OnCheckboxDynmcUseanlClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USEANL in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USEANL in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_ANLTMP
 */

void DynamicsPanelGUI::OnTextctrlDynmcAnltmpEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_ANLTMP in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_ANLTMP in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_IANL
 */

void DynamicsPanelGUI::OnTextctrlDynmcIanlEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_IANL in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_IANL in DynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FANL
 */

void DynamicsPanelGUI::OnTextctrlDynmcFanlEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FANL in DynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FANL in DynamicsPanelGUI. 
}

/*!
 * Should we show tooltips?
 */

bool DynamicsPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap DynamicsPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin DynamicsPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end DynamicsPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon DynamicsPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin DynamicsPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end DynamicsPanelGUI icon retrieval
}
