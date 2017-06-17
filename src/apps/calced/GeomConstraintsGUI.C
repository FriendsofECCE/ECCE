/////////////////////////////////////////////////////////////////////////////
// Name:        GeomConstraintsGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "GeomConstraintsGUI.H"
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

#include "GeomConstraintsGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID GeomConstraintsGUI::ID_STATICBOXSIZER7 = wxNewId();
const wxWindowID GeomConstraintsGUI::ID_GEOM_FIX_ATOM_BUTTON = wxNewId();
const wxWindowID GeomConstraintsGUI::ID_STATICBOXSIZER4 = wxNewId();
const wxWindowID GeomConstraintsGUI::ID_GEOM_RESTRAIN_BUTTON = wxNewId();
const wxWindowID GeomConstraintsGUI::ID_GEOM_CLEAR_ALL_BUTTON = wxNewId();
const wxWindowID GeomConstraintsGUI::ID_STATICBOXSIZER3 = wxNewId();
const wxWindowID GeomConstraintsGUI::ID_STATICBOXSIZER1 = wxNewId();
const wxWindowID GeomConstraintsGUI::ID_DIALOG = wxNewId();
const wxWindowID GeomConstraintsGUI::ID_GEOM_REMOVE_BUTTON = wxNewId();
const wxWindowID GeomConstraintsGUI::ID_STATICBOXSIZER2 = wxNewId();
const wxWindowID GeomConstraintsGUI::ID_STATICBOXSIZER6 = wxNewId();
const wxWindowID GeomConstraintsGUI::ID_STATICBOXSIZER5 = wxNewId();
const wxWindowID GeomConstraintsGUI::ID_GEOM_CONSTRAIN_BUTTON = wxNewId();

/*!
 * GeomConstraintsGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( GeomConstraintsGUI, ewxDialog )

/*!
 * GeomConstraintsGUI event table definition
 */

BEGIN_EVENT_TABLE( GeomConstraintsGUI, ewxDialog )

////@begin GeomConstraintsGUI event table entries
    EVT_CLOSE( GeomConstraintsGUI::OnCloseWindow )

    EVT_BUTTON( ID_GEOM_CONSTRAIN_BUTTON, GeomConstraintsGUI::OnConstrainButtonClick )

    EVT_BUTTON( ID_GEOM_RESTRAIN_BUTTON, GeomConstraintsGUI::OnRestrainButtonClick )

    EVT_BUTTON( ID_GEOM_FIX_ATOM_BUTTON, GeomConstraintsGUI::OnFixAtomsButtonClick )

    EVT_BUTTON( ID_GEOM_REMOVE_BUTTON, GeomConstraintsGUI::OnRemoveButtonClick )

    EVT_BUTTON( ID_GEOM_CLEAR_ALL_BUTTON, GeomConstraintsGUI::OnClearAllButtonClick )

    EVT_BUTTON( wxID_CLOSE, GeomConstraintsGUI::OnCloseButtonClick )

    EVT_BUTTON( wxID_HELP, GeomConstraintsGUI::OnHelpButtonClick )

////@end GeomConstraintsGUI event table entries

END_EVENT_TABLE()

/*!
 * GeomConstraintsGUI constructors
 */

GeomConstraintsGUI::GeomConstraintsGUI( )
{
}

GeomConstraintsGUI::GeomConstraintsGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * GeomConstraintsGUI creator
 */

bool GeomConstraintsGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin GeomConstraintsGUI member initialisation
    p_allSizers = NULL;
    p_bondConstraintSizer = NULL;
    p_angleConstraintSizer = NULL;
    p_torsionConstraintSizer = NULL;
    p_bondRestraintSizer = NULL;
    p_angleRestraintSizer = NULL;
    p_torsionRestraintSizer = NULL;
    p_fixedAtomSizer = NULL;
////@end GeomConstraintsGUI member initialisation

////@begin GeomConstraintsGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end GeomConstraintsGUI creation
    return true;
}

/*!
 * Control creation for GeomConstraintsGUI
 */

