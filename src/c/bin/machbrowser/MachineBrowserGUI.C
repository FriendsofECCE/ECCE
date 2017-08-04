/////////////////////////////////////////////////////////////////////////////
// Name:        MachineBrowserGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "MachineBrowserGUI.H"
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
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxFrame.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "MachineBrowserGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID MachineBrowserGUI::ID_STATICTEXT_MACHINEBROWSER_MACHINENAME = wxNewId();
const wxWindowID MachineBrowserGUI::ID_BUTTON_MACHINEBROWSER_VIEWDETACH = wxNewId();
const wxWindowID MachineBrowserGUI::ID_STATIC_MACHINEBROWSER_MODE = wxNewId();
const wxWindowID MachineBrowserGUI::ID_MENUITEM_MACHINEBROWSER_HELPSUPPORT = wxNewId();
const wxWindowID MachineBrowserGUI::ID_MACHINEBROWSER_FRAME = wxNewId();
const wxWindowID MachineBrowserGUI::ID_MENU_MACHINEBROWSER_MACHINEREGISTER = wxNewId();
const wxWindowID MachineBrowserGUI::ID_BITMAP_MACHINEBROWSER_CHECKMARK = wxNewId();
const wxWindowID MachineBrowserGUI::ID_PANEL_MACHINEBROWSER_MACHINES = wxNewId();
const wxWindowID MachineBrowserGUI::ID_BUTTON_MACHINEBROWSER_QUERYPROCESS = wxNewId();
const wxWindowID MachineBrowserGUI::ID_TEXTCTRL_MACHINEBROWSER_STATUS = wxNewId();
const wxWindowID MachineBrowserGUI::ID_BUTTON_MACHINEBROWSER_QUERYDISK = wxNewId();
const wxWindowID MachineBrowserGUI::ID_PANEL_MACHINEBROWSER_MAIN = wxNewId();
const wxWindowID MachineBrowserGUI::ID_BUTTON_MACHINEBROWSER_QUERYMACHINE = wxNewId();
const wxWindowID MachineBrowserGUI::ID_BUTTON_MACHINEBROWSER_CONFIGURE = wxNewId();

/*!
 * MachineBrowserGUI type definition
 */

IMPLEMENT_CLASS( MachineBrowserGUI, ewxFrame )

/*!
 * MachineBrowserGUI event table definition
 */

BEGIN_EVENT_TABLE( MachineBrowserGUI, ewxFrame )

////@begin MachineBrowserGUI event table entries
    EVT_CLOSE( MachineBrowserGUI::mainWindowCloseCB )

    EVT_BUTTON( ID_BUTTON_MACHINEBROWSER_QUERYMACHINE, MachineBrowserGUI::machineQueryButtonClickCB )

    EVT_BUTTON( ID_BUTTON_MACHINEBROWSER_QUERYDISK, MachineBrowserGUI::diskQueryButtonClickCB )

    EVT_BUTTON( ID_BUTTON_MACHINEBROWSER_QUERYPROCESS, MachineBrowserGUI::processQueryButtonClickCB )

    EVT_BUTTON( ID_BUTTON_MACHINEBROWSER_CONFIGURE, MachineBrowserGUI::configureButtonClickCB )

    EVT_BUTTON( ID_BUTTON_MACHINEBROWSER_VIEWDETACH, MachineBrowserGUI::viewDetachButtonClickCB )

    EVT_MENU( ID_MENU_MACHINEBROWSER_MACHINEREGISTER, MachineBrowserGUI::machineRegisterMenuItemClickCB )

    EVT_MENU( wxID_EXIT, MachineBrowserGUI::exitMenuitemClickCB )

    EVT_MENU( wxID_HELP, MachineBrowserGUI::helpAppMenuitemClickCB )

    EVT_MENU( ID_MENUITEM_MACHINEBROWSER_HELPSUPPORT, MachineBrowserGUI::helpSupportMenuitemClickCB )

////@end MachineBrowserGUI event table entries

END_EVENT_TABLE()

/*!
 * MachineBrowserGUI constructors
 */

MachineBrowserGUI::MachineBrowserGUI( )
{
}

MachineBrowserGUI::MachineBrowserGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * MachineBrowserGUI creator
 */

bool MachineBrowserGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin MachineBrowserGUI member initialisation
////@end MachineBrowserGUI member initialisation

////@begin MachineBrowserGUI creation
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end MachineBrowserGUI creation
    return true;
}

/*!
 * Control creation for MachineBrowserGUI
 */

