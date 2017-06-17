/////////////////////////////////////////////////////////////////////////////
// Name:        MetaPotentialsGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "MetaPotentialsGUI.H"
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
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxButton.H"
////@end includes

#include "MetaPotentialsGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID MetaPotentialsGUI::ID_DIALOG = wxNewId();
const wxWindowID MetaPotentialsGUI::ID_GEOM_REMOVE_BUTTON = wxNewId();
const wxWindowID MetaPotentialsGUI::ID_STATICBOXSIZER2 = wxNewId();
const wxWindowID MetaPotentialsGUI::ID_METADYNAMICS_POTENTIAL_BUTTON = wxNewId();
const wxWindowID MetaPotentialsGUI::ID_GEOM_CLEAR_ALL_BUTTON = wxNewId();
const wxWindowID MetaPotentialsGUI::ID_STATICBOXSIZER3 = wxNewId();

/*!
 * MetaPotentialsGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( MetaPotentialsGUI, ewxDialog )

/*!
 * MetaPotentialsGUI event table definition
 */

BEGIN_EVENT_TABLE( MetaPotentialsGUI, ewxDialog )

////@begin MetaPotentialsGUI event table entries
    EVT_CLOSE( MetaPotentialsGUI::OnCloseWindow )

    EVT_BUTTON( ID_METADYNAMICS_POTENTIAL_BUTTON, MetaPotentialsGUI::OnPotentialButtonClick )

    EVT_BUTTON( ID_GEOM_REMOVE_BUTTON, MetaPotentialsGUI::OnRemoveButtonClick )

    EVT_BUTTON( ID_GEOM_CLEAR_ALL_BUTTON, MetaPotentialsGUI::OnClearAllButtonClick )

    EVT_BUTTON( wxID_CLOSE, MetaPotentialsGUI::OnCloseButtonClick )

    EVT_BUTTON( wxID_HELP, MetaPotentialsGUI::OnHelpButtonClick )

////@end MetaPotentialsGUI event table entries

END_EVENT_TABLE()

/*!
 * MetaPotentialsGUI constructors
 */

MetaPotentialsGUI::MetaPotentialsGUI( )
{
}

MetaPotentialsGUI::MetaPotentialsGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * MetaPotentialsGUI creator
 */

bool MetaPotentialsGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin MetaPotentialsGUI member initialisation
    p_allSizers = NULL;
    p_bondPotentialSizer = NULL;
    p_anglePotentialSizer = NULL;
////@end MetaPotentialsGUI member initialisation

////@begin MetaPotentialsGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end MetaPotentialsGUI creation
    return true;
}

/*!
 * Control creation for MetaPotentialsGUI
 */

void MetaPotentialsGUI::CreateControls()
{    
////@begin MetaPotentialsGUI content construction
    MetaPotentialsGUI* itemDialog1 = this;

    p_allSizers = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(p_allSizers);

    wxStaticBox* itemStaticBoxSizer3Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER3, _("Bond Metadynamics Potentials"));
    p_bondPotentialSizer = new wxStaticBoxSizer(itemStaticBoxSizer3Static, wxVERTICAL);
    p_allSizers->Add(p_bondPotentialSizer, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer4Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER2, _("Angle Metadynamics Potentials"));
    p_anglePotentialSizer = new wxStaticBoxSizer(itemStaticBoxSizer4Static, wxVERTICAL);
    p_allSizers->Add(p_anglePotentialSizer, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    p_allSizers->Add(itemBoxSizer5, 0, wxGROW, 5);

    ewxButton* itemButton6 = new ewxButton( itemDialog1, ID_METADYNAMICS_POTENTIAL_BUTTON, _("Add New Potential"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemButton6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton7 = new ewxButton( itemDialog1, ID_GEOM_REMOVE_BUTTON, _("Remove Selected Potential"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemButton7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton8 = new ewxButton( itemDialog1, ID_GEOM_CLEAR_ALL_BUTTON, _("Clear All Potentials"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemButton8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticLine* itemStaticLine9 = new ewxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    p_allSizers->Add(itemStaticLine9, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    p_allSizers->Add(itemBoxSizer10, 0, wxGROW, 5);

    itemBoxSizer10->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL, 5);

    ewxButton* itemButton12 = new ewxButton( itemDialog1, wxID_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer10->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL, 5);

    ewxButton* itemButton14 = new ewxButton( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer10->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL, 5);

////@end MetaPotentialsGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
 */

void MetaPotentialsGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_DIALOG in MetaPotentialsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_DIALOG in MetaPotentialsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_METADYNAMICS_POTENTIAL_BUTTON
 */

void MetaPotentialsGUI::OnPotentialButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_METADYNAMICS_POTENTIAL_BUTTON in MetaPotentialsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_METADYNAMICS_POTENTIAL_BUTTON in MetaPotentialsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_REMOVE_BUTTON
 */

void MetaPotentialsGUI::OnRemoveButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_REMOVE_BUTTON in MetaPotentialsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_REMOVE_BUTTON in MetaPotentialsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLEAR_ALL_BUTTON
 */

void MetaPotentialsGUI::OnClearAllButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLEAR_ALL_BUTTON in MetaPotentialsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLEAR_ALL_BUTTON in MetaPotentialsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void MetaPotentialsGUI::OnCloseButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in MetaPotentialsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in MetaPotentialsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void MetaPotentialsGUI::OnHelpButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP in MetaPotentialsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP in MetaPotentialsGUI. 
}

/*!
 * Should we show tooltips?
 */

bool MetaPotentialsGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap MetaPotentialsGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin MetaPotentialsGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end MetaPotentialsGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon MetaPotentialsGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin MetaPotentialsGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end MetaPotentialsGUI icon retrieval
}
