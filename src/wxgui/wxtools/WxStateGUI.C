/////////////////////////////////////////////////////////////////////////////
// Name:        WxStateGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxStateGUI.H"
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
////@end includes

#include "wxgui/WxStateGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxStateGUI::ID_STATE = wxNewId();

/*!
 * WxStateGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WxStateGUI, ewxPanel )

/*!
 * WxStateGUI event table definition
 */

BEGIN_EVENT_TABLE( WxStateGUI, ewxPanel )

////@begin WxStateGUI event table entries
    EVT_PAINT( WxStateGUI::OnPaint )

////@end WxStateGUI event table entries

END_EVENT_TABLE()

/*!
 * WxStateGUI constructors
 */

WxStateGUI::WxStateGUI( )
{
}

WxStateGUI::WxStateGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * WxStateGUI creator
 */

bool WxStateGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxStateGUI member initialisation
////@end WxStateGUI member initialisation

////@begin WxStateGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    Centre();
////@end WxStateGUI creation
    return true;
}

/*!
 * Control creation for WxStateGUI
 */

void WxStateGUI::CreateControls()
{    
////@begin WxStateGUI content construction
#if 000
    WxStateGUI* itemPanel1 = this;
#endif

////@end WxStateGUI content construction
}

/*!
 * wxEVT_PAINT event handler for ID_STATE
 */

void WxStateGUI::OnPaint( wxPaintEvent& event )
{
////@begin wxEVT_PAINT event handler for ID_STATE in WxStateGUI.
    // Before editing this code, remove the block markers.
    wxPaintDC dc(this);
////@end wxEVT_PAINT event handler for ID_STATE in WxStateGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxStateGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxStateGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxStateGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxStateGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxStateGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxStateGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxStateGUI icon retrieval
}
