/////////////////////////////////////////////////////////////////////////////
// Name:        DnaGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "DnaGUI.H"
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
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "DnaGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID DnaGUI::ID_BUTTON_DNA_GENERATE = wxNewId();
const wxWindowID DnaGUI::ID_BUTTON_DNA_TA = wxNewId();
const wxWindowID DnaGUI::ID_TEXTCTRL_DNA_SEQUENCE = wxNewId();
const wxWindowID DnaGUI::ID_DIALOG_DNA = wxNewId();
const wxWindowID DnaGUI::ID_BUTTON_DNA_AT = wxNewId();
const wxWindowID DnaGUI::ID_BUTTON_DNA_CG = wxNewId();
const wxWindowID DnaGUI::ID_BUTTON_DNA_GC = wxNewId();

/*!
 * DnaGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( DnaGUI, ewxPanel )

/*!
 * DnaGUI event table definition
 */

BEGIN_EVENT_TABLE( DnaGUI, ewxPanel )

////@begin DnaGUI event table entries
    EVT_TEXT( ID_TEXTCTRL_DNA_SEQUENCE, DnaGUI::OnTextctrlDnaSequenceUpdated )
    EVT_TEXT_ENTER( ID_TEXTCTRL_DNA_SEQUENCE, DnaGUI::OnTextctrlDnaSequenceEnter )

    EVT_BUTTON( ID_BUTTON_DNA_AT, DnaGUI::OnButtonDnaAtClick )

    EVT_BUTTON( ID_BUTTON_DNA_TA, DnaGUI::OnButtonDnaTaClick )

    EVT_BUTTON( ID_BUTTON_DNA_GC, DnaGUI::OnButtonDnaGcClick )

    EVT_BUTTON( ID_BUTTON_DNA_CG, DnaGUI::OnButtonDnaCgClick )

    EVT_BUTTON( ID_BUTTON_DNA_GENERATE, DnaGUI::OnButtonDnaGenerateClick )

////@end DnaGUI event table entries

END_EVENT_TABLE()

/*!
 * DnaGUI constructors
 */

DnaGUI::DnaGUI( )
{
}

DnaGUI::DnaGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * DnaGUI creator
 */

bool DnaGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin DnaGUI member initialisation
////@end DnaGUI member initialisation

////@begin DnaGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end DnaGUI creation
    return true;
}

/*!
 * Control creation for DnaGUI
 */

void DnaGUI::CreateControls()
{    
////@begin DnaGUI content construction
    DnaGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    ewxTextCtrl* itemTextCtrl3 = new ewxTextCtrl( itemPanel1, ID_TEXTCTRL_DNA_SEQUENCE, _T(""), wxDefaultPosition, wxSize(-1, 50), wxTE_MULTILINE );
    if (ShowToolTips())
        itemTextCtrl3->SetToolTip(_("Type in sequences or use the buttons to add them."));
    itemBoxSizer2->Add(itemTextCtrl3, 1, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer4, 0, wxGROW|wxALL, 2);

    ewxStaticText* itemStaticText5 = new ewxStaticText( itemPanel1, wxID_STATIC, _("+"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemStaticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxButton* itemButton6 = new ewxButton( itemPanel1, ID_BUTTON_DNA_AT, _("A"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton6->SetToolTip(_("Add A-T pair to DNA strand"));
    itemBoxSizer4->Add(itemButton6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxButton* itemButton7 = new ewxButton( itemPanel1, ID_BUTTON_DNA_TA, _("T"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton7->SetToolTip(_("Add T-A pair to DNA strand"));
    itemBoxSizer4->Add(itemButton7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxButton* itemButton8 = new ewxButton( itemPanel1, ID_BUTTON_DNA_GC, _("G"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton8->SetToolTip(_("Add G-C pair to DNA strand"));
    itemBoxSizer4->Add(itemButton8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxButton* itemButton9 = new ewxButton( itemPanel1, ID_BUTTON_DNA_CG, _("C"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton9->SetToolTip(_("Add C-G pair to DNA strand"));
    itemBoxSizer4->Add(itemButton9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 1);

    itemBoxSizer4->Add(20, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton11 = new ewxButton( itemPanel1, ID_BUTTON_DNA_GENERATE, _("Generate"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton11->SetToolTip(_("Generate DNA fragment from string"));
    itemBoxSizer4->Add(itemButton11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 1);

////@end DnaGUI content construction
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_DNA_SEQUENCE
 */

void DnaGUI::OnTextctrlDnaSequenceUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_DNA_SEQUENCE in DnaGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_DNA_SEQUENCE in DnaGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DNA_SEQUENCE
 */

void DnaGUI::OnTextctrlDnaSequenceEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DNA_SEQUENCE in DnaGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DNA_SEQUENCE in DnaGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_AT
 */

void DnaGUI::OnButtonDnaAtClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_AT in DnaGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_AT in DnaGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_TA
 */

void DnaGUI::OnButtonDnaTaClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_TA in DnaGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_TA in DnaGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_GC
 */

void DnaGUI::OnButtonDnaGcClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_GC in DnaGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_GC in DnaGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_CG
 */

void DnaGUI::OnButtonDnaCgClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_CG in DnaGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_CG in DnaGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_GENERATE
 */

void DnaGUI::OnButtonDnaGenerateClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_GENERATE in DnaGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_GENERATE in DnaGUI. 
}

/*!
 * Should we show tooltips?
 */

bool DnaGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap DnaGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin DnaGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end DnaGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon DnaGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin DnaGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end DnaGUI icon retrieval
}
