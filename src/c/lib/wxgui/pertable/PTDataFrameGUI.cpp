/////////////////////////////////////////////////////////////////////////////
// Name:        PTDataFrameGUI.C
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "PTDataFrameGUI.H"
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
#include "wxgui/ewxGrid.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxFrame.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxgui/PTDataFrameGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID PTDataFrameGUI::ID_PTEDITCOLOR_BUTTON = wxNewId();
const wxWindowID PTDataFrameGUI::ID_PTRESETALLRADII_BUTTON = wxNewId();
const wxWindowID PTDataFrameGUI::ID_NOTEBOOK = wxNewId();
const wxWindowID PTDataFrameGUI::ID_PTDATA_POTENTPANEL = wxNewId();
const wxWindowID PTDataFrameGUI::ID_PTDATAFRAMEGUI = wxNewId();
const wxWindowID PTDataFrameGUI::ID_GRID = wxNewId();
const wxWindowID PTDataFrameGUI::ID_PANEL4 = wxNewId();
const wxWindowID PTDataFrameGUI::ID_PTDATA_COLORPANEL = wxNewId();
const wxWindowID PTDataFrameGUI::ID_BFT_INPUT = wxNewId();
const wxWindowID PTDataFrameGUI::ID_COVRAD_INPUT = wxNewId();
const wxWindowID PTDataFrameGUI::ID_PANEL = wxNewId();
const wxWindowID PTDataFrameGUI::ID_CUSTOMCOLOR_PANEL = wxNewId();
const wxWindowID PTDataFrameGUI::ID_VDWRAD_INPUT = wxNewId();
const wxWindowID PTDataFrameGUI::ID_DEFAULTCOLOR_PANEL = wxNewId();
const wxWindowID PTDataFrameGUI::ID_PTDATA_HELP = wxNewId();
const wxWindowID PTDataFrameGUI::ID_PTDATA_REFPANEL = wxNewId();
const wxWindowID PTDataFrameGUI::ID_PTDATA_CLOSE = wxNewId();
const wxWindowID PTDataFrameGUI::ID_PTRESETALLCOLOR_BUTTON = wxNewId();
const wxWindowID PTDataFrameGUI::ID_PTRESETRADII_BUTTON = wxNewId();
const wxWindowID PTDataFrameGUI::ID_PTRESETCOLOR_BUTTON = wxNewId();

/*!
 * PTDataFrameGUI type definition
 */

IMPLEMENT_CLASS( PTDataFrameGUI, ewxFrame )

/*!
 * PTDataFrameGUI event table definition
 */

BEGIN_EVENT_TABLE( PTDataFrameGUI, ewxFrame )

////@begin PTDataFrameGUI event table entries
    EVT_CLOSE( PTDataFrameGUI::OnCloseWindow )

    EVT_BUTTON( ID_PTEDITCOLOR_BUTTON, PTDataFrameGUI::OnEditColorClicked )

    EVT_BUTTON( ID_PTRESETCOLOR_BUTTON, PTDataFrameGUI::OnResetColorClicked )

    EVT_BUTTON( ID_PTRESETALLCOLOR_BUTTON, PTDataFrameGUI::OnResetAllColorClicked )

    EVT_TEXT_ENTER( ID_COVRAD_INPUT, PTDataFrameGUI::OnRadiiUpdated )

    EVT_TEXT_ENTER( ID_VDWRAD_INPUT, PTDataFrameGUI::OnRadiiUpdated )

    EVT_TEXT_ENTER( ID_BFT_INPUT, PTDataFrameGUI::OnRadiiUpdated )

    EVT_BUTTON( ID_PTRESETRADII_BUTTON, PTDataFrameGUI::OnResetRadiiClicked )

    EVT_BUTTON( ID_PTRESETALLRADII_BUTTON, PTDataFrameGUI::OnResetAllRadiiClicked )

    EVT_BUTTON( ID_PTDATA_CLOSE, PTDataFrameGUI::OnCloseClicked )

////@end PTDataFrameGUI event table entries

END_EVENT_TABLE()

/*!
 * PTDataFrameGUI constructors
 */

PTDataFrameGUI::PTDataFrameGUI( )
{
}

PTDataFrameGUI::PTDataFrameGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * PTDataFrameGUI creator
 */

