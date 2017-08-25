/////////////////////////////////////////////////////////////////////////////
// Name:        MetaEdGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "MetaEdGUI.H"
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
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxFrame.H"
////@end includes

#include "MetaEdGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID MetaEdGUI::ID_BUTTON_METAED_QMMM = wxNewId();
const wxWindowID MetaEdGUI::ID_STATICTEXT_METAED_ELECTRONS = wxNewId();
const wxWindowID MetaEdGUI::ID_BUTTON_METAED_THEORY = wxNewId();
const wxWindowID MetaEdGUI::ID_COMBOBOX_METAED_CHARGE = wxNewId();
const wxWindowID MetaEdGUI::ID_COMBOBOX_METAED_SPIN_MULT = wxNewId();
const wxWindowID MetaEdGUI::ID_STATICTEXT_METAED_ATOMS = wxNewId();
const wxWindowID MetaEdGUI::ID_METAED_FRAME = wxNewId();
const wxWindowID MetaEdGUI::ID_BUTTON_METAED_RUNTYPE = wxNewId();
const wxWindowID MetaEdGUI::ID_CHECKBOX_METAED_IRREDUCIBLE = wxNewId();
const wxWindowID MetaEdGUI::ID_BUTTON_METAED_BUILDER = wxNewId();
const wxWindowID MetaEdGUI::ID_STATIC_METAED_SPIN_MULT = wxNewId();
const wxWindowID MetaEdGUI::ID_STATIC_METAED_CHARGE = wxNewId();
const wxWindowID MetaEdGUI::ID_MENU_FEEDBACK = wxNewId();
const wxWindowID MetaEdGUI::ID_BUTTON_METAED_FINAL_EDIT = wxNewId();
const wxWindowID MetaEdGUI::ID_STATICTEXT_METAED_OPEN_SHELLS = wxNewId();
const wxWindowID MetaEdGUI::ID_BUTTON_METAED_LAUNCH = wxNewId();
const wxWindowID MetaEdGUI::ID_BUTTON_METAED_CONSTRAINT = wxNewId();
const wxWindowID MetaEdGUI::ID_STATICTEXT_METAED_SYMMETRY = wxNewId();

/*!
 * MetaEdGUI type definition
 */

IMPLEMENT_CLASS( MetaEdGUI, ewxFrame )

/*!
 * MetaEdGUI event table definition
 */

BEGIN_EVENT_TABLE( MetaEdGUI, ewxFrame )

////@begin MetaEdGUI event table entries
    EVT_CLOSE( MetaEdGUI::OnCloseWindow )

    EVT_MENU( wxID_SAVE, MetaEdGUI::OnSaveClick )

    EVT_MENU( wxID_EXIT, MetaEdGUI::OnExitClick )

    EVT_MENU( wxID_HELP, MetaEdGUI::OnHelpClick )

    EVT_MENU( ID_MENU_FEEDBACK, MetaEdGUI::OnMenuFeedbackClick )

    EVT_COMBOBOX( ID_COMBOBOX_METAED_CHARGE, MetaEdGUI::OnComboboxMetaedChargeSelected )
    EVT_TEXT_ENTER( ID_COMBOBOX_METAED_CHARGE, MetaEdGUI::OnComboboxMetaedChargeEnter )

    EVT_COMBOBOX( ID_COMBOBOX_METAED_SPIN_MULT, MetaEdGUI::OnComboboxMetaedSpinMultSelected )
    EVT_TEXT_ENTER( ID_COMBOBOX_METAED_SPIN_MULT, MetaEdGUI::OnComboboxMetaedSpinMultEnter )

    EVT_CHECKBOX( ID_CHECKBOX_METAED_IRREDUCIBLE, MetaEdGUI::OnCheckboxMetaedIrreducibleClick )

    EVT_BUTTON( ID_BUTTON_METAED_THEORY, MetaEdGUI::OnButtonMetaedTheoryClick )

    EVT_BUTTON( ID_BUTTON_METAED_RUNTYPE, MetaEdGUI::OnButtonMetaedRuntypeClick )

    EVT_BUTTON( ID_BUTTON_METAED_CONSTRAINT, MetaEdGUI::OnButtonMetaedPotentialClick )

    EVT_BUTTON( ID_BUTTON_METAED_QMMM, MetaEdGUI::OnButtonMetaedQmmmClick )

    EVT_BUTTON( ID_BUTTON_METAED_FINAL_EDIT, MetaEdGUI::OnButtonMetaedFinalEditClick )

    EVT_BUTTON( ID_BUTTON_METAED_LAUNCH, MetaEdGUI::OnButtonMetaedLaunchClick )

