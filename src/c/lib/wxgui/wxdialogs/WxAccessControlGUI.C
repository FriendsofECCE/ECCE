/////////////////////////////////////////////////////////////////////////////
// Name:        WxAccessControlGUI.C
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxAccessControlGUI.H"
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
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxFrame.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxgui/WxAccessControlGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxAccessControlGUI::ID_BUTTON_HELP = wxNewId();
const wxWindowID WxAccessControlGUI::ID_BUTTON_CLOSE = wxNewId();
const wxWindowID WxAccessControlGUI::ID_TEXTCTRL_READONLY = wxNewId();
const wxWindowID WxAccessControlGUI::ID_BUTTON_CHANGE = wxNewId();
const wxWindowID WxAccessControlGUI::ID_TEXTCTRL_READWRITE = wxNewId();
const wxWindowID WxAccessControlGUI::ID_FRAME_ACCESS_CONTROL_TOP = wxNewId();

/*!
 * WxAccessControlGUI type definition
 */

IMPLEMENT_CLASS( WxAccessControlGUI, ewxFrame )

/*!
 * WxAccessControlGUI event table definition
 */

BEGIN_EVENT_TABLE( WxAccessControlGUI, ewxFrame )

////@begin WxAccessControlGUI event table entries
    EVT_CLOSE( WxAccessControlGUI::OnCloseWindow )

    EVT_BUTTON( ID_BUTTON_CHANGE, WxAccessControlGUI::OnButtonChangeClick )

    EVT_BUTTON( ID_BUTTON_CLOSE, WxAccessControlGUI::OnButtonCloseClick )

    EVT_BUTTON( ID_BUTTON_HELP, WxAccessControlGUI::OnButtonHelpClick )

////@end WxAccessControlGUI event table entries

END_EVENT_TABLE()

/*!
 * WxAccessControlGUI constructors
 */

WxAccessControlGUI::WxAccessControlGUI( )
{
}

WxAccessControlGUI::WxAccessControlGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * WxAccessControlGUI creator
 */

bool WxAccessControlGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxAccessControlGUI member initialisation
    p_topSizer = NULL;
    p_projectName = NULL;
    p_readOnly = NULL;
    p_readWrite = NULL;
    p_buttonSizer = NULL;
////@end WxAccessControlGUI member initialisation

////@begin WxAccessControlGUI creation
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WxAccessControlGUI creation
    return true;
}

/*!
 * Control creation for WxAccessControlGUI
 */

void WxAccessControlGUI::CreateControls()
{    
////@begin WxAccessControlGUI content construction
    WxAccessControlGUI* itemFrame1 = this;

    p_topSizer = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(p_topSizer);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    p_topSizer->Add(itemBoxSizer3, 0, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText4 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Changing Access to:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    p_projectName = new ewxStaticText( itemFrame1, wxID_STATIC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(p_projectName, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText6 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Enter user names separated by commas or spaces to change access permissions."), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    p_topSizer->Add(itemStaticText6, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemFrame1, wxID_STATIC, _("The ALL keyword is used to give access to all users.                                      "), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    p_topSizer->Add(itemStaticText7, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    p_topSizer->Add(itemBoxSizer8, 0, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Read-Only:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemStaticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    p_readOnly = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_READONLY, _T(""), wxDefaultPosition, wxSize(365, -1), 0 );
    itemBoxSizer8->Add(p_readOnly, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxHORIZONTAL);
    p_topSizer->Add(itemBoxSizer11, 0, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText12 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Read Write:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemStaticText12, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    p_readWrite = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_READWRITE, _T(""), wxDefaultPosition, wxSize(365, -1), 0 );
    itemBoxSizer11->Add(p_readWrite, 1, wxGROW|wxALL, 5);

    ewxStaticLine* itemStaticLine14 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    p_topSizer->Add(itemStaticLine14, 0, wxGROW|wxALL, 0);

    p_buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    p_topSizer->Add(p_buttonSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    ewxButton* itemButton16 = new ewxButton( itemFrame1, ID_BUTTON_CHANGE, _("Change"), wxDefaultPosition, wxDefaultSize, 0 );
    p_buttonSizer->Add(itemButton16, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_buttonSizer->Add(50, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton18 = new ewxButton( itemFrame1, ID_BUTTON_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    p_buttonSizer->Add(itemButton18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_buttonSizer->Add(50, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton20 = new ewxButton( itemFrame1, ID_BUTTON_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    p_buttonSizer->Add(itemButton20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end WxAccessControlGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_FRAME_ACCESS_CONTROL_TOP
 */

void WxAccessControlGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_FRAME_ACCESS_CONTROL_TOP in WxAccessControlGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_FRAME_ACCESS_CONTROL_TOP in WxAccessControlGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CHANGE
 */

void WxAccessControlGUI::OnButtonChangeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CHANGE in WxAccessControlGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CHANGE in WxAccessControlGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CLOSE
 */

void WxAccessControlGUI::OnButtonCloseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CLOSE in WxAccessControlGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CLOSE in WxAccessControlGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP
 */

void WxAccessControlGUI::OnButtonHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP in WxAccessControlGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP in WxAccessControlGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxAccessControlGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxAccessControlGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxAccessControlGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxAccessControlGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxAccessControlGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxAccessControlGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxAccessControlGUI icon retrieval
}
