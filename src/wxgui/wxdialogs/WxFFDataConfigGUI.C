/////////////////////////////////////////////////////////////////////////////
// Name:        WxFFDataConfigGUI.C
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxFFDataConfigGUI.H"
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
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxStaticText.H"
////@end includes

#include "wxgui/WxFFDataConfigGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxFFDataConfigGUI::ID_BUTTON_SEG_UP = wxNewId();
const wxWindowID WxFFDataConfigGUI::ID_CHOICE_FORCE_FIELD = wxNewId();
const wxWindowID WxFFDataConfigGUI::ID_BUTTON_SEG_DOWN = wxNewId();
const wxWindowID WxFFDataConfigGUI::ID_BUTTON_FF_BROWSE = wxNewId();
const wxWindowID WxFFDataConfigGUI::ID_LISTBOX_SEG = wxNewId();
const wxWindowID WxFFDataConfigGUI::ID_BUTTON_SEG_REMOVE = wxNewId();
const wxWindowID WxFFDataConfigGUI::ID_DIALOG = wxNewId();
const wxWindowID WxFFDataConfigGUI::ID_BUTTON_FF_DOWN = wxNewId();
const wxWindowID WxFFDataConfigGUI::ID_LISTBOX_FF = wxNewId();
const wxWindowID WxFFDataConfigGUI::ID_BUTTON_DEFAULTS = wxNewId();
const wxWindowID WxFFDataConfigGUI::ID_BUTTON_FF_REMOVE = wxNewId();
const wxWindowID WxFFDataConfigGUI::ID_BUTTON_FF_UP = wxNewId();
const wxWindowID WxFFDataConfigGUI::ID_BUTTON_SEG_BROWSE = wxNewId();

/*!
 * WxFFDataConfigGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WxFFDataConfigGUI, ewxDialog )

/*!
 * WxFFDataConfigGUI event table definition
 */

BEGIN_EVENT_TABLE( WxFFDataConfigGUI, ewxDialog )

////@begin WxFFDataConfigGUI event table entries
    EVT_CLOSE( WxFFDataConfigGUI::OnCloseWindow )

    EVT_CHOICE( ID_CHOICE_FORCE_FIELD, WxFFDataConfigGUI::OnChoiceForceFieldSelected )

    EVT_BUTTON( ID_BUTTON_FF_BROWSE, WxFFDataConfigGUI::OnButtonFfBrowseClick )

    EVT_BUTTON( ID_BUTTON_FF_REMOVE, WxFFDataConfigGUI::OnButtonFfRemoveClick )

    EVT_BUTTON( ID_BUTTON_FF_UP, WxFFDataConfigGUI::OnButtonFfUpClick )

    EVT_BUTTON( ID_BUTTON_FF_DOWN, WxFFDataConfigGUI::OnButtonFfDownClick )

    EVT_BUTTON( ID_BUTTON_SEG_BROWSE, WxFFDataConfigGUI::OnButtonSegBrowseClick )

    EVT_BUTTON( ID_BUTTON_SEG_REMOVE, WxFFDataConfigGUI::OnButtonSegRemoveClick )

    EVT_BUTTON( ID_BUTTON_SEG_UP, WxFFDataConfigGUI::OnButtonSegUpClick )

    EVT_BUTTON( ID_BUTTON_SEG_DOWN, WxFFDataConfigGUI::OnButtonSegDownClick )

    EVT_BUTTON( ID_BUTTON_DEFAULTS, WxFFDataConfigGUI::OnButtonDefaultsClick )

    EVT_BUTTON( wxID_CLOSE, WxFFDataConfigGUI::OnCloseClick )

    EVT_BUTTON( wxID_HELP, WxFFDataConfigGUI::OnHelpClick )

////@end WxFFDataConfigGUI event table entries

END_EVENT_TABLE()

/*!
 * WxFFDataConfigGUI constructors
 */

WxFFDataConfigGUI::WxFFDataConfigGUI( )
{
}

WxFFDataConfigGUI::WxFFDataConfigGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * WxFFDataConfigGUI creator
 */

bool WxFFDataConfigGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxFFDataConfigGUI member initialisation
////@end WxFFDataConfigGUI member initialisation

////@begin WxFFDataConfigGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WxFFDataConfigGUI creation
    return true;
}

/*!
 * Control creation for WxFFDataConfigGUI
 */

