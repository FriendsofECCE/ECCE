/////////////////////////////////////////////////////////////////////////////
// Name:        SymmetryPanelGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "SymmetryPanelGUI.H"
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

#include "SymmetryPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID SymmetryPanelGUI::ID_TEXTCTRL_SYMM_THRESHOLD = wxNewId();
const wxWindowID SymmetryPanelGUI::ID_BUTTON_SYMM_FIND_IRREDUCIBLE = wxNewId();
const wxWindowID SymmetryPanelGUI::ID_DIALOG_SYMMETRY = wxNewId();
const wxWindowID SymmetryPanelGUI::ID_BUTTON_SYMM_GENERATE_MOLECULE = wxNewId();
const wxWindowID SymmetryPanelGUI::ID_BUTTON_DEL_GHOSTS = wxNewId();
const wxWindowID SymmetryPanelGUI::ID_STATIC_SYMM_THRESHOLD_UNIT = wxNewId();
const wxWindowID SymmetryPanelGUI::ID_BUTTON_SYMM_FIND = wxNewId();
const wxWindowID SymmetryPanelGUI::ID_BUTTON_SYMM_GENERATE_GHOSTS = wxNewId();
const wxWindowID SymmetryPanelGUI::ID_BUTTON_SYMM_CLEAN = wxNewId();

/*!
 * SymmetryPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( SymmetryPanelGUI, ewxPanel )

/*!
 * SymmetryPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( SymmetryPanelGUI, ewxPanel )

////@begin SymmetryPanelGUI event table entries
    EVT_BUTTON( ID_BUTTON_SYMM_FIND, SymmetryPanelGUI::OnButtonSymmFindClick )

    EVT_BUTTON( ID_BUTTON_SYMM_CLEAN, SymmetryPanelGUI::OnButtonSymmCleanClick )

    EVT_BUTTON( ID_BUTTON_SYMM_FIND_IRREDUCIBLE, SymmetryPanelGUI::OnButtonSymmFindIrreducibleClick )

    EVT_BUTTON( ID_BUTTON_SYMM_GENERATE_GHOSTS, SymmetryPanelGUI::OnButtonSymmGenerateGhostsClick )

    EVT_BUTTON( ID_BUTTON_DEL_GHOSTS, SymmetryPanelGUI::OnButtonDelGhostsClick )

    EVT_BUTTON( ID_BUTTON_SYMM_GENERATE_MOLECULE, SymmetryPanelGUI::OnButtonSymmGenerateMoleculeClick )

////@end SymmetryPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * SymmetryPanelGUI constructors
 */

SymmetryPanelGUI::SymmetryPanelGUI( )
{
}

SymmetryPanelGUI::SymmetryPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * SymmetryPanelGUI creator
 */

bool SymmetryPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin SymmetryPanelGUI member initialisation
    p_topSizer = NULL;
////@end SymmetryPanelGUI member initialisation

////@begin SymmetryPanelGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end SymmetryPanelGUI creation
    return true;
}

/*!
 * Control creation for SymmetryPanelGUI
 */

void SymmetryPanelGUI::CreateControls()
{    
////@begin SymmetryPanelGUI content construction
    SymmetryPanelGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    p_topSizer = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(p_topSizer, 0, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText4 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Symmetry:"), wxDefaultPosition, wxDefaultSize, 0 );
    p_topSizer->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxButton* itemButton5 = new ewxButton( itemPanel1, ID_BUTTON_SYMM_FIND, _("Find "), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton5->SetToolTip(_("Find symmetry group automatically"));
    p_topSizer->Add(itemButton5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    p_topSizer->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton7 = new ewxButton( itemPanel1, ID_BUTTON_SYMM_CLEAN, _("Sym. Clean"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton7->SetToolTip(_("Adjust atom positions so that molecule is exactly symmetric, within numerical accuracy"));
    p_topSizer->Add(itemButton7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer8, 0, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Threshold:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemStaticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxTextCtrl* itemTextCtrl10 = new ewxTextCtrl( itemPanel1, ID_TEXTCTRL_SYMM_THRESHOLD, _("0.01"), wxDefaultPosition, wxSize(50, -1), 0 );
    if (ShowToolTips())
        itemTextCtrl10->SetToolTip(_("Threshold (in angstroms) for symmetry search"));
    itemBoxSizer8->Add(itemTextCtrl10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxStaticText* itemStaticText11 = new ewxStaticText( itemPanel1, ID_STATIC_SYMM_THRESHOLD_UNIT, _("Angstrom"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText11->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemBoxSizer8->Add(itemStaticText11, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 2);

    itemBoxSizer8->Add(1, 1, 1, wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxButton* itemButton13 = new ewxButton( itemPanel1, ID_BUTTON_SYMM_FIND_IRREDUCIBLE, _("Find Irreduc. Frag."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton13->SetToolTip(_("Generate a fragment containing only the symmetry unique atoms"));
    itemBoxSizer8->Add(itemButton13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer14, 0, wxGROW|wxBOTTOM, 5);

    ewxButton* itemButton15 = new ewxButton( itemPanel1, ID_BUTTON_SYMM_GENERATE_GHOSTS, _("Gen. Ghosts"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton15->SetToolTip(_("Generate ghost atoms with point group symmetry"));
    itemBoxSizer14->Add(itemButton15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    itemBoxSizer14->Add(5, 1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxButton* itemButton17 = new ewxButton( itemPanel1, ID_BUTTON_DEL_GHOSTS, _("Del. Ghosts"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton17->SetToolTip(_("Delete ghost atoms from builder"));
    itemBoxSizer14->Add(itemButton17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    itemBoxSizer14->Add(1, 1, 1, wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxButton* itemButton19 = new ewxButton( itemPanel1, ID_BUTTON_SYMM_GENERATE_MOLECULE, _("Gen. Molecule"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton19->SetToolTip(_("Generate full molecule from irreducible fragment using symmetry group operations"));
    itemBoxSizer14->Add(itemButton19, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

////@end SymmetryPanelGUI content construction
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_FIND
 */

void SymmetryPanelGUI::OnButtonSymmFindClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_FIND in SymmetryPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_FIND in SymmetryPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_CLEAN
 */

void SymmetryPanelGUI::OnButtonSymmCleanClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_CLEAN in SymmetryPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_CLEAN in SymmetryPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_FIND_IRREDUCIBLE
 */

void SymmetryPanelGUI::OnButtonSymmFindIrreducibleClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_FIND_IRREDUCIBLE in SymmetryPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_FIND_IRREDUCIBLE in SymmetryPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_GENERATE_GHOSTS
 */

void SymmetryPanelGUI::OnButtonSymmGenerateGhostsClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_GENERATE_GHOSTS in SymmetryPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_GENERATE_GHOSTS in SymmetryPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DEL_GHOSTS
 */

void SymmetryPanelGUI::OnButtonDelGhostsClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DEL_GHOSTS in SymmetryPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DEL_GHOSTS in SymmetryPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_GENERATE_MOLECULE
 */

void SymmetryPanelGUI::OnButtonSymmGenerateMoleculeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_GENERATE_MOLECULE in SymmetryPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_GENERATE_MOLECULE in SymmetryPanelGUI. 
}

/*!
 * Should we show tooltips?
 */

bool SymmetryPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap SymmetryPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin SymmetryPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end SymmetryPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon SymmetryPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin SymmetryPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end SymmetryPanelGUI icon retrieval
}