////@end MetaEdGUI event table entries

END_EVENT_TABLE()

/*!
 * MetaEdGUI constructors
 */

MetaEdGUI::MetaEdGUI( )
{
}

MetaEdGUI::MetaEdGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * MetaEdGUI creator
 */

bool MetaEdGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin MetaEdGUI member initialisation
    p_detailsBox = NULL;
    p_theoryDetailsSizer = NULL;
    p_runtypeDetailsSizer = NULL;
////@end MetaEdGUI member initialisation

////@begin MetaEdGUI creation
    SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
////@end MetaEdGUI creation
    return true;
}

/*!
 * Control creation for MetaEdGUI
 */

void MetaEdGUI::CreateControls()
{    
////@begin MetaEdGUI content construction
    MetaEdGUI* itemFrame1 = this;

    ewxMenuBar* menuBar = new ewxMenuBar;
    wxMenu* itemMenu3 = new wxMenu;
    itemMenu3->Append(wxID_SAVE, _("Save\tCtrl+s"), _T(""), wxITEM_NORMAL);
    itemMenu3->AppendSeparator();
    itemMenu3->Append(wxID_EXIT, _("Quit\tCtrl+q"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu3, _("File"));
    wxMenu* itemMenu7 = new wxMenu;
    itemMenu7->Append(wxID_HELP, _("on Metadynamics Editor..."), _T(""), wxITEM_NORMAL);
    itemMenu7->Append(ID_MENU_FEEDBACK, _("Support..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu7, _("Help"));
    itemFrame1->SetMenuBar(menuBar);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(itemBoxSizer10);

    wxStaticBox* itemStaticBoxSizer11Static = new wxStaticBox(itemFrame1, wxID_ANY, _("Chemical System"));
    wxStaticBoxSizer* itemStaticBoxSizer11 = new wxStaticBoxSizer(itemStaticBoxSizer11Static, wxHORIZONTAL);
    itemBoxSizer10->Add(itemStaticBoxSizer11, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    ewxButton* itemButton12 = new ewxButton( itemFrame1, ID_BUTTON_METAED_BUILDER, _("Builder"), wxDefaultPosition, wxSize(64, 64), 0 );
    itemStaticBoxSizer11->Add(itemButton12, 0, wxALIGN_TOP|wxALL, 5);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxVERTICAL);
    itemStaticBoxSizer11->Add(itemBoxSizer13, 0, wxALIGN_CENTER_VERTICAL, 5);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer13->Add(itemBoxSizer14, 0, wxALIGN_LEFT, 5);

    wxBoxSizer* itemBoxSizer15 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer14->Add(itemBoxSizer15, 0, wxALIGN_CENTER_VERTICAL, 5);

    ewxStaticText* itemStaticText16 = new ewxStaticText( itemFrame1, ID_STATIC_METAED_CHARGE, _("Charge:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(itemStaticText16, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemComboBox17Strings[] = {
        _("-2"),
        _("-1"),
        _("0"),
        _("1"),
        _("2")
    };
    ewxComboBox* itemComboBox17 = new ewxComboBox( itemFrame1, ID_COMBOBOX_METAED_CHARGE, _("0"), wxDefaultPosition, wxSize(50, -1), 5, itemComboBox17Strings, wxCB_DROPDOWN );
    itemComboBox17->SetStringSelection(_("0"));
    itemBoxSizer15->Add(itemComboBox17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer14->Add(itemBoxSizer18, 0, wxALIGN_CENTER_VERTICAL, 5);

    ewxStaticText* itemStaticText19 = new ewxStaticText( itemFrame1, ID_STATIC_METAED_SPIN_MULT, _("Spin Mult.:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText19, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemComboBox20Strings[] = {
        _("Singlet"),
        _("Triplet"),
        _("Quintet"),
        _("Septet"),
        _("Nontet")
    };
    ewxComboBox* itemComboBox20 = new ewxComboBox( itemFrame1, ID_COMBOBOX_METAED_SPIN_MULT, _("Singlet"), wxDefaultPosition, wxSize(75, -1), 5, itemComboBox20Strings, wxCB_DROPDOWN );
    itemComboBox20->SetStringSelection(_("Singlet"));
    itemBoxSizer18->Add(itemComboBox20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxCheckBox* itemCheckBox21 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_METAED_IRREDUCIBLE, _("Use As Irreducible\nFragment In Input"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox21->SetValue(false);
    itemBoxSizer14->Add(itemCheckBox21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer22 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer13->Add(itemBoxSizer22, 0, wxALIGN_LEFT|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer23 = new wxFlexGridSizer(0, 2, 0, 2);
    itemFlexGridSizer23->AddGrowableCol(0);
    itemBoxSizer22->Add(itemFlexGridSizer23, 0, wxALIGN_TOP, 2);

    ewxNonBoldLabel* itemStaticText24 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC, _("Atoms:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer23->Add(itemStaticText24, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText25 = new ewxStaticText( itemFrame1, ID_STATICTEXT_METAED_ATOMS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer23->Add(itemStaticText25, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    ewxNonBoldLabel* itemStaticText26 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC, _("Electrons:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer23->Add(itemStaticText26, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText27 = new ewxStaticText( itemFrame1, ID_STATICTEXT_METAED_ELECTRONS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer23->Add(itemStaticText27, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    itemBoxSizer22->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer29 = new wxFlexGridSizer(2, 2, 0, 2);
    itemBoxSizer22->Add(itemFlexGridSizer29, 0, wxALIGN_TOP, 5);

    ewxNonBoldLabel* itemStaticText30 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC, _("Symmetry:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(itemStaticText30, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText31 = new ewxStaticText( itemFrame1, ID_STATICTEXT_METAED_SYMMETRY, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(itemStaticText31, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    ewxNonBoldLabel* itemStaticText32 = new ewxNonBoldLabel( itemFrame1, wxID_STATIC, _("Open Shells:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(itemStaticText32, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText33 = new ewxStaticText( itemFrame1, ID_STATICTEXT_METAED_OPEN_SHELLS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(itemStaticText33, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 2);

    p_detailsBox = new wxStaticBox(itemFrame1, wxID_ANY, _("Theory/Runtype Settings"));
    wxStaticBoxSizer* itemStaticBoxSizer34 = new wxStaticBoxSizer(p_detailsBox, wxHORIZONTAL);
    itemBoxSizer10->Add(itemStaticBoxSizer34, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer35 = new wxBoxSizer(wxVERTICAL);
    itemStaticBoxSizer34->Add(itemBoxSizer35, 0, wxALIGN_TOP, 5);

    wxBoxSizer* itemBoxSizer36 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer35->Add(itemBoxSizer36, 0, wxALIGN_LEFT, 5);

    wxBoxSizer* itemBoxSizer37 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer36->Add(itemBoxSizer37, 0, wxALIGN_LEFT|wxLEFT, 5);

    ewxStaticText* itemStaticText38 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Theory:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer37->Add(itemStaticText38, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText39 = new ewxStaticText( itemFrame1, wxID_STATIC, _("PSPW"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer37->Add(itemStaticText39, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxButton* itemButton40 = new ewxButton( itemFrame1, ID_BUTTON_METAED_THEORY, _("Theory Details..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer36->Add(itemButton40, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    p_theoryDetailsSizer = new wxFlexGridSizer(0, 2, 0, 0);
    itemBoxSizer35->Add(p_theoryDetailsSizer, 0, wxALIGN_LEFT|wxLEFT, 5);

    itemStaticBoxSizer34->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer43 = new wxBoxSizer(wxVERTICAL);
    itemStaticBoxSizer34->Add(itemBoxSizer43, 0, wxALIGN_TOP, 5);

    wxBoxSizer* itemBoxSizer44 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer43->Add(itemBoxSizer44, 0, wxALIGN_LEFT, 5);

    wxBoxSizer* itemBoxSizer45 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer44->Add(itemBoxSizer45, 0, wxALIGN_LEFT|wxLEFT, 5);

    ewxStaticText* itemStaticText46 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Runtype:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer45->Add(itemStaticText46, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText47 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Car-Parrinello"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer45->Add(itemStaticText47, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer48 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer44->Add(itemBoxSizer48, 0, wxALIGN_LEFT, 5);

    ewxButton* itemButton49 = new ewxButton( itemFrame1, ID_BUTTON_METAED_RUNTYPE, _("Runtype Details..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemButton49, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    p_runtypeDetailsSizer = new wxFlexGridSizer(0, 2, 0, 0);
    itemBoxSizer43->Add(p_runtypeDetailsSizer, 0, wxALIGN_LEFT|wxLEFT, 5);

    wxBoxSizer* itemBoxSizer51 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer10->Add(itemBoxSizer51, 0, wxGROW|wxRIGHT|wxBOTTOM, 5);

    ewxButton* itemButton52 = new ewxButton( itemFrame1, ID_BUTTON_METAED_CONSTRAINT, _("Metadynamics Potentials..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer51->Add(itemButton52, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    ewxButton* itemButton53 = new ewxButton( itemFrame1, ID_BUTTON_METAED_QMMM, _("QM/MM Potentials..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer51->Add(itemButton53, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    ewxButton* itemButton54 = new ewxButton( itemFrame1, ID_BUTTON_METAED_FINAL_EDIT, _("Final Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer51->Add(itemButton54, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    ewxButton* itemButton55 = new ewxButton( itemFrame1, ID_BUTTON_METAED_LAUNCH, _("Launch..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer51->Add(itemButton55, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    ewxStaticLine* itemStaticLine56 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer10->Add(itemStaticLine56, 0, wxGROW|wxBOTTOM, 5);

////@end MetaEdGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_METAED_FRAME
 */

void MetaEdGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_METAED_FRAME in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_METAED_FRAME in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
 */

void MetaEdGUI::OnSaveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 */

void MetaEdGUI::OnExitClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
 */

void MetaEdGUI::OnHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK
 */

void MetaEdGUI::OnMenuFeedbackClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_METAED_CHARGE
 */

void MetaEdGUI::OnComboboxMetaedChargeSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_METAED_CHARGE in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_METAED_CHARGE in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_METAED_CHARGE
 */

void MetaEdGUI::OnComboboxMetaedChargeEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_METAED_CHARGE in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_METAED_CHARGE in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_METAED_SPIN_MULT
 */

void MetaEdGUI::OnComboboxMetaedSpinMultSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_METAED_SPIN_MULT in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_METAED_SPIN_MULT in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_METAED_SPIN_MULT
 */

void MetaEdGUI::OnComboboxMetaedSpinMultEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_METAED_SPIN_MULT in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_METAED_SPIN_MULT in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_METAED_IRREDUCIBLE
 */

void MetaEdGUI::OnCheckboxMetaedIrreducibleClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_METAED_IRREDUCIBLE in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_METAED_IRREDUCIBLE in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_THEORY
 */

void MetaEdGUI::OnButtonMetaedTheoryClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_THEORY in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_THEORY in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_RUNTYPE
 */

void MetaEdGUI::OnButtonMetaedRuntypeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_RUNTYPE in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_RUNTYPE in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_CONSTRAINT
 */

void MetaEdGUI::OnButtonMetaedPotentialClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_CONSTRAINT in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_CONSTRAINT in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_QMMM
 */

void MetaEdGUI::OnButtonMetaedQmmmClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_QMMM in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_QMMM in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_FINAL_EDIT
 */

void MetaEdGUI::OnButtonMetaedFinalEditClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_FINAL_EDIT in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_FINAL_EDIT in MetaEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_LAUNCH
 */

void MetaEdGUI::OnButtonMetaedLaunchClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_LAUNCH in MetaEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_METAED_LAUNCH in MetaEdGUI. 
}

/*!
 * Should we show tooltips?
 */

bool MetaEdGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap MetaEdGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin MetaEdGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end MetaEdGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon MetaEdGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin MetaEdGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end MetaEdGUI icon retrieval
}
