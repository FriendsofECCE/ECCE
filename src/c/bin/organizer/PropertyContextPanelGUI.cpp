/////////////////////////////////////////////////////////////////////////////
// Name:        PropertyContextPanelGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "PropertyContextPanelGUI.H"
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

#include "PropertyContextPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID PropertyContextPanelGUI::ID_DIALOG = wxNewId();

/*!
 * PropertyContextPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( PropertyContextPanelGUI, ewxPanel )

/*!
 * PropertyContextPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( PropertyContextPanelGUI, ewxPanel )

////@begin PropertyContextPanelGUI event table entries
////@end PropertyContextPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * PropertyContextPanelGUI constructors
 */

PropertyContextPanelGUI::PropertyContextPanelGUI( )
{
}

PropertyContextPanelGUI::PropertyContextPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * PropertyContextPanelGUI creator
 */

bool PropertyContextPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin PropertyContextPanelGUI member initialisation
    p_gridSizer = NULL;
////@end PropertyContextPanelGUI member initialisation

////@begin PropertyContextPanelGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end PropertyContextPanelGUI creation
    return true;
}

/*!
 * Control creation for PropertyContextPanelGUI
 */

void PropertyContextPanelGUI::CreateControls()
{    
////@begin PropertyContextPanelGUI content construction
    PropertyContextPanelGUI* itemPanel1 = this;

    p_gridSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(p_gridSizer);

////@end PropertyContextPanelGUI content construction
}

/*!
 * Should we show tooltips?
 */

bool PropertyContextPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap PropertyContextPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin PropertyContextPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end PropertyContextPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon PropertyContextPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin PropertyContextPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end PropertyContextPanelGUI icon retrieval
}
