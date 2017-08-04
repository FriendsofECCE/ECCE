/////////////////////////////////////////////////////////////////////////////
// Name:        WxContractionEditorGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxContractionEditorGUI.H"
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
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/WxParameterEdit.H"
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxButton.H"
////@end includes

#include "WxContractionEditorGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxContractionEditorGUI::ID_GRID_WXCONTRACTIONEDITOR_SUMMARY = wxNewId();
const wxWindowID WxContractionEditorGUI::ID_BUTTON_WXCONTRACTIONEDITOR_HELP = wxNewId();
const wxWindowID WxContractionEditorGUI::ID_PANEL = wxNewId();
const wxWindowID WxContractionEditorGUI::ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS = wxNewId();
const wxWindowID WxContractionEditorGUI::ID_BUTTON_WXCONTRACTIONEDIT_CLOSE = wxNewId();
const wxWindowID WxContractionEditorGUI::ID_PARAMEDIT_WXCONTRACTIONEDITOR_DIFFUSEFUNCTIONS = wxNewId();
const wxWindowID WxContractionEditorGUI::ID_DIALOG = wxNewId();
const wxWindowID WxContractionEditorGUI::ID_CHOICE_WXCONTRACTIONEDITOR_ELEMENTS = wxNewId();

/*!
 * WxContractionEditorGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WxContractionEditorGUI, ewxDialog )

/*!
 * WxContractionEditorGUI event table definition
 */

BEGIN_EVENT_TABLE( WxContractionEditorGUI, ewxDialog )

////@begin WxContractionEditorGUI event table entries
    EVT_CHOICE( ID_CHOICE_WXCONTRACTIONEDITOR_ELEMENTS, WxContractionEditorGUI::elementsChoiceSelectedCB )

    EVT_GRID_CMD_CELL_CHANGE( ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS, WxContractionEditorGUI::contractGridCellChangedCB )
    EVT_GRID_CMD_SELECT_CELL( ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS, WxContractionEditorGUI::contractGridCellSelectedCB )
    EVT_GRID_CMD_EDITOR_HIDDEN( ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS, WxContractionEditorGUI::contractGridEditorHiddenCB )

    EVT_GRID_CMD_CELL_CHANGE( ID_GRID_WXCONTRACTIONEDITOR_SUMMARY, WxContractionEditorGUI::summaryGridCellChangedCB )
    EVT_GRID_CMD_SELECT_CELL( ID_GRID_WXCONTRACTIONEDITOR_SUMMARY, WxContractionEditorGUI::summaryGridCellSelectedCB )
    EVT_GRID_CMD_EDITOR_HIDDEN( ID_GRID_WXCONTRACTIONEDITOR_SUMMARY, WxContractionEditorGUI::summaryGridCellEditorHiddenCB )

    EVT_COMMAND(ID_PARAMEDIT_WXCONTRACTIONEDITOR_DIFFUSEFUNCTIONS, ewxEVT_PARAMETER_VALUE_CHANGED, WxContractionEditorGUI::diffuseParamEditValueChangedCB)

    EVT_BUTTON( ID_BUTTON_WXCONTRACTIONEDIT_CLOSE, WxContractionEditorGUI::closeButtonClickCB )

////@end WxContractionEditorGUI event table entries

END_EVENT_TABLE()

/*!
 * WxContractionEditorGUI constructors
 */

WxContractionEditorGUI::WxContractionEditorGUI( )
{
}

WxContractionEditorGUI::WxContractionEditorGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * WxContractionEditorGUI creator
 */

bool WxContractionEditorGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxContractionEditorGUI member initialisation
////@end WxContractionEditorGUI member initialisation

////@begin WxContractionEditorGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WxContractionEditorGUI creation
    return true;
}

/*!
 * Control creation for WxContractionEditorGUI
 */

