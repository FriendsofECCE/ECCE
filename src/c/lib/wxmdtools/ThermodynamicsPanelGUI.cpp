/////////////////////////////////////////////////////////////////////////////
// Name:        ThermodynamicsPanelGUI.C
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ThermodynamicsPanelGUI.H"
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
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxgui/ThermodynamicsPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID ThermodynamicsPanelGUI::ID_CHECKBOX_THERM_INTEG = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_TEXTCTRL_THERM_PMF_HARMONIC = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_SPINCTRL_THERM_INTEG_ENSEMBLES_NUM = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_TEXTCTRL_THERM_INTEG_ERROR = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_TEXTCTRL_THERM_PMF_SCALED = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_DIALOG_THERM = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_RADIOBUTTON_THERM_PMF_SCALED = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_PANEL_THERM_INTEG = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_PANEL_THERM_PMF = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_RADIOBUTTON_THERM_PMF_HARMONIC = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::wxID_STATIC_THERM_INTEG_ERROR = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_SPINCTRL_THERM_INTEG_ENSEMBLES_MAX = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_RADIOBUTTON_THERM_PMF_STANDARD = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_RADIOBUTTON_THERM_INTEG_NEW = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_TEXTCTRL_THERM_INTEG_DRIFT = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_RADIOBUTTON_THERM_INTEG_EXTEND = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::wxID_STATIC_THERM_INTEG_DRIFT = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_CHECKBOX_THERM_PMF = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_RADIOBUTTON_THERM_INTEG_RENEW = wxNewId();
const wxWindowID ThermodynamicsPanelGUI::ID_CHOICE_THERM_INTEG_DIRECTION = wxNewId();

/*!
 * ThermodynamicsPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( ThermodynamicsPanelGUI, ewxPanel )

/*!
 * ThermodynamicsPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( ThermodynamicsPanelGUI, ewxPanel )

////@begin ThermodynamicsPanelGUI event table entries
    EVT_CHECKBOX( ID_CHECKBOX_THERM_PMF, ThermodynamicsPanelGUI::OnCheckboxThermPmfClick )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_THERM_PMF_STANDARD, ThermodynamicsPanelGUI::OnRadiobuttonThermPmfStandardSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_THERM_PMF_SCALED, ThermodynamicsPanelGUI::OnRadiobuttonThermPmfScaledSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_THERM_PMF_SCALED, ThermodynamicsPanelGUI::OnTextctrlThermPmfScaledEnter )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_THERM_PMF_HARMONIC, ThermodynamicsPanelGUI::OnRadiobuttonThermPmfHarmonicSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_THERM_PMF_HARMONIC, ThermodynamicsPanelGUI::OnTextctrlThermPmfHarmonicEnter )

    EVT_CHECKBOX( ID_CHECKBOX_THERM_INTEG, ThermodynamicsPanelGUI::OnCheckboxThermIntegClick )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_THERM_INTEG_NEW, ThermodynamicsPanelGUI::OnRadiobuttonThermIntegNewSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_THERM_INTEG_RENEW, ThermodynamicsPanelGUI::OnRadiobuttonThermIntegRenewSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_THERM_INTEG_EXTEND, ThermodynamicsPanelGUI::OnRadiobuttonThermIntegExtendSelected )

    EVT_CHOICE( ID_CHOICE_THERM_INTEG_DIRECTION, ThermodynamicsPanelGUI::OnChoiceThermIntegDirectionSelected )

    EVT_SPINCTRL( ID_SPINCTRL_THERM_INTEG_ENSEMBLES_NUM, ThermodynamicsPanelGUI::OnSpinctrlThermIntegEnsemblesNumUpdated )

    EVT_SPINCTRL( ID_SPINCTRL_THERM_INTEG_ENSEMBLES_MAX, ThermodynamicsPanelGUI::OnSpinctrlThermIntegEnsemblesMaxUpdated )

    EVT_TEXT_ENTER( ID_TEXTCTRL_THERM_INTEG_ERROR, ThermodynamicsPanelGUI::OnTextctrlThermIntegErrorEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_THERM_INTEG_DRIFT, ThermodynamicsPanelGUI::OnTextctrlThermIntegDriftEnter )

////@end ThermodynamicsPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * ThermodynamicsPanelGUI constructors
 */

