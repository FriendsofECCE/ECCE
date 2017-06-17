/////////////////////////////////////////////////////////////////////////////
// Name:        SelectionPanelGUI.C
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "SelectionPanelGUI.H"
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
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxviz/SelectionPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_RESNAM = wxNewId();
const wxWindowID SelectionPanelGUI::ID_BUTTON_DIS_SELECT = wxNewId();
const wxWindowID SelectionPanelGUI::ID_BUTTON_RES_INVERSE = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_FLRES = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_DISTANCE = wxNewId();
const wxWindowID SelectionPanelGUI::ID_TEXTCTRL_RES_ATYP = wxNewId();
const wxWindowID SelectionPanelGUI::ID_BUTTON_RES_CLEAR = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_AT_ALL = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_ANAM = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_CHAIN = wxNewId();
const wxWindowID SelectionPanelGUI::ID_TEXTCTRL_RES_ANAM = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_AT_MOL = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_AT_ELMNT = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_BBONE = wxNewId();
const wxWindowID SelectionPanelGUI::ID_COMBOBOX_ELMNT = wxNewId();
const wxWindowID SelectionPanelGUI::ID_BUTTON_DIS_INVERSE = wxNewId();
const wxWindowID SelectionPanelGUI::ID_BUTTON_DIS_CLEAR = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_BONDS = wxNewId();
const wxWindowID SelectionPanelGUI::ID_BUTTON_DIS_XSELECT = wxNewId();
const wxWindowID SelectionPanelGUI::ID_SPINCTRL_LAST_AT = wxNewId();
const wxWindowID SelectionPanelGUI::ID_SPINCTRL_FRST_AT = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_SCHAIN = wxNewId();
const wxWindowID SelectionPanelGUI::ID_CHECKBOX_FULL_RES = wxNewId();
const wxWindowID SelectionPanelGUI::ID_ATOM_INVERSE = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_SLVNT = wxNewId();
const wxWindowID SelectionPanelGUI::ID_SPINCTRL_MAXBND = wxNewId();
const wxWindowID SelectionPanelGUI::ID_ATOM_SELECT = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_AT_IDX = wxNewId();
const wxWindowID SelectionPanelGUI::ID_ATOMS = wxNewId();
const wxWindowID SelectionPanelGUI::ID_TEXTCTRL_CHAIN = wxNewId();
const wxWindowID SelectionPanelGUI::ID_ATOM_CLEAR = wxNewId();
const wxWindowID SelectionPanelGUI::ID_PANEL2 = wxNewId();
const wxWindowID SelectionPanelGUI::ID_TEXTCTRL_MINDIS = wxNewId();
const wxWindowID SelectionPanelGUI::ID_BUTTON_RES_XSELECT = wxNewId();
const wxWindowID SelectionPanelGUI::ID_TEXTCTRL_MAXDIS = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RESIDUES = wxNewId();
const wxWindowID SelectionPanelGUI::ID_TEXTCTRL_RESNAM = wxNewId();
const wxWindowID SelectionPanelGUI::ID_ATOM_XSELECT = wxNewId();
const wxWindowID SelectionPanelGUI::ID_DIALOG_SEL = wxNewId();
const wxWindowID SelectionPanelGUI::ID_BUTTON_RES_SELECT = wxNewId();
const wxWindowID SelectionPanelGUI::ID_SPINCTRL_MINBND = wxNewId();
const wxWindowID SelectionPanelGUI::ID_RADIOBUTTON_ATYP = wxNewId();