bool PTDataFrameGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin PTDataFrameGUI member initialisation
    p_dataPanel = NULL;
    p_atomicNumLabel = NULL;
    p_atomicWeightLabel = NULL;
    p_atomicSymbolLabel = NULL;
    p_electronConfigLabel = NULL;
    p_elementNameLabel = NULL;
    p_electronAffinityLabel = NULL;
    p_refLabel = NULL;
    p_isotopeSizer = NULL;
    p_isoTableLabel = NULL;
    p_ionPotentGrid = NULL;
    p_ref2Label = NULL;
    p_preferencePanel = NULL;
    p_customColorPanel = NULL;
    p_editColorButton = NULL;
    p_defaultColorPanel = NULL;
    p_resetColorButton = NULL;
    p_resetAllColorButton = NULL;
    p_covRadLabel = NULL;
    p_covRadInput = NULL;
    p_defCovRadLabel = NULL;
    p_vdwRadLabel = NULL;
    p_vdwRadInput = NULL;
    p_defVdwRadLabel = NULL;
    p_bftLabel = NULL;
    p_bftInput = NULL;
    p_defBftLabel = NULL;
    p_resetRadiiButton = NULL;
    p_resetAllRadiiButton = NULL;
////@end PTDataFrameGUI member initialisation

////@begin PTDataFrameGUI creation
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    Centre();
////@end PTDataFrameGUI creation
    return true;
}

/*!
 * Control creation for PTDataFrameGUI
 */

