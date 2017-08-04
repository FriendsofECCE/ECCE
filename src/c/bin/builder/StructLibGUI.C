/////////////////////////////////////////////////////////////////////////////
// Name:        StructLibGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "StructLibGUI.H"
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
#include "wxgui/ewxBitmapButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxBitmap.H"
////@end includes

#include "StructLibGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID StructLibGUI::ID_STATIC_STRUCTLIB_NULLSELECTION = wxNewId();
const wxWindowID StructLibGUI::ID_CHOICE_STRUCTLIB_BOOKMARKS = wxNewId();
const wxWindowID StructLibGUI::ID_SPLITTERWINDOW_STRUCTLIB_VIEWER = wxNewId();
const wxWindowID StructLibGUI::ID_PANEL_STRUCTLIB_FRAGVIEW = wxNewId();
const wxWindowID StructLibGUI::ID_BUTTON_UP1LEVEL = wxNewId();
const wxWindowID StructLibGUI::ID_LISTCTRL_STRUCTLIB_STRUCTURES = wxNewId();
const wxWindowID StructLibGUI::ID_DIALOG_WXBUILDER_STRUCTLIBTOOL = wxNewId();

/*!
 * StructLibGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( StructLibGUI, ewxPanel )

/*!
 * StructLibGUI event table definition
 */

BEGIN_EVENT_TABLE( StructLibGUI, ewxPanel )

////@begin StructLibGUI event table entries
    EVT_CLOSE( StructLibGUI::windowClosedCB )
    EVT_SIZE( StructLibGUI::windowSizedCB )

    EVT_SPLITTER_SASH_POS_CHANGING( ID_SPLITTERWINDOW_STRUCTLIB_VIEWER, StructLibGUI::mainSplitterSashChangingCB )

    EVT_CHOICE( ID_CHOICE_STRUCTLIB_BOOKMARKS, StructLibGUI::bookmarksChoiceSelectedCB )

    EVT_BUTTON( ID_BUTTON_UP1LEVEL, StructLibGUI::upOneLevelButtonClickCB )

    EVT_LIST_ITEM_SELECTED( ID_LISTCTRL_STRUCTLIB_STRUCTURES, StructLibGUI::structuresListItemSelectedCB )
    EVT_LIST_ITEM_ACTIVATED( ID_LISTCTRL_STRUCTLIB_STRUCTURES, StructLibGUI::structuresListItemActivatedCB )

////@end StructLibGUI event table entries

END_EVENT_TABLE()

/*!
 * StructLibGUI constructors
 */

StructLibGUI::StructLibGUI( )
{
}

StructLibGUI::StructLibGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * StructLibGUI creator
 */

bool StructLibGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin StructLibGUI member initialisation
////@end StructLibGUI member initialisation

////@begin StructLibGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    Centre();
////@end StructLibGUI creation
    return true;
}

/*!
 * Control creation for StructLibGUI
 */

