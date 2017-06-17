/////////////////////////////////////////////////////////////////////////////
// Name:        MDTaskPanelGUI.C
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "MDTaskPanelGUI.H"
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

#include "MDTaskPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID MDTaskPanelGUI::ID_DIALOG = wxNewId();

/*!
 * MDTaskPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( MDTaskPanelGUI, ewxPanel )

/*!
 * MDTaskPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( MDTaskPanelGUI, ewxPanel )

////@begin MDTaskPanelGUI event table entries
////@end MDTaskPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * MDTaskPanelGUI constructors
 */

MDTaskPanelGUI::MDTaskPanelGUI( )
{
}

MDTaskPanelGUI::MDTaskPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * MDTaskPanelGUI creator
 */

bool MDTaskPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin MDTaskPanelGUI member initialisation
    p_topSizer = NULL;
    p_nameSizer = NULL;
    p_name = NULL;
    p_codeSizer = NULL;
    p_codeBitmap = NULL;
    p_code = NULL;
////@end MDTaskPanelGUI member initialisation

////@begin MDTaskPanelGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    Centre();
////@end MDTaskPanelGUI creation
    return true;
}

/*!
 * Control creation for MDTaskPanelGUI
 */

void MDTaskPanelGUI::CreateControls()
{    
////@begin MDTaskPanelGUI content construction
    MDTaskPanelGUI* itemPanel1 = this;

    p_topSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(p_topSizer);

    p_nameSizer = new wxBoxSizer(wxHORIZONTAL);
    p_topSizer->Add(p_nameSizer, 0, wxALIGN_LEFT|wxADJUST_MINSIZE, 5);

    p_name = new ewxStaticText( itemPanel1, wxID_STATIC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    p_nameSizer->Add(p_name, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    p_codeSizer = new wxBoxSizer(wxHORIZONTAL);
    p_topSizer->Add(p_codeSizer, 0, wxALIGN_LEFT|wxADJUST_MINSIZE, 5);

    wxBitmap p_codeBitmapBitmap(wxNullBitmap);
    p_codeBitmap = new wxStaticBitmap( itemPanel1, wxID_STATIC, p_codeBitmapBitmap, wxDefaultPosition, wxDefaultSize, 0 );
    p_codeSizer->Add(p_codeBitmap, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    p_code = new ewxStaticText( itemPanel1, wxID_STATIC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    p_codeSizer->Add(p_code, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

////@end MDTaskPanelGUI content construction
}

/*!
 * Should we show tooltips?
 */

bool MDTaskPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap MDTaskPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin MDTaskPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end MDTaskPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon MDTaskPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin MDTaskPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end MDTaskPanelGUI icon retrieval
}
