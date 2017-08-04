/////////////////////////////////////////////////////////////////////////////
// Name:        ProjectContextPanelGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ProjectContextPanelGUI.H"
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

#include "ProjectContextPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID ProjectContextPanelGUI::ID_DIALOG = wxNewId();

/*!
 * ProjectContextPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( ProjectContextPanelGUI, ewxPanel )

/*!
 * ProjectContextPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( ProjectContextPanelGUI, ewxPanel )

////@begin ProjectContextPanelGUI event table entries
////@end ProjectContextPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * ProjectContextPanelGUI constructors
 */

ProjectContextPanelGUI::ProjectContextPanelGUI( )
{
}

ProjectContextPanelGUI::ProjectContextPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * ProjectContextPanelGUI creator
 */

bool ProjectContextPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ProjectContextPanelGUI member initialisation
////@end ProjectContextPanelGUI member initialisation

////@begin ProjectContextPanelGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
////@end ProjectContextPanelGUI creation
    return true;
}

/*!
 * Control creation for ProjectContextPanelGUI
 */

void ProjectContextPanelGUI::CreateControls()
{    
////@begin ProjectContextPanelGUI content construction
    ProjectContextPanelGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel1->SetSizer(itemBoxSizer2);

////@end ProjectContextPanelGUI content construction
}

/*!
 * Should we show tooltips?
 */

bool ProjectContextPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap ProjectContextPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ProjectContextPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end ProjectContextPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ProjectContextPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ProjectContextPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end ProjectContextPanelGUI icon retrieval
}
