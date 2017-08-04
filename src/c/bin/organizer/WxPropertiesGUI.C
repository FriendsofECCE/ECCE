/////////////////////////////////////////////////////////////////////////////
// Name:        WxPropertiesGUI.C
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxPropertiesGUI.H"
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
#include "wxgui/ewxFrame.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "WxPropertiesGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxPropertiesGUI::ID_BUTTON_CLOSE_PROPERTIES = wxNewId();
const wxWindowID WxPropertiesGUI::ID_TEXTCTRL_URL = wxNewId();
const wxWindowID WxPropertiesGUI::ID_FRAME_PROPERTIES_DIALOG = wxNewId();

/*!
 * WxPropertiesGUI type definition
 */

IMPLEMENT_CLASS( WxPropertiesGUI, ewxFrame )

/*!
 * WxPropertiesGUI event table definition
 */

BEGIN_EVENT_TABLE( WxPropertiesGUI, ewxFrame )

////@begin WxPropertiesGUI event table entries
    EVT_CLOSE( WxPropertiesGUI::OnCloseWindow )

    EVT_BUTTON( ID_BUTTON_CLOSE_PROPERTIES, WxPropertiesGUI::OnButtonClosePropertiesClick )

////@end WxPropertiesGUI event table entries

END_EVENT_TABLE()

/*!
 * WxPropertiesGUI constructors
 */

WxPropertiesGUI::WxPropertiesGUI( )
{
}

WxPropertiesGUI::WxPropertiesGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * WxPropertiesGUI creator
 */

bool WxPropertiesGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxPropertiesGUI member initialisation
    p_topSizer = NULL;
    p_propSizer = NULL;
////@end WxPropertiesGUI member initialisation

////@begin WxPropertiesGUI creation
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    Centre();
////@end WxPropertiesGUI creation
    return true;
}

/*!
 * Control creation for WxPropertiesGUI
 */

void WxPropertiesGUI::CreateControls()
{    
////@begin WxPropertiesGUI content construction
    WxPropertiesGUI* itemFrame1 = this;

    p_topSizer = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(p_topSizer);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    p_topSizer->Add(itemBoxSizer3, 0, wxGROW|wxALL, 5);

    ewxTextCtrl* itemTextCtrl4 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_URL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemTextCtrl4, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_propSizer = new wxBoxSizer(wxHORIZONTAL);
    p_topSizer->Add(p_propSizer, 1, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine6 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    p_topSizer->Add(itemStaticLine6, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    p_topSizer->Add(itemBoxSizer7, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    ewxButton* itemButton8 = new ewxButton( itemFrame1, ID_BUTTON_CLOSE_PROPERTIES, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemButton8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end WxPropertiesGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_FRAME_PROPERTIES_DIALOG
 */

void WxPropertiesGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_FRAME_PROPERTIES_DIALOG in WxPropertiesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_FRAME_PROPERTIES_DIALOG in WxPropertiesGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CLOSE_PROPERTIES
 */

void WxPropertiesGUI::OnButtonClosePropertiesClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CLOSE_PROPERTIES in WxPropertiesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CLOSE_PROPERTIES in WxPropertiesGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxPropertiesGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxPropertiesGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxPropertiesGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxPropertiesGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxPropertiesGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxPropertiesGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxPropertiesGUI icon retrieval
}
