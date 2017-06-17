/////////////////////////////////////////////////////////////////////////////
// Name:        BookmarkEditorGUI.H
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _BOOKMARKEDITORGUI_H_
#define _BOOKMARKEDITORGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "BookmarkEditorGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wxgui/ewxDialog.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxTextCtrl;
class ewxListBox;
class ewxButton;
class ewxDialog;
class ewxButton;
class ewxListBox;
class ewxTextCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_BOOKMARKEDITORGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxDIALOG_MODAL
#define SYMBOL_BOOKMARKEDITORGUI_TITLE _("Bookmark Editor")
#define SYMBOL_BOOKMARKEDITORGUI_IDNAME ID_DIALOG
#define SYMBOL_BOOKMARKEDITORGUI_SIZE wxSize(600, 500)
#define SYMBOL_BOOKMARKEDITORGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * BookmarkEditorGUI class declaration
 */

class BookmarkEditorGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( BookmarkEditorGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    BookmarkEditorGUI( );
    BookmarkEditorGUI( wxWindow* parent, wxWindowID id = SYMBOL_BOOKMARKEDITORGUI_IDNAME, const wxString& caption = SYMBOL_BOOKMARKEDITORGUI_TITLE, const wxPoint& pos = SYMBOL_BOOKMARKEDITORGUI_POSITION, const wxSize& size = SYMBOL_BOOKMARKEDITORGUI_SIZE, long style = SYMBOL_BOOKMARKEDITORGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_BOOKMARKEDITORGUI_IDNAME, const wxString& caption = SYMBOL_BOOKMARKEDITORGUI_TITLE, const wxPoint& pos = SYMBOL_BOOKMARKEDITORGUI_POSITION, const wxSize& size = SYMBOL_BOOKMARKEDITORGUI_SIZE, long style = SYMBOL_BOOKMARKEDITORGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin BookmarkEditorGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_BOOKMARK_NAME
    virtual    void OnBookmarkNameUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_BOOKMARK_URL
    virtual    void OnBookmarkUrlUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX
    virtual    void OnListboxSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX
    virtual    void OnListboxDoubleClicked( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_ADD
    virtual    void OnBookmarkAddClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_DEL
    virtual    void OnBookmarkDelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_UP
    virtual    void OnBookmarkUpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_DOWN
    virtual    void OnBookmarkDownClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
    virtual    void OnCloseClick( wxCommandEvent& event );

////@end BookmarkEditorGUI event handler declarations

////@begin BookmarkEditorGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end BookmarkEditorGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin BookmarkEditorGUI member variables
    ewxTextCtrl* p_name;
    ewxTextCtrl* p_url;
    ewxListBox* p_listBox;
    ewxButton* p_addBut;
    ewxButton* p_delBut;
    ewxButton* p_upBut;
    ewxButton* p_downBut;
    static const wxWindowID ID_BOOKMARK_URL;
    static const wxWindowID ID_PANEL;
    static const wxWindowID ID_LISTBOX;
    static const wxWindowID ID_BOOKMARK_DEL;
    static const wxWindowID ID_DIALOG;
    static const wxWindowID ID_BOOKMARK_UP;
    static const wxWindowID ID_BOOKMARK_NAME;
    static const wxWindowID ID_BOOKMARK_ADD;
    static const wxWindowID ID_BOOKMARK_DOWN;
////@end BookmarkEditorGUI member variables
};

#endif
    // _BOOKMARKEDITORGUI_H_
