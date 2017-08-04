/////////////////////////////////////////////////////////////////////////////
// Name:        QMMMPotentialsGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "QMMMPotentialsGUI.H"
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
#include "wxgui/ewxCheckBox.H"
////@end includes

#include "QMMMPotentialsGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID QMMMPotentialsGUI::ID_ATMT_USE_SHAKE_CHECKBOX = wxNewId();
const wxWindowID QMMMPotentialsGUI::ID_STATICBOXSIZER3 = wxNewId();
const wxWindowID QMMMPotentialsGUI::ID_STATICBOXSIZER1 = wxNewId();
const wxWindowID QMMMPotentialsGUI::ID_DIALOG = wxNewId();
const wxWindowID QMMMPotentialsGUI::ID_STATICBOXSIZER2 = wxNewId();
const wxWindowID QMMMPotentialsGUI::ID_ATMT_CLEAR_ALL_BUTTON = wxNewId();
const wxWindowID QMMMPotentialsGUI::ID_STATICBOXSIZER6 = wxNewId();
const wxWindowID QMMMPotentialsGUI::ID_STATICBOXSIZER5 = wxNewId();

/*!
 * QMMMPotentialsGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( QMMMPotentialsGUI, ewxDialog )

/*!
 * QMMMPotentialsGUI event table definition
 */

BEGIN_EVENT_TABLE( QMMMPotentialsGUI, ewxDialog )

////@begin QMMMPotentialsGUI event table entries
    EVT_CLOSE( QMMMPotentialsGUI::OnCloseWindow )

    EVT_BUTTON( ID_ATMT_CLEAR_ALL_BUTTON, QMMMPotentialsGUI::OnClearAllButtonClick )

    EVT_CHECKBOX( ID_ATMT_USE_SHAKE_CHECKBOX, QMMMPotentialsGUI::OnAtmtUseShakeCheckboxClick )

    EVT_BUTTON( wxID_CLOSE, QMMMPotentialsGUI::OnCloseButtonClick )

    EVT_BUTTON( wxID_HELP, QMMMPotentialsGUI::OnHelpButtonClick )

////@end QMMMPotentialsGUI event table entries

END_EVENT_TABLE()

/*!
 * QMMMPotentialsGUI constructors
 */

QMMMPotentialsGUI::QMMMPotentialsGUI( )
{
}

QMMMPotentialsGUI::QMMMPotentialsGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * QMMMPotentialsGUI creator
 */

bool QMMMPotentialsGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin QMMMPotentialsGUI member initialisation
    p_allSizers = NULL;
    p_soluteAtomsSizer = NULL;
    p_solventAtomsSizer = NULL;
    p_solventPseudPotSizer = NULL;
    p_bondStretchSizer = NULL;
    p_angleBendSizer = NULL;
////@end QMMMPotentialsGUI member initialisation

////@begin QMMMPotentialsGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end QMMMPotentialsGUI creation
    return true;
}

/*!
 * Control creation for QMMMPotentialsGUI
 */

void QMMMPotentialsGUI::CreateControls()
{    
////@begin QMMMPotentialsGUI content construction
    QMMMPotentialsGUI* itemDialog1 = this;

    p_allSizers = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(p_allSizers);

    wxStaticBox* itemStaticBoxSizer3Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER3, _("Solute Atom Lennard-Jones Potentials"));
    p_soluteAtomsSizer = new wxStaticBoxSizer(itemStaticBoxSizer3Static, wxVERTICAL);
    p_allSizers->Add(p_soluteAtomsSizer, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer4Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER2, _("Solvent Atom Lennard-Jones Potentials"));
    p_solventAtomsSizer = new wxStaticBoxSizer(itemStaticBoxSizer4Static, wxVERTICAL);
    p_allSizers->Add(p_solventAtomsSizer, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer5Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER5, _("Solvent Atom Pseudo-Potentials"));
    p_solventPseudPotSizer = new wxStaticBoxSizer(itemStaticBoxSizer5Static, wxVERTICAL);
    p_allSizers->Add(p_solventPseudPotSizer, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer6Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER1, _("Bond Stretch Potentials"));
    p_bondStretchSizer = new wxStaticBoxSizer(itemStaticBoxSizer6Static, wxVERTICAL);
    p_allSizers->Add(p_bondStretchSizer, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer7Static = new wxStaticBox(itemDialog1, ID_STATICBOXSIZER6, _("Angle Bend Potentials"));
    p_angleBendSizer = new wxStaticBoxSizer(itemStaticBoxSizer7Static, wxVERTICAL);
    p_allSizers->Add(p_angleBendSizer, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    p_allSizers->Add(itemBoxSizer8, 0, wxGROW, 5);

    ewxButton* itemButton9 = new ewxButton( itemDialog1, ID_ATMT_CLEAR_ALL_BUTTON, _("Clear All"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemButton9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer8->Add(itemBoxSizer10, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxCheckBox* itemCheckBox11 = new ewxCheckBox( itemDialog1, ID_ATMT_USE_SHAKE_CHECKBOX, _("Use Shake Constraints"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox11->SetValue(false);
    itemBoxSizer8->Add(itemCheckBox11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticLine* itemStaticLine12 = new ewxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    p_allSizers->Add(itemStaticLine12, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    p_allSizers->Add(itemBoxSizer13, 0, wxGROW, 5);

    itemBoxSizer13->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL, 5);

    ewxButton* itemButton15 = new ewxButton( itemDialog1, wxID_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemButton15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer13->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL, 5);

    ewxButton* itemButton17 = new ewxButton( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemButton17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer13->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL, 5);

////@end QMMMPotentialsGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
 */

void QMMMPotentialsGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_DIALOG in QMMMPotentialsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_DIALOG in QMMMPotentialsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATMT_CLEAR_ALL_BUTTON
 */

void QMMMPotentialsGUI::OnClearAllButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATMT_CLEAR_ALL_BUTTON in QMMMPotentialsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATMT_CLEAR_ALL_BUTTON in QMMMPotentialsGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_ATMT_USE_SHAKE_CHECKBOX
 */

void QMMMPotentialsGUI::OnAtmtUseShakeCheckboxClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_ATMT_USE_SHAKE_CHECKBOX in QMMMPotentialsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_ATMT_USE_SHAKE_CHECKBOX in QMMMPotentialsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void QMMMPotentialsGUI::OnCloseButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in QMMMPotentialsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in QMMMPotentialsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void QMMMPotentialsGUI::OnHelpButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP in QMMMPotentialsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP in QMMMPotentialsGUI. 
}

/*!
 * Should we show tooltips?
 */

bool QMMMPotentialsGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap QMMMPotentialsGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin QMMMPotentialsGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end QMMMPotentialsGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon QMMMPotentialsGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin QMMMPotentialsGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end QMMMPotentialsGUI icon retrieval
}
