/////////////////////////////////////////////////////////////////////////////
// Name:        WxConfigureAccessGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxConfigureAccessGUI.H"
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
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "WxConfigureAccessGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxConfigureAccessGUI::ID_STATIC_WXCONFIGUREACCESS_PASSWORD2 = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_PANEL_WXCONFIGUREACCESS_PASSWORD1 = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD2 = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_TEXTCTRL_WXCONFIGUREACCESS_USERNAME = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_PANEL = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_PANEL_WXCONFIGUREACCESS_PASSWORD2 = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD1 = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_STATIC_WXCONFIGUREACCESS_PASSWORD1 = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_CHOICE_WXCONFIGUREACCESS_REMOTESHELL = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_BUTTON_WXCONFIGUREACCESS_CANCEL = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_WXCONFIGURACCESS_FRAME = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_BUTTON_WXCONFIGUREACCESS_OKAY = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_PANEL_WXCONFIGUREACCESS_USERNAME = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_PANEL1 = wxNewId();
const wxWindowID WxConfigureAccessGUI::ID_PANEL_WXCONFIGUREACCESS_REMOTESHELL = wxNewId();

/*!
 * WxConfigureAccessGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WxConfigureAccessGUI, ewxDialog )

/*!
 * WxConfigureAccessGUI event table definition
 */

BEGIN_EVENT_TABLE( WxConfigureAccessGUI, ewxDialog )

////@begin WxConfigureAccessGUI event table entries
    EVT_CHOICE( ID_CHOICE_WXCONFIGUREACCESS_REMOTESHELL, WxConfigureAccessGUI::remoteShellChoiceSelectedCB )

    EVT_TEXT( ID_TEXTCTRL_WXCONFIGUREACCESS_USERNAME, WxConfigureAccessGUI::usernameTextCtrlUpdatedCB )

    EVT_TEXT( ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD1, WxConfigureAccessGUI::password1TextCtrlUpdatedCB )

    EVT_TEXT( ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD2, WxConfigureAccessGUI::password2TextCtrlUpdatedCB )

    EVT_BUTTON( ID_BUTTON_WXCONFIGUREACCESS_OKAY, WxConfigureAccessGUI::okayButtonClickCB )

    EVT_BUTTON( ID_BUTTON_WXCONFIGUREACCESS_CANCEL, WxConfigureAccessGUI::cancelButtonClickCB )

////@end WxConfigureAccessGUI event table entries

END_EVENT_TABLE()

/*!
 * WxConfigureAccessGUI constructors
 */

WxConfigureAccessGUI::WxConfigureAccessGUI( )
{
}

WxConfigureAccessGUI::WxConfigureAccessGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * WxConfigureAccessGUI creator
 */

bool WxConfigureAccessGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxConfigureAccessGUI member initialisation
////@end WxConfigureAccessGUI member initialisation

////@begin WxConfigureAccessGUI creation
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WxConfigureAccessGUI creation
    return true;
}

/*!
 * Control creation for WxConfigureAccessGUI
 */

