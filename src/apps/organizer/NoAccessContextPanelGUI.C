/////////////////////////////////////////////////////////////////////////////
// Name:        NoAccessContextPanelGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "NoAccessContextPanelGUI.H"
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
#include "wxgui/ewxStaticText.H"
////@end includes

#include "NoAccessContextPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID NoAccessContextPanelGUI::ID_DIALOG = wxNewId();

/*!
 * NoAccessContextPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( NoAccessContextPanelGUI, ewxPanel )

/*!
 * NoAccessContextPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( NoAccessContextPanelGUI, ewxPanel )

////@begin NoAccessContextPanelGUI event table entries
////@end NoAccessContextPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * NoAccessContextPanelGUI constructors
 */

NoAccessContextPanelGUI::NoAccessContextPanelGUI( )
{
}

NoAccessContextPanelGUI::NoAccessContextPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * NoAccessContextPanelGUI creator
 */

bool NoAccessContextPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin NoAccessContextPanelGUI member initialisation
////@end NoAccessContextPanelGUI member initialisation

////@begin NoAccessContextPanelGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
////@end NoAccessContextPanelGUI creation
    return true;
}

/*!
 * Control creation for NoAccessContextPanelGUI
 */

void NoAccessContextPanelGUI::CreateControls()
{    
////@begin NoAccessContextPanelGUI content construction
    NoAccessContextPanelGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    ewxStaticText* itemStaticText3 = new ewxStaticText( itemPanel1, wxID_STATIC, _("The content of this project is password protected.\nTo re-enter password, click on the refresh button\non the tool bar."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText3, 0, wxALIGN_LEFT|wxALL|wxFIXED_MINSIZE, 20);

////@end NoAccessContextPanelGUI content construction
}

/*!
 * Should we show tooltips?
 */

bool NoAccessContextPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap NoAccessContextPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin NoAccessContextPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end NoAccessContextPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon NoAccessContextPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin NoAccessContextPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end NoAccessContextPanelGUI icon retrieval
}
