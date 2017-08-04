/////////////////////////////////////////////////////////////////////////////
// Name:        WxBasisSetDetailGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxBasisSetDetailGUI.H"
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
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxScrolledWindow.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "WxBasisSetDetailGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxBasisSetDetailGUI::ID_PANEL_WXBASISSETDETAIL_MAIN = wxNewId();
const wxWindowID WxBasisSetDetailGUI::ID_BUTTON_WXBASISSETDETAIL_HELP = wxNewId();
const wxWindowID WxBasisSetDetailGUI::ID_BUTTON_WXBASISSETDETAIL_CLOSE = wxNewId();
const wxWindowID WxBasisSetDetailGUI::ID_PANEL_WXBASISSETDETAIL_REFERENCES = wxNewId();
const wxWindowID WxBasisSetDetailGUI::ID_DIALOG_WXBASISSETDETAIL_MAIN = wxNewId();
const wxWindowID WxBasisSetDetailGUI::ID_SCROLLEDWINDOW_WXBASISSETDETAIL_GRAPH = wxNewId();
const wxWindowID WxBasisSetDetailGUI::ID_PANEL_WXBASISSETDETAIL_DETAILS = wxNewId();
const wxWindowID WxBasisSetDetailGUI::ID_BITMAP_WXBASISSETDETAIL_GRAPH = wxNewId();
const wxWindowID WxBasisSetDetailGUI::ID_TEXTCTRL_WXBASISSETDETAIL_DESCRIPTION = wxNewId();
const wxWindowID WxBasisSetDetailGUI::ID_PANEL_WXBASISSETDETAIL_DESCRIPTION = wxNewId();
const wxWindowID WxBasisSetDetailGUI::ID_TEXTCTRL_WXBASISSETDETAIL_REFERENCES = wxNewId();
const wxWindowID WxBasisSetDetailGUI::ID_NOTEBOOK_WXBASISSETDETAIL_DETAILS = wxNewId();

/*!
 * WxBasisSetDetailGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WxBasisSetDetailGUI, ewxDialog )

/*!
 * WxBasisSetDetailGUI event table definition
 */

BEGIN_EVENT_TABLE( WxBasisSetDetailGUI, ewxDialog )

////@begin WxBasisSetDetailGUI event table entries
    EVT_BUTTON( ID_BUTTON_WXBASISSETDETAIL_CLOSE, WxBasisSetDetailGUI::closeButtonClickCB )

////@end WxBasisSetDetailGUI event table entries

END_EVENT_TABLE()

/*!
 * WxBasisSetDetailGUI constructors
 */

WxBasisSetDetailGUI::WxBasisSetDetailGUI( )
{
}

WxBasisSetDetailGUI::WxBasisSetDetailGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * WxBasisSetDetailGUI creator
 */

bool WxBasisSetDetailGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxBasisSetDetailGUI member initialisation
////@end WxBasisSetDetailGUI member initialisation

////@begin WxBasisSetDetailGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    Centre();
////@end WxBasisSetDetailGUI creation
    return true;
}

/*!
 * Control creation for WxBasisSetDetailGUI
 */

void WxBasisSetDetailGUI::CreateControls()
{    
////@begin WxBasisSetDetailGUI content construction
    WxBasisSetDetailGUI* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    ewxPanel* itemPanel3 = new ewxPanel( itemDialog1, ID_PANEL_WXBASISSETDETAIL_MAIN, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer2->Add(itemPanel3, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    itemPanel3->SetSizer(itemBoxSizer4);

    ewxPanel* itemPanel5 = new ewxPanel( itemPanel3, ID_PANEL_WXBASISSETDETAIL_REFERENCES, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer4->Add(itemPanel5, 0, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxVERTICAL);
    itemPanel5->SetSizer(itemBoxSizer6);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemPanel5, wxID_STATIC, _("References"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemStaticText7, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl8 = new ewxTextCtrl( itemPanel5, ID_TEXTCTRL_WXBASISSETDETAIL_REFERENCES, _T(""), wxDefaultPosition, wxSize(-1, 150), wxTE_MULTILINE|wxTE_READONLY );
    itemTextCtrl8->SetFont(wxFont(10, wxTELETYPE, wxNORMAL, wxNORMAL, false, _T("Courier 10 Pitch")));
    itemBoxSizer6->Add(itemTextCtrl8, 1, wxGROW|wxALL, 3);

    ewxPanel* itemPanel9 = new ewxPanel( itemPanel3, ID_PANEL_WXBASISSETDETAIL_DETAILS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer4->Add(itemPanel9, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxVERTICAL);
    itemPanel9->SetSizer(itemBoxSizer10);

    ewxNotebook* itemNotebook11 = new ewxNotebook( itemPanel9, ID_NOTEBOOK_WXBASISSETDETAIL_DETAILS, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

    ewxPanel* itemPanel12 = new ewxPanel( itemNotebook11, ID_PANEL_WXBASISSETDETAIL_DESCRIPTION, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxVERTICAL);
    itemPanel12->SetSizer(itemBoxSizer13);

    ewxTextCtrl* itemTextCtrl14 = new ewxTextCtrl( itemPanel12, ID_TEXTCTRL_WXBASISSETDETAIL_DESCRIPTION, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
    itemTextCtrl14->SetFont(wxFont(10, wxTELETYPE, wxNORMAL, wxNORMAL, false, _T("Courier 10 Pitch")));
    itemBoxSizer13->Add(itemTextCtrl14, 1, wxGROW|wxALL, 3);

    itemNotebook11->AddPage(itemPanel12, _("Description"));

    ewxScrolledWindow* itemScrolledWindow15 = new ewxScrolledWindow( itemNotebook11, ID_SCROLLEDWINDOW_WXBASISSETDETAIL_GRAPH, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxHSCROLL|wxVSCROLL );
    itemScrolledWindow15->SetScrollbars(1, 1, 0, 0);
    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxVERTICAL);
    itemScrolledWindow15->SetSizer(itemBoxSizer16);

    wxBitmap itemStaticBitmap17Bitmap(wxNullBitmap);
    wxStaticBitmap* itemStaticBitmap17 = new wxStaticBitmap( itemScrolledWindow15, ID_BITMAP_WXBASISSETDETAIL_GRAPH, itemStaticBitmap17Bitmap, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER );
    itemBoxSizer16->Add(itemStaticBitmap17, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

    itemNotebook11->AddPage(itemScrolledWindow15, _("Graph"));

    itemBoxSizer10->Add(itemNotebook11, 1, wxGROW|wxALL, 3);

    wxGridSizer* itemGridSizer18 = new wxGridSizer(1, 2, 0, 0);
    itemBoxSizer4->Add(itemGridSizer18, 0, wxGROW, 3);

    ewxButton* itemButton19 = new ewxButton( itemPanel3, ID_BUTTON_WXBASISSETDETAIL_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer18->Add(itemButton19, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton20 = new ewxButton( itemPanel3, ID_BUTTON_WXBASISSETDETAIL_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer18->Add(itemButton20, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

////@end WxBasisSetDetailGUI content construction
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISSETDETAIL_CLOSE
 */

void WxBasisSetDetailGUI::closeButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISSETDETAIL_CLOSE in WxBasisSetDetailGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISSETDETAIL_CLOSE in WxBasisSetDetailGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxBasisSetDetailGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxBasisSetDetailGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxBasisSetDetailGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxBasisSetDetailGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxBasisSetDetailGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxBasisSetDetailGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxBasisSetDetailGUI icon retrieval
}
