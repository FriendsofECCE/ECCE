/////////////////////////////////////////////////////////////////////////////
// Name:        PartialChargeGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "PartialChargeGUI.H"
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
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "PartialChargeGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID PartialChargeGUI::ID_TEXTCTRL_GROUP2 = wxNewId();
const wxWindowID PartialChargeGUI::ID_BUTTON_CONSTRAIN_ALL_METHYL_GROUPS = wxNewId();
const wxWindowID PartialChargeGUI::ID_BUTTON_SHOW_MULTIPLY_CONSTRAINED_ATOMS = wxNewId();
const wxWindowID PartialChargeGUI::ID_BUTTON_GROUP1 = wxNewId();
const wxWindowID PartialChargeGUI::ID_DIALOG = wxNewId();
const wxWindowID PartialChargeGUI::ID_LISTBOX_CMDLIST = wxNewId();
const wxWindowID PartialChargeGUI::ID_GROUP2 = wxNewId();
const wxWindowID PartialChargeGUI::ID_TEXTCTRL_FIX_VALUE = wxNewId();
const wxWindowID PartialChargeGUI::ID_BUTTON_DELETE_CMD = wxNewId();
const wxWindowID PartialChargeGUI::ID_TOTAL_CHARGE = wxNewId();
const wxWindowID PartialChargeGUI::ID_BUTTON_CONSTRAIN_ALL_HYDROGENS = wxNewId();
const wxWindowID PartialChargeGUI::ID_BUTTON_SHOW_UNCONSTRAINED_ATOMS = wxNewId();
const wxWindowID PartialChargeGUI::ID_CHOICE_CONSTRAINTS = wxNewId();
const wxWindowID PartialChargeGUI::ID_GROUP1 = wxNewId();
const wxWindowID PartialChargeGUI::ID_FIXED_CHARGE = wxNewId();
const wxWindowID PartialChargeGUI::ID_TEXTCTRL_GROUP1 = wxNewId();
const wxWindowID PartialChargeGUI::ID_BUTTON_ADD_CMD = wxNewId();
const wxWindowID PartialChargeGUI::ID_BUTTON_GROUP2 = wxNewId();
const wxWindowID PartialChargeGUI::ID_TEXTCTRL_TOTAL_CHRG = wxNewId();

/*!
 * PartialChargeGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( PartialChargeGUI, ewxDialog )

/*!
 * PartialChargeGUI event table definition
 */

BEGIN_EVENT_TABLE( PartialChargeGUI, ewxDialog )

////@begin PartialChargeGUI event table entries
    EVT_BUTTON( ID_BUTTON_CONSTRAIN_ALL_METHYL_GROUPS, PartialChargeGUI::OnButtonConstrainAllMethylGroupsClick )

    EVT_BUTTON( ID_BUTTON_CONSTRAIN_ALL_HYDROGENS, PartialChargeGUI::OnButtonConstrainAllHydrogensClick )

    EVT_CHOICE( ID_CHOICE_CONSTRAINTS, PartialChargeGUI::OnChoiceConstraintsSelected )

    EVT_BUTTON( ID_BUTTON_GROUP1, PartialChargeGUI::OnButtonGroup1Click )

    EVT_BUTTON( ID_BUTTON_GROUP2, PartialChargeGUI::OnButtonGroup2Click )

    EVT_BUTTON( ID_BUTTON_ADD_CMD, PartialChargeGUI::OnButtonAddCmdClick )

    EVT_BUTTON( ID_BUTTON_DELETE_CMD, PartialChargeGUI::OnButtonDeleteCmdClick )

    EVT_LISTBOX( ID_LISTBOX_CMDLIST, PartialChargeGUI::OnListboxCmdlistSelected )

    EVT_BUTTON( ID_BUTTON_SHOW_UNCONSTRAINED_ATOMS, PartialChargeGUI::OnButtonShowUnconstrainedAtomsClick )

    EVT_BUTTON( ID_BUTTON_SHOW_MULTIPLY_CONSTRAINED_ATOMS, PartialChargeGUI::OnButtonShowMultiplyConstrainedAtomsClick )

    EVT_BUTTON( wxID_CLOSE, PartialChargeGUI::OnCloseClick )

    EVT_BUTTON( wxID_HELP, PartialChargeGUI::OnHelpClick )

////@end PartialChargeGUI event table entries

END_EVENT_TABLE()

/*!
 * PartialChargeGUI constructors
 */

PartialChargeGUI::PartialChargeGUI( )
{
}

PartialChargeGUI::PartialChargeGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * PartialChargeGUI creator
 */

bool PartialChargeGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin PartialChargeGUI member initialisation
////@end PartialChargeGUI member initialisation

