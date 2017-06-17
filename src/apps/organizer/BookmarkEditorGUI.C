/////////////////////////////////////////////////////////////////////////////
// Name:        BookmarkEditorGUI.C
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "BookmarkEditorGUI.H"
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
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "BookmarkEditorGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID BookmarkEditorGUI::ID_BOOKMARK_URL = wxNewId();
const wxWindowID BookmarkEditorGUI::ID_PANEL = wxNewId();
const wxWindowID BookmarkEditorGUI::ID_LISTBOX = wxNewId();
const wxWindowID BookmarkEditorGUI::ID_BOOKMARK_DEL = wxNewId();
const wxWindowID BookmarkEditorGUI::ID_DIALOG = wxNewId();
const wxWindowID BookmarkEditorGUI::ID_BOOKMARK_UP = wxNewId();
const wxWindowID BookmarkEditorGUI::ID_BOOKMARK_NAME = wxNewId();
const wxWindowID BookmarkEditorGUI::ID_BOOKMARK_ADD = wxNewId();
const wxWindowID BookmarkEditorGUI::ID_BOOKMARK_DOWN = wxNewId();

/*!
 * BookmarkEditorGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( BookmarkEditorGUI, ewxDialog )

/*!
 * BookmarkEditorGUI event table definition
 */

BEGIN_EVENT_TABLE( BookmarkEditorGUI, ewxDialog )

////@begin BookmarkEditorGUI event table entries
    EVT_CLOSE( BookmarkEditorGUI::OnCloseWindow )

    EVT_TEXT( ID_BOOKMARK_NAME, BookmarkEditorGUI::OnBookmarkNameUpdated )

    EVT_TEXT( ID_BOOKMARK_URL, BookmarkEditorGUI::OnBookmarkUrlUpdated )

    EVT_LISTBOX( ID_LISTBOX, BookmarkEditorGUI::OnListboxSelected )
    EVT_LISTBOX_DCLICK( ID_LISTBOX, BookmarkEditorGUI::OnListboxDoubleClicked )

    EVT_BUTTON( ID_BOOKMARK_ADD, BookmarkEditorGUI::OnBookmarkAddClick )

    EVT_BUTTON( ID_BOOKMARK_DEL, BookmarkEditorGUI::OnBookmarkDelClick )

    EVT_BUTTON( ID_BOOKMARK_UP, BookmarkEditorGUI::OnBookmarkUpClick )

    EVT_BUTTON( ID_BOOKMARK_DOWN, BookmarkEditorGUI::OnBookmarkDownClick )

    EVT_BUTTON( wxID_CLOSE, BookmarkEditorGUI::OnCloseClick )

////@end BookmarkEditorGUI event table entries

END_EVENT_TABLE()

/*!
 * BookmarkEditorGUI constructors
 */

BookmarkEditorGUI::BookmarkEditorGUI( )
{
}

BookmarkEditorGUI::BookmarkEditorGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * BookmarkEditorGUI creator
 */

bool BookmarkEditorGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin BookmarkEditorGUI member initialisation
    p_name = NULL;
    p_url = NULL;
    p_listBox = NULL;
    p_addBut = NULL;
    p_delBut = NULL;
    p_upBut = NULL;
    p_downBut = NULL;
////@end BookmarkEditorGUI member initialisation

////@begin BookmarkEditorGUI creation
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    Centre();
////@end BookmarkEditorGUI creation
    return true;
}

/*!
 * Control creation for BookmarkEditorGUI
 */

void BookmarkEditorGUI::CreateControls()
{    
////@begin BookmarkEditorGUI content construction
    BookmarkEditorGUI* itemDialog1 = this;

    ewxPanel* itemPanel2 = new ewxPanel( itemDialog1, ID_PANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemPanel2->SetSizer(itemBoxSizer3);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer4, 0, wxGROW, 5);

    ewxStaticText* itemStaticText5 = new ewxStaticText( itemPanel2, wxID_STATIC, _("Name:"), wxDefaultPosition, wxSize(45, -1), wxALIGN_RIGHT );
    itemBoxSizer4->Add(itemStaticText5, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    p_name = new ewxTextCtrl( itemPanel2, ID_BOOKMARK_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(p_name, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer7, 0, wxGROW, 5);

    ewxStaticText* itemStaticText8 = new ewxStaticText( itemPanel2, wxID_STATIC, _("URL:"), wxDefaultPosition, wxSize(45, -1), wxALIGN_RIGHT );
    itemBoxSizer7->Add(itemStaticText8, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    p_url = new ewxTextCtrl( itemPanel2, ID_BOOKMARK_URL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(p_url, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer10, 1, wxGROW, 5);

    wxString* p_listBoxStrings = NULL;
    p_listBox = new ewxListBox( itemPanel2, ID_LISTBOX, wxDefaultPosition, wxDefaultSize, 0, p_listBoxStrings, wxLB_SINGLE );
    itemBoxSizer10->Add(p_listBox, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer10->Add(itemBoxSizer12, 0, wxGROW|wxALL, 5);

    p_addBut = new ewxButton( itemPanel2, ID_BOOKMARK_ADD, _("&Add"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(p_addBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    p_delBut = new ewxButton( itemPanel2, ID_BOOKMARK_DEL, _("&Delete"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(p_delBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    p_upBut = new ewxButton( itemPanel2, ID_BOOKMARK_UP, _("Move &Up"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(p_upBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    p_downBut = new ewxButton( itemPanel2, ID_BOOKMARK_DOWN, _("Move Dow&n"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(p_downBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    itemBoxSizer12->Add(5, 5, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxButton* itemButton18 = new ewxButton( itemPanel2, wxID_CLOSE, _("&Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemButton18, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

////@end BookmarkEditorGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
 */

void BookmarkEditorGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_DIALOG in BookmarkEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_DIALOG in BookmarkEditorGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_BOOKMARK_NAME
 */

void BookmarkEditorGUI::OnBookmarkNameUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_BOOKMARK_NAME in BookmarkEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_BOOKMARK_NAME in BookmarkEditorGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_BOOKMARK_URL
 */

void BookmarkEditorGUI::OnBookmarkUrlUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_BOOKMARK_URL in BookmarkEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_BOOKMARK_URL in BookmarkEditorGUI. 
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX
 */

void BookmarkEditorGUI::OnListboxSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX in BookmarkEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX in BookmarkEditorGUI. 
}

/*!
 * wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX
 */

void BookmarkEditorGUI::OnListboxDoubleClicked( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX in BookmarkEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX in BookmarkEditorGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_ADD
 */

void BookmarkEditorGUI::OnBookmarkAddClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_ADD in BookmarkEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_ADD in BookmarkEditorGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_DEL
 */

void BookmarkEditorGUI::OnBookmarkDelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_DEL in BookmarkEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_DEL in BookmarkEditorGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_UP
 */

void BookmarkEditorGUI::OnBookmarkUpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_UP in BookmarkEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_UP in BookmarkEditorGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_DOWN
 */

void BookmarkEditorGUI::OnBookmarkDownClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_DOWN in BookmarkEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_DOWN in BookmarkEditorGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void BookmarkEditorGUI::OnCloseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in BookmarkEditorGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE in BookmarkEditorGUI. 
}

/*!
 * Should we show tooltips?
 */

bool BookmarkEditorGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap BookmarkEditorGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin BookmarkEditorGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end BookmarkEditorGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon BookmarkEditorGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin BookmarkEditorGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end BookmarkEditorGUI icon retrieval
}
