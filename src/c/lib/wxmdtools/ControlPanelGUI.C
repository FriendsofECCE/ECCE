/////////////////////////////////////////////////////////////////////////////
// Name:        ControlPanelGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ControlPanelGUI.H"
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
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxOpenCloseToggle.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxgui/ControlPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID ControlPanelGUI::ID_CHECKBOX_CNTRL_CCNFG = wxNewId();
const wxWindowID ControlPanelGUI::ID_TEXTCTRL_CNTRL_MAXSLU = wxNewId();
const wxWindowID ControlPanelGUI::ID_CHECKBOX_CNTRL_PACK = wxNewId();
const wxWindowID ControlPanelGUI::ID_CHECKBOX_CNTRL_MAXSLU = wxNewId();
const wxWindowID ControlPanelGUI::ID_CHOICE_CNTRL_TSYNCH = wxNewId();
const wxWindowID ControlPanelGUI::ID_TEXTCTRL_CNTRL_CY = wxNewId();
const wxWindowID ControlPanelGUI::wxID_STATIC_CNTRL_CFREQ = wxNewId();
const wxWindowID ControlPanelGUI::ID_TEXTCTRL_CNTRL_CX = wxNewId();
const wxWindowID ControlPanelGUI::ID_TEXTCTRL_CNTRL_STRNGTH = wxNewId();
const wxWindowID ControlPanelGUI::ID_CHECKBOX_CNTRL_PCNFG = wxNewId();
const wxWindowID ControlPanelGUI::ID_PANEL_CNTRL_RDPAR = wxNewId();
const wxWindowID ControlPanelGUI::ID_PANEL_CNTRL_NCELL = wxNewId();
const wxWindowID ControlPanelGUI::ID_PANEL_CNTRL_STRNGTH = wxNewId();
const wxWindowID ControlPanelGUI::ID_CHECKBOX_CNTRL_USELOAD = wxNewId();
const wxWindowID ControlPanelGUI::ID_DIALOG_CNTRL = wxNewId();
const wxWindowID ControlPanelGUI::ID_PANEL_CNTRL_ADJUST = wxNewId();
const wxWindowID ControlPanelGUI::wxID_STATIC_CNTRL_TSYNCH = wxNewId();
const wxWindowID ControlPanelGUI::ID_PANEL_CNTRL_NPROC = wxNewId();
const wxWindowID ControlPanelGUI::ID_CHECKBOX_CNTRL_ZONLY = wxNewId();
const wxWindowID ControlPanelGUI::ID_CHECKBOX_CNTRL_RDSTRB = wxNewId();
const wxWindowID ControlPanelGUI::ID_TEXTCTRL_CNTRL_PX = wxNewId();
const wxWindowID ControlPanelGUI::ID_TEXTCTRL_CNTRL_MAXSLV = wxNewId();
const wxWindowID ControlPanelGUI::ID_TEXTCTRL_CNTRL_PZ = wxNewId();
const wxWindowID ControlPanelGUI::ID_CHECKBOX_CNTRL_CELLSZ = wxNewId();
const wxWindowID ControlPanelGUI::ID_CHECKBOX_CNTRL_MAXSLV = wxNewId();
const wxWindowID ControlPanelGUI::ID_TEXTCTRL_CNTRL_PY = wxNewId();
const wxWindowID ControlPanelGUI::ID_TEXTCTRL_CNTRL_CFREQ = wxNewId();
const wxWindowID ControlPanelGUI::ID_PANEL_CNTRL_CNFG = wxNewId();
const wxWindowID ControlPanelGUI::ID_TEXTCTRL_CNTRL_CZ = wxNewId();
const wxWindowID ControlPanelGUI::ID_TOGGLE_CNTRL_CNFG = wxNewId();

