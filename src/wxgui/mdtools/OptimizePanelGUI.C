/////////////////////////////////////////////////////////////////////////////
// Name:        OptimizePanelGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "OptimizePanelGUI.H"
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
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxgui/OptimizePanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID OptimizePanelGUI::ID_TEXTCTRL_OPT_MAXSZ = wxNewId();
const wxWindowID OptimizePanelGUI::ID_TEXTCTRL_OPT_SD_MAXSTP = wxNewId();
const wxWindowID OptimizePanelGUI::ID_PANEL_OPT_CGPAR = wxNewId();
const wxWindowID OptimizePanelGUI::ID_TEXTCTRL_OPT_TOL = wxNewId();
const wxWindowID OptimizePanelGUI::ID_TEXTCTRL_OPT_ISTEP = wxNewId();
const wxWindowID OptimizePanelGUI::ID_DIALOG = wxNewId();
const wxWindowID OptimizePanelGUI::wxID_STATIC_OPT_THRESH = wxNewId();
const wxWindowID OptimizePanelGUI::ID_TEXTCTRL_OPT_THRESH = wxNewId();
const wxWindowID OptimizePanelGUI::ID_TEXTCTRL_OPT_ISRCH = wxNewId();
const wxWindowID OptimizePanelGUI::ID_PANEL_OPT_SDPAR = wxNewId();
const wxWindowID OptimizePanelGUI::wxID_STATIC_OPT_MAXSZ = wxNewId();
const wxWindowID OptimizePanelGUI::wxID_STATIC_OPT_ISRCH = wxNewId();
const wxWindowID OptimizePanelGUI::wxID_STATIC_OPT_ISTEP = wxNewId();
const wxWindowID OptimizePanelGUI::ID_TEXTCTRL_OPT_CG_MAXSTP = wxNewId();
const wxWindowID OptimizePanelGUI::wxID_STATIC_OPT_TOL = wxNewId();
const wxWindowID OptimizePanelGUI::ID_TEXTCTRL_OPT_REFRESH = wxNewId();
const wxWindowID OptimizePanelGUI::ID_CHECKBOX_OPT_USESD = wxNewId();
const wxWindowID OptimizePanelGUI::ID_CHECKBOX_OPT_USECG = wxNewId();

/*!
 * OptimizePanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( OptimizePanelGUI, ewxPanel )

/*!
 * OptimizePanelGUI event table definition
 */

BEGIN_EVENT_TABLE( OptimizePanelGUI, ewxPanel )

////@begin OptimizePanelGUI event table entries
    EVT_CHECKBOX( ID_CHECKBOX_OPT_USESD, OptimizePanelGUI::OnCheckboxOptUsesdClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_OPT_SD_MAXSTP, OptimizePanelGUI::OnTextctrlOptSdMaxstpEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_OPT_ISTEP, OptimizePanelGUI::OnTextctrlOptIstepEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_OPT_TOL, OptimizePanelGUI::OnTextctrlOptTolEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_OPT_MAXSZ, OptimizePanelGUI::OnTextctrlOptMaxszEnter )

    EVT_CHECKBOX( ID_CHECKBOX_OPT_USECG, OptimizePanelGUI::OnCheckboxOptUsecgClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_OPT_CG_MAXSTP, OptimizePanelGUI::OnTextctrlOptCgMaxstpEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_OPT_ISRCH, OptimizePanelGUI::OnTextctrlOptIsrchEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_OPT_THRESH, OptimizePanelGUI::OnTextctrlOptThreshEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_OPT_REFRESH, OptimizePanelGUI::OnTextctrlOptRefreshEnter )

////@end OptimizePanelGUI event table entries

END_EVENT_TABLE()

/*!
 * OptimizePanelGUI constructors
 */

OptimizePanelGUI::OptimizePanelGUI( )
{
}

OptimizePanelGUI::OptimizePanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * OptimizePanelGUI creator
 */

bool OptimizePanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin OptimizePanelGUI member initialisation
    p_optimizeVSizer = NULL;
    p_SDPanel = NULL;
    p_CGPanel = NULL;
////@end OptimizePanelGUI member initialisation

////@begin OptimizePanelGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_VALIDATE_RECURSIVELY|wxWS_EX_BLOCK_EVENTS);
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end OptimizePanelGUI creation
    return true;
}

/*!
 * Control creation for OptimizePanelGUI
 */