void WxFFDataConfigGUI::CreateControls()
{    
////@begin WxFFDataConfigGUI content construction
    WxFFDataConfigGUI* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText4 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Force Field:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemChoice5Strings[] = {
        _("NWChem")
    };
    ewxChoice* itemChoice5 = new ewxChoice( itemDialog1, ID_CHOICE_FORCE_FIELD, wxDefaultPosition, wxDefaultSize, 1, itemChoice5Strings, 0 );
    itemChoice5->SetStringSelection(_("NWChem"));
    itemBoxSizer3->Add(itemChoice5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer6, 0, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemDialog1, wxID_STATIC, _("NWChem Parameter Files"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemStaticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer8, 1, wxGROW|wxRIGHT|wxTOP|wxBOTTOM, 0);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer8->Add(itemBoxSizer9, 1, wxGROW|wxALL, 0);

    wxString* itemListBox10Strings = NULL;
    ewxListBox* itemListBox10 = new ewxListBox( itemDialog1, ID_LISTBOX_FF, wxDefaultPosition, wxDefaultSize, 0, itemListBox10Strings, wxLB_SINGLE );
    itemBoxSizer9->Add(itemListBox10, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer8->Add(itemBoxSizer11, 0, wxALIGN_TOP|wxALL, 0);

    ewxButton* itemButton12 = new ewxButton( itemDialog1, ID_BUTTON_FF_BROWSE, _("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemButton12, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxButton* itemButton13 = new ewxButton( itemDialog1, ID_BUTTON_FF_REMOVE, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemButton13, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxButton* itemButton14 = new ewxButton( itemDialog1, ID_BUTTON_FF_UP, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemButton14, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxButton* itemButton15 = new ewxButton( itemDialog1, ID_BUTTON_FF_DOWN, _("Move Down"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemButton15, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer16, 0, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText17 = new ewxStaticText( itemDialog1, wxID_STATIC, _("NWChem Segment Directories"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add(itemStaticText17, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer18, 1, wxGROW|wxRIGHT|wxTOP|wxBOTTOM, 0);

    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer18->Add(itemBoxSizer19, 1, wxGROW|wxALL, 0);

    wxString* itemListBox20Strings = NULL;
    ewxListBox* itemListBox20 = new ewxListBox( itemDialog1, ID_LISTBOX_SEG, wxDefaultPosition, wxDefaultSize, 0, itemListBox20Strings, wxLB_SINGLE );
    itemBoxSizer19->Add(itemListBox20, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer18->Add(itemBoxSizer21, 0, wxALIGN_TOP|wxALL, 0);

    ewxButton* itemButton22 = new ewxButton( itemDialog1, ID_BUTTON_SEG_BROWSE, _("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(itemButton22, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxButton* itemButton23 = new ewxButton( itemDialog1, ID_BUTTON_SEG_REMOVE, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(itemButton23, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxButton* itemButton24 = new ewxButton( itemDialog1, ID_BUTTON_SEG_UP, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(itemButton24, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxButton* itemButton25 = new ewxButton( itemDialog1, ID_BUTTON_SEG_DOWN, _("Move Down"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(itemButton25, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer26 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer26, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    itemBoxSizer26->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton28 = new ewxButton( itemDialog1, ID_BUTTON_DEFAULTS, _("Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemButton28, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer26->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton30 = new ewxButton( itemDialog1, wxID_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemButton30, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer26->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton32 = new ewxButton( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer26->Add(itemButton32, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer26->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end WxFFDataConfigGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
 */

void WxFFDataConfigGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_DIALOG in WxFFDataConfigGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_DIALOG in WxFFDataConfigGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_FORCE_FIELD
 */

void WxFFDataConfigGUI::OnChoiceForceFieldSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_FORCE_FIELD in WxFFDataConfigGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_FORCE_FIELD in WxFFDataConfigGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_BROWSE
 */

void WxFFDataConfigGUI::OnButtonFfBrowseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_BROWSE in WxFFDataConfigGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_BROWSE in WxFFDataConfigGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_REMOVE
 */

void WxFFDataConfigGUI::OnButtonFfRemoveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_REMOVE in WxFFDataConfigGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_REMOVE in WxFFDataConfigGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_UP
 */

void WxFFDataConfigGUI::OnButtonFfUpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_UP in WxFFDataConfigGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_UP in WxFFDataConfigGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_DOWN
 */

void WxFFDataConfigGUI::OnButtonFfDownClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_DOWN in WxFFDataConfigGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_DOWN in WxFFDataConfigGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_BROWSE
 */

void WxFFDataConfigGUI::OnButtonSegBrowseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_BROWSE in WxFFDataConfigGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_BROWSE in WxFFDataConfigGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_REMOVE
 */

void WxFFDataConfigGUI::OnButtonSegRemoveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_REMOVE in WxFFDataConfigGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_REMOVE in WxFFDataConfigGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_UP
 */

void WxFFDataConfigGUI::OnButtonSegUpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_UP in WxFFDataConfigGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_UP in WxFFDataConfigGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_DOWN
 */

void WxFFDataConfigGUI::OnButtonSegDownClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_DOWN in WxFFDataConfigGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_DOWN in WxFFDataConfigGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DEFAULTS
 */

void WxFFDataConfigGUI::OnButtonDefaultsClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DEFAULTS in WxFFDataConfigGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DEFAULTS in WxFFDataConfigGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void WxFFDataConfigGUI::OnCloseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in WxFFDataConfigGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in WxFFDataConfigGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void WxFFDataConfigGUI::OnHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP in WxFFDataConfigGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP in WxFFDataConfigGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxFFDataConfigGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxFFDataConfigGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxFFDataConfigGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxFFDataConfigGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxFFDataConfigGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxFFDataConfigGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxFFDataConfigGUI icon retrieval
}
