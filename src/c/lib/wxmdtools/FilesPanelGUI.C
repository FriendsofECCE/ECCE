/////////////////////////////////////////////////////////////////////////////
// Name:        FilesPanelGUI.C
// Purpose:     
// Author:      FilesPanelGUI
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "FilesPanelGUI.H"
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
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxgui/FilesPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_PRPFRQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_SLVCRD = wxNewId();
const wxWindowID FilesPanelGUI::ID_PANEL_FILE_TRJOPT = wxNewId();
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_RSTFRQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_STAT = wxNewId();
const wxWindowID FilesPanelGUI::wxID_STATIC_FILE_FSUFRQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_SLTCRD = wxNewId();
const wxWindowID FilesPanelGUI::wxID_STATIC_FILE_VSUFRQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_VSUFRQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_SUBOND = wxNewId();
const wxWindowID FilesPanelGUI::ID_PANEL_FILE = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_USEAVG = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_TOP = wxNewId();
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_STFREQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_PANEL_FILE_WRTPRP = wxNewId();
const wxWindowID FilesPanelGUI::wxID_STATIC_FILE_VSVFRQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_NBOND = wxNewId();
const wxWindowID FilesPanelGUI::ID_PANEL_FILE_WRTFRC = wxNewId();
const wxWindowID FilesPanelGUI::wxID_STATIC_FILE_SFFREQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_SLVVEL = wxNewId();
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_CSVFRQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_BTCSZ = wxNewId();
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_CSUFRQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_PANEL_FILE_WRTVEL = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_SLVPAR = wxNewId();
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_MDFREQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_PANEL_FILE_WRTRST = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_KEEP = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_SLTVEL = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_EBOND = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_SLTFRC = wxNewId();
const wxWindowID FilesPanelGUI::ID_PANEL_FILE_MDOPT = wxNewId();
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_FSUFRQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_SLVFRC = wxNewId();
const wxWindowID FilesPanelGUI::wxID_STATIC_FILE_SEFREQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_NOTEBOOK = wxNewId();
const wxWindowID FilesPanelGUI::wxID_STATIC_FILE_FSVFRQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_SEFREQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_USEBTC = wxNewId();
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_FSVFRQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_PANEL_FILE_WRTOUT = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_ESOL = wxNewId();
const wxWindowID FilesPanelGUI::ID_PANEL_FILE_WRTTRJ = wxNewId();
const wxWindowID FilesPanelGUI::ID_PANEL_FILE_TOPOPT = wxNewId();
const wxWindowID FilesPanelGUI::wxID_STATIC_FILE_CSUFRQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_VSVFRQ = wxNewId();
const wxWindowID FilesPanelGUI::wxID_STATIC_FILE_MDFREQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_SLUPAR = wxNewId();
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_PMFFREQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_PRTMD = wxNewId();
const wxWindowID FilesPanelGUI::ID_TEXTCTRL_FILE_SFFREQ = wxNewId();
const wxWindowID FilesPanelGUI::wxID_STATIC_FILE_CSVFRQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHOICE_FILE_TRJSTR = wxNewId();
const wxWindowID FilesPanelGUI::wxID_STATIC_FILE_BTCSZ = wxNewId();
const wxWindowID FilesPanelGUI::wxID_STATIC_FILE_PMFFREQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_PMF = wxNewId();
const wxWindowID FilesPanelGUI::ID_CHECKBOX_FILE_FRC = wxNewId();
const wxWindowID FilesPanelGUI::wxID_STATIC_FILE_STFREQ = wxNewId();
const wxWindowID FilesPanelGUI::ID_PANEL_FILE_WRTCRD = wxNewId();

