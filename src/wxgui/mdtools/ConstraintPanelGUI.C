/////////////////////////////////////////////////////////////////////////////
// Name:        ConstraintPanelGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ConstraintPanelGUI.H"
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
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxgui/ConstraintPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID ConstraintPanelGUI::ID_TEXTCTRL_CONST_SLUTOL = wxNewId();
const wxWindowID ConstraintPanelGUI::wxID_STATIC_CONST_SLVTOL_UNITS = wxNewId();
const wxWindowID ConstraintPanelGUI::wxID_STATIC_CONST_RADIUS = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_CHECKBOX_CONST_PERM = wxNewId();
const wxWindowID ConstraintPanelGUI::wxID_STATIC_CONST_MAXRES = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_TEXTCTRL_CONST_ATCENT = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_CHOICE_CONST_FIXOPT = wxNewId();
const wxWindowID ConstraintPanelGUI::wxID_STATIC_CONST_RADIUS_UNITS = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_PANEL_CONST_SHAKE = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_PANEL_CONST_RADIUS = wxNewId();
const wxWindowID ConstraintPanelGUI::wxID_STATIC_CONST_SLVITER = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_BUTTON_CONST_FIXDEL = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_TEXTCTRL_CONST_SLUITER = wxNewId();
const wxWindowID ConstraintPanelGUI::wxID_STATIC_CONST_SLUTOL_UNITS = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_TEXTCTRL_CONST_SLVTOL = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_BUTTON_CONST_CLEAR = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_RADIOBUTTON_CONST_RADIN = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_TEXTCTRL_CONST_MINRES = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_CHECKBOX_CONST_USESHK = wxNewId();
const wxWindowID ConstraintPanelGUI::wxID_STATIC_CONST_SLUITER = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_BUTTON_CONST_FIXADD = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_TEXTCTRL_CONST_SLVITER = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_LISTBOX_CONST_FIXAT = wxNewId();
const wxWindowID ConstraintPanelGUI::wxID_STATIC_CONST_MINRES = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_BUTTON_CONST_GETSEL = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_CHECKBOX_CONST_HONLY = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_PANEL_CONST_RSDUE = wxNewId();
const wxWindowID ConstraintPanelGUI::wxID_STATIC_CONST_SLVTOL = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_RADIOBUTTON_CONST_RADOUT = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_DIALOG_CONST = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_CHOICE_CONST_SELOPT = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_CHOICE_CONST_SHKOPT = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_TEXTCTRL_CONST_MAXRES = wxNewId();
const wxWindowID ConstraintPanelGUI::ID_TEXTCTRL_CONST_RADIUS = wxNewId();
const wxWindowID ConstraintPanelGUI::wxID_STATIC_CONST_ATCENT = wxNewId();
const wxWindowID ConstraintPanelGUI::wxID_STATIC_CONST_SLUTOL = wxNewId();

/*!
 * ConstraintPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( ConstraintPanelGUI, ewxPanel )

/*!
 * ConstraintPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( ConstraintPanelGUI, ewxPanel )

////@begin ConstraintPanelGUI event table entries
    EVT_CHECKBOX( ID_CHECKBOX_CONST_USESHK, ConstraintPanelGUI::OnCheckboxConstUseshkClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CONST_SLVITER, ConstraintPanelGUI::OnTextctrlConstSlviterEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CONST_SLVTOL, ConstraintPanelGUI::OnTextctrlConstSlvtolEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CONST_SLUITER, ConstraintPanelGUI::OnTextctrlConstSluiterEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_CONST_SLUTOL, ConstraintPanelGUI::OnTextctrlConstSlutolEnter )

    EVT_CHOICE( ID_CHOICE_CONST_SHKOPT, ConstraintPanelGUI::OnChoiceConstShkoptSelected )

    EVT_CHOICE( ID_CHOICE_CONST_FIXOPT, ConstraintPanelGUI::OnChoiceConstFixoptSelected )

    EVT_CHECKBOX( ID_CHECKBOX_CONST_HONLY, ConstraintPanelGUI::OnCheckboxConstHonlyClick )

    EVT_CHECKBOX( ID_CHECKBOX_CONST_PERM, ConstraintPanelGUI::OnCheckboxConstPermClick )

    EVT_CHOICE( ID_CHOICE_CONST_SELOPT, ConstraintPanelGUI::OnChoiceConstSeloptSelected )

    EVT_BUTTON( ID_BUTTON_CONST_GETSEL, ConstraintPanelGUI::OnButtonConstGetselClick )

    EVT_BUTTON( ID_BUTTON_CONST_CLEAR, ConstraintPanelGUI::OnButtonConstClearClick )

    EVT_BUTTON( ID_BUTTON_CONST_FIXADD, ConstraintPanelGUI::OnButtonConstFixaddClick )

    EVT_BUTTON( ID_BUTTON_CONST_FIXDEL, ConstraintPanelGUI::OnButtonConstFixdelClick )

////@end ConstraintPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * ConstraintPanelGUI constructors
 */