void OptimizePanelGUI::CreateControls()
{    
////@begin OptimizePanelGUI content construction
    OptimizePanelGUI* itemPanel1 = this;

    p_optimizeVSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(p_optimizeVSizer);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    p_optimizeVSizer->Add(itemBoxSizer3, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox4 = new ewxCheckBox( itemPanel1, ID_CHECKBOX_OPT_USESD, _("Use Steepest Descent Optimization"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox4->SetValue(false);
    itemBoxSizer3->Add(itemCheckBox4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_SDPanel = new ewxPanel( itemPanel1, ID_PANEL_OPT_SDPAR, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_optimizeVSizer->Add(p_SDPanel, 0, wxALIGN_LEFT|wxLEFT, 15);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxVERTICAL);
    p_SDPanel->SetSizer(itemBoxSizer6);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer6->Add(itemBoxSizer7, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText8 = new ewxStaticText( p_SDPanel, wxID_STATIC, _("Maximum Steps:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemStaticText8, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl9 = new ewxTextCtrl( p_SDPanel, ID_TEXTCTRL_OPT_SD_MAXSTP, _("100"), wxDefaultPosition, wxSize(50, -1), 0 );
    itemBoxSizer7->Add(itemTextCtrl9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer7->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText11 = new ewxStaticText( p_SDPanel, wxID_STATIC, _("Initial Step Size:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemStaticText11, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl12 = new ewxTextCtrl( p_SDPanel, ID_TEXTCTRL_OPT_ISTEP, _("0.01"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemTextCtrl12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText13 = new ewxStaticText( p_SDPanel, wxID_STATIC_OPT_ISTEP, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemStaticText13, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer6->Add(itemBoxSizer14, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText15 = new ewxStaticText( p_SDPanel, wxID_STATIC, _("Convergence Tolerance:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemStaticText15, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl16 = new ewxTextCtrl( p_SDPanel, ID_TEXTCTRL_OPT_TOL, _("0.0001"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemTextCtrl16, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText17 = new ewxStaticText( p_SDPanel, wxID_STATIC_OPT_TOL, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemStaticText17, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer6->Add(itemBoxSizer18, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText19 = new ewxStaticText( p_SDPanel, wxID_STATIC, _("Maximum Step Size:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText19, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl20 = new ewxTextCtrl( p_SDPanel, ID_TEXTCTRL_OPT_MAXSZ, _("0.1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemTextCtrl20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText21 = new ewxStaticText( p_SDPanel, wxID_STATIC_OPT_MAXSZ, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText21, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer22 = new wxBoxSizer(wxHORIZONTAL);
    p_optimizeVSizer->Add(itemBoxSizer22, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox23 = new ewxCheckBox( itemPanel1, ID_CHECKBOX_OPT_USECG, _("Use Conjugate Gradient Optimization"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox23->SetValue(false);
    itemBoxSizer22->Add(itemCheckBox23, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_CGPanel = new ewxPanel( itemPanel1, ID_PANEL_OPT_CGPAR, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_optimizeVSizer->Add(p_CGPanel, 0, wxALIGN_LEFT|wxLEFT, 15);

    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxVERTICAL);
    p_CGPanel->SetSizer(itemBoxSizer25);

    wxBoxSizer* itemBoxSizer26 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer25->Add(itemBoxSizer26, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText27 = new ewxStaticText( p_CGPanel, wxID_STATIC, _("Maximum Steps:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemStaticText27, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl28 = new ewxTextCtrl( p_CGPanel, ID_TEXTCTRL_OPT_CG_MAXSTP, _("100"), wxDefaultPosition, wxSize(50, -1), 0 );
    itemBoxSizer26->Add(itemTextCtrl28, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer26->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText30 = new ewxStaticText( p_CGPanel, wxID_STATIC, _("Initial Search Interval Size:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemStaticText30, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl31 = new ewxTextCtrl( p_CGPanel, ID_TEXTCTRL_OPT_ISRCH, _("0.001"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemTextCtrl31, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText32 = new ewxStaticText( p_CGPanel, wxID_STATIC_OPT_ISRCH, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemStaticText32, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer33 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer25->Add(itemBoxSizer33, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText34 = new ewxStaticText( p_CGPanel, wxID_STATIC, _("Threshold Step Size:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer33->Add(itemStaticText34, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl35 = new ewxTextCtrl( p_CGPanel, ID_TEXTCTRL_OPT_THRESH, _("0.0001"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer33->Add(itemTextCtrl35, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText36 = new ewxStaticText( p_CGPanel, wxID_STATIC_OPT_THRESH, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer33->Add(itemStaticText36, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer37 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer25->Add(itemBoxSizer37, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText38 = new ewxStaticText( p_CGPanel, wxID_STATIC, _("Conjugate Gradient Refresh Cycle:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer37->Add(itemStaticText38, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl39 = new ewxTextCtrl( p_CGPanel, ID_TEXTCTRL_OPT_REFRESH, _("10"), wxDefaultPosition, wxSize(50, -1), 0 );
    itemBoxSizer37->Add(itemTextCtrl39, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

////@end OptimizePanelGUI content construction
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_OPT_USESD
 */

void OptimizePanelGUI::OnCheckboxOptUsesdClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_OPT_USESD in OptimizePanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_OPT_USESD in OptimizePanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_SD_MAXSTP
 */

void OptimizePanelGUI::OnTextctrlOptSdMaxstpEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_SD_MAXSTP in OptimizePanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_SD_MAXSTP in OptimizePanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_ISTEP
 */

void OptimizePanelGUI::OnTextctrlOptIstepEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_ISTEP in OptimizePanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_ISTEP in OptimizePanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_TOL
 */

void OptimizePanelGUI::OnTextctrlOptTolEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_TOL in OptimizePanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_TOL in OptimizePanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_MAXSZ
 */

void OptimizePanelGUI::OnTextctrlOptMaxszEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_MAXSZ in OptimizePanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_MAXSZ in OptimizePanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_OPT_USECG
 */

void OptimizePanelGUI::OnCheckboxOptUsecgClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_OPT_USECG in OptimizePanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_OPT_USECG in OptimizePanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_CG_MAXSTP
 */

void OptimizePanelGUI::OnTextctrlOptCgMaxstpEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_CG_MAXSTP in OptimizePanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_CG_MAXSTP in OptimizePanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_ISRCH
 */

void OptimizePanelGUI::OnTextctrlOptIsrchEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_ISRCH in OptimizePanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_ISRCH in OptimizePanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_THRESH
 */

void OptimizePanelGUI::OnTextctrlOptThreshEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_THRESH in OptimizePanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_THRESH in OptimizePanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_REFRESH
 */

void OptimizePanelGUI::OnTextctrlOptRefreshEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_REFRESH in OptimizePanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_REFRESH in OptimizePanelGUI. 
}

/*!
 * Should we show tooltips?
 */

bool OptimizePanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap OptimizePanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin OptimizePanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end OptimizePanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon OptimizePanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin OptimizePanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end OptimizePanelGUI icon retrieval
}