void StructLibGUI::CreateControls()
{    
////@begin StructLibGUI content construction
    StructLibGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxSplitterWindow* itemSplitterWindow3 = new wxSplitterWindow( itemPanel1, ID_SPLITTERWINDOW_STRUCTLIB_VIEWER, wxDefaultPosition, wxSize(300, 500), wxSP_3DBORDER );

    ewxPanel* itemPanel4 = new ewxPanel( itemSplitterWindow3, wxID_ANY, wxDefaultPosition, wxSize(250, 250), wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemPanel4->SetSizer(itemBoxSizer5);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer5->Add(itemBoxSizer6, 0, wxGROW|wxFIXED_MINSIZE, 3);
    ewxStaticText* itemStaticText7 = new ewxStaticText( itemPanel4, wxID_STATIC, _("Libraries"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemStaticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemChoice8Strings = NULL;
    ewxChoice* itemChoice8 = new ewxChoice( itemPanel4, ID_CHOICE_STRUCTLIB_BOOKMARKS, wxDefaultPosition, wxDefaultSize, 0, itemChoice8Strings, 0 );
    itemBoxSizer6->Add(itemChoice8, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBitmap itemBitmapButton9Bitmap(itemPanel1->GetBitmapResource(wxT("../../../data/client/pixmaps/goup.xpm")));
    ewxBitmapButton* itemBitmapButton9 = new ewxBitmapButton( itemPanel4, ID_BUTTON_UP1LEVEL, itemBitmapButton9Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
    if (ShowToolTips())
        itemBitmapButton9->SetToolTip(_("Go up one level"));
    itemBoxSizer6->Add(itemBitmapButton9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxListCtrl* itemListCtrl10 = new wxListCtrl( itemPanel4, ID_LISTCTRL_STRUCTLIB_STRUCTURES, wxDefaultPosition, wxDefaultSize, wxLC_LIST |wxLC_NO_HEADER|wxLC_SINGLE_SEL|wxRAISED_BORDER );
    itemBoxSizer5->Add(itemListCtrl10, 1, wxGROW|wxALL, 3);

    ewxPanel* itemPanel11 = new ewxPanel( itemSplitterWindow3, ID_PANEL_STRUCTLIB_FRAGVIEW, wxDefaultPosition, wxSize(350, 350), wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel11->SetBackgroundColour(wxColour(0, 0, 0));
    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel11->SetSizer(itemBoxSizer12);

    ewxStaticText* itemStaticText13 = new ewxStaticText( itemPanel11, ID_STATIC_STRUCTLIB_NULLSELECTION, _("No Current Selection"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    itemStaticText13->SetForegroundColour(wxColour(255, 255, 255));
    itemBoxSizer12->Add(itemStaticText13, 1, wxALIGN_CENTER_VERTICAL, 5);

    itemSplitterWindow3->SplitHorizontally(itemPanel4, itemPanel11, 250);
    itemBoxSizer2->Add(itemSplitterWindow3, 1, wxGROW, 3);

////@end StructLibGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_DIALOG_WXBUILDER_STRUCTLIBTOOL
 */

void StructLibGUI::windowClosedCB( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_DIALOG_WXBUILDER_STRUCTLIBTOOL in StructLibGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_DIALOG_WXBUILDER_STRUCTLIBTOOL in StructLibGUI. 
}

/*!
 * wxEVT_SIZE event handler for ID_DIALOG_WXBUILDER_STRUCTLIBTOOL
 */

void StructLibGUI::windowSizedCB( wxSizeEvent& event )
{
////@begin wxEVT_SIZE event handler for ID_DIALOG_WXBUILDER_STRUCTLIBTOOL in StructLibGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_SIZE event handler for ID_DIALOG_WXBUILDER_STRUCTLIBTOOL in StructLibGUI. 
}

/*!
 * wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING event handler for ID_SPLITTERWINDOW_STRUCTLIB_VIEWER
 */

void StructLibGUI::mainSplitterSashChangingCB( wxSplitterEvent& event )
{
////@begin wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING event handler for ID_SPLITTERWINDOW_STRUCTLIB_VIEWER in StructLibGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING event handler for ID_SPLITTERWINDOW_STRUCTLIB_VIEWER in StructLibGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_STRUCTLIB_BOOKMARKS
 */

void StructLibGUI::bookmarksChoiceSelectedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_STRUCTLIB_BOOKMARKS in StructLibGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_STRUCTLIB_BOOKMARKS in StructLibGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_UP1LEVEL
 */

void StructLibGUI::upOneLevelButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_UP1LEVEL in StructLibGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_UP1LEVEL in StructLibGUI. 
}

/*!
 * wxEVT_COMMAND_LIST_ITEM_SELECTED event handler for ID_LISTCTRL_STRUCTLIB_STRUCTURES
 */

void StructLibGUI::structuresListItemSelectedCB( wxListEvent& event )
{
////@begin wxEVT_COMMAND_LIST_ITEM_SELECTED event handler for ID_LISTCTRL_STRUCTLIB_STRUCTURES in StructLibGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LIST_ITEM_SELECTED event handler for ID_LISTCTRL_STRUCTLIB_STRUCTURES in StructLibGUI. 
}

/*!
 * wxEVT_COMMAND_LIST_ITEM_ACTIVATED event handler for ID_LISTCTRL_STRUCTLIB_STRUCTURES
 */

void StructLibGUI::structuresListItemActivatedCB( wxListEvent& event )
{
////@begin wxEVT_COMMAND_LIST_ITEM_ACTIVATED event handler for ID_LISTCTRL_STRUCTLIB_STRUCTURES in StructLibGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LIST_ITEM_ACTIVATED event handler for ID_LISTCTRL_STRUCTLIB_STRUCTURES in StructLibGUI. 
}

/*!
 * Should we show tooltips?
 */

bool StructLibGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap StructLibGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin StructLibGUI bitmap retrieval
    wxUnusedVar(name);
    if (name == wxT("../../../data/client/pixmaps/goup.xpm"))
    {
        ewxBitmap bitmap(_T("../../../data/client/pixmaps/goup.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    return wxNullBitmap;
////@end StructLibGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon StructLibGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin StructLibGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end StructLibGUI icon retrieval
}
