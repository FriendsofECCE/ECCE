/////////////////////////////////////////////////////////////////////////////
// Name:        SliceGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "SliceGUI.H"
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
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
////@end includes

#include "SliceGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID SliceGUI::ID_BUTTON_SLICE_COLOR_1 = wxNewId();
const wxWindowID SliceGUI::ID_DIALOG_SLICE = wxNewId();
const wxWindowID SliceGUI::ID_BUTTON_SLICER_XFORM = wxNewId();
const wxWindowID SliceGUI::ID_CHECKBOX_SLICE_TRANSPARENT_2 = wxNewId();
const wxWindowID SliceGUI::ID_CHECKBOX_SLICE_TRANSPARENT_1 = wxNewId();
const wxWindowID SliceGUI::ID_SPINCTRL_SLICE_OFFSET_2 = wxNewId();
const wxWindowID SliceGUI::ID_SPINCTRL_SLICE_L = wxNewId();
const wxWindowID SliceGUI::ID_PANEL_SLICE_FIELDS = wxNewId();
const wxWindowID SliceGUI::ID_SPINCTRL_SLICE_H = wxNewId();
const wxWindowID SliceGUI::ID_SPINCTRL_SLICE_OFFSET_1 = wxNewId();
const wxWindowID SliceGUI::ID_BUTTON_SLICER_SLICE = wxNewId();
const wxWindowID SliceGUI::ID_BUTTON_SLICE_COLOR_2 = wxNewId();
const wxWindowID SliceGUI::ID_SPINCTRL_SLICE_K = wxNewId();
const wxWindowID SliceGUI::ID_CHECKBOX_SHOWPLANE = wxNewId();

/*!
 * SliceGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( SliceGUI, ewxPanel )

/*!
 * SliceGUI event table definition
 */

BEGIN_EVENT_TABLE( SliceGUI, ewxPanel )

////@begin SliceGUI event table entries
    EVT_SPINCTRL( ID_SPINCTRL_SLICE_H, SliceGUI::OnSpinctrlSliceHUpdated )

    EVT_SPINCTRL( ID_SPINCTRL_SLICE_K, SliceGUI::OnSpinctrlSliceKUpdated )

    EVT_SPINCTRL( ID_SPINCTRL_SLICE_L, SliceGUI::OnSpinctrlSliceLUpdated )

    EVT_CHECKBOX( ID_CHECKBOX_SHOWPLANE, SliceGUI::OnCheckboxShowplaneClick )

    EVT_BUTTON( ID_BUTTON_SLICE_COLOR_1, SliceGUI::OnButtonSliceColor1Click )

    EVT_CHECKBOX( ID_CHECKBOX_SLICE_TRANSPARENT_1, SliceGUI::OnCheckboxSliceTransparent1Click )

    EVT_SPINCTRL( ID_SPINCTRL_SLICE_OFFSET_1, SliceGUI::OnSpinctrlSliceOffset1Updated )

    EVT_BUTTON( ID_BUTTON_SLICE_COLOR_2, SliceGUI::OnButtonSliceColor2Click )

    EVT_CHECKBOX( ID_CHECKBOX_SLICE_TRANSPARENT_2, SliceGUI::OnCheckboxSliceTransparent2Click )

    EVT_SPINCTRL( ID_SPINCTRL_SLICE_OFFSET_2, SliceGUI::OnSpinctrlSliceOffset2Updated )

    EVT_BUTTON( ID_BUTTON_SLICER_XFORM, SliceGUI::OnButtonSlicerXformClick )

    EVT_BUTTON( ID_BUTTON_SLICER_SLICE, SliceGUI::OnButtonSlicerSliceClick )

////@end SliceGUI event table entries

END_EVENT_TABLE()

/*!
 * SliceGUI constructors
 */

SliceGUI::SliceGUI( )
{
}

SliceGUI::SliceGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * SliceGUI creator
 */

bool SliceGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin SliceGUI member initialisation
////@end SliceGUI member initialisation

////@begin SliceGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end SliceGUI creation
    return true;
}

/*!
 * Control creation for SliceGUI
 */