/*!
 * FilesPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( FilesPanelGUI, ewxPanel )

/*!
 * FilesPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( FilesPanelGUI, ewxPanel )

////@begin FilesPanelGUI event table entries
    EVT_CHOICE( ID_CHOICE_FILE_TRJSTR, FilesPanelGUI::OnChoiceFileTrjstrSelected )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_USEBTC, FilesPanelGUI::OnCheckboxFileUsebtcClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_BTCSZ, FilesPanelGUI::OnTextctrlFileBtcszEnter )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_SLTCRD, FilesPanelGUI::OnCheckboxFileSltcrdClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_CSUFRQ, FilesPanelGUI::OnTextctrlFileCsufrqEnter )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_SLVCRD, FilesPanelGUI::OnCheckboxFileSlvcrdClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_CSVFRQ, FilesPanelGUI::OnTextctrlFileCsvfrqEnter )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_SLTVEL, FilesPanelGUI::OnCheckboxFileSltvelClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_VSUFRQ, FilesPanelGUI::OnTextctrlFileVsufrqEnter )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_SLVVEL, FilesPanelGUI::OnCheckboxFileSlvvelClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_VSVFRQ, FilesPanelGUI::OnTextctrlFileVsvfrqEnter )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_SLTFRC, FilesPanelGUI::OnCheckboxFileSltfrcClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_FSUFRQ, FilesPanelGUI::OnTextctrlFileFsufrqEnter )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_SLVFRC, FilesPanelGUI::OnCheckboxFileSlvfrcClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_FSVFRQ, FilesPanelGUI::OnTextctrlFileFsvfrqEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_PRPFRQ, FilesPanelGUI::OnTextctrlFilePrpfrqEnter )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_USEAVG, FilesPanelGUI::OnCheckboxFileUseavgClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_RSTFRQ, FilesPanelGUI::OnTextctrlFileRstfrqEnter )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_KEEP, FilesPanelGUI::OnCheckboxFileKeepClick )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_PRTMD, FilesPanelGUI::OnCheckboxFilePrtmdClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_MDFREQ, FilesPanelGUI::OnTextctrlFileMdfreqEnter )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_ESOL, FilesPanelGUI::OnCheckboxFileEsolClick )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_EBOND, FilesPanelGUI::OnCheckboxFileEbondClick )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_STAT, FilesPanelGUI::OnCheckboxFileStatClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_STFREQ, FilesPanelGUI::OnTextctrlFileStfreqEnter )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_SUBOND, FilesPanelGUI::OnCheckboxFileSubondClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_SEFREQ, FilesPanelGUI::OnTextctrlFileSefreqEnter )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_FRC, FilesPanelGUI::OnCheckboxFileFrcClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_SFFREQ, FilesPanelGUI::OnTextctrlFileSffreqEnter )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_PMF, FilesPanelGUI::OnCheckboxFilePmfClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_FILE_PMFFREQ, FilesPanelGUI::OnTextctrlFilePmffreqEnter )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_TOP, FilesPanelGUI::OnCheckboxFileTopClick )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_NBOND, FilesPanelGUI::OnCheckboxFileNbondClick )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_SLVPAR, FilesPanelGUI::OnCheckboxFileSlvparClick )

    EVT_CHECKBOX( ID_CHECKBOX_FILE_SLUPAR, FilesPanelGUI::OnCheckboxFileSluparClick )

////@end FilesPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * FilesPanelGUI constructors
 */

FilesPanelGUI::FilesPanelGUI( )
{
}

FilesPanelGUI::FilesPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * FilesPanelGUI creator
 */

bool FilesPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin FilesPanelGUI member initialisation
    p_trajectoryPanel = NULL;
    p_trajectoryVSizer = NULL;
    p_coordPanel = NULL;
    p_velocPanel = NULL;
    p_forcePanel = NULL;
    p_propPanel = NULL;
    p_restartPanel = NULL;
////@end FilesPanelGUI member initialisation

////@begin FilesPanelGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_VALIDATE_RECURSIVELY|wxWS_EX_BLOCK_EVENTS);
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
////@end FilesPanelGUI creation
    return true;
}

/*!
 * Control creation for FilesPanelGUI
 */