ThermodynamicsPanelGUI::ThermodynamicsPanelGUI( )
{
}

ThermodynamicsPanelGUI::ThermodynamicsPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * ThermodynamicsPanelGUI creator
 */

bool ThermodynamicsPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ThermodynamicsPanelGUI member initialisation
////@end ThermodynamicsPanelGUI member initialisation

////@begin ThermodynamicsPanelGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ThermodynamicsPanelGUI creation
    return true;
}

/*!
 * Control creation for ThermodynamicsPanelGUI
 */

void ThermodynamicsPanelGUI::CreateControls()
{    
////@begin ThermodynamicsPanelGUI content construction
    ThermodynamicsPanelGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW, 5);

    ewxCheckBox* itemCheckBox4 = new ewxCheckBox( itemPanel1, ID_CHECKBOX_THERM_PMF, _("Use Potential Mean Force"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox4->SetValue(false);
    itemBoxSizer3->Add(itemCheckBox4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer5, 0, wxGROW, 5);

    ewxPanel* itemPanel6 = new ewxPanel( itemPanel1, ID_PANEL_THERM_PMF, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer5->Add(itemPanel6, 1, wxALIGN_CENTER_VERTICAL, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxVERTICAL);
    itemPanel6->SetSizer(itemBoxSizer7);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer7->Add(itemBoxSizer8, 0, wxGROW, 5);

    ewxRadioButton* itemRadioButton9 = new ewxRadioButton( itemPanel6, ID_RADIOBUTTON_THERM_PMF_STANDARD, _("Standard"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton9->SetValue(false);
    if (ShowToolTips())
        itemRadioButton9->SetToolTip(_("Any PMF force functions defined in the topology files are to be used"));
    itemBoxSizer8->Add(itemRadioButton9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxRadioButton* itemRadioButton10 = new ewxRadioButton( itemPanel6, ID_RADIOBUTTON_THERM_PMF_SCALED, _("Scaled"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton10->SetValue(false);
    if (ShowToolTips())
        itemRadioButton10->SetToolTip(_("All PMF force constraints are scaled by the given factor"));
    itemBoxSizer8->Add(itemRadioButton10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxTextCtrl* itemTextCtrl11 = new ewxTextCtrl( itemPanel6, ID_TEXTCTRL_THERM_PMF_SCALED, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemTextCtrl11->SetToolTip(_("amount by which all pmf force constraints are scaled"));
    itemBoxSizer8->Add(itemTextCtrl11, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxRadioButton* itemRadioButton12 = new ewxRadioButton( itemPanel6, ID_RADIOBUTTON_THERM_PMF_HARMONIC, _("Harmonic"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton12->SetValue(false);
    if (ShowToolTips())
        itemRadioButton12->SetToolTip(_("The initial dynamics steps will use a harmonic potential instead of any PMF constraint"));
    itemBoxSizer8->Add(itemRadioButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxTextCtrl* itemTextCtrl13 = new ewxTextCtrl( itemPanel6, ID_TEXTCTRL_THERM_PMF_HARMONIC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemTextCtrl13->SetToolTip(_("How many initial dynamics steps will use a harmonic potential instead of any PMF constraint"));
    itemBoxSizer8->Add(itemTextCtrl13, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer14, 0, wxGROW, 5);

    ewxCheckBox* itemCheckBox15 = new ewxCheckBox( itemPanel1, ID_CHECKBOX_THERM_INTEG, _("Use Thermodynamic Integration"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox15->SetValue(false);
    itemBoxSizer14->Add(itemCheckBox15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer16, 0, wxGROW, 5);

    ewxPanel* itemPanel17 = new ewxPanel( itemPanel1, ID_PANEL_THERM_INTEG, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer16->Add(itemPanel17, 1, wxALIGN_CENTER_VERTICAL, 5);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxVERTICAL);
    itemPanel17->SetSizer(itemBoxSizer18);

    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer18->Add(itemBoxSizer19, 0, wxGROW, 5);

    ewxStaticText* itemStaticText20 = new ewxStaticText( itemPanel17, wxID_STATIC, _("Initial Conditions:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemStaticText20, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxRadioButton* itemRadioButton21 = new ewxRadioButton( itemPanel17, ID_RADIOBUTTON_THERM_INTEG_NEW, _("New"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton21->SetValue(false);
    if (ShowToolTips())
        itemRadioButton21->SetToolTip(_("this is the initial multi-configuration thermodynamics integration"));
    itemBoxSizer19->Add(itemRadioButton21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxRadioButton* itemRadioButton22 = new ewxRadioButton( itemPanel17, ID_RADIOBUTTON_THERM_INTEG_RENEW, _("Renew"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton22->SetValue(false);
    if (ShowToolTips())
        itemRadioButton22->SetToolTip(_("obtain the initial conditions for each lambda from the mro-file from a previous multi-configuration thermodynamics integration"));
    itemBoxSizer19->Add(itemRadioButton22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxRadioButton* itemRadioButton23 = new ewxRadioButton( itemPanel17, ID_RADIOBUTTON_THERM_INTEG_EXTEND, _("Extend"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton23->SetValue(false);
    if (ShowToolTips())
        itemRadioButton23->SetToolTip(_("extend a previous multi-configuration calculation from the data read from an mri-file"));
    itemBoxSizer19->Add(itemRadioButton23, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer18->Add(itemBoxSizer24, 0, wxGROW, 5);

    wxString itemChoice25Strings[] = {
        _("forward"),
        _("reverse")
    };
    ewxChoice* itemChoice25 = new ewxChoice( itemPanel17, ID_CHOICE_THERM_INTEG_DIRECTION, wxDefaultPosition, wxDefaultSize, 2, itemChoice25Strings, 0 );
    itemChoice25->SetStringSelection(_("forward"));
    if (ShowToolTips())
        itemChoice25->SetToolTip(_("direction of integration steps"));
    itemBoxSizer24->Add(itemChoice25, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxSpinCtrl* itemSpinCtrl26 = new ewxSpinCtrl( itemPanel17, ID_SPINCTRL_THERM_INTEG_ENSEMBLES_NUM, _("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0 );
    itemBoxSizer24->Add(itemSpinCtrl26, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText27 = new ewxStaticText( itemPanel17, wxID_STATIC, _("ensembles out of"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemStaticText27, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxSpinCtrl* itemSpinCtrl28 = new ewxSpinCtrl( itemPanel17, ID_SPINCTRL_THERM_INTEG_ENSEMBLES_MAX, _("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0 );
    itemBoxSizer24->Add(itemSpinCtrl28, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer29 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer18->Add(itemBoxSizer29, 0, wxGROW, 5);

    ewxStaticText* itemStaticText30 = new ewxStaticText( itemPanel17, wxID_STATIC, _("Max Statistical Error for each Ensemble:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer29->Add(itemStaticText30, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl31 = new ewxTextCtrl( itemPanel17, ID_TEXTCTRL_THERM_INTEG_ERROR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer29->Add(itemTextCtrl31, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText32 = new ewxStaticText( itemPanel17, wxID_STATIC_THERM_INTEG_ERROR, _("kJ/mol"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer29->Add(itemStaticText32, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer33 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer18->Add(itemBoxSizer33, 0, wxGROW, 5);

    ewxStaticText* itemStaticText34 = new ewxStaticText( itemPanel17, wxID_STATIC, _("Max Drift in the Free Energy Result:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer33->Add(itemStaticText34, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl35 = new ewxTextCtrl( itemPanel17, ID_TEXTCTRL_THERM_INTEG_DRIFT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer33->Add(itemTextCtrl35, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText36 = new ewxStaticText( itemPanel17, wxID_STATIC_THERM_INTEG_DRIFT, _("kJ/mol/ps"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer33->Add(itemStaticText36, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

////@end ThermodynamicsPanelGUI content construction
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_THERM_PMF
 */

void ThermodynamicsPanelGUI::OnCheckboxThermPmfClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_THERM_PMF in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_THERM_PMF in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_PMF_STANDARD
 */

void ThermodynamicsPanelGUI::OnRadiobuttonThermPmfStandardSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_PMF_STANDARD in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_PMF_STANDARD in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_PMF_SCALED
 */

void ThermodynamicsPanelGUI::OnRadiobuttonThermPmfScaledSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_PMF_SCALED in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_PMF_SCALED in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_PMF_SCALED
 */

void ThermodynamicsPanelGUI::OnTextctrlThermPmfScaledEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_PMF_SCALED in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_PMF_SCALED in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_PMF_HARMONIC
 */

void ThermodynamicsPanelGUI::OnRadiobuttonThermPmfHarmonicSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_PMF_HARMONIC in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_PMF_HARMONIC in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_PMF_HARMONIC
 */

void ThermodynamicsPanelGUI::OnTextctrlThermPmfHarmonicEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_PMF_HARMONIC in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_PMF_HARMONIC in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_THERM_INTEG
 */

void ThermodynamicsPanelGUI::OnCheckboxThermIntegClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_THERM_INTEG in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_THERM_INTEG in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_INTEG_NEW
 */

void ThermodynamicsPanelGUI::OnRadiobuttonThermIntegNewSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_INTEG_NEW in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_INTEG_NEW in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_INTEG_RENEW
 */

void ThermodynamicsPanelGUI::OnRadiobuttonThermIntegRenewSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_INTEG_RENEW in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_INTEG_RENEW in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_INTEG_EXTEND
 */

void ThermodynamicsPanelGUI::OnRadiobuttonThermIntegExtendSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_INTEG_EXTEND in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_THERM_INTEG_EXTEND in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_THERM_INTEG_DIRECTION
 */

void ThermodynamicsPanelGUI::OnChoiceThermIntegDirectionSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_THERM_INTEG_DIRECTION in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_THERM_INTEG_DIRECTION in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_THERM_INTEG_ENSEMBLES_NUM
 */

void ThermodynamicsPanelGUI::OnSpinctrlThermIntegEnsemblesNumUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_THERM_INTEG_ENSEMBLES_NUM in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_THERM_INTEG_ENSEMBLES_NUM in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_THERM_INTEG_ENSEMBLES_MAX
 */

void ThermodynamicsPanelGUI::OnSpinctrlThermIntegEnsemblesMaxUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_THERM_INTEG_ENSEMBLES_MAX in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_THERM_INTEG_ENSEMBLES_MAX in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_INTEG_ERROR
 */

void ThermodynamicsPanelGUI::OnTextctrlThermIntegErrorEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_INTEG_ERROR in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_INTEG_ERROR in ThermodynamicsPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_INTEG_DRIFT
 */

void ThermodynamicsPanelGUI::OnTextctrlThermIntegDriftEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_INTEG_DRIFT in ThermodynamicsPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_INTEG_DRIFT in ThermodynamicsPanelGUI. 
}

/*!
 * Should we show tooltips?
 */

bool ThermodynamicsPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap ThermodynamicsPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ThermodynamicsPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end ThermodynamicsPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ThermodynamicsPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ThermodynamicsPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end ThermodynamicsPanelGUI icon retrieval
}