ConstraintPanelGUI::ConstraintPanelGUI( )
{
}

ConstraintPanelGUI::ConstraintPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * ConstraintPanelGUI creator
 */

bool ConstraintPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ConstraintPanelGUI member initialisation
    p_constraintVSizer = NULL;
    p_constrainResidue = NULL;
    p_constrainRadius = NULL;
////@end ConstraintPanelGUI member initialisation

////@begin ConstraintPanelGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_VALIDATE_RECURSIVELY|wxWS_EX_BLOCK_EVENTS);
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ConstraintPanelGUI creation
    return true;
}

/*!
 * Control creation for ConstraintPanelGUI
 */

void ConstraintPanelGUI::CreateControls()
{    
////@begin ConstraintPanelGUI content construction
    ConstraintPanelGUI* itemPanel1 = this;

    p_constraintVSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(p_constraintVSizer);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    p_constraintVSizer->Add(itemBoxSizer3, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox4 = new ewxCheckBox( itemPanel1, ID_CHECKBOX_CONST_USESHK, _("Use SHAKE Constraints:"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox4->SetValue(false);
    itemBoxSizer3->Add(itemCheckBox4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel5 = new ewxPanel( itemPanel1, ID_PANEL_CONST_SHAKE, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_constraintVSizer->Add(itemPanel5, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxVERTICAL);
    itemPanel5->SetSizer(itemBoxSizer6);

    wxFlexGridSizer* itemFlexGridSizer7 = new wxFlexGridSizer(2, 5, 0, 0);
    itemBoxSizer6->Add(itemFlexGridSizer7, 0, wxALIGN_LEFT|wxLEFT, 15);

    ewxStaticText* itemStaticText8 = new ewxStaticText( itemPanel5, wxID_STATIC_CONST_SLVITER, _("Maximum Solvent SHAKE Iterations:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(itemStaticText8, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl9 = new ewxTextCtrl( itemPanel5, ID_TEXTCTRL_CONST_SLVITER, _("100"), wxDefaultPosition, wxSize(50, -1), 0 );
    itemFlexGridSizer7->Add(itemTextCtrl9, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemFlexGridSizer7->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText11 = new ewxStaticText( itemPanel5, wxID_STATIC_CONST_SLVTOL, _("SHAKE Tolerance:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(itemStaticText11, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer7->Add(itemBoxSizer12, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxTextCtrl* itemTextCtrl13 = new ewxTextCtrl( itemPanel5, ID_TEXTCTRL_CONST_SLVTOL, _("0.001"), wxDefaultPosition, wxSize(70, -1), 0 );
    itemBoxSizer12->Add(itemTextCtrl13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText14 = new ewxStaticText( itemPanel5, wxID_STATIC_CONST_SLVTOL_UNITS, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemStaticText14, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText15 = new ewxStaticText( itemPanel5, wxID_STATIC_CONST_SLUITER, _("Maximum Solute SHAKE Iterations:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(itemStaticText15, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl16 = new ewxTextCtrl( itemPanel5, ID_TEXTCTRL_CONST_SLUITER, _("100"), wxDefaultPosition, wxSize(50, -1), 0 );
    itemFlexGridSizer7->Add(itemTextCtrl16, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemFlexGridSizer7->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText18 = new ewxStaticText( itemPanel5, wxID_STATIC_CONST_SLUTOL, _("SHAKE Tolerance:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer7->Add(itemStaticText18, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer7->Add(itemBoxSizer19, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxTextCtrl* itemTextCtrl20 = new ewxTextCtrl( itemPanel5, ID_TEXTCTRL_CONST_SLUTOL, _("0.001"), wxDefaultPosition, wxSize(70, -1), 0 );
    itemBoxSizer19->Add(itemTextCtrl20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText21 = new ewxStaticText( itemPanel5, wxID_STATIC_CONST_SLUTOL_UNITS, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemStaticText21, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer22 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer6->Add(itemBoxSizer22, 0, wxALIGN_LEFT|wxLEFT, 15);

    ewxStaticText* itemStaticText23 = new ewxStaticText( itemPanel5, wxID_STATIC, _("SHAKE Options:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer22->Add(itemStaticText23, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice24Strings[] = {
        _("Constrain All Molecules"),
        _("Constrain Solvent Only"),
        _("Constrain Solute Only")
    };
    ewxChoice* itemChoice24 = new ewxChoice( itemPanel5, ID_CHOICE_CONST_SHKOPT, wxDefaultPosition, wxDefaultSize, 3, itemChoice24Strings, 0 );
    itemChoice24->SetStringSelection(_("Constrain All Molecules"));
    itemBoxSizer22->Add(itemChoice24, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxHORIZONTAL);
    p_constraintVSizer->Add(itemBoxSizer25, 0, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine26 = new ewxStaticLine( itemPanel1, wxID_STATIC, wxDefaultPosition, wxSize(20, -1), wxLI_HORIZONTAL );
    itemBoxSizer25->Add(itemStaticLine26, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText27 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Add Fixed Atoms"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer25->Add(itemStaticText27, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine28 = new ewxStaticLine( itemPanel1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer25->Add(itemStaticLine28, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer29 = new wxBoxSizer(wxHORIZONTAL);
    p_constraintVSizer->Add(itemBoxSizer29, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText30 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Fix Atom Options:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer29->Add(itemStaticText30, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemChoice31Strings[] = {
        _("Fix Solvent Atoms"),
        _("Fix Solute Atoms"),
        _("Free Solvent Atoms"),
        _("Free Solute Atoms")
    };
    ewxChoice* itemChoice31 = new ewxChoice( itemPanel1, ID_CHOICE_CONST_FIXOPT, wxDefaultPosition, wxDefaultSize, 4, itemChoice31Strings, 0 );
    itemBoxSizer29->Add(itemChoice31, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer29->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer33 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer29->Add(itemBoxSizer33, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxCheckBox* itemCheckBox34 = new ewxCheckBox( itemPanel1, ID_CHECKBOX_CONST_HONLY, _("Select Heavy Atoms Only"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox34->SetValue(false);
    itemBoxSizer33->Add(itemCheckBox34, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox35 = new ewxCheckBox( itemPanel1, ID_CHECKBOX_CONST_PERM, _("Add Change to Restart File"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox35->SetValue(false);
    itemBoxSizer33->Add(itemCheckBox35, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer36 = new wxBoxSizer(wxHORIZONTAL);
    p_constraintVSizer->Add(itemBoxSizer36, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText37 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Selection Options:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer36->Add(itemStaticText37, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemChoice38Strings[] = {
        _("Select Residue Range"),
        _("Radius Selection")
    };
    ewxChoice* itemChoice38 = new ewxChoice( itemPanel1, ID_CHOICE_CONST_SELOPT, wxDefaultPosition, wxDefaultSize, 2, itemChoice38Strings, 0 );
    itemBoxSizer36->Add(itemChoice38, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_constrainResidue = new ewxPanel( itemPanel1, ID_PANEL_CONST_RSDUE, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_constraintVSizer->Add(p_constrainResidue, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer40 = new wxBoxSizer(wxHORIZONTAL);
    p_constrainResidue->SetSizer(itemBoxSizer40);

    itemBoxSizer40->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText42 = new ewxStaticText( p_constrainResidue, wxID_STATIC_CONST_MINRES, _("Minimum Residue Index:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemStaticText42, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl43 = new ewxTextCtrl( p_constrainResidue, ID_TEXTCTRL_CONST_MINRES, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemTextCtrl43, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer40->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText45 = new ewxStaticText( p_constrainResidue, wxID_STATIC_CONST_MAXRES, _("Maximum Residue Index:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemStaticText45, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl46 = new ewxTextCtrl( p_constrainResidue, ID_TEXTCTRL_CONST_MAXRES, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemTextCtrl46, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_constrainRadius = new ewxPanel( itemPanel1, ID_PANEL_CONST_RADIUS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_constraintVSizer->Add(p_constrainRadius, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer48 = new wxBoxSizer(wxHORIZONTAL);
    p_constrainRadius->SetSizer(itemBoxSizer48);

    itemBoxSizer48->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxRadioButton* itemRadioButton50 = new ewxRadioButton( p_constrainRadius, ID_RADIOBUTTON_CONST_RADIN, _("Within"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton50->SetValue(false);
    itemBoxSizer48->Add(itemRadioButton50, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton51 = new ewxRadioButton( p_constrainRadius, ID_RADIOBUTTON_CONST_RADOUT, _("Beyond"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton51->SetValue(false);
    itemBoxSizer48->Add(itemRadioButton51, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText52 = new ewxStaticText( p_constrainRadius, wxID_STATIC_CONST_RADIUS, _("Radius:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemStaticText52, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl53 = new ewxTextCtrl( p_constrainRadius, ID_TEXTCTRL_CONST_RADIUS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemTextCtrl53, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText54 = new ewxStaticText( p_constrainRadius, wxID_STATIC_CONST_RADIUS_UNITS, _("nm"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemStaticText54, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer48->Add(4, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText56 = new ewxStaticText( p_constrainRadius, wxID_STATIC_CONST_ATCENT, _("Atom Center:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemStaticText56, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl57 = new ewxTextCtrl( p_constrainRadius, ID_TEXTCTRL_CONST_ATCENT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemTextCtrl57, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer58 = new wxBoxSizer(wxHORIZONTAL);
    p_constraintVSizer->Add(itemBoxSizer58, 0, wxALIGN_RIGHT|wxALL, 0);

    ewxButton* itemButton59 = new ewxButton( itemPanel1, ID_BUTTON_CONST_GETSEL, _("Get Selection"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer58->Add(itemButton59, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer58->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton61 = new ewxButton( itemPanel1, ID_BUTTON_CONST_CLEAR, _("Clear Selection"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer58->Add(itemButton61, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer58->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer63 = new wxBoxSizer(wxHORIZONTAL);
    p_constraintVSizer->Add(itemBoxSizer63, 0, wxALIGN_LEFT|wxLEFT|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer64 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer63->Add(itemBoxSizer64, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton65 = new ewxButton( itemPanel1, ID_BUTTON_CONST_FIXADD, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer64->Add(itemButton65, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

    ewxButton* itemButton66 = new ewxButton( itemPanel1, ID_BUTTON_CONST_FIXDEL, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer64->Add(itemButton66, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer67 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer63->Add(itemBoxSizer67, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText68 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Fixed Atom List:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer67->Add(itemStaticText68, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 3);

    wxString* itemListBox69Strings = NULL;
    ewxListBox* itemListBox69 = new ewxListBox( itemPanel1, ID_LISTBOX_CONST_FIXAT, wxDefaultPosition, wxSize(350, 80), 0, itemListBox69Strings, wxLB_SINGLE );
    itemBoxSizer67->Add(itemListBox69, 1, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 3);

////@end ConstraintPanelGUI content construction
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_USESHK
 */

void ConstraintPanelGUI::OnCheckboxConstUseshkClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_USESHK in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_USESHK in ConstraintPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLVITER
 */

void ConstraintPanelGUI::OnTextctrlConstSlviterEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLVITER in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLVITER in ConstraintPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLVTOL
 */

void ConstraintPanelGUI::OnTextctrlConstSlvtolEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLVTOL in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLVTOL in ConstraintPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLUITER
 */

void ConstraintPanelGUI::OnTextctrlConstSluiterEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLUITER in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLUITER in ConstraintPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLUTOL
 */

void ConstraintPanelGUI::OnTextctrlConstSlutolEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLUTOL in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLUTOL in ConstraintPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_SHKOPT
 */

void ConstraintPanelGUI::OnChoiceConstShkoptSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_SHKOPT in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_SHKOPT in ConstraintPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_FIXOPT
 */

void ConstraintPanelGUI::OnChoiceConstFixoptSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_FIXOPT in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_FIXOPT in ConstraintPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_HONLY
 */

void ConstraintPanelGUI::OnCheckboxConstHonlyClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_HONLY in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_HONLY in ConstraintPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_PERM
 */

void ConstraintPanelGUI::OnCheckboxConstPermClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_PERM in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_PERM in ConstraintPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_SELOPT
 */

void ConstraintPanelGUI::OnChoiceConstSeloptSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_SELOPT in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_SELOPT in ConstraintPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_GETSEL
 */

void ConstraintPanelGUI::OnButtonConstGetselClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_GETSEL in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_GETSEL in ConstraintPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_CLEAR
 */

void ConstraintPanelGUI::OnButtonConstClearClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_CLEAR in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_CLEAR in ConstraintPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_FIXADD
 */

void ConstraintPanelGUI::OnButtonConstFixaddClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_FIXADD in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_FIXADD in ConstraintPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_FIXDEL
 */

void ConstraintPanelGUI::OnButtonConstFixdelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_FIXDEL in ConstraintPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_FIXDEL in ConstraintPanelGUI. 
}

/*!
 * Should we show tooltips?
 */

bool ConstraintPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap ConstraintPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ConstraintPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end ConstraintPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ConstraintPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ConstraintPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end ConstraintPanelGUI icon retrieval
}