void FilesPanelGUI::CreateControls()
{    
////@begin FilesPanelGUI content construction
    FilesPanelGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    itemBoxSizer2->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxNotebook* itemNotebook4 = new ewxNotebook( itemPanel1, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP|wxNO_BORDER );

    ewxPanel* itemPanel5 = new ewxPanel( itemNotebook4, ID_PANEL_FILE_WRTTRJ, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxVERTICAL);
    itemPanel5->SetSizer(itemBoxSizer6);

    p_trajectoryPanel = new ewxPanel( itemPanel5, ID_PANEL_FILE_TRJOPT, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer6->Add(p_trajectoryPanel, 1, wxGROW|wxALL, 0);
    p_trajectoryVSizer = new wxBoxSizer(wxVERTICAL);
    p_trajectoryPanel->SetSizer(p_trajectoryVSizer);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
    p_trajectoryVSizer->Add(itemBoxSizer9, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText10 = new ewxStaticText( p_trajectoryPanel, wxID_STATIC, _("Trajectory and Restart File Storage:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemStaticText10, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemChoice11Strings[] = {
        _("Compute Resource Only"),
        _("NWFS Archive"),
        _("Data Server")
    };
    ewxChoice* itemChoice11 = new ewxChoice( p_trajectoryPanel, ID_CHOICE_FILE_TRJSTR, wxDefaultPosition, wxDefaultSize, 3, itemChoice11Strings, 0 );
    itemChoice11->SetStringSelection(_("Compute Resource Only"));
    itemBoxSizer9->Add(itemChoice11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxHORIZONTAL);
    p_trajectoryVSizer->Add(itemBoxSizer12, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox13 = new ewxCheckBox( p_trajectoryPanel, ID_CHECKBOX_FILE_USEBTC, _("Divide Trajectory File into Batches"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox13->SetValue(false);
    itemBoxSizer12->Add(itemCheckBox13, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 3);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    p_trajectoryVSizer->Add(itemBoxSizer14, 0, wxALIGN_LEFT|wxALL, 0);
    itemBoxSizer14->Add(20, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText16 = new ewxStaticText( p_trajectoryPanel, wxID_STATIC_FILE_BTCSZ, _("Number of Steps per Batch:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemStaticText16, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl17 = new ewxTextCtrl( p_trajectoryPanel, ID_TEXTCTRL_FILE_BTCSZ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemTextCtrl17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxHORIZONTAL);
    p_trajectoryVSizer->Add(itemBoxSizer18, 1, wxGROW|wxALL, 0);
    ewxStaticLine* itemStaticLine19 = new ewxStaticLine( p_trajectoryPanel, wxID_STATIC, wxDefaultPosition, wxSize(10, -1), wxLI_HORIZONTAL );
    itemBoxSizer18->Add(itemStaticLine19, 0, wxGROW|wxALL, 3);

    ewxStaticText* itemStaticText20 = new ewxStaticText( p_trajectoryPanel, wxID_STATIC, _("Coordinates in Trajectory File"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText20, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine21 = new ewxStaticLine( p_trajectoryPanel, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer18->Add(itemStaticLine21, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer22 = new wxBoxSizer(wxHORIZONTAL);
    p_trajectoryVSizer->Add(itemBoxSizer22, 0, wxALIGN_LEFT|wxALL, 0);
    itemBoxSizer22->Add(3, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    p_coordPanel = new ewxPanel( p_trajectoryPanel, ID_PANEL_FILE_WRTCRD, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer22->Add(p_coordPanel, 0, wxALIGN_TOP|wxALL, 0);
    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxVERTICAL);
    p_coordPanel->SetSizer(itemBoxSizer25);

    wxFlexGridSizer* itemFlexGridSizer26 = new wxFlexGridSizer(2, 3, 0, 0);
    itemBoxSizer25->Add(itemFlexGridSizer26, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox27 = new ewxCheckBox( p_coordPanel, ID_CHECKBOX_FILE_SLTCRD, _("Print Solute/System Coordinates"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox27->SetValue(false);
    itemFlexGridSizer26->Add(itemCheckBox27, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText28 = new ewxStaticText( p_coordPanel, wxID_STATIC_FILE_CSUFRQ, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer26->Add(itemStaticText28, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 10);

    ewxTextCtrl* itemTextCtrl29 = new ewxTextCtrl( p_coordPanel, ID_TEXTCTRL_FILE_CSUFRQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer26->Add(itemTextCtrl29, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox30 = new ewxCheckBox( p_coordPanel, ID_CHECKBOX_FILE_SLVCRD, _("Print Solvent Coordinates"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox30->SetValue(false);
    itemFlexGridSizer26->Add(itemCheckBox30, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText31 = new ewxStaticText( p_coordPanel, wxID_STATIC_FILE_CSVFRQ, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer26->Add(itemStaticText31, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 10);

    ewxTextCtrl* itemTextCtrl32 = new ewxTextCtrl( p_coordPanel, ID_TEXTCTRL_FILE_CSVFRQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer26->Add(itemTextCtrl32, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer33 = new wxBoxSizer(wxHORIZONTAL);
    p_trajectoryVSizer->Add(itemBoxSizer33, 1, wxGROW|wxALL, 0);
    ewxStaticLine* itemStaticLine34 = new ewxStaticLine( p_trajectoryPanel, wxID_STATIC, wxDefaultPosition, wxSize(10, -1), wxLI_HORIZONTAL );
    itemBoxSizer33->Add(itemStaticLine34, 0, wxGROW|wxALL, 3);

    ewxStaticText* itemStaticText35 = new ewxStaticText( p_trajectoryPanel, wxID_STATIC, _("Velocities in Trajectory File"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer33->Add(itemStaticText35, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine36 = new ewxStaticLine( p_trajectoryPanel, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer33->Add(itemStaticLine36, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer37 = new wxBoxSizer(wxHORIZONTAL);
    p_trajectoryVSizer->Add(itemBoxSizer37, 0, wxALIGN_LEFT|wxALL, 0);
    itemBoxSizer37->Add(3, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    p_velocPanel = new ewxPanel( p_trajectoryPanel, ID_PANEL_FILE_WRTVEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer37->Add(p_velocPanel, 0, wxALIGN_TOP|wxALL, 0);
    wxBoxSizer* itemBoxSizer40 = new wxBoxSizer(wxVERTICAL);
    p_velocPanel->SetSizer(itemBoxSizer40);

    wxFlexGridSizer* itemFlexGridSizer41 = new wxFlexGridSizer(2, 3, 0, 0);
    itemBoxSizer40->Add(itemFlexGridSizer41, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox42 = new ewxCheckBox( p_velocPanel, ID_CHECKBOX_FILE_SLTVEL, _("Print Solute/System Velocities"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox42->SetValue(false);
    itemFlexGridSizer41->Add(itemCheckBox42, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText43 = new ewxStaticText( p_velocPanel, wxID_STATIC_FILE_VSUFRQ, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer41->Add(itemStaticText43, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 10);

    ewxTextCtrl* itemTextCtrl44 = new ewxTextCtrl( p_velocPanel, ID_TEXTCTRL_FILE_VSUFRQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer41->Add(itemTextCtrl44, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox45 = new ewxCheckBox( p_velocPanel, ID_CHECKBOX_FILE_SLVVEL, _("Print Solvent Velocities"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox45->SetValue(false);
    itemFlexGridSizer41->Add(itemCheckBox45, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText46 = new ewxStaticText( p_velocPanel, wxID_STATIC_FILE_VSVFRQ, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer41->Add(itemStaticText46, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 10);

    ewxTextCtrl* itemTextCtrl47 = new ewxTextCtrl( p_velocPanel, ID_TEXTCTRL_FILE_VSVFRQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer41->Add(itemTextCtrl47, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer48 = new wxBoxSizer(wxHORIZONTAL);
    p_trajectoryVSizer->Add(itemBoxSizer48, 1, wxGROW|wxALL, 0);
    ewxStaticLine* itemStaticLine49 = new ewxStaticLine( p_trajectoryPanel, wxID_STATIC, wxDefaultPosition, wxSize(10, -1), wxLI_HORIZONTAL );
    itemBoxSizer48->Add(itemStaticLine49, 0, wxGROW|wxALL, 3);

    ewxStaticText* itemStaticText50 = new ewxStaticText( p_trajectoryPanel, wxID_STATIC, _("Forces in Trajectory File"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemStaticText50, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine51 = new ewxStaticLine( p_trajectoryPanel, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer48->Add(itemStaticLine51, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer52 = new wxBoxSizer(wxHORIZONTAL);
    p_trajectoryVSizer->Add(itemBoxSizer52, 0, wxALIGN_LEFT|wxALL, 0);
    itemBoxSizer52->Add(3, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    p_forcePanel = new ewxPanel( p_trajectoryPanel, ID_PANEL_FILE_WRTFRC, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer52->Add(p_forcePanel, 0, wxALIGN_TOP|wxALL, 0);
    wxBoxSizer* itemBoxSizer55 = new wxBoxSizer(wxVERTICAL);
    p_forcePanel->SetSizer(itemBoxSizer55);

    wxFlexGridSizer* itemFlexGridSizer56 = new wxFlexGridSizer(2, 3, 0, 0);
    itemBoxSizer55->Add(itemFlexGridSizer56, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox57 = new ewxCheckBox( p_forcePanel, ID_CHECKBOX_FILE_SLTFRC, _("Print Solute/System Forces"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox57->SetValue(false);
    itemFlexGridSizer56->Add(itemCheckBox57, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText58 = new ewxStaticText( p_forcePanel, wxID_STATIC_FILE_FSUFRQ, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer56->Add(itemStaticText58, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 10);

    ewxTextCtrl* itemTextCtrl59 = new ewxTextCtrl( p_forcePanel, ID_TEXTCTRL_FILE_FSUFRQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer56->Add(itemTextCtrl59, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox60 = new ewxCheckBox( p_forcePanel, ID_CHECKBOX_FILE_SLVFRC, _("Print Solvent Forces"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox60->SetValue(false);
    itemFlexGridSizer56->Add(itemCheckBox60, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText61 = new ewxStaticText( p_forcePanel, wxID_STATIC_FILE_FSVFRQ, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer56->Add(itemStaticText61, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 10);

    ewxTextCtrl* itemTextCtrl62 = new ewxTextCtrl( p_forcePanel, ID_TEXTCTRL_FILE_FSVFRQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer56->Add(itemTextCtrl62, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook4->AddPage(itemPanel5, _("Trajectory"));

    p_propPanel = new ewxPanel( itemNotebook4, ID_PANEL_FILE_WRTPRP, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer64 = new wxBoxSizer(wxVERTICAL);
    p_propPanel->SetSizer(itemBoxSizer64);

    wxBoxSizer* itemBoxSizer65 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer64->Add(itemBoxSizer65, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText66 = new ewxStaticText( p_propPanel, wxID_STATIC, _("Frequency to Print Properties:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer65->Add(itemStaticText66, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl67 = new ewxTextCtrl( p_propPanel, ID_TEXTCTRL_FILE_PRPFRQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer65->Add(itemTextCtrl67, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer68 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer64->Add(itemBoxSizer68, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox69 = new ewxCheckBox( p_propPanel, ID_CHECKBOX_FILE_USEAVG, _("Write Averages to File"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox69->SetValue(false);
    itemBoxSizer68->Add(itemCheckBox69, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook4->AddPage(p_propPanel, _("Properties"));

    p_restartPanel = new ewxPanel( itemNotebook4, ID_PANEL_FILE_WRTRST, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer71 = new wxBoxSizer(wxVERTICAL);
    p_restartPanel->SetSizer(itemBoxSizer71);

    wxBoxSizer* itemBoxSizer72 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer71->Add(itemBoxSizer72, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText73 = new ewxStaticText( p_restartPanel, wxID_STATIC, _("Frequency to Write Restart File:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer72->Add(itemStaticText73, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl74 = new ewxTextCtrl( p_restartPanel, ID_TEXTCTRL_FILE_RSTFRQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer72->Add(itemTextCtrl74, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer75 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer71->Add(itemBoxSizer75, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox76 = new ewxCheckBox( p_restartPanel, ID_CHECKBOX_FILE_KEEP, _("Keep All Restart Files"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox76->SetValue(false);
    itemBoxSizer75->Add(itemCheckBox76, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook4->AddPage(p_restartPanel, _("Restart"));

    ewxPanel* itemPanel77 = new ewxPanel( itemNotebook4, ID_PANEL_FILE_WRTOUT, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer78 = new wxBoxSizer(wxVERTICAL);
    itemPanel77->SetSizer(itemBoxSizer78);

    wxBoxSizer* itemBoxSizer79 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer78->Add(itemBoxSizer79, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox80 = new ewxCheckBox( itemPanel77, ID_CHECKBOX_FILE_PRTMD, _("Print Molecular Dynamics Step Information"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox80->SetValue(false);
    itemBoxSizer79->Add(itemCheckBox80, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer79->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText82 = new ewxStaticText( itemPanel77, wxID_STATIC_FILE_MDFREQ, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer79->Add(itemStaticText82, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl83 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_FILE_MDFREQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer79->Add(itemTextCtrl83, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 3);

    wxBoxSizer* itemBoxSizer84 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer78->Add(itemBoxSizer84, 0, wxALIGN_LEFT|wxALL, 0);
    itemBoxSizer84->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxPanel* itemPanel86 = new ewxPanel( itemPanel77, ID_PANEL_FILE_MDOPT, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel86->Enable(false);
    itemBoxSizer84->Add(itemPanel86, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
    wxBoxSizer* itemBoxSizer87 = new wxBoxSizer(wxVERTICAL);
    itemPanel86->SetSizer(itemBoxSizer87);

    wxBoxSizer* itemBoxSizer88 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer87->Add(itemBoxSizer88, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox89 = new ewxCheckBox( itemPanel86, ID_CHECKBOX_FILE_ESOL, _("Print Additional Energies for Solute and Solvent"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox89->SetValue(false);
    itemBoxSizer88->Add(itemCheckBox89, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 3);

    wxBoxSizer* itemBoxSizer90 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer87->Add(itemBoxSizer90, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox91 = new ewxCheckBox( itemPanel86, ID_CHECKBOX_FILE_EBOND, _("Print All Bonded Solute Interactions"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox91->SetValue(false);
    itemBoxSizer90->Add(itemCheckBox91, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 3);

    wxBoxSizer* itemBoxSizer92 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer78->Add(itemBoxSizer92, 0, wxGROW|wxALL, 0);
    ewxStaticLine* itemStaticLine93 = new ewxStaticLine( itemPanel77, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer92->Add(itemStaticLine93, 1, wxGROW|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer94 = new wxFlexGridSizer(3, 3, 0, 0);
    itemFlexGridSizer94->AddGrowableCol(0);
    itemBoxSizer78->Add(itemFlexGridSizer94, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox95 = new ewxCheckBox( itemPanel77, ID_CHECKBOX_FILE_STAT, _("Print Properties Statistics"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox95->SetValue(false);
    itemFlexGridSizer94->Add(itemCheckBox95, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText96 = new ewxStaticText( itemPanel77, wxID_STATIC_FILE_STFREQ, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer94->Add(itemStaticText96, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl97 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_FILE_STFREQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer94->Add(itemTextCtrl97, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox98 = new ewxCheckBox( itemPanel77, ID_CHECKBOX_FILE_SUBOND, _("Print Solute Bonded Energies"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox98->SetValue(false);
    itemFlexGridSizer94->Add(itemCheckBox98, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText99 = new ewxStaticText( itemPanel77, wxID_STATIC_FILE_SEFREQ, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer94->Add(itemStaticText99, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl100 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_FILE_SEFREQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer94->Add(itemTextCtrl100, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox101 = new ewxCheckBox( itemPanel77, ID_CHECKBOX_FILE_FRC, _("Print Solute Forces"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox101->SetValue(false);
    itemFlexGridSizer94->Add(itemCheckBox101, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText102 = new ewxStaticText( itemPanel77, wxID_STATIC_FILE_SFFREQ, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer94->Add(itemStaticText102, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl103 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_FILE_SFFREQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer94->Add(itemTextCtrl103, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox104 = new ewxCheckBox( itemPanel77, ID_CHECKBOX_FILE_PMF, _("Print PMF data"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox104->SetValue(false);
    itemFlexGridSizer94->Add(itemCheckBox104, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText105 = new ewxStaticText( itemPanel77, wxID_STATIC_FILE_PMFFREQ, _("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer94->Add(itemStaticText105, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl106 = new ewxTextCtrl( itemPanel77, ID_TEXTCTRL_FILE_PMFFREQ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer94->Add(itemTextCtrl106, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer107 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer78->Add(itemBoxSizer107, 0, wxGROW|wxALL, 0);
    ewxStaticLine* itemStaticLine108 = new ewxStaticLine( itemPanel77, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer107->Add(itemStaticLine108, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer109 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer78->Add(itemBoxSizer109, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox110 = new ewxCheckBox( itemPanel77, ID_CHECKBOX_FILE_TOP, _("Print Topology Information"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox110->SetValue(false);
    itemBoxSizer109->Add(itemCheckBox110, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer111 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer78->Add(itemBoxSizer111, 0, wxALIGN_LEFT|wxALL, 0);
    itemBoxSizer111->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxPanel* itemPanel113 = new ewxPanel( itemPanel77, ID_PANEL_FILE_TOPOPT, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer111->Add(itemPanel113, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
    wxBoxSizer* itemBoxSizer114 = new wxBoxSizer(wxVERTICAL);
    itemPanel113->SetSizer(itemBoxSizer114);

    wxBoxSizer* itemBoxSizer115 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer114->Add(itemBoxSizer115, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox116 = new ewxCheckBox( itemPanel113, ID_CHECKBOX_FILE_NBOND, _("Nonbonded Interaction Parameters"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox116->SetValue(false);
    itemBoxSizer115->Add(itemCheckBox116, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 3);

    wxBoxSizer* itemBoxSizer117 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer114->Add(itemBoxSizer117, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox118 = new ewxCheckBox( itemPanel113, ID_CHECKBOX_FILE_SLVPAR, _("Solvent Bonded Interaction Parameters"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox118->SetValue(false);
    itemBoxSizer117->Add(itemCheckBox118, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 3);

    wxBoxSizer* itemBoxSizer119 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer114->Add(itemBoxSizer119, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox120 = new ewxCheckBox( itemPanel113, ID_CHECKBOX_FILE_SLUPAR, _("Solute Bonded Interaction Parameters"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox120->SetValue(false);
    itemBoxSizer119->Add(itemCheckBox120, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 3);

    itemNotebook4->AddPage(itemPanel77, _("Output"));

    itemBoxSizer2->Add(itemNotebook4, 0, wxALIGN_TOP|wxALL, 0);

    itemBoxSizer2->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end FilesPanelGUI content construction
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_FILE_TRJSTR
 */

void FilesPanelGUI::OnChoiceFileTrjstrSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_FILE_TRJSTR in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_FILE_TRJSTR in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_USEBTC
 */

void FilesPanelGUI::OnCheckboxFileUsebtcClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_USEBTC in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_USEBTC in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_BTCSZ
 */

void FilesPanelGUI::OnTextctrlFileBtcszEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_BTCSZ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_BTCSZ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTCRD
 */

void FilesPanelGUI::OnCheckboxFileSltcrdClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTCRD in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTCRD in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_CSUFRQ
 */

void FilesPanelGUI::OnTextctrlFileCsufrqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_CSUFRQ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_CSUFRQ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVCRD
 */

void FilesPanelGUI::OnCheckboxFileSlvcrdClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVCRD in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVCRD in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_CSVFRQ
 */

void FilesPanelGUI::OnTextctrlFileCsvfrqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_CSVFRQ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_CSVFRQ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTVEL
 */

void FilesPanelGUI::OnCheckboxFileSltvelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTVEL in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTVEL in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_VSUFRQ
 */

void FilesPanelGUI::OnTextctrlFileVsufrqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_VSUFRQ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_VSUFRQ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVVEL
 */

void FilesPanelGUI::OnCheckboxFileSlvvelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVVEL in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVVEL in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_VSVFRQ
 */

void FilesPanelGUI::OnTextctrlFileVsvfrqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_VSVFRQ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_VSVFRQ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTFRC
 */

void FilesPanelGUI::OnCheckboxFileSltfrcClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTFRC in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTFRC in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_FSUFRQ
 */

void FilesPanelGUI::OnTextctrlFileFsufrqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_FSUFRQ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_FSUFRQ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVFRC
 */

void FilesPanelGUI::OnCheckboxFileSlvfrcClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVFRC in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVFRC in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_FSVFRQ
 */

void FilesPanelGUI::OnTextctrlFileFsvfrqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_FSVFRQ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_FSVFRQ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_PRPFRQ
 */

void FilesPanelGUI::OnTextctrlFilePrpfrqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_PRPFRQ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_PRPFRQ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_USEAVG
 */

void FilesPanelGUI::OnCheckboxFileUseavgClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_USEAVG in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_USEAVG in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_RSTFRQ
 */

void FilesPanelGUI::OnTextctrlFileRstfrqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_RSTFRQ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_RSTFRQ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_KEEP
 */

void FilesPanelGUI::OnCheckboxFileKeepClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_KEEP in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_KEEP in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_PRTMD
 */

void FilesPanelGUI::OnCheckboxFilePrtmdClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_PRTMD in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_PRTMD in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_MDFREQ
 */

void FilesPanelGUI::OnTextctrlFileMdfreqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_MDFREQ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_MDFREQ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_ESOL
 */

void FilesPanelGUI::OnCheckboxFileEsolClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_ESOL in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_ESOL in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_EBOND
 */

void FilesPanelGUI::OnCheckboxFileEbondClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_EBOND in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_EBOND in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_STAT
 */

void FilesPanelGUI::OnCheckboxFileStatClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_STAT in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_STAT in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_STFREQ
 */

void FilesPanelGUI::OnTextctrlFileStfreqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_STFREQ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_STFREQ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SUBOND
 */

void FilesPanelGUI::OnCheckboxFileSubondClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SUBOND in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SUBOND in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_SEFREQ
 */

void FilesPanelGUI::OnTextctrlFileSefreqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_SEFREQ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_SEFREQ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_FRC
 */

void FilesPanelGUI::OnCheckboxFileFrcClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_FRC in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_FRC in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_SFFREQ
 */

void FilesPanelGUI::OnTextctrlFileSffreqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_SFFREQ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_SFFREQ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_PMF
 */

void FilesPanelGUI::OnCheckboxFilePmfClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_PMF in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_PMF in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_PMFFREQ
 */

void FilesPanelGUI::OnTextctrlFilePmffreqEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_PMFFREQ in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_PMFFREQ in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_TOP
 */

void FilesPanelGUI::OnCheckboxFileTopClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_TOP in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_TOP in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_NBOND
 */

void FilesPanelGUI::OnCheckboxFileNbondClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_NBOND in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_NBOND in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVPAR
 */

void FilesPanelGUI::OnCheckboxFileSlvparClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVPAR in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVPAR in FilesPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLUPAR
 */

void FilesPanelGUI::OnCheckboxFileSluparClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLUPAR in FilesPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLUPAR in FilesPanelGUI. 
}

/*!
 * Should we show tooltips?
 */

bool FilesPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap FilesPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin FilesPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end FilesPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon FilesPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin FilesPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end FilesPanelGUI icon retrieval
}