void WxConfigureAccessGUI::CreateControls()
{    
////@begin WxConfigureAccessGUI content construction
    WxConfigureAccessGUI* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    ewxPanel* itemPanel3 = new ewxPanel( itemDialog1, ID_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer2->Add(itemPanel3, 1, wxALIGN_CENTER_HORIZONTAL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel3->SetSizer(itemBoxSizer4);

    ewxPanel* itemPanel5 = new ewxPanel( itemPanel3, ID_PANEL1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer4->Add(itemPanel5, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxVERTICAL);
    itemPanel5->SetSizer(itemBoxSizer6);

    ewxPanel* itemPanel7 = new ewxPanel( itemPanel5, ID_PANEL_WXCONFIGUREACCESS_REMOTESHELL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer6->Add(itemPanel7, 0, wxGROW, 3);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel7->SetSizer(itemBoxSizer8);

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemPanel7, wxID_STATIC, _("Remote Shell:"), wxDefaultPosition, wxSize(90, -1), wxALIGN_RIGHT );
    itemBoxSizer8->Add(itemStaticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemChoice10Strings = NULL;
    ewxChoice* itemChoice10 = new ewxChoice( itemPanel7, ID_CHOICE_WXCONFIGUREACCESS_REMOTESHELL, wxDefaultPosition, wxDefaultSize, 0, itemChoice10Strings, 0 );
    itemBoxSizer8->Add(itemChoice10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer6->Add(itemBoxSizer11, 0, wxGROW, 3);

    ewxPanel* itemPanel12 = new ewxPanel( itemPanel5, ID_PANEL_WXCONFIGUREACCESS_USERNAME, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer11->Add(itemPanel12, 1, wxGROW, 3);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel12->SetSizer(itemBoxSizer13);

    ewxStaticText* itemStaticText14 = new ewxStaticText( itemPanel12, wxID_STATIC, _("Username:"), wxDefaultPosition, wxSize(90, -1), wxALIGN_RIGHT );
    itemBoxSizer13->Add(itemStaticText14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxTextCtrl* itemTextCtrl15 = new ewxTextCtrl( itemPanel12, ID_TEXTCTRL_WXCONFIGUREACCESS_USERNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemTextCtrl15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel16 = new ewxPanel( itemPanel5, ID_PANEL_WXCONFIGUREACCESS_PASSWORD1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer11->Add(itemPanel16, 1, wxALIGN_CENTER_VERTICAL, 3);

    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel16->SetSizer(itemBoxSizer17);

    ewxStaticText* itemStaticText18 = new ewxStaticText( itemPanel16, ID_STATIC_WXCONFIGUREACCESS_PASSWORD1, _("Password:"), wxDefaultPosition, wxSize(90, -1), wxALIGN_RIGHT );
    itemBoxSizer17->Add(itemStaticText18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxTextCtrl* itemTextCtrl19 = new ewxTextCtrl( itemPanel16, ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD1, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemBoxSizer17->Add(itemTextCtrl19, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel20 = new ewxPanel( itemPanel5, ID_PANEL_WXCONFIGUREACCESS_PASSWORD2, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer6->Add(itemPanel20, 0, wxGROW, 3);

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel20->SetSizer(itemBoxSizer21);

    itemBoxSizer21->Add(90, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText23 = new ewxStaticText( itemPanel20, ID_STATIC_WXCONFIGUREACCESS_PASSWORD2, _("Globus Pass Phrase:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
    itemBoxSizer21->Add(itemStaticText23, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxTextCtrl* itemTextCtrl24 = new ewxTextCtrl( itemPanel20, ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD2, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemBoxSizer21->Add(itemTextCtrl24, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer4->Add(itemBoxSizer25, 0, wxGROW, 5);

    ewxButton* itemButton26 = new ewxButton( itemPanel3, ID_BUTTON_WXCONFIGUREACCESS_OKAY, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton26->SetDefault();
    itemButton26->Enable(false);
    itemBoxSizer25->Add(itemButton26, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

    ewxButton* itemButton27 = new ewxButton( itemPanel3, ID_BUTTON_WXCONFIGUREACCESS_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer25->Add(itemButton27, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

////@end WxConfigureAccessGUI content construction
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXCONFIGUREACCESS_REMOTESHELL
 */

void WxConfigureAccessGUI::remoteShellChoiceSelectedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXCONFIGUREACCESS_REMOTESHELL in WxConfigureAccessGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXCONFIGUREACCESS_REMOTESHELL in WxConfigureAccessGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXCONFIGUREACCESS_USERNAME
 */

void WxConfigureAccessGUI::usernameTextCtrlUpdatedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXCONFIGUREACCESS_USERNAME in WxConfigureAccessGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXCONFIGUREACCESS_USERNAME in WxConfigureAccessGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD1
 */

void WxConfigureAccessGUI::password1TextCtrlUpdatedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD1 in WxConfigureAccessGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD1 in WxConfigureAccessGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD2
 */

void WxConfigureAccessGUI::password2TextCtrlUpdatedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD2 in WxConfigureAccessGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD2 in WxConfigureAccessGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXCONFIGUREACCESS_OKAY
 */

void WxConfigureAccessGUI::okayButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXCONFIGUREACCESS_OKAY in WxConfigureAccessGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXCONFIGUREACCESS_OKAY in WxConfigureAccessGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXCONFIGUREACCESS_CANCEL
 */

void WxConfigureAccessGUI::cancelButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXCONFIGUREACCESS_CANCEL in WxConfigureAccessGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXCONFIGUREACCESS_CANCEL in WxConfigureAccessGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxConfigureAccessGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxConfigureAccessGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxConfigureAccessGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxConfigureAccessGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxConfigureAccessGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxConfigureAccessGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxConfigureAccessGUI icon retrieval
}
