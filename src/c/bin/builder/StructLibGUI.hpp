/////////////////////////////////////////////////////////////////////////////
// Name:        StructLibGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _STRUCTLIBGUI_H_
#define _STRUCTLIBGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "StructLibGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/splitter.h"
#include "wx/listctrl.h"
#include "wxgui/ewxPanel.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxPanel;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_STRUCTLIBGUI_STYLE wxRESIZE_BORDER|wxSUNKEN_BORDER
#define SYMBOL_STRUCTLIBGUI_TITLE _("StructLibGUI")
#define SYMBOL_STRUCTLIBGUI_IDNAME ID_DIALOG_WXBUILDER_STRUCTLIBTOOL
#define SYMBOL_STRUCTLIBGUI_SIZE wxSize(300, 500)
#define SYMBOL_STRUCTLIBGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * StructLibGUI class declaration
 */

class StructLibGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( StructLibGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    StructLibGUI( );
    StructLibGUI( wxWindow* parent, wxWindowID id = SYMBOL_STRUCTLIBGUI_IDNAME, const wxPoint& pos = SYMBOL_STRUCTLIBGUI_POSITION, const wxSize& size = SYMBOL_STRUCTLIBGUI_SIZE, long style = SYMBOL_STRUCTLIBGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_STRUCTLIBGUI_IDNAME, const wxPoint& pos = SYMBOL_STRUCTLIBGUI_POSITION, const wxSize& size = SYMBOL_STRUCTLIBGUI_SIZE, long style = SYMBOL_STRUCTLIBGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin StructLibGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_DIALOG_WXBUILDER_STRUCTLIBTOOL
    virtual    void windowClosedCB( wxCloseEvent& event );

    /// wxEVT_SIZE event handler for ID_DIALOG_WXBUILDER_STRUCTLIBTOOL
    virtual    void windowSizedCB( wxSizeEvent& event );

    /// wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING event handler for ID_SPLITTERWINDOW_STRUCTLIB_VIEWER
    virtual    void mainSplitterSashChangingCB( wxSplitterEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_STRUCTLIB_BOOKMARKS
    virtual    void bookmarksChoiceSelectedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_UP1LEVEL
    virtual    void upOneLevelButtonClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_LIST_ITEM_SELECTED event handler for ID_LISTCTRL_STRUCTLIB_STRUCTURES
    virtual    void structuresListItemSelectedCB( wxListEvent& event );

    /// wxEVT_COMMAND_LIST_ITEM_ACTIVATED event handler for ID_LISTCTRL_STRUCTLIB_STRUCTURES
    virtual    void structuresListItemActivatedCB( wxListEvent& event );

////@end StructLibGUI event handler declarations

////@begin StructLibGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end StructLibGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin StructLibGUI member variables
    static const wxWindowID ID_STATIC_STRUCTLIB_NULLSELECTION;
    static const wxWindowID ID_CHOICE_STRUCTLIB_BOOKMARKS;
    static const wxWindowID ID_SPLITTERWINDOW_STRUCTLIB_VIEWER;
    static const wxWindowID ID_PANEL_STRUCTLIB_FRAGVIEW;
    static const wxWindowID ID_BUTTON_UP1LEVEL;
    static const wxWindowID ID_LISTCTRL_STRUCTLIB_STRUCTURES;
    static const wxWindowID ID_DIALOG_WXBUILDER_STRUCTLIBTOOL;
////@end StructLibGUI member variables
};

#endif
    // _STRUCTLIBGUI_H_
