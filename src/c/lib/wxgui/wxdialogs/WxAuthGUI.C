/////////////////////////////////////////////////////////////////////////////
// Name:        WxAuthGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxAuthGUI.H"
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
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxgui/WxAuthGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxAuthGUI::wxID_STATIC_AUTH_PASSWD = wxNewId();
const wxWindowID WxAuthGUI::ID_TEXTCTRL_AUTH_NEWPASSWORD = wxNewId();
const wxWindowID WxAuthGUI::ID_DIALOG = wxNewId();
const wxWindowID WxAuthGUI::wxID_STATIC_AUTH_NEWPASSWD = wxNewId();
const wxWindowID WxAuthGUI::ID_TEXTCTRL_AUTH_USER = wxNewId();
const wxWindowID WxAuthGUI::wxID_STATIC_AUTH_PROMPT_LABEL = wxNewId();
const wxWindowID WxAuthGUI::wxID_CHANGE = wxNewId();
const wxWindowID WxAuthGUI::wxID_STATIC_AUTH_SERVER_LABEL = wxNewId();
const wxWindowID WxAuthGUI::wxID_STATIC_AUTH_SERVER_VALUE = wxNewId();
const wxWindowID WxAuthGUI::wxID_STATIC_AUTH_USER_LABEL = wxNewId();
const wxWindowID WxAuthGUI::ID_CHECKBOX_AUTH_SAVEPASSWORDS = wxNewId();
const wxWindowID WxAuthGUI::ID_TEXTCTRL_AUTH_PASSWORD = wxNewId();

/*!
 * WxAuthGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WxAuthGUI, ewxDialog )

/*!
 * WxAuthGUI event table definition
 */

BEGIN_EVENT_TABLE( WxAuthGUI, ewxDialog )

////@begin WxAuthGUI event table entries
    EVT_CLOSE( WxAuthGUI::OnCloseWindow )

    EVT_TEXT_ENTER( ID_TEXTCTRL_AUTH_USER, WxAuthGUI::OnTextctrlAuthUserEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_AUTH_PASSWORD, WxAuthGUI::OnTextctrlAuthPasswordEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_AUTH_NEWPASSWORD, WxAuthGUI::OnTextctrlAuthNewpasswordEnter )

    EVT_BUTTON( wxID_CHANGE, WxAuthGUI::OnChange )

    EVT_CHECKBOX( ID_CHECKBOX_AUTH_SAVEPASSWORDS, WxAuthGUI::OnSavePasswordsClick )

////@end WxAuthGUI event table entries

END_EVENT_TABLE()

/*!
 * WxAuthGUI constructors
 */

WxAuthGUI::WxAuthGUI( )
{
}

WxAuthGUI::WxAuthGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * WxAuthGUI creator
 */

bool WxAuthGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxAuthGUI member initialisation
////@end WxAuthGUI member initialisation

////@begin WxAuthGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    Centre();
////@end WxAuthGUI creation
    return true;
}

/*!
 * Control creation for WxAuthGUI
 */

void WxAuthGUI::CreateControls()
{    
////@begin WxAuthGUI content construction
    WxAuthGUI* itemDialog1 = this;

    wxBitmap itemStaticBitmap2Bitmap(itemDialog1->GetBitmapResource(wxT("passprompt.xpm")));
    (void)new wxStaticBitmap( itemDialog1, wxID_STATIC, itemStaticBitmap2Bitmap, wxDefaultPosition, wxDefaultSize, 0 );

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer3);

    ewxStaticText* itemStaticText4 = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_PROMPT_LABEL, _("Please enter your data server user name and password:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer5, 0, wxALIGN_RIGHT|wxALL, 2);

    ewxStaticText* itemStaticText6 = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_SERVER_LABEL, _("Server:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText6, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 2);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_SERVER_VALUE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText7, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 2);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer8, 0, wxALIGN_RIGHT|wxLEFT|wxRIGHT|wxBOTTOM, 2);

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_USER_LABEL, _("User name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemStaticText9, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 2);

    ewxTextCtrl* itemTextCtrl10 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_AUTH_USER, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemTextCtrl10, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 2);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer11, 0, wxALIGN_RIGHT|wxLEFT|wxRIGHT, 2);

    ewxStaticText* itemStaticText12 = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_PASSWD, _("Password:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemStaticText12, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 2);

    ewxTextCtrl* itemTextCtrl13 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_AUTH_PASSWORD, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemBoxSizer11->Add(itemTextCtrl13, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 2);

    ewxStaticText* itemStaticText14 = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_NEWPASSWD, _("New Password:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemStaticText14, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 2);

    ewxTextCtrl* itemTextCtrl15 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_AUTH_NEWPASSWORD, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemBoxSizer11->Add(itemTextCtrl15, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 2);

    itemBoxSizer3->Add(1, 60, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer17, 0, wxGROW|wxLEFT|wxRIGHT, 2);

    ewxButton* itemButton18 = new ewxButton( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemButton18->SetDefault();
    itemBoxSizer17->Add(itemButton18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer17->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton20 = new ewxButton( itemDialog1, wxID_CHANGE, _("Change..."), wxDefaultPosition, wxSize(75, -1), 0 );
    itemButton20->Show(false);
    itemBoxSizer17->Add(itemButton20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton21 = new ewxButton( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxSize(70, -1), 0 );
    itemBoxSizer17->Add(itemButton21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxCheckBox* itemCheckBox22 = new ewxCheckBox( itemDialog1, ID_CHECKBOX_AUTH_SAVEPASSWORDS, _("Save Passwords Between Invocations"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox22->SetValue(false);
    itemCheckBox22->Show(false);
    itemBoxSizer3->Add(itemCheckBox22, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end WxAuthGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
 */

void WxAuthGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_DIALOG in WxAuthGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_DIALOG in WxAuthGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_AUTH_USER
 */

void WxAuthGUI::OnTextctrlAuthUserEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_AUTH_USER in WxAuthGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_AUTH_USER in WxAuthGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_AUTH_PASSWORD
 */

void WxAuthGUI::OnTextctrlAuthPasswordEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_AUTH_PASSWORD in WxAuthGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_AUTH_PASSWORD in WxAuthGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_AUTH_NEWPASSWORD
 */

void WxAuthGUI::OnTextctrlAuthNewpasswordEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_AUTH_NEWPASSWORD in WxAuthGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_AUTH_NEWPASSWORD in WxAuthGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CHANGE
 */

void WxAuthGUI::OnChange( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CHANGE in WxAuthGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CHANGE in WxAuthGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_AUTH_SAVEPASSWORDS
 */

void WxAuthGUI::OnSavePasswordsClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_AUTH_SAVEPASSWORDS in WxAuthGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_AUTH_SAVEPASSWORDS in WxAuthGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxAuthGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxAuthGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxAuthGUI bitmap retrieval
    wxUnusedVar(name);
    if (name == wxT("passprompt.xpm"))
    {
        ewxBitmap bitmap(_T("passprompt.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    return wxNullBitmap;
////@end WxAuthGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxAuthGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxAuthGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxAuthGUI icon retrieval
}
