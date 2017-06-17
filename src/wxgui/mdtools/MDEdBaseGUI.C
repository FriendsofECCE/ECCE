/////////////////////////////////////////////////////////////////////////////
// Name:        MDEdBaseGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "MDEdBaseGUI.H"
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
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxFrame.H"
////@end includes

#include "wxgui/MDEdBaseGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID MDEdBaseGUI::ID_BUTTON_BUILDER = wxNewId();
const wxWindowID MDEdBaseGUI::ID_MENU_HELP = wxNewId();
const wxWindowID MDEdBaseGUI::ID_BUTTON_FINALEDIT = wxNewId();
const wxWindowID MDEdBaseGUI::ID_FRAME = wxNewId();
const wxWindowID MDEdBaseGUI::ID_BUTTON_RESETW = wxNewId();
const wxWindowID MDEdBaseGUI::ID_FEEDBACK = wxNewId();
const wxWindowID MDEdBaseGUI::ID_BUTTON_RESETA = wxNewId();
const wxWindowID MDEdBaseGUI::ID_MENU_SUPPORT = wxNewId();
const wxWindowID MDEdBaseGUI::ID_NOTEBOOK_ENERGY = wxNewId();
const wxWindowID MDEdBaseGUI::ID_BUTTON_LAUNCH = wxNewId();

/*!
 * MDEdBaseGUI type definition
 */

IMPLEMENT_CLASS( MDEdBaseGUI, ewxFrame )

/*!
 * MDEdBaseGUI event table definition
 */

BEGIN_EVENT_TABLE( MDEdBaseGUI, ewxFrame )

////@begin MDEdBaseGUI event table entries
    EVT_CLOSE( MDEdBaseGUI::OnCloseWindow )

    EVT_MENU( wxID_SAVE, MDEdBaseGUI::OnSaveClick )

    EVT_MENU( wxID_EXIT, MDEdBaseGUI::OnExitClick )

    EVT_MENU( ID_MENU_HELP, MDEdBaseGUI::OnMenuHelpClick )

    EVT_MENU( ID_MENU_SUPPORT, MDEdBaseGUI::OnMenuSupportClick )

    EVT_BUTTON( ID_BUTTON_BUILDER, MDEdBaseGUI::OnButtonBuilderClick )

    EVT_BUTTON( ID_BUTTON_FINALEDIT, MDEdBaseGUI::OnButtonFinaleditClick )

    EVT_BUTTON( ID_BUTTON_LAUNCH, MDEdBaseGUI::OnButtonLaunchClick )

    EVT_BUTTON( ID_BUTTON_RESETW, MDEdBaseGUI::OnButtonResetwClick )

    EVT_BUTTON( ID_BUTTON_RESETA, MDEdBaseGUI::OnButtonResetaClick )

////@end MDEdBaseGUI event table entries

END_EVENT_TABLE()

/*!
 * MDEdBaseGUI constructors
 */

MDEdBaseGUI::MDEdBaseGUI( )
{
}

MDEdBaseGUI::MDEdBaseGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * MDEdBaseGUI creator
 */

bool MDEdBaseGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin MDEdBaseGUI member initialisation
    p_notebook = NULL;
    p_feedbackSizer = NULL;
////@end MDEdBaseGUI member initialisation

////@begin MDEdBaseGUI creation
    SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end MDEdBaseGUI creation
    return true;
}

/*!
 * Control creation for MDEdBaseGUI
 */

void MDEdBaseGUI::CreateControls()
{    
////@begin MDEdBaseGUI content construction
    MDEdBaseGUI* itemFrame1 = this;

    ewxMenuBar* menuBar = new ewxMenuBar;
    wxMenu* itemMenu3 = new wxMenu;
    itemMenu3->Append(wxID_SAVE, _("&Save\tCtrl+s"), _T(""), wxITEM_NORMAL);
    itemMenu3->AppendSeparator();
    itemMenu3->Append(wxID_EXIT, _("&Quit\tCtrl+q"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu3, _("File"));
    wxMenu* itemMenu7 = new wxMenu;
    itemMenu7->Append(ID_MENU_HELP, _("on NWChem MD Task..."), _T(""), wxITEM_NORMAL);
    itemMenu7->Append(ID_MENU_SUPPORT, _("Support..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu7, _("Help"));
    itemFrame1->SetMenuBar(menuBar);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(itemBoxSizer10);

    itemBoxSizer10->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer10->Add(itemBoxSizer12, 1, wxGROW|wxALL, 5);

    p_notebook = new ewxNotebook( itemFrame1, ID_NOTEBOOK_ENERGY, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

    itemBoxSizer12->Add(p_notebook, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer10->Add(itemBoxSizer14, 0, wxALIGN_LEFT|wxALL, 0);

    itemBoxSizer14->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton16 = new ewxButton( itemFrame1, ID_BUTTON_BUILDER, _("Builder..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemButton16, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 3);

    itemBoxSizer14->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton18 = new ewxButton( itemFrame1, ID_BUTTON_FINALEDIT, _("Final Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemButton18, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 3);

    itemBoxSizer14->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton20 = new ewxButton( itemFrame1, ID_BUTTON_LAUNCH, _("Launch..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemButton20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer14->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton22 = new ewxButton( itemFrame1, ID_BUTTON_RESETW, _("Reset Panel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemButton22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer14->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton24 = new ewxButton( itemFrame1, ID_BUTTON_RESETA, _("Reset All"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemButton24, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer10->Add(itemBoxSizer25, 0, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine26 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer25->Add(itemStaticLine26, 1, wxGROW|wxTOP|wxBOTTOM, 5);

    p_feedbackSizer = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer10->Add(p_feedbackSizer, 0, wxGROW|wxALL, 0);

    WxFeedback* itemWxFeedback28 = new WxFeedback( itemFrame1, ID_FEEDBACK, wxDefaultPosition, wxDefaultSize, 0 );
    p_feedbackSizer->Add(itemWxFeedback28, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end MDEdBaseGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_FRAME
 */

void MDEdBaseGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_FRAME in MDEdBaseGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_FRAME in MDEdBaseGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
 */

void MDEdBaseGUI::OnSaveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in MDEdBaseGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in MDEdBaseGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 */

void MDEdBaseGUI::OnExitClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in MDEdBaseGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in MDEdBaseGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_HELP
 */

void MDEdBaseGUI::OnMenuHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_HELP in MDEdBaseGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_HELP in MDEdBaseGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_SUPPORT
 */

void MDEdBaseGUI::OnMenuSupportClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_SUPPORT in MDEdBaseGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_SUPPORT in MDEdBaseGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_BUILDER
 */

void MDEdBaseGUI::OnButtonBuilderClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_BUILDER in MDEdBaseGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_BUILDER in MDEdBaseGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FINALEDIT
 */

void MDEdBaseGUI::OnButtonFinaleditClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FINALEDIT in MDEdBaseGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FINALEDIT in MDEdBaseGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_LAUNCH
 */

void MDEdBaseGUI::OnButtonLaunchClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_LAUNCH in MDEdBaseGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_LAUNCH in MDEdBaseGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RESETW
 */

void MDEdBaseGUI::OnButtonResetwClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RESETW in MDEdBaseGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RESETW in MDEdBaseGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RESETA
 */

void MDEdBaseGUI::OnButtonResetaClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RESETA in MDEdBaseGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RESETA in MDEdBaseGUI. 
}

/*!
 * Should we show tooltips?
 */

bool MDEdBaseGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap MDEdBaseGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin MDEdBaseGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end MDEdBaseGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon MDEdBaseGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin MDEdBaseGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end MDEdBaseGUI icon retrieval
}
