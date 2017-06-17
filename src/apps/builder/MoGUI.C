/////////////////////////////////////////////////////////////////////////////
// Name:        MoGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "MoGUI.H"
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
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "MoGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID MoGUI::ID_PANEL = wxNewId();
const wxWindowID MoGUI::ID_CHOICE_MO_TYPE = wxNewId();
const wxWindowID MoGUI::ID_PANEL_MO = wxNewId();
const wxWindowID MoGUI::ID_BUTTON_MO_VIEWCOEF = wxNewId();
const wxWindowID MoGUI::ID_GRID_MO = wxNewId();
const wxWindowID MoGUI::ID_TEXTCTRL_MO_CUTOFF = wxNewId();
const wxWindowID MoGUI::ID_BUTTON_MO_COMPUTE = wxNewId();

/*!
 * MoGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( MoGUI, VizPropertyPanel )

/*!
 * MoGUI event table definition
 */

BEGIN_EVENT_TABLE( MoGUI, VizPropertyPanel )

////@begin MoGUI event table entries
    EVT_BUTTON( ID_BUTTON_MO_COMPUTE, MoGUI::OnButtonMoComputeClick )

    EVT_BUTTON( ID_BUTTON_MO_VIEWCOEF, MoGUI::OnButtonMoViewcoefClick )

////@end MoGUI event table entries

END_EVENT_TABLE()

/*!
 * MoGUI constructors
 */

MoGUI::MoGUI( )
{
}

MoGUI::MoGUI( IPropCalculation* calculation, wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(calculation, parent, id, pos, size, style);
}

/*!
 * MoGUI creator
 */

bool MoGUI::Create( IPropCalculation* calculation, wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin MoGUI member initialisation
    p_sliderSizer = NULL;
////@end MoGUI member initialisation

////@begin MoGUI creation
    VizPropertyPanel::Create( calculation, parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
////@end MoGUI creation
    return true;
}

/*!
 * Control creation for MoGUI
 */

void MoGUI::CreateControls()
{    
////@begin MoGUI content construction
    MoGUI* itemVizPropertyPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemVizPropertyPanel1->SetSizer(itemBoxSizer2);

    ewxGrid* itemGrid3 = new ewxGrid( itemVizPropertyPanel1, ID_GRID_MO, wxDefaultPosition, wxSize(200, 150), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    itemGrid3->SetDefaultColSize(50);
    itemGrid3->SetDefaultRowSize(25);
    itemGrid3->SetColLabelSize(25);
    itemGrid3->SetRowLabelSize(50);
    itemGrid3->CreateGrid(5, 5, wxGrid::wxGridSelectRows);
    itemBoxSizer2->Add(itemGrid3, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer4, 0, wxALIGN_LEFT|wxALL, 0);

    wxString itemChoice5Strings[] = {
        _("MO"),
        _("Density"),
        _("Spin Density")
    };
    ewxChoice* itemChoice5 = new ewxChoice( itemVizPropertyPanel1, ID_CHOICE_MO_TYPE, wxDefaultPosition, wxDefaultSize, 3, itemChoice5Strings, 0 );
    itemChoice5->SetStringSelection(_("MO"));
    itemBoxSizer4->Add(itemChoice5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton6 = new ewxButton( itemVizPropertyPanel1, ID_BUTTON_MO_COMPUTE, _("Compute"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemButton6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_sliderSizer = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(p_sliderSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxPanel* itemPanel8 = new ewxPanel( itemVizPropertyPanel1, ID_PANEL, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer2->Add(itemPanel8, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel8->SetSizer(itemBoxSizer9);

    ewxStaticText* itemStaticText10 = new ewxStaticText( itemPanel8, wxID_STATIC, _("Cuttoff Abs >"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemStaticText10, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl11 = new ewxTextCtrl( itemPanel8, ID_TEXTCTRL_MO_CUTOFF, _("0.001"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemTextCtrl11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton12 = new ewxButton( itemPanel8, ID_BUTTON_MO_VIEWCOEF, _("View Coeff..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(itemButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end MoGUI content construction
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MO_COMPUTE
 */

void MoGUI::OnButtonMoComputeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MO_COMPUTE in MoGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MO_COMPUTE in MoGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MO_VIEWCOEF
 */

void MoGUI::OnButtonMoViewcoefClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MO_VIEWCOEF in MoGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MO_VIEWCOEF in MoGUI. 
}

/*!
 * Should we show tooltips?
 */

bool MoGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap MoGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin MoGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end MoGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon MoGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin MoGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end MoGUI icon retrieval
}