void MachineBrowserGUI::CreateControls()
{    
////@begin MachineBrowserGUI content construction
    MachineBrowserGUI* itemFrame1 = this;

    ewxMenuBar* menuBar = new ewxMenuBar;
    wxMenu* itemMenu27 = new wxMenu;
    itemMenu27->Append(ID_MENU_MACHINEBROWSER_MACHINEREGISTER, _("Register Machines..."), _T(""), wxITEM_NORMAL);
    itemMenu27->AppendSeparator();
    itemMenu27->Append(wxID_EXIT, _("Quit\tCtrl+Q"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu27, _("Machine"));
    wxMenu* itemMenu31 = new wxMenu;
    itemMenu31->Append(wxID_HELP, _("on Machine Browser..."), _T(""), wxITEM_NORMAL);
    itemMenu31->Append(ID_MENUITEM_MACHINEBROWSER_HELPSUPPORT, _("Support..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu31, _("Help"));
    itemFrame1->SetMenuBar(menuBar);

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(itemBoxSizer2);

    ewxPanel* itemPanel3 = new ewxPanel( itemFrame1, ID_PANEL_MACHINEBROWSER_MAIN, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer2->Add(itemPanel3, 1, wxGROW|wxALL|wxFIXED_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    itemPanel3->SetSizer(itemBoxSizer4);

    ewxPanel* itemPanel5 = new ewxPanel( itemPanel3, ID_PANEL_MACHINEBROWSER_MACHINES, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer4->Add(itemPanel5, 3, wxGROW, 3);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxVERTICAL);
    itemPanel5->SetSizer(itemBoxSizer6);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer7, 0, wxGROW, 3);

    itemBoxSizer7->Add(0, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBitmap itemStaticBitmap9Bitmap(itemFrame1->GetBitmapResource(wxT("../../../data/client/pixmaps/check16.xpm")));
    wxStaticBitmap* itemStaticBitmap9 = new wxStaticBitmap( itemPanel3, ID_BITMAP_MACHINEBROWSER_CHECKMARK, itemStaticBitmap9Bitmap, wxDefaultPosition, wxSize(16, 16), wxNO_BORDER );
    itemBoxSizer7->Add(itemStaticBitmap9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText10 = new ewxStaticText( itemPanel3, wxID_STATIC, _("= Setup for Remote Access"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemStaticText10, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer7->Add(15, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText12 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Query"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
    itemBoxSizer7->Add(itemStaticText12, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxButton* itemButton13 = new ewxButton( itemPanel3, ID_BUTTON_MACHINEBROWSER_QUERYMACHINE, _("Machine"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemButton13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton14 = new ewxButton( itemPanel3, ID_BUTTON_MACHINEBROWSER_QUERYDISK, _("Disk Usage"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemButton14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton15 = new ewxButton( itemPanel3, ID_BUTTON_MACHINEBROWSER_QUERYPROCESS, _("Process Status"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemButton15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticLine* itemStaticLine16 = new ewxStaticLine( itemPanel3, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer4->Add(itemStaticLine16, 0, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer17, 0, wxGROW, 3);

    ewxStaticText* itemStaticText18 = new ewxStaticText( itemPanel3, ID_STATICTEXT_MACHINEBROWSER_MACHINENAME, _("(machine full name)"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer17->Add(itemStaticText18, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer17->Add(0, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText20 = new ewxStaticText( itemPanel3, ID_STATIC_MACHINEBROWSER_MODE, _("(Mode)"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemBoxSizer17->Add(itemStaticText20, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer17->Add(15, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton22 = new ewxButton( itemPanel3, ID_BUTTON_MACHINEBROWSER_CONFIGURE, _("Setup Remote Access..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton22->Enable(false);
    itemBoxSizer17->Add(itemButton22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton23 = new ewxButton( itemPanel3, ID_BUTTON_MACHINEBROWSER_VIEWDETACH, _("Show in Child Window..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton23->Enable(false);
    itemBoxSizer17->Add(itemButton23, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3);

    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer4->Add(itemBoxSizer24, 3, wxGROW|wxALL, 3);

    ewxTextCtrl* itemTextCtrl25 = new ewxTextCtrl( itemPanel3, ID_TEXTCTRL_MACHINEBROWSER_STATUS, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxHSCROLL );
    itemTextCtrl25->SetFont(wxFont(10, wxTELETYPE, wxNORMAL, wxNORMAL, false, _T("Courier 10 Pitch")));
    itemBoxSizer24->Add(itemTextCtrl25, 1, wxGROW|wxALL, 3);

////@end MachineBrowserGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_MACHINEBROWSER_FRAME
 */

void MachineBrowserGUI::mainWindowCloseCB( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_MACHINEBROWSER_FRAME in MachineBrowserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_MACHINEBROWSER_FRAME in MachineBrowserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_QUERYMACHINE
 */

void MachineBrowserGUI::machineQueryButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_QUERYMACHINE in MachineBrowserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_QUERYMACHINE in MachineBrowserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_QUERYDISK
 */

void MachineBrowserGUI::diskQueryButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_QUERYDISK in MachineBrowserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_QUERYDISK in MachineBrowserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_QUERYPROCESS
 */

void MachineBrowserGUI::processQueryButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_QUERYPROCESS in MachineBrowserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_QUERYPROCESS in MachineBrowserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_CONFIGURE
 */

void MachineBrowserGUI::configureButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_CONFIGURE in MachineBrowserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_CONFIGURE in MachineBrowserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_VIEWDETACH
 */

void MachineBrowserGUI::viewDetachButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_VIEWDETACH in MachineBrowserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_VIEWDETACH in MachineBrowserGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_MACHINEBROWSER_MACHINEREGISTER
 */

void MachineBrowserGUI::machineRegisterMenuItemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_MACHINEBROWSER_MACHINEREGISTER in MachineBrowserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_MACHINEBROWSER_MACHINEREGISTER in MachineBrowserGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 */

void MachineBrowserGUI::exitMenuitemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in MachineBrowserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in MachineBrowserGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
 */

void MachineBrowserGUI::helpAppMenuitemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in MachineBrowserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in MachineBrowserGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_MACHINEBROWSER_HELPSUPPORT
 */

void MachineBrowserGUI::helpSupportMenuitemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_MACHINEBROWSER_HELPSUPPORT in MachineBrowserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_MACHINEBROWSER_HELPSUPPORT in MachineBrowserGUI. 
}

/*!
 * Should we show tooltips?
 */

bool MachineBrowserGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap MachineBrowserGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin MachineBrowserGUI bitmap retrieval
    wxUnusedVar(name);
    if (name == wxT("../../../data/client/pixmaps/check16.xpm"))
    {
        ewxBitmap bitmap(_T("../../../data/client/pixmaps/check16.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    return wxNullBitmap;
////@end MachineBrowserGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon MachineBrowserGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin MachineBrowserGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end MachineBrowserGUI icon retrieval
}