////@begin PartialChargeGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end PartialChargeGUI creation
    return true;
}

/*!
 * Control creation for PartialChargeGUI
 */

void PartialChargeGUI::CreateControls()
{    
////@begin PartialChargeGUI content construction
    PartialChargeGUI* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW, 0);

    ewxStaticLine* itemStaticLine4 = new ewxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxSize(30, -1), wxLI_HORIZONTAL );
    itemBoxSizer3->Add(itemStaticLine4, 0, wxGROW|wxALL, 5);

    ewxStaticText* itemStaticText5 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Group Command"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine6 = new ewxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer3->Add(itemStaticLine6, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer7, 0, wxGROW, 0);

    ewxButton* itemButton8 = new ewxButton( itemDialog1, ID_BUTTON_CONSTRAIN_ALL_METHYL_GROUPS, _("Constrain All Methyl Groups"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemButton8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton9 = new ewxButton( itemDialog1, ID_BUTTON_CONSTRAIN_ALL_HYDROGENS, _("Constrain All Hydrogens"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemButton9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer10, 0, wxGROW, 0);

    ewxStaticLine* itemStaticLine11 = new ewxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxSize(30, -1), wxLI_HORIZONTAL );
    itemBoxSizer10->Add(itemStaticLine11, 0, wxGROW|wxALL, 5);

    ewxStaticText* itemStaticText12 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Set Command"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemStaticText12, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine13 = new ewxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer10->Add(itemStaticLine13, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer14, 0, wxALIGN_LEFT, 0);

    wxString itemChoice15Strings[] = {
        _("Fix Atom Charge"),
        _("All Charges Equal"),
        _("Total Charge Fixed"),
        _("Constrain Group 1 to 2"),
        _("Constrain Methyl Group"),
        _("Methyl-Type Constraint")
    };
    ewxChoice* itemChoice15 = new ewxChoice( itemDialog1, ID_CHOICE_CONSTRAINTS, wxDefaultPosition, wxDefaultSize, 6, itemChoice15Strings, 0 );
    itemChoice15->SetStringSelection(_("Fix Atom Charge"));
    itemBoxSizer14->Add(itemChoice15, 0, wxALIGN_TOP|wxALL, 5);

    ewxPanel* itemPanel16 = new ewxPanel( itemDialog1, ID_FIXED_CHARGE, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer2->Add(itemPanel16, 0, wxALIGN_LEFT, 0);

    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel16->SetSizer(itemBoxSizer17);

    ewxStaticText* itemStaticText18 = new ewxStaticText( itemPanel16, wxID_STATIC, _("Fixed Atom Charge Value:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemStaticText18, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl19 = new ewxTextCtrl( itemPanel16, ID_TEXTCTRL_FIX_VALUE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemTextCtrl19, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxPanel* itemPanel20 = new ewxPanel( itemDialog1, ID_TOTAL_CHARGE, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer2->Add(itemPanel20, 0, wxALIGN_LEFT, 0);

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel20->SetSizer(itemBoxSizer21);

    ewxStaticText* itemStaticText22 = new ewxStaticText( itemPanel20, wxID_STATIC, _("Total Fixed Charge Value:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(itemStaticText22, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl23 = new ewxTextCtrl( itemPanel20, ID_TEXTCTRL_TOTAL_CHRG, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(itemTextCtrl23, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxPanel* itemPanel24 = new ewxPanel( itemDialog1, ID_GROUP1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer2->Add(itemPanel24, 0, wxGROW, 0);

    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel24->SetSizer(itemBoxSizer25);

    ewxButton* itemButton26 = new ewxButton( itemPanel24, ID_BUTTON_GROUP1, _("Populate Group 1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer25->Add(itemButton26, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxTextCtrl* itemTextCtrl27 = new ewxTextCtrl( itemPanel24, ID_TEXTCTRL_GROUP1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer25->Add(itemTextCtrl27, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxPanel* itemPanel28 = new ewxPanel( itemDialog1, ID_GROUP2, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    itemBoxSizer2->Add(itemPanel28, 0, wxGROW, 0);

    wxBoxSizer* itemBoxSizer29 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel28->SetSizer(itemBoxSizer29);

    ewxButton* itemButton30 = new ewxButton( itemPanel28, ID_BUTTON_GROUP2, _("Populate Group 2"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer29->Add(itemButton30, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxTextCtrl* itemTextCtrl31 = new ewxTextCtrl( itemPanel28, ID_TEXTCTRL_GROUP2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer29->Add(itemTextCtrl31, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer32 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer32, 0, wxALIGN_LEFT, 0);

    ewxStaticText* itemStaticText33 = new ewxStaticText( itemDialog1, wxID_STATIC, _("ESP Command List:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(itemStaticText33, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer34 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer34, 1, wxGROW, 0);

    wxBoxSizer* itemBoxSizer35 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer34->Add(itemBoxSizer35, 0, wxALIGN_TOP|wxLEFT, 5);

    ewxButton* itemButton36 = new ewxButton( itemDialog1, ID_BUTTON_ADD_CMD, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer35->Add(itemButton36, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxButton* itemButton37 = new ewxButton( itemDialog1, ID_BUTTON_DELETE_CMD, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer35->Add(itemButton37, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxString* itemListBox38Strings = NULL;
    ewxListBox* itemListBox38 = new ewxListBox( itemDialog1, ID_LISTBOX_CMDLIST, wxDefaultPosition, wxSize(-1, 100), 0, itemListBox38Strings, wxLB_EXTENDED|wxLB_NEEDED_SB );
    itemBoxSizer34->Add(itemListBox38, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer39 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer39, 0, wxGROW, 5);

    ewxButton* itemButton40 = new ewxButton( itemDialog1, ID_BUTTON_SHOW_UNCONSTRAINED_ATOMS, _("Show Unconstrained Atoms"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer39->Add(itemButton40, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton41 = new ewxButton( itemDialog1, ID_BUTTON_SHOW_MULTIPLY_CONSTRAINED_ATOMS, _("Show Multiply Constrained Atoms"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer39->Add(itemButton41, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticLine* itemStaticLine42 = new ewxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer2->Add(itemStaticLine42, 0, wxGROW, 5);

    wxBoxSizer* itemBoxSizer43 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer43, 0, wxGROW, 5);

    itemBoxSizer43->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton45 = new ewxButton( itemDialog1, wxID_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer43->Add(itemButton45, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer43->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton47 = new ewxButton( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer43->Add(itemButton47, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer43->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end PartialChargeGUI content construction
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONSTRAIN_ALL_METHYL_GROUPS
 */

void PartialChargeGUI::OnButtonConstrainAllMethylGroupsClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONSTRAIN_ALL_METHYL_GROUPS in PartialChargeGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONSTRAIN_ALL_METHYL_GROUPS in PartialChargeGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONSTRAIN_ALL_HYDROGENS
 */

void PartialChargeGUI::OnButtonConstrainAllHydrogensClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONSTRAIN_ALL_HYDROGENS in PartialChargeGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONSTRAIN_ALL_HYDROGENS in PartialChargeGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONSTRAINTS
 */

void PartialChargeGUI::OnChoiceConstraintsSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONSTRAINTS in PartialChargeGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONSTRAINTS in PartialChargeGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_GROUP1
 */

void PartialChargeGUI::OnButtonGroup1Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_GROUP1 in PartialChargeGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_GROUP1 in PartialChargeGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_GROUP2
 */

void PartialChargeGUI::OnButtonGroup2Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_GROUP2 in PartialChargeGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_GROUP2 in PartialChargeGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ADD_CMD
 */

void PartialChargeGUI::OnButtonAddCmdClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ADD_CMD in PartialChargeGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ADD_CMD in PartialChargeGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DELETE_CMD
 */

void PartialChargeGUI::OnButtonDeleteCmdClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DELETE_CMD in PartialChargeGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DELETE_CMD in PartialChargeGUI. 
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_CMDLIST
 */

void PartialChargeGUI::OnListboxCmdlistSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_CMDLIST in PartialChargeGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_CMDLIST in PartialChargeGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SHOW_UNCONSTRAINED_ATOMS
 */

void PartialChargeGUI::OnButtonShowUnconstrainedAtomsClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SHOW_UNCONSTRAINED_ATOMS in PartialChargeGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SHOW_UNCONSTRAINED_ATOMS in PartialChargeGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SHOW_MULTIPLY_CONSTRAINED_ATOMS
 */

void PartialChargeGUI::OnButtonShowMultiplyConstrainedAtomsClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SHOW_MULTIPLY_CONSTRAINED_ATOMS in PartialChargeGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SHOW_MULTIPLY_CONSTRAINED_ATOMS in PartialChargeGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void PartialChargeGUI::OnCloseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in PartialChargeGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in PartialChargeGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void PartialChargeGUI::OnHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP in PartialChargeGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP in PartialChargeGUI. 
}

/*!
 * Should we show tooltips?
 */

bool PartialChargeGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap PartialChargeGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin PartialChargeGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end PartialChargeGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon PartialChargeGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin PartialChargeGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end PartialChargeGUI icon retrieval
}
