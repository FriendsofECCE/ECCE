/////////////////////////////////////////////////////////////////////////////
// Name:        WxConfigureNwfsArchiveGUI.C
// Purpose:     
// Author:      Jack Chatterton
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxConfigureNwfsArchiveGUI.H"
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
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "WxConfigureNwfsArchiveGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxConfigureNwfsArchiveGUI::ID_CANCEL = wxNewId();
const wxWindowID WxConfigureNwfsArchiveGUI::ID_ROOTDIR = wxNewId();
const wxWindowID WxConfigureNwfsArchiveGUI::ID_PASSWORD = wxNewId();
const wxWindowID WxConfigureNwfsArchiveGUI::ID_USERNAME = wxNewId();
const wxWindowID WxConfigureNwfsArchiveGUI::ID_OK = wxNewId();
const wxWindowID WxConfigureNwfsArchiveGUI::wxID_CONFIGURE_NWFS_ARCHIVE = wxNewId();

/*!
 * WxConfigureNwfsArchiveGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WxConfigureNwfsArchiveGUI, ewxDialog )

/*!
 * WxConfigureNwfsArchiveGUI event table definition
 */

BEGIN_EVENT_TABLE( WxConfigureNwfsArchiveGUI, ewxDialog )

////@begin WxConfigureNwfsArchiveGUI event table entries
    EVT_BUTTON( ID_OK, WxConfigureNwfsArchiveGUI::OnOkClick )

    EVT_BUTTON( ID_CANCEL, WxConfigureNwfsArchiveGUI::OnCancelClick )

////@end WxConfigureNwfsArchiveGUI event table entries

END_EVENT_TABLE()

/*!
 * WxConfigureNwfsArchiveGUI constructors
 */

WxConfigureNwfsArchiveGUI::WxConfigureNwfsArchiveGUI( )
{
}

WxConfigureNwfsArchiveGUI::WxConfigureNwfsArchiveGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * WxConfigureNwfsArchiveGUI creator
 */

bool WxConfigureNwfsArchiveGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxConfigureNwfsArchiveGUI member initialisation
////@end WxConfigureNwfsArchiveGUI member initialisation

////@begin WxConfigureNwfsArchiveGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WxConfigureNwfsArchiveGUI creation
    return true;
}

/*!
 * Control creation for WxConfigureNwfsArchiveGUI
 */

void WxConfigureNwfsArchiveGUI::CreateControls()
{    
////@begin WxConfigureNwfsArchiveGUI content construction
    WxConfigureNwfsArchiveGUI* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(2, 4, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(2);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText4 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Username:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl5 = new ewxTextCtrl( itemDialog1, ID_USERNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemTextCtrl5, 0, wxALIGN_LEFT|wxGROW|wxALL, 5);

    ewxStaticText* itemStaticText6 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Password:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
    itemFlexGridSizer3->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl7 = new ewxTextCtrl( itemDialog1, ID_PASSWORD, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    itemFlexGridSizer3->Add(itemTextCtrl7, 0, wxALIGN_LEFT|wxGROW|wxALL, 5);

    ewxStaticText* itemStaticText8 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Root Directory:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText8, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl9 = new ewxTextCtrl( itemDialog1, ID_ROOTDIR, _T(""), wxDefaultPosition, wxSize(125, -1), 0 );
    itemFlexGridSizer3->Add(itemTextCtrl9, 100, wxGROW|wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton11 = new ewxButton( itemDialog1, ID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton11, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxButton* itemButton12 = new ewxButton( itemDialog1, ID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemButton12, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

////@end WxConfigureNwfsArchiveGUI content construction
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_OK
 */

void WxConfigureNwfsArchiveGUI::OnOkClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_OK in WxConfigureNwfsArchiveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_OK in WxConfigureNwfsArchiveGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CANCEL
 */

void WxConfigureNwfsArchiveGUI::OnCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CANCEL in WxConfigureNwfsArchiveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CANCEL in WxConfigureNwfsArchiveGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxConfigureNwfsArchiveGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxConfigureNwfsArchiveGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxConfigureNwfsArchiveGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxConfigureNwfsArchiveGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxConfigureNwfsArchiveGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxConfigureNwfsArchiveGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxConfigureNwfsArchiveGUI icon retrieval
}