void SliceGUI::CreateControls()
{    
////@begin SliceGUI content construction
    SliceGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_LEFT|wxALL, 3);

    ewxStaticText* itemStaticText4 = new ewxStaticText( itemPanel1, wxID_STATIC, _("h: "), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 1);

    ewxSpinCtrl* itemSpinCtrl5 = new ewxSpinCtrl( itemPanel1, ID_SPINCTRL_SLICE_H, _("1"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, -100, 100, 1 );
    itemBoxSizer3->Add(itemSpinCtrl5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    itemBoxSizer3->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemPanel1, wxID_STATIC, _("k:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 1);

    ewxSpinCtrl* itemSpinCtrl8 = new ewxSpinCtrl( itemPanel1, ID_SPINCTRL_SLICE_K, _("1"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, -100, 100, 1 );
    itemBoxSizer3->Add(itemSpinCtrl8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    itemBoxSizer3->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText10 = new ewxStaticText( itemPanel1, wxID_STATIC, _("l:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText10, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 1);

    ewxSpinCtrl* itemSpinCtrl11 = new ewxSpinCtrl( itemPanel1, ID_SPINCTRL_SLICE_L, _("1"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, -100, 100, 1 );
    itemBoxSizer3->Add(itemSpinCtrl11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    itemBoxSizer3->Add(35, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxCheckBox* itemCheckBox13 = new ewxCheckBox( itemPanel1, ID_CHECKBOX_SHOWPLANE, _("Show"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox13->SetValue(false);
    itemBoxSizer3->Add(itemCheckBox13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer14, 0, wxGROW|wxALL, 3);

    ewxPanel* itemPanel15 = new ewxPanel( itemPanel1, ID_PANEL_SLICE_FIELDS, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer14->Add(itemPanel15, 1, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxVERTICAL);
    itemPanel15->SetSizer(itemBoxSizer16);

    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer16->Add(itemBoxSizer17, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText18 = new ewxStaticText( itemPanel15, wxID_STATIC, _("Plane 1:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemStaticText18, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxButton* itemButton19 = new ewxButton( itemPanel15, ID_BUTTON_SLICE_COLOR_1, _T(""), wxDefaultPosition, wxSize(24, 24), 0 );
    itemButton19->SetBackgroundColour(wxColour(0, 0, 11));
    itemBoxSizer17->Add(itemButton19, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer17->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxCheckBox* itemCheckBox21 = new ewxCheckBox( itemPanel15, ID_CHECKBOX_SLICE_TRANSPARENT_1, _("Transparent"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox21->SetValue(false);
    itemBoxSizer17->Add(itemCheckBox21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer17->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText23 = new ewxStaticText( itemPanel15, wxID_STATIC, _("Offset "), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemStaticText23, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxSpinCtrl* itemSpinCtrl24 = new ewxSpinCtrl( itemPanel15, ID_SPINCTRL_SLICE_OFFSET_1, _("30"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 0, 100, 30 );
    itemBoxSizer17->Add(itemSpinCtrl24, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer16->Add(itemBoxSizer25, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText26 = new ewxStaticText( itemPanel15, wxID_STATIC, _("Plane 2:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer25->Add(itemStaticText26, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxButton* itemButton27 = new ewxButton( itemPanel15, ID_BUTTON_SLICE_COLOR_2, _T(""), wxDefaultPosition, wxSize(24, 24), 0 );
    itemBoxSizer25->Add(itemButton27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer25->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxCheckBox* itemCheckBox29 = new ewxCheckBox( itemPanel15, ID_CHECKBOX_SLICE_TRANSPARENT_2, _("Transparent"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox29->SetValue(true);
    itemBoxSizer25->Add(itemCheckBox29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer25->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText31 = new ewxStaticText( itemPanel15, wxID_STATIC, _("Offset "), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer25->Add(itemStaticText31, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxSpinCtrl* itemSpinCtrl32 = new ewxSpinCtrl( itemPanel15, ID_SPINCTRL_SLICE_OFFSET_2, _("70"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 0, 100, 70 );
    itemBoxSizer25->Add(itemSpinCtrl32, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxBoxSizer* itemBoxSizer33 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer33, 0, wxALIGN_LEFT|wxALL, 0);

    ewxButton* itemButton34 = new ewxButton( itemPanel1, ID_BUTTON_SLICER_XFORM, _("Transform Lattice"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer33->Add(itemButton34, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer33->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton36 = new ewxButton( itemPanel1, ID_BUTTON_SLICER_SLICE, _("Slice"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer33->Add(itemButton36, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

////@end SliceGUI content construction
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_H
 */

void SliceGUI::OnSpinctrlSliceHUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_H in SliceGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_H in SliceGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_K
 */

void SliceGUI::OnSpinctrlSliceKUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_K in SliceGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_K in SliceGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_L
 */

void SliceGUI::OnSpinctrlSliceLUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_L in SliceGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_L in SliceGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_SHOWPLANE
 */

void SliceGUI::OnCheckboxShowplaneClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_SHOWPLANE in SliceGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_SHOWPLANE in SliceGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICE_COLOR_1
 */

void SliceGUI::OnButtonSliceColor1Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICE_COLOR_1 in SliceGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICE_COLOR_1 in SliceGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_SLICE_TRANSPARENT_1
 */

void SliceGUI::OnCheckboxSliceTransparent1Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_SLICE_TRANSPARENT_1 in SliceGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_SLICE_TRANSPARENT_1 in SliceGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_OFFSET_1
 */

void SliceGUI::OnSpinctrlSliceOffset1Updated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_OFFSET_1 in SliceGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_OFFSET_1 in SliceGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICE_COLOR_2
 */

void SliceGUI::OnButtonSliceColor2Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICE_COLOR_2 in SliceGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICE_COLOR_2 in SliceGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_SLICE_TRANSPARENT_2
 */

void SliceGUI::OnCheckboxSliceTransparent2Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_SLICE_TRANSPARENT_2 in SliceGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_SLICE_TRANSPARENT_2 in SliceGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_OFFSET_2
 */

void SliceGUI::OnSpinctrlSliceOffset2Updated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_OFFSET_2 in SliceGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_OFFSET_2 in SliceGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICER_XFORM
 */

void SliceGUI::OnButtonSlicerXformClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICER_XFORM in SliceGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICER_XFORM in SliceGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICER_SLICE
 */

void SliceGUI::OnButtonSlicerSliceClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICER_SLICE in SliceGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICER_SLICE in SliceGUI. 
}

/*!
 * Should we show tooltips?
 */

bool SliceGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap SliceGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin SliceGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end SliceGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon SliceGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin SliceGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end SliceGUI icon retrieval
}
