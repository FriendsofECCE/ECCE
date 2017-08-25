/////////////////////////////////////////////////////////////////////////////
// Name:        WxMachineStatusGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxMachineStatusGUI.H"
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
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "WxMachineStatusGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxMachineStatusGUI::ID_STATIC_WXMACHINESTATUS_MODE = wxNewId();
const wxWindowID WxMachineStatusGUI::ID_TEXTCTRL_WXMACHINESTATUS_STATUS = wxNewId();
const wxWindowID WxMachineStatusGUI::ID_PANEL_WXMACHINESTATUS_MAIN = wxNewId();
const wxWindowID WxMachineStatusGUI::ID_WXMACHINESTATUS_FRAME = wxNewId();
const wxWindowID WxMachineStatusGUI::ID_BUTTON_WXMACHINESTATUS_CLOSE = wxNewId();

/*!
 * WxMachineStatusGUI type definition
 */

IMPLEMENT_CLASS( WxMachineStatusGUI, ewxDialog )

/*!
 * WxMachineStatusGUI event table definition
 */

BEGIN_EVENT_TABLE( WxMachineStatusGUI, ewxDialog )

////@begin WxMachineStatusGUI event table entries
    EVT_CLOSE( WxMachineStatusGUI::mainWindowCloseCB )

    EVT_BUTTON( ID_BUTTON_WXMACHINESTATUS_CLOSE, WxMachineStatusGUI::closeButtonClickCB )

////@end WxMachineStatusGUI event table entries

END_EVENT_TABLE()

/*!
 * WxMachineStatusGUI constructors
 */

WxMachineStatusGUI::WxMachineStatusGUI( )
{
}

WxMachineStatusGUI::WxMachineStatusGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * WxMachineStatusGUI creator
 */

bool WxMachineStatusGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxMachineStatusGUI member initialisation
////@end WxMachineStatusGUI member initialisation

////@begin WxMachineStatusGUI creation
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    Centre();
////@end WxMachineStatusGUI creation
    return true;
}

/*!
 * Control creation for WxMachineStatusGUI
 */

void WxMachineStatusGUI::CreateControls()
{    
////@begin WxMachineStatusGUI content construction
    WxMachineStatusGUI* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    ewxPanel* itemPanel3 = new ewxPanel( itemDialog1, ID_PANEL_WXMACHINESTATUS_MAIN, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer2->Add(itemPanel3, 1, wxGROW|wxALL|wxFIXED_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    itemPanel3->SetSizer(itemBoxSizer4);

    ewxStaticText* itemStaticText5 = new ewxStaticText( itemPanel3, ID_STATIC_WXMACHINESTATUS_MODE, _("(Mode)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText5->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemBoxSizer4->Add(itemStaticText5, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl6 = new ewxTextCtrl( itemPanel3, ID_TEXTCTRL_WXMACHINESTATUS_STATUS, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxHSCROLL );
    itemTextCtrl6->SetFont(wxFont(10, wxTELETYPE, wxNORMAL, wxNORMAL, false, _T("Courier 10 Pitch")));
    itemBoxSizer4->Add(itemTextCtrl6, 1, wxGROW|wxALL, 3);

    ewxButton* itemButton7 = new ewxButton( itemPanel3, ID_BUTTON_WXMACHINESTATUS_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemButton7, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

////@end WxMachineStatusGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_WXMACHINESTATUS_FRAME
 */

void WxMachineStatusGUI::mainWindowCloseCB( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_WXMACHINESTATUS_FRAME in WxMachineStatusGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_WXMACHINESTATUS_FRAME in WxMachineStatusGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXMACHINESTATUS_CLOSE
 */

void WxMachineStatusGUI::closeButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXMACHINESTATUS_CLOSE in WxMachineStatusGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXMACHINESTATUS_CLOSE in WxMachineStatusGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxMachineStatusGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxMachineStatusGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxMachineStatusGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxMachineStatusGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxMachineStatusGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxMachineStatusGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxMachineStatusGUI icon retrieval
}