/*!
 * ControlPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( ControlPanelGUI, ewxPanel )

/*!
 * ControlPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( ControlPanelGUI, ewxPanel )

////@begin ControlPanelGUI event table entries
    EVT_CHECKBOX( ID_CHECKBOX_CNTRL_USELOAD, ControlPanelGUI::OnCheckboxCntrlUseloadClick )

    EVT_CHECKBOX( ID_CHECKBOX_CNTRL_CELLSZ, ControlPanelGUI::OnCheckboxCntrlCellszClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CNTRL_STRNGTH, ControlPanelGUI::OnTextctrlCntrlStrngthEnter )

    EVT_CHECKBOX( ID_CHECKBOX_CNTRL_ZONLY, ControlPanelGUI::OnCheckboxCntrlZonlyClick )

    EVT_CHECKBOX( ID_CHECKBOX_CNTRL_RDSTRB, ControlPanelGUI::OnCheckboxCntrlRdstrbClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CNTRL_CFREQ, ControlPanelGUI::OnTextctrlCntrlCfreqEnter )

    EVT_CHOICE( ID_CHOICE_CNTRL_TSYNCH, ControlPanelGUI::OnChoiceCntrlTsynchSelected )

    EVT_BUTTON( ID_TOGGLE_CNTRL_CNFG, ControlPanelGUI::OnToggleCntrlCnfgClick )

    EVT_CHECKBOX( ID_CHECKBOX_CNTRL_PCNFG, ControlPanelGUI::OnCheckboxCntrlPcnfgClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CNTRL_PX, ControlPanelGUI::OnTextctrlCntrlPxEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CNTRL_PY, ControlPanelGUI::OnTextctrlCntrlPyEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CNTRL_PZ, ControlPanelGUI::OnTextctrlCntrlPzEnter )

    EVT_CHECKBOX( ID_CHECKBOX_CNTRL_CCNFG, ControlPanelGUI::OnCheckboxCntrlCcnfgClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CNTRL_CX, ControlPanelGUI::OnTextctrlCntrlCxEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CNTRL_CY, ControlPanelGUI::OnTextctrlCntrlCyEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CNTRL_CZ, ControlPanelGUI::OnTextctrlCntrlCzEnter )

    EVT_CHECKBOX( ID_CHECKBOX_CNTRL_MAXSLV, ControlPanelGUI::OnCheckboxCntrlMaxslvClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CNTRL_MAXSLV, ControlPanelGUI::OnTextctrlCntrlMaxslvEnter )

    EVT_CHECKBOX( ID_CHECKBOX_CNTRL_MAXSLU, ControlPanelGUI::OnCheckboxCntrlMaxsluClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CNTRL_MAXSLU, ControlPanelGUI::OnTextctrlCntrlMaxsluEnter )

    EVT_CHECKBOX( ID_CHECKBOX_CNTRL_PACK, ControlPanelGUI::OnCheckboxCntrlPackClick )

////@end ControlPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * ControlPanelGUI constructors
 */

ControlPanelGUI::ControlPanelGUI( )
{
}

ControlPanelGUI::ControlPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * ControlPanelGUI creator
 */

bool ControlPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ControlPanelGUI member initialisation
    p_controlVSizer = NULL;
    p_balancePanel = NULL;
    p_configVSizer = NULL;
    p_configPanel = NULL;
////@end ControlPanelGUI member initialisation

////@begin ControlPanelGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_VALIDATE_RECURSIVELY|wxWS_EX_BLOCK_EVENTS);
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ControlPanelGUI creation
    return true;
}

/*!
 * Control creation for ControlPanelGUI
 */

