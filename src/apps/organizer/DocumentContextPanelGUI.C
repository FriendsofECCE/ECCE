/////////////////////////////////////////////////////////////////////////////
// Name:        DocumentContextPanelGUI.C
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "DocumentContextPanelGUI.H"
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
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "DocumentContextPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID DocumentContextPanelGUI::ID_PANEL_DOCUMENT_CONTEXT_TOP = wxNewId();
const wxWindowID DocumentContextPanelGUI::ID_TEXTCTRL = wxNewId();

/*!
 * DocumentContextPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( DocumentContextPanelGUI, ewxPanel )

/*!
 * DocumentContextPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( DocumentContextPanelGUI, ewxPanel )

////@begin DocumentContextPanelGUI event table entries
////@end DocumentContextPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * DocumentContextPanelGUI constructors
 */

DocumentContextPanelGUI::DocumentContextPanelGUI( )
{
}

DocumentContextPanelGUI::DocumentContextPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * DocumentContextPanelGUI creator
 */

bool DocumentContextPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin DocumentContextPanelGUI member initialisation
    p_topSizer = NULL;
    p_textDisplay = NULL;
////@end DocumentContextPanelGUI member initialisation

////@begin DocumentContextPanelGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end DocumentContextPanelGUI creation
    return true;
}

/*!
 * Control creation for DocumentContextPanelGUI
 */

void DocumentContextPanelGUI::CreateControls()
{    
////@begin DocumentContextPanelGUI content construction
    DocumentContextPanelGUI* itemPanel1 = this;

    p_topSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(p_topSizer);

    p_textDisplay = new ewxTextCtrl( itemPanel1, ID_TEXTCTRL, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxHSCROLL|wxSUNKEN_BORDER );
    p_topSizer->Add(p_textDisplay, 1, wxGROW, 5);

////@end DocumentContextPanelGUI content construction
}

/*!
 * Should we show tooltips?
 */

bool DocumentContextPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap DocumentContextPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin DocumentContextPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end DocumentContextPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon DocumentContextPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin DocumentContextPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end DocumentContextPanelGUI icon retrieval
}
