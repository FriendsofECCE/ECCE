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
const wxWindowID WxAuthGUI::wxID_STATIC_AUTH_SECURITY = wxNewId();
const wxWindowID WxAuthGUI::wxID_STATIC_AUTH_STATUS = wxNewId();
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
    GetSizer()->SetSizeHints(this);
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

    // GNOME HIG: labels right-aligned beside their fields, one border unit
    // between related items and two around groups, the buttons in the
    // platform's own order (wxStdDialogButtonSizer).
    const int gap = wxSizerFlags::GetDefaultBorder();

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(topSizer);

    wxBoxSizer* mainRow = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(mainRow, wxSizerFlags(1).Expand().DoubleBorder(wxLEFT|wxRIGHT|wxTOP));

    wxImage logoImage(itemDialog1->GetBitmapResource(wxT("passprompt.xpm")).ConvertToImage());
    logoImage.Rescale(114, 64, wxIMAGE_QUALITY_HIGH);
    wxStaticBitmap* logo = new wxStaticBitmap( itemDialog1, wxID_STATIC, wxBitmap(logoImage), wxDefaultPosition, wxDefaultSize, 0 );
    mainRow->Add(logo, wxSizerFlags().Top().Border(wxRIGHT, 3*gap));

    wxBoxSizer* column = new wxBoxSizer(wxVERTICAL);
    mainRow->Add(column, 1, wxEXPAND, 0);

    ewxStaticText* itemStaticText4 = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_PROMPT_LABEL, _("Please enter your data server\nuser name and password:"), wxDefaultPosition, wxDefaultSize, 0 );
    column->Add(itemStaticText4, wxSizerFlags().Left().Border(wxBOTTOM, 2*gap));

    // Every row's spacing sits on its items, not in the grid, so a hidden
    // row (New Password) leaves no gap behind.
    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 0, gap);
    grid->AddGrowableCol(1);
    column->Add(grid, wxSizerFlags().Expand());
    const wxSizerFlags lab = wxSizerFlags().Right().CenterVertical().Border(wxBOTTOM, gap);
    const wxSizerFlags fld = wxSizerFlags(1).Expand().CenterVertical().Border(wxBOTTOM, gap);
    const wxSize fieldSize = FromDIP(wxSize(240, -1));

    ewxStaticText* itemStaticText6 = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_SERVER_LABEL, _("Server:"), wxDefaultPosition, wxDefaultSize, 0 );
    grid->Add(itemStaticText6, lab);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_SERVER_VALUE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText7->SetFont(itemStaticText7->GetFont().Bold());
    grid->Add(itemStaticText7, wxSizerFlags().Left().CenterVertical().Border(wxBOTTOM, gap));

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_USER_LABEL, _("User name:"), wxDefaultPosition, wxDefaultSize, 0 );
    grid->Add(itemStaticText9, lab);

    ewxTextCtrl* itemTextCtrl10 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_AUTH_USER, _T(""), wxDefaultPosition, fieldSize, 0 );
    grid->Add(itemTextCtrl10, fld);

    ewxStaticText* itemStaticText12 = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_PASSWD, _("Password:"), wxDefaultPosition, wxDefaultSize, 0 );
    grid->Add(itemStaticText12, lab);

    ewxTextCtrl* itemTextCtrl13 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_AUTH_PASSWORD, _T(""), wxDefaultPosition, fieldSize, wxTE_PASSWORD );
    grid->Add(itemTextCtrl13, fld);

    ewxStaticText* itemStaticText14 = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_NEWPASSWD, _("New Password:"), wxDefaultPosition, wxDefaultSize, 0 );
    grid->Add(itemStaticText14, lab);

    ewxTextCtrl* itemTextCtrl15 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_AUTH_NEWPASSWORD, _T(""), wxDefaultPosition, fieldSize, wxTE_PASSWORD );
    grid->Add(itemTextCtrl15, fld);

    // Both lines are empty and hidden until a caller sets them.
    ewxStaticText* security = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_SECURITY, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    security->Show(false);
    column->Add(security, wxSizerFlags().Left().Border(wxTOP, gap));

    ewxStaticText* status = new ewxStaticText( itemDialog1, wxID_STATIC_AUTH_STATUS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    status->Show(false);
    column->Add(status, wxSizerFlags().Left().Border(wxTOP, gap));

    ewxCheckBox* itemCheckBox22 = new ewxCheckBox( itemDialog1, ID_CHECKBOX_AUTH_SAVEPASSWORDS, _("Save Passwords Between Invocations"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox22->SetValue(false);
    itemCheckBox22->Show(false);
    column->Add(itemCheckBox22, wxSizerFlags().Left().Border(wxTOP, gap));

    wxBoxSizer* buttonRow = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(buttonRow, wxSizerFlags().Expand().DoubleBorder());

    // Not a standard button, so it stays outside the wxStdDialogButtonSizer.
    ewxButton* itemButton20 = new ewxButton( itemDialog1, wxID_CHANGE, _("Change..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton20->Show(false);
    buttonRow->Add(itemButton20, wxSizerFlags().CenterVertical());
    buttonRow->AddStretchSpacer(1);

    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
    ewxButton* itemButton18 = new ewxButton( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton18->SetDefault();
    buttons->AddButton(itemButton18);
    ewxButton* itemButton21 = new ewxButton( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    buttons->AddButton(itemButton21);
    buttons->Realize();
    buttonRow->Add(buttons, wxSizerFlags().CenterVertical());

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