/*!
 * SelectionPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( SelectionPanelGUI, ewxPanel )

/*!
 * SelectionPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( SelectionPanelGUI, ewxPanel )

////@begin SelectionPanelGUI event table entries
    EVT_COMBOBOX( ID_COMBOBOX_ELMNT, SelectionPanelGUI::OnComboboxElmntSelected )
    EVT_TEXT( ID_COMBOBOX_ELMNT, SelectionPanelGUI::OnComboboxElmntUpdated )
    EVT_TEXT_ENTER( ID_COMBOBOX_ELMNT, SelectionPanelGUI::OnComboboxElmntEnter )

    EVT_SPINCTRL( ID_SPINCTRL_FRST_AT, SelectionPanelGUI::OnSpinctrlFrstAtUpdated )
    EVT_TEXT( ID_SPINCTRL_FRST_AT, SelectionPanelGUI::OnSpinctrlFrstAtTextUpdated )

    EVT_SPINCTRL( ID_SPINCTRL_LAST_AT, SelectionPanelGUI::OnSpinctrlLastAtUpdated )
    EVT_TEXT( ID_SPINCTRL_LAST_AT, SelectionPanelGUI::OnSpinctrlLastAtTextUpdated )

    EVT_BUTTON( ID_ATOM_SELECT, SelectionPanelGUI::OnAtomSelectClick )

    EVT_BUTTON( ID_ATOM_XSELECT, SelectionPanelGUI::OnAtomXselectClick )

    EVT_BUTTON( ID_ATOM_INVERSE, SelectionPanelGUI::OnAtomInverseClick )

    EVT_BUTTON( ID_ATOM_CLEAR, SelectionPanelGUI::OnAtomClearClick )

    EVT_TEXT( ID_TEXTCTRL_RES_ATYP, SelectionPanelGUI::OnTextctrlResAtypUpdated )

    EVT_TEXT( ID_TEXTCTRL_RES_ANAM, SelectionPanelGUI::OnTextctrlResAnamUpdated )

    EVT_TEXT( ID_TEXTCTRL_RESNAM, SelectionPanelGUI::OnTextctrlResnamUpdated )

    EVT_TEXT( ID_TEXTCTRL_CHAIN, SelectionPanelGUI::OnTextctrlChainUpdated )

    EVT_BUTTON( ID_BUTTON_RES_SELECT, SelectionPanelGUI::OnButtonResSelectClick )

    EVT_BUTTON( ID_BUTTON_RES_XSELECT, SelectionPanelGUI::OnButtonResXselectClick )

    EVT_BUTTON( ID_BUTTON_RES_INVERSE, SelectionPanelGUI::OnButtonResInverseClick )

    EVT_BUTTON( ID_BUTTON_RES_CLEAR, SelectionPanelGUI::OnButtonResClearClick )

    EVT_TEXT( ID_TEXTCTRL_MINDIS, SelectionPanelGUI::OnTextctrlMindisUpdated )
    EVT_TEXT_ENTER( ID_TEXTCTRL_MINDIS, SelectionPanelGUI::OnTextctrlMindisEnter )

    EVT_TEXT( ID_TEXTCTRL_MAXDIS, SelectionPanelGUI::OnTextctrlMaxdisUpdated )
    EVT_TEXT_ENTER( ID_TEXTCTRL_MAXDIS, SelectionPanelGUI::OnTextctrlMaxdisEnter )

    EVT_SPINCTRL( ID_SPINCTRL_MINBND, SelectionPanelGUI::OnSpinctrlMinbndUpdated )
    EVT_TEXT( ID_SPINCTRL_MINBND, SelectionPanelGUI::OnSpinctrlMinbndTextUpdated )

    EVT_SPINCTRL( ID_SPINCTRL_MAXBND, SelectionPanelGUI::OnSpinctrlMaxbndUpdated )
    EVT_TEXT( ID_SPINCTRL_MAXBND, SelectionPanelGUI::OnSpinctrlMaxbndTextUpdated )

    EVT_BUTTON( ID_BUTTON_DIS_SELECT, SelectionPanelGUI::OnButtonDisSelectClick )

    EVT_BUTTON( ID_BUTTON_DIS_XSELECT, SelectionPanelGUI::OnButtonDisXselectClick )

    EVT_BUTTON( ID_BUTTON_DIS_INVERSE, SelectionPanelGUI::OnButtonDisInverseClick )

    EVT_BUTTON( ID_BUTTON_DIS_CLEAR, SelectionPanelGUI::OnButtonDisClearClick )

////@end SelectionPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * SelectionPanelGUI constructors
 */

SelectionPanelGUI::SelectionPanelGUI( )
{
}