void WxContractionEditorGUI::CreateControls()
{    
////@begin WxContractionEditorGUI content construction
    WxContractionEditorGUI* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    ewxPanel* itemPanel3 = new ewxPanel( itemDialog1, ID_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer2->Add(itemPanel3, 0, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    itemPanel3->SetSizer(itemBoxSizer4);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer5, 0, wxGROW, 3);

    ewxStaticText* itemStaticText6 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Element:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText6, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemChoice7Strings = NULL;
    ewxChoice* itemChoice7 = new ewxChoice( itemPanel3, ID_CHOICE_WXCONTRACTIONEDITOR_ELEMENTS, wxDefaultPosition, wxDefaultSize, 0, itemChoice7Strings, 0 );
    itemBoxSizer5->Add(itemChoice7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticLine* itemStaticLine8 = new ewxStaticLine( itemPanel3, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer4->Add(itemStaticLine8, 0, wxGROW|wxALL, 3);

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Contractions"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemStaticText9, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 3);

    ewxGrid* itemGrid10 = new ewxGrid( itemPanel3, ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS, wxDefaultPosition, wxSize(200, 150), wxDOUBLE_BORDER|wxHSCROLL|wxVSCROLL );
    itemGrid10->SetDefaultColSize(50);
    itemGrid10->SetDefaultRowSize(25);
    itemGrid10->SetColLabelSize(25);
    itemGrid10->SetRowLabelSize(0);
    itemGrid10->CreateGrid(2, 4, wxGrid::wxGridSelectCells);
    itemBoxSizer4->Add(itemGrid10, 1, wxGROW|wxALL, 3);

    ewxStaticLine* itemStaticLine11 = new ewxStaticLine( itemPanel3, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer4->Add(itemStaticLine11, 0, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer12, 0, wxALIGN_CENTER_HORIZONTAL, 3);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer12->Add(itemBoxSizer13, 1, wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText14 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Summary"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemStaticText14, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 3);

    ewxGrid* itemGrid15 = new ewxGrid( itemPanel3, ID_GRID_WXCONTRACTIONEDITOR_SUMMARY, wxDefaultPosition, wxSize(300, 150), wxNO_BORDER|wxHSCROLL|wxVSCROLL );
    itemGrid15->SetDefaultColSize(50);
    itemGrid15->SetDefaultRowSize(25);
    itemGrid15->SetColLabelSize(25);
    itemGrid15->SetRowLabelSize(0);
    itemGrid15->CreateGrid(2, 5, wxGrid::wxGridSelectCells);
    itemBoxSizer13->Add(itemGrid15, 0, wxALIGN_LEFT|wxALL, 3);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer12->Add(itemBoxSizer16, 0, wxALIGN_CENTER_VERTICAL, 3);

    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer16->Add(itemBoxSizer17, 0, wxALIGN_CENTER_HORIZONTAL, 3);

    ewxStaticText* itemStaticText18 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(itemStaticText18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    WxParameterEdit* itemWxParameterEdit19 = new WxParameterEdit( itemPanel3, ID_PARAMEDIT_WXCONTRACTIONEDITOR_DIFFUSEFUNCTIONS, wxDefaultPosition, wxSize(180, 30), wxNO_BORDER|wxTAB_TRAVERSAL );
    itemWxParameterEdit19->setMaximum(100);
    itemWxParameterEdit19->setMinimum(1);
    itemWxParameterEdit19->setRangeVisible(1);
    itemWxParameterEdit19->setUnitsLabel(_T(""));
    itemWxParameterEdit19->setUnitsVisible(0);
    itemWxParameterEdit19->setValue(1);
    itemBoxSizer17->Add(itemWxParameterEdit19, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText20 = new ewxStaticText( itemPanel3, wxID_STATIC, _("diffuse functions to each L-value"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    itemBoxSizer16->Add(itemStaticText20, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticLine* itemStaticLine21 = new ewxStaticLine( itemPanel3, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer4->Add(itemStaticLine21, 0, wxGROW|wxALL, 3);

    wxGridSizer* itemGridSizer22 = new wxGridSizer(2, 2, 0, 0);
    itemBoxSizer4->Add(itemGridSizer22, 0, wxGROW, 3);

    ewxButton* itemButton23 = new ewxButton( itemPanel3, ID_BUTTON_WXCONTRACTIONEDIT_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer22->Add(itemButton23, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton24 = new ewxButton( itemPanel3, ID_BUTTON_WXCONTRACTIONEDITOR_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer22->Add(itemButton24, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

////@end WxContractionEditorGUI content construction
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXCONTRACTIONEDITOR_ELEMENTS
 */

void WxContractionEditorGUI::elementsChoiceSelectedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXCONTRACTIONEDITOR_ELEMENTS in WxContractionEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXCONTRACTIONEDITOR_ELEMENTS in WxContractionEditorGUI. 
}

/*!
 * wxEVT_GRID_CMD_CELL_CHANGE event handler for ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS
 */

void WxContractionEditorGUI::contractGridCellChangedCB( wxGridEvent& event )
{
////@begin wxEVT_GRID_CMD_CELL_CHANGE event handler for ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS in WxContractionEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_GRID_CMD_CELL_CHANGE event handler for ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS in WxContractionEditorGUI. 
}

/*!
 * wxEVT_GRID_CMD_SELECT_CELL event handler for ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS
 */

void WxContractionEditorGUI::contractGridCellSelectedCB( wxGridEvent& event )
{
////@begin wxEVT_GRID_CMD_SELECT_CELL event handler for ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS in WxContractionEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_GRID_CMD_SELECT_CELL event handler for ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS in WxContractionEditorGUI. 
}

/*!
 * wxEVT_GRID_CMD_EDITOR_HIDDEN event handler for ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS
 */

void WxContractionEditorGUI::contractGridEditorHiddenCB( wxGridEvent& event )
{
////@begin wxEVT_GRID_CMD_EDITOR_HIDDEN event handler for ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS in WxContractionEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_GRID_CMD_EDITOR_HIDDEN event handler for ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS in WxContractionEditorGUI. 
}

/*!
 * wxEVT_GRID_CMD_CELL_CHANGE event handler for ID_GRID_WXCONTRACTIONEDITOR_SUMMARY
 */

void WxContractionEditorGUI::summaryGridCellChangedCB( wxGridEvent& event )
{
////@begin wxEVT_GRID_CMD_CELL_CHANGE event handler for ID_GRID_WXCONTRACTIONEDITOR_SUMMARY in WxContractionEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_GRID_CMD_CELL_CHANGE event handler for ID_GRID_WXCONTRACTIONEDITOR_SUMMARY in WxContractionEditorGUI. 
}

/*!
 * wxEVT_GRID_CMD_SELECT_CELL event handler for ID_GRID_WXCONTRACTIONEDITOR_SUMMARY
 */

void WxContractionEditorGUI::summaryGridCellSelectedCB( wxGridEvent& event )
{
////@begin wxEVT_GRID_CMD_SELECT_CELL event handler for ID_GRID_WXCONTRACTIONEDITOR_SUMMARY in WxContractionEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_GRID_CMD_SELECT_CELL event handler for ID_GRID_WXCONTRACTIONEDITOR_SUMMARY in WxContractionEditorGUI. 
}

/*!
 * wxEVT_GRID_CMD_EDITOR_HIDDEN event handler for ID_GRID_WXCONTRACTIONEDITOR_SUMMARY
 */

void WxContractionEditorGUI::summaryGridCellEditorHiddenCB( wxGridEvent& event )
{
////@begin wxEVT_GRID_CMD_EDITOR_HIDDEN event handler for ID_GRID_WXCONTRACTIONEDITOR_SUMMARY in WxContractionEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_GRID_CMD_EDITOR_HIDDEN event handler for ID_GRID_WXCONTRACTIONEDITOR_SUMMARY in WxContractionEditorGUI. 
}

/*!
 * ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXCONTRACTIONEDITOR_DIFFUSEFUNCTIONS
 */

void WxContractionEditorGUI::diffuseParamEditValueChangedCB( wxCommandEvent& event )
{
////@begin ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXCONTRACTIONEDITOR_DIFFUSEFUNCTIONS in WxContractionEditorGUI.
    // Before editing this code, remove the block markers.
   event.Skip();
////@end ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXCONTRACTIONEDITOR_DIFFUSEFUNCTIONS in WxContractionEditorGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXCONTRACTIONEDIT_CLOSE
 */

void WxContractionEditorGUI::closeButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXCONTRACTIONEDIT_CLOSE in WxContractionEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXCONTRACTIONEDIT_CLOSE in WxContractionEditorGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxContractionEditorGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxContractionEditorGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxContractionEditorGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxContractionEditorGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxContractionEditorGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxContractionEditorGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxContractionEditorGUI icon retrieval
}