void ControlPanelGUI::CreateControls()
{    
////@begin ControlPanelGUI content construction
    ControlPanelGUI* itemPanel1 = this;

    p_controlVSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(p_controlVSizer);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    p_controlVSizer->Add(itemBoxSizer3, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox4 = new ewxCheckBox( itemPanel1, ID_CHECKBOX_CNTRL_USELOAD, _("Set Dynamic Load Balancing"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox4->SetValue(true);
    itemBoxSizer3->Add(itemCheckBox4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    p_controlVSizer->Add(itemBoxSizer5, 0, wxALIGN_LEFT|wxALL, 0);

    p_balancePanel = new ewxPanel( itemPanel1, ID_PANEL_CNTRL_ADJUST, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer5->Add(p_balancePanel, 0, wxGROW|wxALL, 0);

    p_configVSizer = new wxBoxSizer(wxVERTICAL);
    p_balancePanel->SetSizer(p_configVSizer);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    p_configVSizer->Add(itemBoxSizer8, 0, wxALIGN_LEFT|wxLEFT, 15);

    ewxCheckBox* itemCheckBox9 = new ewxCheckBox( p_balancePanel, ID_CHECKBOX_CNTRL_CELLSZ, _("Adjust Cell Sizes"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox9->SetValue(false);
    itemBoxSizer8->Add(itemCheckBox9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel10 = new ewxPanel( p_balancePanel, ID_PANEL_CNTRL_STRNGTH, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_configVSizer->Add(itemPanel10, 0, wxALIGN_LEFT|wxLEFT, 30);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel10->SetSizer(itemBoxSizer11);

    ewxStaticText* itemStaticText12 = new ewxStaticText( itemPanel10, wxID_STATIC, _("Load Balancing Coupling Strength:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemStaticText12, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl13 = new ewxTextCtrl( itemPanel10, ID_TEXTCTRL_CNTRL_STRNGTH, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemTextCtrl13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer11->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox15 = new ewxCheckBox( itemPanel10, ID_CHECKBOX_CNTRL_ZONLY, _("Adjust Z-Axis Only"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox15->SetValue(false);
    itemBoxSizer11->Add(itemCheckBox15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    p_configVSizer->Add(itemBoxSizer16, 0, wxALIGN_LEFT|wxLEFT, 15);

    ewxCheckBox* itemCheckBox17 = new ewxCheckBox( p_balancePanel, ID_CHECKBOX_CNTRL_RDSTRB, _("Redistribute Cell-Cell Pair List"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox17->SetValue(false);
    itemBoxSizer16->Add(itemCheckBox17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel18 = new ewxPanel( p_balancePanel, ID_PANEL_CNTRL_RDPAR, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_configVSizer->Add(itemPanel18, 0, wxALIGN_LEFT|wxLEFT, 15);

    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxVERTICAL);
    itemPanel18->SetSizer(itemBoxSizer19);

    wxBoxSizer* itemBoxSizer20 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer19->Add(itemBoxSizer20, 0, wxALIGN_LEFT|wxLEFT, 15);

    ewxStaticText* itemStaticText21 = new ewxStaticText( itemPanel18, wxID_STATIC_CNTRL_CFREQ, _("Cell Size Adjustment Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer20->Add(itemStaticText21, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl22 = new ewxTextCtrl( itemPanel18, ID_TEXTCTRL_CNTRL_CFREQ, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer20->Add(itemTextCtrl22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer23 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer19->Add(itemBoxSizer23, 0, wxALIGN_LEFT|wxLEFT, 15);

    ewxStaticText* itemStaticText24 = new ewxStaticText( itemPanel18, wxID_STATIC_CNTRL_TSYNCH, _("Synchronization Time for Rebalancing:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer23->Add(itemStaticText24, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice25Strings[] = {
        _("From Last Step"),
        _("Minimum since Last Rebalancing"),
        _("Average since Last Rebalancing")
    };
    ewxChoice* itemChoice25 = new ewxChoice( itemPanel18, ID_CHOICE_CNTRL_TSYNCH, wxDefaultPosition, wxDefaultSize, 3, itemChoice25Strings, 0 );
    itemBoxSizer23->Add(itemChoice25, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer26 = new wxBoxSizer(wxHORIZONTAL);
    p_configVSizer->Add(itemBoxSizer26, 0, wxALIGN_LEFT|wxLEFT, 15);

    wxBitmap itemBitmapButton27Bitmap(wxNullBitmap);
    ewxOpenCloseToggle* itemBitmapButton27 = new ewxOpenCloseToggle( p_balancePanel, ID_TOGGLE_CNTRL_CNFG, itemBitmapButton27Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer26->Add(itemBitmapButton27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText28 = new ewxStaticText( p_balancePanel, wxID_STATIC, _("Advanced Configuration Options"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemStaticText28, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer29 = new wxBoxSizer(wxHORIZONTAL);
    p_configVSizer->Add(itemBoxSizer29, 0, wxALIGN_LEFT|wxLEFT, 15);

    itemBoxSizer29->Add(20, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    p_configPanel = new ewxPanel( p_balancePanel, ID_PANEL_CNTRL_CNFG, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer29->Add(p_configPanel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxBoxSizer* itemBoxSizer32 = new wxBoxSizer(wxVERTICAL);
    p_configPanel->SetSizer(itemBoxSizer32);

    wxBoxSizer* itemBoxSizer33 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer32->Add(itemBoxSizer33, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox34 = new ewxCheckBox( p_configPanel, ID_CHECKBOX_CNTRL_PCNFG, _("Processor Configuration"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox34->SetValue(false);
    itemBoxSizer33->Add(itemCheckBox34, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer35 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer32->Add(itemBoxSizer35, 0, wxALIGN_LEFT|wxALL, 0);

    ewxPanel* itemPanel36 = new ewxPanel( p_configPanel, ID_PANEL_CNTRL_NPROC, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer35->Add(itemPanel36, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer37 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel36->SetSizer(itemBoxSizer37);

    itemBoxSizer37->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText39 = new ewxStaticText( itemPanel36, wxID_STATIC, _("NX:"), wxPoint(1, -1), wxDefaultSize, 0 );
    itemBoxSizer37->Add(itemStaticText39, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl40 = new ewxTextCtrl( itemPanel36, ID_TEXTCTRL_CNTRL_PX, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer37->Add(itemTextCtrl40, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer37->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText42 = new ewxStaticText( itemPanel36, wxID_STATIC, _("NY:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer37->Add(itemStaticText42, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl43 = new ewxTextCtrl( itemPanel36, ID_TEXTCTRL_CNTRL_PY, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer37->Add(itemTextCtrl43, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer37->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText45 = new ewxStaticText( itemPanel36, wxID_STATIC, _("NZ:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer37->Add(itemStaticText45, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl46 = new ewxTextCtrl( itemPanel36, ID_TEXTCTRL_CNTRL_PZ, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer37->Add(itemTextCtrl46, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer47 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer32->Add(itemBoxSizer47, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox48 = new ewxCheckBox( p_configPanel, ID_CHECKBOX_CNTRL_CCNFG, _("Simulation Cell Configuration"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox48->SetValue(false);
    itemBoxSizer47->Add(itemCheckBox48, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer49 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer32->Add(itemBoxSizer49, 0, wxALIGN_LEFT|wxALL, 0);

    ewxPanel* itemPanel50 = new ewxPanel( p_configPanel, ID_PANEL_CNTRL_NCELL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer49->Add(itemPanel50, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer51 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel50->SetSizer(itemBoxSizer51);

    itemBoxSizer51->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText53 = new ewxStaticText( itemPanel50, wxID_STATIC, _("NX:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer51->Add(itemStaticText53, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl54 = new ewxTextCtrl( itemPanel50, ID_TEXTCTRL_CNTRL_CX, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer51->Add(itemTextCtrl54, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer51->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText56 = new ewxStaticText( itemPanel50, wxID_STATIC, _("NY:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer51->Add(itemStaticText56, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl57 = new ewxTextCtrl( itemPanel50, ID_TEXTCTRL_CNTRL_CY, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer51->Add(itemTextCtrl57, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer51->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText59 = new ewxStaticText( itemPanel50, wxID_STATIC, _("NZ:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer51->Add(itemStaticText59, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl60 = new ewxTextCtrl( itemPanel50, ID_TEXTCTRL_CNTRL_CZ, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer51->Add(itemTextCtrl60, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer61 = new wxBoxSizer(wxHORIZONTAL);
    p_controlVSizer->Add(itemBoxSizer61, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox62 = new ewxCheckBox( itemPanel1, ID_CHECKBOX_CNTRL_MAXSLV, _("Maximum Number of Solvent Molecules Per Processor:"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox62->SetValue(false);
    itemBoxSizer61->Add(itemCheckBox62, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxTextCtrl* itemTextCtrl63 = new ewxTextCtrl( itemPanel1, ID_TEXTCTRL_CNTRL_MAXSLV, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer61->Add(itemTextCtrl63, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer64 = new wxBoxSizer(wxHORIZONTAL);
    p_controlVSizer->Add(itemBoxSizer64, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox65 = new ewxCheckBox( itemPanel1, ID_CHECKBOX_CNTRL_MAXSLU, _("Maximum Number of Solute Atoms Per Processor:"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox65->SetValue(false);
    itemBoxSizer64->Add(itemCheckBox65, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxTextCtrl* itemTextCtrl66 = new ewxTextCtrl( itemPanel1, ID_TEXTCTRL_CNTRL_MAXSLU, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer64->Add(itemTextCtrl66, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer67 = new wxBoxSizer(wxHORIZONTAL);
    p_controlVSizer->Add(itemBoxSizer67, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox68 = new ewxCheckBox( itemPanel1, ID_CHECKBOX_CNTRL_PACK, _("Used Packed Data for Communication"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox68->SetValue(true);
    itemBoxSizer67->Add(itemCheckBox68, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

////@end ControlPanelGUI content construction
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_USELOAD
 */

void ControlPanelGUI::OnCheckboxCntrlUseloadClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_USELOAD in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_USELOAD in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_CELLSZ
 */

void ControlPanelGUI::OnCheckboxCntrlCellszClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_CELLSZ in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_CELLSZ in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_STRNGTH
 */

void ControlPanelGUI::OnTextctrlCntrlStrngthEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_STRNGTH in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_STRNGTH in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_ZONLY
 */

void ControlPanelGUI::OnCheckboxCntrlZonlyClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_ZONLY in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_ZONLY in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_RDSTRB
 */

void ControlPanelGUI::OnCheckboxCntrlRdstrbClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_RDSTRB in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_RDSTRB in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CFREQ
 */

void ControlPanelGUI::OnTextctrlCntrlCfreqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CFREQ in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CFREQ in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CNTRL_TSYNCH
 */

void ControlPanelGUI::OnChoiceCntrlTsynchSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CNTRL_TSYNCH in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CNTRL_TSYNCH in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TOGGLE_CNTRL_CNFG
 */

void ControlPanelGUI::OnToggleCntrlCnfgClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TOGGLE_CNTRL_CNFG in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TOGGLE_CNTRL_CNFG in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_PCNFG
 */

void ControlPanelGUI::OnCheckboxCntrlPcnfgClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_PCNFG in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_PCNFG in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_PX
 */

void ControlPanelGUI::OnTextctrlCntrlPxEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_PX in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_PX in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_PY
 */

void ControlPanelGUI::OnTextctrlCntrlPyEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_PY in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_PY in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_PZ
 */

void ControlPanelGUI::OnTextctrlCntrlPzEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_PZ in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_PZ in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_CCNFG
 */

void ControlPanelGUI::OnCheckboxCntrlCcnfgClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_CCNFG in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_CCNFG in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CX
 */

void ControlPanelGUI::OnTextctrlCntrlCxEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CX in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CX in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CY
 */

void ControlPanelGUI::OnTextctrlCntrlCyEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CY in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CY in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CZ
 */

void ControlPanelGUI::OnTextctrlCntrlCzEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CZ in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CZ in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_MAXSLV
 */

void ControlPanelGUI::OnCheckboxCntrlMaxslvClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_MAXSLV in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_MAXSLV in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_MAXSLV
 */

void ControlPanelGUI::OnTextctrlCntrlMaxslvEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_MAXSLV in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_MAXSLV in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_MAXSLU
 */

void ControlPanelGUI::OnCheckboxCntrlMaxsluClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_MAXSLU in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_MAXSLU in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_MAXSLU
 */

void ControlPanelGUI::OnTextctrlCntrlMaxsluEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_MAXSLU in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_MAXSLU in ControlPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_PACK
 */

void ControlPanelGUI::OnCheckboxCntrlPackClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_PACK in ControlPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_PACK in ControlPanelGUI. 
}

/*!
 * Should we show tooltips?
 */

bool ControlPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap ControlPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ControlPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end ControlPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ControlPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ControlPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end ControlPanelGUI icon retrieval
}
