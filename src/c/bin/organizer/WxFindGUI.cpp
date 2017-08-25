/////////////////////////////////////////////////////////////////////////////
// Name:        WxFindGUI.C
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxFindGUI.H"
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
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "WxFindGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxFindGUI::ID_FIND_BUTTON_HELP = wxNewId();
const wxWindowID WxFindGUI::ID_FIND_BUTTON_FIND = wxNewId();
const wxWindowID WxFindGUI::ID_FIND_BUTTON_CLOSE = wxNewId();
const wxWindowID WxFindGUI::ID_LISTBOX_RESULTS = wxNewId();
const wxWindowID WxFindGUI::ID_TEXTCTRL_FROM = wxNewId();
const wxWindowID WxFindGUI::ID_TEXTCTRL_FIND = wxNewId();
const wxWindowID WxFindGUI::ID_DIALOG = wxNewId();
const wxWindowID WxFindGUI::ID_FIND_BUTTON_GO = wxNewId();

/*!
 * WxFindGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WxFindGUI, ewxDialog )

/*!
 * WxFindGUI event table definition
 */

BEGIN_EVENT_TABLE( WxFindGUI, ewxDialog )

////@begin WxFindGUI event table entries
    EVT_TEXT_ENTER( ID_TEXTCTRL_FIND, WxFindGUI::OnFindButtonFindClick )

    EVT_LISTBOX( ID_LISTBOX_RESULTS, WxFindGUI::OnFindResultsSelected )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_RESULTS, WxFindGUI::OnFindButtonGoClick )

    EVT_BUTTON( ID_FIND_BUTTON_FIND, WxFindGUI::OnFindButtonFindClick )

    EVT_BUTTON( ID_FIND_BUTTON_GO, WxFindGUI::OnFindButtonGoClick )

    EVT_BUTTON( ID_FIND_BUTTON_CLOSE, WxFindGUI::OnFindButtonCloseClick )

    EVT_BUTTON( ID_FIND_BUTTON_HELP, WxFindGUI::OnFindButtonHelpClick )

////@end WxFindGUI event table entries

END_EVENT_TABLE()

/*!
 * WxFindGUI constructors
 */

WxFindGUI::WxFindGUI( )
{
}

WxFindGUI::WxFindGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * WxFindGUI creator
 */

bool WxFindGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxFindGUI member initialisation
    p_topSizer = NULL;
    p_find = NULL;
    p_from = NULL;
    p_results = NULL;
////@end WxFindGUI member initialisation

////@begin WxFindGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    Centre();
////@end WxFindGUI creation
    return true;
}

/*!
 * Control creation for WxFindGUI
 */

void WxFindGUI::CreateControls()
{    
////@begin WxFindGUI content construction
    WxFindGUI* itemDialog1 = this;

    p_topSizer = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(p_topSizer);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    p_topSizer->Add(itemBoxSizer3, 0, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText4 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Find: "), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    p_find = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_FIND, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(p_find, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    p_topSizer->Add(itemBoxSizer6, 0, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemDialog1, wxID_STATIC, _("From:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemStaticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    p_from = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_FROM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(p_from, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticLine* itemStaticLine9 = new ewxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    p_topSizer->Add(itemStaticLine9, 0, wxGROW|wxALL, 5);

    ewxStaticText* itemStaticText10 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Results:"), wxDefaultPosition, wxDefaultSize, 0 );
    p_topSizer->Add(itemStaticText10, 0, wxALIGN_CENTER_HORIZONTAL|wxADJUST_MINSIZE, 5);

    wxString* p_resultsStrings = NULL;
    p_results = new ewxListBox( itemDialog1, ID_LISTBOX_RESULTS, wxDefaultPosition, wxDefaultSize, 0, p_resultsStrings, wxLB_SINGLE|wxLB_NEEDED_SB|wxLB_SORT );
    p_topSizer->Add(p_results, 1, wxGROW|wxALL, 5);

    ewxStaticLine* itemStaticLine12 = new ewxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    p_topSizer->Add(itemStaticLine12, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    p_topSizer->Add(itemBoxSizer13, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    ewxButton* itemButton14 = new ewxButton( itemDialog1, ID_FIND_BUTTON_FIND, _("Find"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemButton14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton15 = new ewxButton( itemDialog1, ID_FIND_BUTTON_GO, _("Open"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemButton15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton16 = new ewxButton( itemDialog1, ID_FIND_BUTTON_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemButton16, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton17 = new ewxButton( itemDialog1, ID_FIND_BUTTON_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemButton17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end WxFindGUI content construction
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FIND
 */

void WxFindGUI::OnFindButtonFindClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FIND in WxFindGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FIND in WxFindGUI. 
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_RESULTS
 */

void WxFindGUI::OnFindResultsSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_RESULTS in WxFindGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_RESULTS in WxFindGUI. 
}

/*!
 * wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX_RESULTS
 */

void WxFindGUI::OnFindButtonGoClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX_RESULTS in WxFindGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX_RESULTS in WxFindGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_BUTTON_CLOSE
 */

void WxFindGUI::OnFindButtonCloseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_BUTTON_CLOSE in WxFindGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_BUTTON_CLOSE in WxFindGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_BUTTON_HELP
 */

void WxFindGUI::OnFindButtonHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_BUTTON_HELP in WxFindGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_BUTTON_HELP in WxFindGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxFindGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxFindGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxFindGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxFindGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxFindGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxFindGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxFindGUI icon retrieval
}