void GeomConstraintsGUI::CreateControls()
{    
////@begin GeomConstraintsGUI content construction
    GeomConstraintsGUI* itemDialog1 = this;

    p_allSizers = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(p_allSizers);

    wxStaticBox* itemStaticBoxSizer3Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER3, _("Bond Constraints"));
    p_bondConstraintSizer = new wxStaticBoxSizer(itemStaticBoxSizer3Static, wxVERTICAL);
    p_allSizers->Add(p_bondConstraintSizer, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer4Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER2, _("Angle Constraints"));
    p_angleConstraintSizer = new wxStaticBoxSizer(itemStaticBoxSizer4Static, wxVERTICAL);
    p_allSizers->Add(p_angleConstraintSizer, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer5Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER1, _("Torsion Constraints"));
    p_torsionConstraintSizer = new wxStaticBoxSizer(itemStaticBoxSizer5Static, wxVERTICAL);
    p_allSizers->Add(p_torsionConstraintSizer, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer6Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER6, _("Bond Restraints"));
    p_bondRestraintSizer = new wxStaticBoxSizer(itemStaticBoxSizer6Static, wxVERTICAL);
    p_allSizers->Add(p_bondRestraintSizer, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer7Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER5, _("Angle Restraints"));
    p_angleRestraintSizer = new wxStaticBoxSizer(itemStaticBoxSizer7Static, wxVERTICAL);
    p_allSizers->Add(p_angleRestraintSizer, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer8Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER4, _("Torsion Restraints"));
    p_torsionRestraintSizer = new wxStaticBoxSizer(itemStaticBoxSizer8Static, wxVERTICAL);
    p_allSizers->Add(p_torsionRestraintSizer, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer9Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER7, _("Fixed Atoms"));
    p_fixedAtomSizer = new wxStaticBoxSizer(itemStaticBoxSizer9Static, wxVERTICAL);
    p_allSizers->Add(p_fixedAtomSizer, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    p_allSizers->Add(itemBoxSizer10, 0, wxGROW, 5);

    ewxButton* itemButton11 = new ewxButton( itemDialog1, ID_GEOM_CONSTRAIN_BUTTON, _("Constrain"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton12 = new ewxButton( itemDialog1, ID_GEOM_RESTRAIN_BUTTON, _("Restrain"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton13 = new ewxButton( itemDialog1, ID_GEOM_FIX_ATOM_BUTTON, _("Fix Atoms"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton14 = new ewxButton( itemDialog1, ID_GEOM_REMOVE_BUTTON, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton15 = new ewxButton( itemDialog1, ID_GEOM_CLEAR_ALL_BUTTON, _("Clear All"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticLine* itemStaticLine16 = new ewxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    p_allSizers->Add(itemStaticLine16, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxHORIZONTAL);
    p_allSizers->Add(itemBoxSizer17, 0, wxGROW, 5);

    itemBoxSizer17->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL, 5);

    ewxButton* itemButton19 = new ewxButton( itemDialog1, wxID_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemButton19, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer17->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL, 5);

    ewxButton* itemButton21 = new ewxButton( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemButton21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer17->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL, 5);

////@end GeomConstraintsGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
 */

void GeomConstraintsGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_DIALOG in GeomConstraintsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_DIALOG in GeomConstraintsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CONSTRAIN_BUTTON
 */

void GeomConstraintsGUI::OnConstrainButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CONSTRAIN_BUTTON in GeomConstraintsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CONSTRAIN_BUTTON in GeomConstraintsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_RESTRAIN_BUTTON
 */

void GeomConstraintsGUI::OnRestrainButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_RESTRAIN_BUTTON in GeomConstraintsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_RESTRAIN_BUTTON in GeomConstraintsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_FIX_ATOM_BUTTON
 */

void GeomConstraintsGUI::OnFixAtomsButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_FIX_ATOM_BUTTON in GeomConstraintsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_FIX_ATOM_BUTTON in GeomConstraintsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_REMOVE_BUTTON
 */

void GeomConstraintsGUI::OnRemoveButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_REMOVE_BUTTON in GeomConstraintsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_REMOVE_BUTTON in GeomConstraintsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLEAR_ALL_BUTTON
 */

void GeomConstraintsGUI::OnClearAllButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLEAR_ALL_BUTTON in GeomConstraintsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GEOM_CLEAR_ALL_BUTTON in GeomConstraintsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void GeomConstraintsGUI::OnCloseButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in GeomConstraintsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in GeomConstraintsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void GeomConstraintsGUI::OnHelpButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP in GeomConstraintsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP in GeomConstraintsGUI. 
}

/*!
 * Should we show tooltips?
 */

bool GeomConstraintsGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap GeomConstraintsGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin GeomConstraintsGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end GeomConstraintsGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon GeomConstraintsGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin GeomConstraintsGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end GeomConstraintsGUI icon retrieval
}
