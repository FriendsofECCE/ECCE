/////////////////////////////////////////////////////////////////////////////
// Name:        PeptideGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "PeptideGUI.H"
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
#include "wxgui/ewxButton.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "PeptideGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID PeptideGUI::ID_TEXTCTRL_PEPTIDE_SEQUENCE = wxNewId();
const wxWindowID PeptideGUI::ID_BUTTON_PEPTIDE_GENERATE = wxNewId();
const wxWindowID PeptideGUI::ID_BUTTON_PEPTIDE_CLEAR = wxNewId();
const wxWindowID PeptideGUI::ID_DIALOG_PEPTIDE = wxNewId();

/*!
 * PeptideGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( PeptideGUI, ewxPanel )

/*!
 * PeptideGUI event table definition
 */

BEGIN_EVENT_TABLE( PeptideGUI, ewxPanel )

////@begin PeptideGUI event table entries
    EVT_TEXT( ID_TEXTCTRL_PEPTIDE_SEQUENCE, PeptideGUI::OnTextctrlPeptideSequenceUpdated )
    EVT_TEXT_ENTER( ID_TEXTCTRL_PEPTIDE_SEQUENCE, PeptideGUI::OnTextctrlPeptideSequenceEnter )

    EVT_BUTTON( ID_BUTTON_PEPTIDE_CLEAR, PeptideGUI::OnButtonPeptideClearClick )

    EVT_BUTTON( ID_BUTTON_PEPTIDE_GENERATE, PeptideGUI::OnButtonPeptideGenerateClick )

////@end PeptideGUI event table entries

END_EVENT_TABLE()

/*!
 * PeptideGUI constructors
 */

PeptideGUI::PeptideGUI( )
{
}

PeptideGUI::PeptideGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * PeptideGUI creator
 */

bool PeptideGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin PeptideGUI member initialisation
    p_buttonSizer = NULL;
////@end PeptideGUI member initialisation

////@begin PeptideGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end PeptideGUI creation
    return true;
}

/*!
 * Control creation for PeptideGUI
 */

void PeptideGUI::CreateControls()
{    
////@begin PeptideGUI content construction
    PeptideGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW|wxALL, 0);

    ewxTextCtrl* itemTextCtrl4 = new ewxTextCtrl( itemPanel1, ID_TEXTCTRL_PEPTIDE_SEQUENCE, _T(""), wxDefaultPosition, wxSize(-1, 50), wxTE_MULTILINE );
    if (ShowToolTips())
        itemTextCtrl4->SetToolTip(_("Type in sequences or use the buttons to add them."));
    itemBoxSizer3->Add(itemTextCtrl4, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    p_buttonSizer = new wxGridSizer(4, 7, 0, 0);
    itemBoxSizer2->Add(p_buttonSizer, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer6, 1, wxGROW|wxALL, 0);

    itemBoxSizer6->Add(20, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton8 = new ewxButton( itemPanel1, ID_BUTTON_PEPTIDE_CLEAR, _("Clear"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemBoxSizer6->Add(itemButton8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer6->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton10 = new ewxButton( itemPanel1, ID_BUTTON_PEPTIDE_GENERATE, _("Generate"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemBoxSizer6->Add(itemButton10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

////@end PeptideGUI content construction
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PEPTIDE_SEQUENCE
 */

void PeptideGUI::OnTextctrlPeptideSequenceUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PEPTIDE_SEQUENCE in PeptideGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PEPTIDE_SEQUENCE in PeptideGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PEPTIDE_SEQUENCE
 */

void PeptideGUI::OnTextctrlPeptideSequenceEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PEPTIDE_SEQUENCE in PeptideGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PEPTIDE_SEQUENCE in PeptideGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PEPTIDE_CLEAR
 */

void PeptideGUI::OnButtonPeptideClearClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PEPTIDE_CLEAR in PeptideGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PEPTIDE_CLEAR in PeptideGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PEPTIDE_GENERATE
 */

void PeptideGUI::OnButtonPeptideGenerateClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PEPTIDE_GENERATE in PeptideGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PEPTIDE_GENERATE in PeptideGUI. 
}

/*!
 * Should we show tooltips?
 */

bool PeptideGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap PeptideGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin PeptideGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end PeptideGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon PeptideGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin PeptideGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end PeptideGUI icon retrieval
}