SelectionPanelGUI::SelectionPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * SelectionPanelGUI creator
 */

bool SelectionPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin SelectionPanelGUI member initialisation
    p_residuePanel = NULL;
////@end SelectionPanelGUI member initialisation

////@begin SelectionPanelGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end SelectionPanelGUI creation
    return true;
}

/*!
 * Control creation for SelectionPanelGUI
 */

void SelectionPanelGUI::CreateControls()
{    
////@begin SelectionPanelGUI content construction
    SelectionPanelGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    ewxNotebook* itemNotebook3 = new ewxNotebook( itemPanel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

    ewxPanel* itemPanel4 = new ewxPanel( itemNotebook3, ID_ATOMS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemPanel4->SetSizer(itemBoxSizer5);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer5->Add(itemBoxSizer6, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 3);
    ewxRadioButton* itemRadioButton7 = new ewxRadioButton( itemPanel4, ID_RADIOBUTTON_AT_ELMNT, _("Element Type"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton7->SetValue(false);
    if (ShowToolTips())
        itemRadioButton7->SetToolTip(_("Select all atoms of a given element"));
    itemBoxSizer6->Add(itemRadioButton7, 0, wxALIGN_CENTER_VERTICAL, 3);

    wxString itemComboBox8Strings[] = {
        _("H"),
        _("C"),
        _("N"),
        _("O")
    };
    ewxComboBox* itemComboBox8 = new ewxComboBox( itemPanel4, ID_COMBOBOX_ELMNT, _("C"), wxDefaultPosition, wxDefaultSize, 4, itemComboBox8Strings, wxCB_DROPDOWN );
    itemComboBox8->SetStringSelection(_("C"));
    itemBoxSizer6->Add(itemComboBox8, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer5->Add(itemBoxSizer9, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 3);
    ewxRadioButton* itemRadioButton10 = new ewxRadioButton( itemPanel4, ID_RADIOBUTTON_AT_IDX, _("Atom Index"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton10->SetValue(false);
    if (ShowToolTips())
        itemRadioButton10->SetToolTip(_("Select all atoms in the range of indices"));
    itemBoxSizer9->Add(itemRadioButton10, 0, wxALIGN_CENTER_VERTICAL, 3);

    ewxSpinCtrl* itemSpinCtrl11 = new ewxSpinCtrl( itemPanel4, ID_SPINCTRL_FRST_AT, _("1"), wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 1, 9999999, 1 );
    itemBoxSizer9->Add(itemSpinCtrl11, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    ewxStaticText* itemStaticText12 = new ewxStaticText( itemPanel4, wxID_STATIC, _("to"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemStaticText12, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 3);

    ewxSpinCtrl* itemSpinCtrl13 = new ewxSpinCtrl( itemPanel4, ID_SPINCTRL_LAST_AT, _("1"), wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 1, 9999999, 1 );
    itemBoxSizer9->Add(itemSpinCtrl13, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    ewxRadioButton* itemRadioButton14 = new ewxRadioButton( itemPanel4, ID_RADIOBUTTON_AT_MOL, _("Molecule that contains current selection"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton14->SetValue(false);
    if (ShowToolTips())
        itemRadioButton14->SetToolTip(_("Select all atoms that are connected by a chain of bonds to the selected atom"));
    itemBoxSizer5->Add(itemRadioButton14, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP, 3);

    ewxRadioButton* itemRadioButton15 = new ewxRadioButton( itemPanel4, ID_RADIOBUTTON_AT_ALL, _("All"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton15->SetValue(false);
    if (ShowToolTips())
        itemRadioButton15->SetToolTip(_("Select entire system"));
    itemBoxSizer5->Add(itemRadioButton15, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP, 3);

    itemBoxSizer5->Add(0, 0, 1, wxALIGN_CENTER_HORIZONTAL, 0);

    ewxStaticLine* itemStaticLine17 = new ewxStaticLine( itemPanel4, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer5->Add(itemStaticLine17, 0, wxGROW|wxLEFT|wxRIGHT, 3);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer5->Add(itemBoxSizer18, 0, wxGROW|wxALL, 3);
    ewxButton* itemButton19 = new ewxButton( itemPanel4, ID_ATOM_SELECT, _("Select"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemButton19->SetHelpText(_("Clear current selection before selecting"));
    if (ShowToolTips())
        itemButton19->SetToolTip(_("Clear current selection before selecting"));
    itemBoxSizer18->Add(itemButton19, 0, wxALIGN_CENTER_VERTICAL, 3);

    ewxButton* itemButton20 = new ewxButton( itemPanel4, ID_ATOM_XSELECT, _("Ext. Sel."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemButton20->SetHelpText(_("Add selection to current selection"));
    if (ShowToolTips())
        itemButton20->SetToolTip(_("Add selection to current selection"));
    itemBoxSizer18->Add(itemButton20, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    itemBoxSizer18->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL, 0);

    ewxButton* itemButton22 = new ewxButton( itemPanel4, ID_ATOM_INVERSE, _("Invert"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemBoxSizer18->Add(itemButton22, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    ewxButton* itemButton23 = new ewxButton( itemPanel4, ID_ATOM_CLEAR, _("Clear"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemBoxSizer18->Add(itemButton23, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    itemNotebook3->AddPage(itemPanel4, _("Atoms"));

    p_residuePanel = new ewxPanel( itemNotebook3, ID_RESIDUES, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxVERTICAL);
    p_residuePanel->SetSizer(itemBoxSizer25);

    wxFlexGridSizer* itemFlexGridSizer26 = new wxFlexGridSizer(4, 4, 2, 2);
    itemBoxSizer25->Add(itemFlexGridSizer26, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 2);
    ewxRadioButton* itemRadioButton27 = new ewxRadioButton( p_residuePanel, ID_RADIOBUTTON_ATYP, _("Atom Type"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton27->SetValue(true);
    if (ShowToolTips())
        itemRadioButton27->SetToolTip(_("Select all atoms based force field atom type"));
    itemFlexGridSizer26->Add(itemRadioButton27, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    ewxTextCtrl* itemTextCtrl28 = new ewxTextCtrl( p_residuePanel, ID_TEXTCTRL_RES_ATYP, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    itemFlexGridSizer26->Add(itemTextCtrl28, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);

    itemFlexGridSizer26->Add(15, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxRadioButton* itemRadioButton30 = new ewxRadioButton( p_residuePanel, ID_RADIOBUTTON_FLRES, _("Full residue"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton30->SetValue(false);
    if (ShowToolTips())
        itemRadioButton30->SetToolTip(_("Find complete residues based on current selection"));
    itemFlexGridSizer26->Add(itemRadioButton30, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    ewxRadioButton* itemRadioButton31 = new ewxRadioButton( p_residuePanel, ID_RADIOBUTTON_ANAM, _("Atom Name"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton31->SetValue(false);
    if (ShowToolTips())
        itemRadioButton31->SetToolTip(_("Select all atoms based on PDB atom name"));
    itemFlexGridSizer26->Add(itemRadioButton31, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    ewxTextCtrl* itemTextCtrl32 = new ewxTextCtrl( p_residuePanel, ID_TEXTCTRL_RES_ANAM, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    itemFlexGridSizer26->Add(itemTextCtrl32, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);

    itemFlexGridSizer26->Add(15, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxRadioButton* itemRadioButton34 = new ewxRadioButton( p_residuePanel, ID_RADIOBUTTON_SLVNT, _("Solvent"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton34->SetValue(false);
    if (ShowToolTips())
        itemRadioButton34->SetToolTip(_("Select all HOH residues"));
    itemFlexGridSizer26->Add(itemRadioButton34, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    ewxRadioButton* itemRadioButton35 = new ewxRadioButton( p_residuePanel, ID_RADIOBUTTON_RESNAM, _("Residue Name"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton35->SetValue(false);
    if (ShowToolTips())
        itemRadioButton35->SetToolTip(_("Select all atoms based on PDB residue name"));
    itemFlexGridSizer26->Add(itemRadioButton35, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    ewxTextCtrl* itemTextCtrl36 = new ewxTextCtrl( p_residuePanel, ID_TEXTCTRL_RESNAM, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    itemFlexGridSizer26->Add(itemTextCtrl36, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);

    itemFlexGridSizer26->Add(15, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxRadioButton* itemRadioButton38 = new ewxRadioButton( p_residuePanel, ID_RADIOBUTTON_BBONE, _("Backbone"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton38->SetValue(false);
    if (ShowToolTips())
        itemRadioButton38->SetToolTip(_("Select all amino acid, DNA, or RNA backbone atoms"));
    itemFlexGridSizer26->Add(itemRadioButton38, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    ewxRadioButton* itemRadioButton39 = new ewxRadioButton( p_residuePanel, ID_RADIOBUTTON_CHAIN, _("Chain"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton39->SetValue(false);
    if (ShowToolTips())
        itemRadioButton39->SetToolTip(_("Select all atoms in the same chain"));
    itemFlexGridSizer26->Add(itemRadioButton39, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxTextCtrl* itemTextCtrl40 = new ewxTextCtrl( p_residuePanel, ID_TEXTCTRL_CHAIN, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    if (ShowToolTips())
        itemTextCtrl40->SetToolTip(_("Specify one or more chains"));
    itemFlexGridSizer26->Add(itemTextCtrl40, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);

    itemFlexGridSizer26->Add(15, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxRadioButton* itemRadioButton42 = new ewxRadioButton( p_residuePanel, ID_RADIOBUTTON_SCHAIN, _("Sidechains"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton42->SetValue(false);
    if (ShowToolTips())
        itemRadioButton42->SetToolTip(_("Select all amino acid, DNA, or RNA sidechain atoms"));
    itemFlexGridSizer26->Add(itemRadioButton42, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    itemBoxSizer25->Add(0, 0, 1, wxALIGN_CENTER_HORIZONTAL, 0);

    ewxStaticLine* itemStaticLine44 = new ewxStaticLine( p_residuePanel, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer25->Add(itemStaticLine44, 0, wxGROW|wxLEFT|wxRIGHT, 3);

    wxBoxSizer* itemBoxSizer45 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer25->Add(itemBoxSizer45, 0, wxGROW|wxALL, 3);
    ewxButton* itemButton46 = new ewxButton( p_residuePanel, ID_BUTTON_RES_SELECT, _("Select"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton46->SetToolTip(_("Clear current selection before selecting"));
    itemBoxSizer45->Add(itemButton46, 0, wxALIGN_CENTER_VERTICAL, 3);

    ewxButton* itemButton47 = new ewxButton( p_residuePanel, ID_BUTTON_RES_XSELECT, _("Ext. Sel."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton47->SetToolTip(_("Add selection to current selection"));
    itemBoxSizer45->Add(itemButton47, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    itemBoxSizer45->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL, 0);

    ewxButton* itemButton49 = new ewxButton( p_residuePanel, ID_BUTTON_RES_INVERSE, _("Invert"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemBoxSizer45->Add(itemButton49, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    ewxButton* itemButton50 = new ewxButton( p_residuePanel, ID_BUTTON_RES_CLEAR, _("Clear"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemBoxSizer45->Add(itemButton50, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    itemNotebook3->AddPage(p_residuePanel, _("Residues"));

    ewxPanel* itemPanel51 = new ewxPanel( itemNotebook3, ID_PANEL2, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer52 = new wxBoxSizer(wxVERTICAL);
    itemPanel51->SetSizer(itemBoxSizer52);

    ewxRadioButton* itemRadioButton53 = new ewxRadioButton( itemPanel51, ID_RADIOBUTTON_DISTANCE, _("Distance (in angstroms) from current selection:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton53->SetValue(true);
    if (ShowToolTips())
        itemRadioButton53->SetToolTip(_("Select all atoms within distance of current selection"));
    itemBoxSizer52->Add(itemRadioButton53, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP, 2);

    wxBoxSizer* itemBoxSizer54 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer52->Add(itemBoxSizer54, 0, wxALIGN_RIGHT, 0);
    ewxTextCtrl* itemTextCtrl55 = new ewxTextCtrl( itemPanel51, ID_TEXTCTRL_MINDIS, _("0.0"), wxDefaultPosition, wxSize(80, -1), wxTE_PROCESS_ENTER );
    itemBoxSizer54->Add(itemTextCtrl55, 0, wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText56 = new ewxStaticText( itemPanel51, wxID_STATIC, _("to"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer54->Add(itemStaticText56, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl57 = new ewxTextCtrl( itemPanel51, ID_TEXTCTRL_MAXDIS, _("5.0"), wxDefaultPosition, wxSize(80, -1), wxTE_PROCESS_ENTER );
    itemBoxSizer54->Add(itemTextCtrl57, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    ewxCheckBox* itemCheckBox58 = new ewxCheckBox( itemPanel51, ID_CHECKBOX_FULL_RES, _("Select Complete Residues"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox58->SetValue(false);
    itemBoxSizer52->Add(itemCheckBox58, 0, wxALIGN_RIGHT, 3);

    ewxRadioButton* itemRadioButton59 = new ewxRadioButton( itemPanel51, ID_RADIOBUTTON_BONDS, _("Bond distance (in hops) from current selection:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton59->SetValue(false);
    if (ShowToolTips())
        itemRadioButton59->SetToolTip(_("Select all atoms that are connected to current selection by specified number of bonds"));
    itemBoxSizer52->Add(itemRadioButton59, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP, 2);

    wxBoxSizer* itemBoxSizer60 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer52->Add(itemBoxSizer60, 0, wxALIGN_RIGHT|wxRIGHT, 3);
    ewxSpinCtrl* itemSpinCtrl61 = new ewxSpinCtrl( itemPanel51, ID_SPINCTRL_MINBND, _("1"), wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 1, 99999, 1 );
    itemBoxSizer60->Add(itemSpinCtrl61, 0, wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText62 = new ewxStaticText( itemPanel51, wxID_STATIC, _("to"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer60->Add(itemStaticText62, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxADJUST_MINSIZE, 3);

    ewxSpinCtrl* itemSpinCtrl63 = new ewxSpinCtrl( itemPanel51, ID_SPINCTRL_MAXBND, _("1"), wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 1, 99999, 1 );
    itemBoxSizer60->Add(itemSpinCtrl63, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    itemBoxSizer52->Add(0, 0, 1, wxALIGN_CENTER_HORIZONTAL, 0);

    ewxStaticLine* itemStaticLine65 = new ewxStaticLine( itemPanel51, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer52->Add(itemStaticLine65, 0, wxGROW|wxLEFT|wxRIGHT, 5);

    wxBoxSizer* itemBoxSizer66 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer52->Add(itemBoxSizer66, 0, wxGROW|wxALL, 3);
    ewxButton* itemButton67 = new ewxButton( itemPanel51, ID_BUTTON_DIS_SELECT, _("Select"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton67->SetToolTip(_("Clear current selection before selecting"));
    itemBoxSizer66->Add(itemButton67, 0, wxALIGN_CENTER_VERTICAL, 3);

    ewxButton* itemButton68 = new ewxButton( itemPanel51, ID_BUTTON_DIS_XSELECT, _("Ext. Sel."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton68->SetToolTip(_("Add selection to current selection"));
    itemBoxSizer66->Add(itemButton68, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    itemBoxSizer66->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL, 0);

    ewxButton* itemButton70 = new ewxButton( itemPanel51, ID_BUTTON_DIS_INVERSE, _("Invert"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemBoxSizer66->Add(itemButton70, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    ewxButton* itemButton71 = new ewxButton( itemPanel51, ID_BUTTON_DIS_CLEAR, _("Clear"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemBoxSizer66->Add(itemButton71, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    itemNotebook3->AddPage(itemPanel51, _("Distance"));

    itemBoxSizer2->Add(itemNotebook3, 1, wxGROW, 5);

////@end SelectionPanelGUI content construction
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_ELMNT
 */

void SelectionPanelGUI::OnComboboxElmntSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_ELMNT in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_ELMNT in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_COMBOBOX_ELMNT
 */

void SelectionPanelGUI::OnComboboxElmntUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_COMBOBOX_ELMNT in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_COMBOBOX_ELMNT in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_ELMNT
 */

void SelectionPanelGUI::OnComboboxElmntEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_ELMNT in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_ELMNT in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_FRST_AT
 */

void SelectionPanelGUI::OnSpinctrlFrstAtUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_FRST_AT in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_FRST_AT in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_FRST_AT
 */

void SelectionPanelGUI::OnSpinctrlFrstAtTextUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_FRST_AT in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_FRST_AT in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_LAST_AT
 */

void SelectionPanelGUI::OnSpinctrlLastAtUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_LAST_AT in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_LAST_AT in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_LAST_AT
 */

void SelectionPanelGUI::OnSpinctrlLastAtTextUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_LAST_AT in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_LAST_AT in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_SELECT
 */

void SelectionPanelGUI::OnAtomSelectClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_SELECT in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_SELECT in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_XSELECT
 */

void SelectionPanelGUI::OnAtomXselectClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_XSELECT in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_XSELECT in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_INVERSE
 */

void SelectionPanelGUI::OnAtomInverseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_INVERSE in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_INVERSE in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_CLEAR
 */

void SelectionPanelGUI::OnAtomClearClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_CLEAR in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_CLEAR in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RES_ATYP
 */

void SelectionPanelGUI::OnTextctrlResAtypUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RES_ATYP in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RES_ATYP in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RES_ANAM
 */

void SelectionPanelGUI::OnTextctrlResAnamUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RES_ANAM in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RES_ANAM in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RESNAM
 */

void SelectionPanelGUI::OnTextctrlResnamUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RESNAM in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RESNAM in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_CHAIN
 */

void SelectionPanelGUI::OnTextctrlChainUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_CHAIN in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_CHAIN in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_SELECT
 */

void SelectionPanelGUI::OnButtonResSelectClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_SELECT in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_SELECT in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_XSELECT
 */

void SelectionPanelGUI::OnButtonResXselectClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_XSELECT in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_XSELECT in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_INVERSE
 */

void SelectionPanelGUI::OnButtonResInverseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_INVERSE in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_INVERSE in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_CLEAR
 */

void SelectionPanelGUI::OnButtonResClearClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_CLEAR in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_CLEAR in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_MINDIS
 */

void SelectionPanelGUI::OnTextctrlMindisUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_MINDIS in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_MINDIS in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MINDIS
 */

void SelectionPanelGUI::OnTextctrlMindisEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MINDIS in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MINDIS in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_MAXDIS
 */

void SelectionPanelGUI::OnTextctrlMaxdisUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_MAXDIS in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_MAXDIS in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MAXDIS
 */

void SelectionPanelGUI::OnTextctrlMaxdisEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MAXDIS in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MAXDIS in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_MINBND
 */

void SelectionPanelGUI::OnSpinctrlMinbndUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_MINBND in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_MINBND in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_MINBND
 */

void SelectionPanelGUI::OnSpinctrlMinbndTextUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_MINBND in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_MINBND in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_MAXBND
 */

void SelectionPanelGUI::OnSpinctrlMaxbndUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_MAXBND in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_MAXBND in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_MAXBND
 */

void SelectionPanelGUI::OnSpinctrlMaxbndTextUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_MAXBND in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_MAXBND in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_SELECT
 */

void SelectionPanelGUI::OnButtonDisSelectClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_SELECT in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_SELECT in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_XSELECT
 */

void SelectionPanelGUI::OnButtonDisXselectClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_XSELECT in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_XSELECT in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_INVERSE
 */

void SelectionPanelGUI::OnButtonDisInverseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_INVERSE in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_INVERSE in SelectionPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_CLEAR
 */

void SelectionPanelGUI::OnButtonDisClearClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_CLEAR in SelectionPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_CLEAR in SelectionPanelGUI. 
}

/*!
 * Should we show tooltips?
 */

bool SelectionPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap SelectionPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin SelectionPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end SelectionPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon SelectionPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin SelectionPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end SelectionPanelGUI icon retrieval
}