void PTDataFrameGUI::CreateControls()
{    
////@begin PTDataFrameGUI content construction
    PTDataFrameGUI* itemFrame1 = this;

    ewxPanel* itemPanel2 = new ewxPanel( itemFrame1, ID_PANEL4, wxDefaultPosition, wxSize(100, 80), wxNO_BORDER|wxTAB_TRAVERSAL );

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemPanel2->SetSizer(itemBoxSizer3);

    ewxNotebook* itemNotebook4 = new ewxNotebook( itemPanel2, ID_NOTEBOOK, wxDefaultPosition, wxSize(200, 200), wxNB_TOP );

    p_dataPanel = new ewxPanel( itemNotebook4, ID_PTDATA_REFPANEL, wxDefaultPosition, wxSize(100, 80), wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    p_dataPanel->SetSizer(itemBoxSizer6);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer6->Add(itemBoxSizer7, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);
    ewxPanel* itemPanel8 = new ewxPanel( p_dataPanel, ID_PANEL, wxDefaultPosition, wxSize(150, 150), wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer7->Add(itemPanel8, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 20);
    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxVERTICAL);
    itemPanel8->SetSizer(itemBoxSizer9);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer9->Add(itemBoxSizer10, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);
    p_atomicNumLabel = new ewxStaticText( itemPanel8, wxID_STATIC, _("-1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(p_atomicNumLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    itemBoxSizer10->Add(40, 0, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    p_atomicWeightLabel = new ewxStaticText( itemPanel8, wxID_STATIC, _("-1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(p_atomicWeightLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    itemBoxSizer9->Add(0, 10, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 0);

    p_atomicSymbolLabel = new ewxStaticText( itemPanel8, wxID_STATIC, _("-1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(p_atomicSymbolLabel, 0, wxALIGN_RIGHT|wxALL|wxADJUST_MINSIZE, 10);

    p_electronConfigLabel = new ewxStaticText( itemPanel8, wxID_STATIC, _("-1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(p_electronConfigLabel, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 0);

    p_elementNameLabel = new ewxStaticText( itemPanel8, wxID_STATIC, _("-1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(p_elementNameLabel, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    p_electronAffinityLabel = new ewxStaticText( p_dataPanel, wxID_STATIC, _("-1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(p_electronAffinityLabel, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    p_refLabel = new ewxStaticText( p_dataPanel, wxID_STATIC, _("Electron Affinities from \"The Elements\"\nBy John Emsley Oxford University Press.\n*by convention electron affinities\nare positive for energy released."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(p_refLabel, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticLine* itemStaticLine20 = new ewxStaticLine( p_dataPanel, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
    itemBoxSizer6->Add(itemStaticLine20, 0, wxGROW|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    p_isotopeSizer = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer6->Add(p_isotopeSizer, 1, wxGROW|wxALL|wxADJUST_MINSIZE, 10);
    p_isoTableLabel = new ewxStaticText( p_dataPanel, wxID_STATIC, _("-1"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    p_isotopeSizer->Add(p_isoTableLabel, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    itemNotebook4->AddPage(p_dataPanel, _("Experimental Reference Data"));

    ewxPanel* itemPanel23 = new ewxPanel( itemNotebook4, ID_PTDATA_POTENTPANEL, wxDefaultPosition, wxSize(100, 80), wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxVERTICAL);
    itemPanel23->SetSizer(itemBoxSizer24);

    p_ionPotentGrid = new ewxGrid( itemPanel23, ID_GRID, wxDefaultPosition, wxSize(200, 150), wxNO_BORDER|wxHSCROLL|wxVSCROLL );
    p_ionPotentGrid->SetDefaultColSize(50);
    p_ionPotentGrid->SetDefaultRowSize(25);
    p_ionPotentGrid->SetColLabelSize(25);
    p_ionPotentGrid->SetRowLabelSize(50);
    p_ionPotentGrid->CreateGrid(95, 21, wxGrid::wxGridSelectCells);
    itemBoxSizer24->Add(p_ionPotentGrid, 1, wxGROW|wxALL, 5);

    p_ref2Label = new ewxStaticText( itemPanel23, wxID_STATIC, _("Numerical values in this table are expressed in electron volts.\nReference: C.E. Moore, Ionization Potentials and Ionization Limits Derived from the Analysis of Optical Spectra, \nNat.Stand. Ref. Data Ser.-Nat. Bur. Stand.(U.S.), No. 34, 1970."), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE|wxNO_BORDER );
    itemBoxSizer24->Add(p_ref2Label, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    itemNotebook4->AddPage(itemPanel23, _("Ionization Potentials"));

    p_preferencePanel = new ewxPanel( itemNotebook4, ID_PTDATA_COLORPANEL, wxDefaultPosition, wxSize(100, 80), wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer28 = new wxBoxSizer(wxVERTICAL);
    p_preferencePanel->SetSizer(itemBoxSizer28);

    wxFlexGridSizer* itemFlexGridSizer29 = new wxFlexGridSizer(3, 4, 0, 5);
    itemBoxSizer28->Add(itemFlexGridSizer29, 0, wxGROW|wxALL, 10);
    ewxStaticText* itemStaticText30 = new ewxStaticText( p_preferencePanel, wxID_STATIC, _("Color Preference:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(itemStaticText30, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    itemFlexGridSizer29->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer29->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer29->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText34 = new ewxStaticText( p_preferencePanel, wxID_STATIC, _("Custom Color"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(itemStaticText34, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 10);

    p_customColorPanel = new ewxPanel( p_preferencePanel, ID_CUSTOMCOLOR_PANEL, wxDefaultPosition, wxSize(50, 20), wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer29->Add(p_customColorPanel, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_editColorButton = new ewxButton( p_preferencePanel, ID_PTEDITCOLOR_BUTTON, _("Edit Color"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(p_editColorButton, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 20);

    itemFlexGridSizer29->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText38 = new ewxStaticText( p_preferencePanel, wxID_STATIC, _("Default Color"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(itemStaticText38, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 10);

    p_defaultColorPanel = new ewxPanel( p_preferencePanel, ID_DEFAULTCOLOR_PANEL, wxDefaultPosition, wxSize(50, 20), wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer29->Add(p_defaultColorPanel, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_resetColorButton = new ewxButton( p_preferencePanel, ID_PTRESETCOLOR_BUTTON, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(p_resetColorButton, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 20);

    p_resetAllColorButton = new ewxButton( p_preferencePanel, ID_PTRESETALLCOLOR_BUTTON, _("Reset Table Colors"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer29->Add(p_resetAllColorButton, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    ewxStaticLine* itemStaticLine42 = new ewxStaticLine( p_preferencePanel, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer28->Add(itemStaticLine42, 0, wxGROW|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    wxFlexGridSizer* itemFlexGridSizer43 = new wxFlexGridSizer(6, 4, 0, 5);
    itemBoxSizer28->Add(itemFlexGridSizer43, 0, wxGROW|wxALL, 10);
    ewxStaticText* itemStaticText44 = new ewxStaticText( p_preferencePanel, wxID_STATIC, _("Radii Preference:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(itemStaticText44, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    itemFlexGridSizer43->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer43->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer43->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_covRadLabel = new ewxStaticText( p_preferencePanel, wxID_STATIC, _("Covalent Radius"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(p_covRadLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    p_covRadInput = new ewxTextCtrl( p_preferencePanel, ID_COVRAD_INPUT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(p_covRadInput, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    ewxStaticText* itemStaticText50 = new ewxStaticText( p_preferencePanel, wxID_STATIC, _("Angstrom"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(itemStaticText50, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    p_defCovRadLabel = new ewxStaticText( p_preferencePanel, wxID_STATIC, _("(Default=-1)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(p_defCovRadLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    p_vdwRadLabel = new ewxStaticText( p_preferencePanel, wxID_STATIC, _("van der Waals Radius"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(p_vdwRadLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    p_vdwRadInput = new ewxTextCtrl( p_preferencePanel, ID_VDWRAD_INPUT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(p_vdwRadInput, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    ewxStaticText* itemStaticText54 = new ewxStaticText( p_preferencePanel, wxID_STATIC, _("Angstrom"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(itemStaticText54, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    p_defVdwRadLabel = new ewxStaticText( p_preferencePanel, wxID_STATIC, _("(Default=-1)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(p_defVdwRadLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    p_bftLabel = new ewxStaticText( p_preferencePanel, wxID_STATIC, _("Bond Formation Tolerance"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(p_bftLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    p_bftInput = new ewxTextCtrl( p_preferencePanel, ID_BFT_INPUT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(p_bftInput, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    ewxStaticText* itemStaticText58 = new ewxStaticText( p_preferencePanel, wxID_STATIC, _("Angstrom"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(itemStaticText58, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    p_defBftLabel = new ewxStaticText( p_preferencePanel, wxID_STATIC, _("(Default=-1)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(p_defBftLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    itemFlexGridSizer43->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer43->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_resetRadiiButton = new ewxButton( p_preferencePanel, ID_PTRESETRADII_BUTTON, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(p_resetRadiiButton, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_resetAllRadiiButton = new ewxButton( p_preferencePanel, ID_PTRESETALLRADII_BUTTON, _("Reset Table Radii"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(p_resetAllRadiiButton, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemNotebook4->AddPage(p_preferencePanel, _("Element Preferences"));

    itemBoxSizer3->Add(itemNotebook4, 1, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer64 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer64, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    itemBoxSizer64->Add(0, 0, 1, wxGROW|wxALL|wxADJUST_MINSIZE, 0);

    ewxButton* itemButton66 = new ewxButton( itemPanel2, ID_PTDATA_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer64->Add(itemButton66, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    itemBoxSizer64->Add(0, 0, 1, wxGROW|wxALL|wxADJUST_MINSIZE, 0);

    ewxButton* itemButton68 = new ewxButton( itemPanel2, ID_PTDATA_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer64->Add(itemButton68, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    itemBoxSizer64->Add(0, 0, 1, wxGROW|wxALL|wxADJUST_MINSIZE, 0);

////@end PTDataFrameGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_PTDATAFRAMEGUI
 */

void PTDataFrameGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_PTDATAFRAMEGUI in PTDataFrameGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_PTDATAFRAMEGUI in PTDataFrameGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTEDITCOLOR_BUTTON
 */

void PTDataFrameGUI::OnEditColorClicked( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTEDITCOLOR_BUTTON in PTDataFrameGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTEDITCOLOR_BUTTON in PTDataFrameGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETCOLOR_BUTTON
 */

void PTDataFrameGUI::OnResetColorClicked( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETCOLOR_BUTTON in PTDataFrameGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETCOLOR_BUTTON in PTDataFrameGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETALLCOLOR_BUTTON
 */

void PTDataFrameGUI::OnResetAllColorClicked( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETALLCOLOR_BUTTON in PTDataFrameGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETALLCOLOR_BUTTON in PTDataFrameGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_COVRAD_INPUT
 */

void PTDataFrameGUI::OnRadiiUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_COVRAD_INPUT in PTDataFrameGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_COVRAD_INPUT in PTDataFrameGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETRADII_BUTTON
 */

void PTDataFrameGUI::OnResetRadiiClicked( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETRADII_BUTTON in PTDataFrameGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETRADII_BUTTON in PTDataFrameGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETALLRADII_BUTTON
 */

void PTDataFrameGUI::OnResetAllRadiiClicked( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETALLRADII_BUTTON in PTDataFrameGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTRESETALLRADII_BUTTON in PTDataFrameGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTDATA_CLOSE
 */

void PTDataFrameGUI::OnCloseClicked( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTDATA_CLOSE in PTDataFrameGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PTDATA_CLOSE in PTDataFrameGUI. 
}

/*!
 * Should we show tooltips?
 */

bool PTDataFrameGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap PTDataFrameGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin PTDataFrameGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end PTDataFrameGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon PTDataFrameGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin PTDataFrameGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end PTDataFrameGUI icon retrieval
}
