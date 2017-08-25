/////////////////////////////////////////////////////////////////////////////
// Name:        CalcMgrGUI.H
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CALCMGRGUI_H_
#define _CALCMGRGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "CalcMgrGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wx/toolbar.h"
#include "wx/splitter.h"
#include "wx/treectrl.h"
#include "wxgui/ewxFrame.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class wxSplitterWindow;
class WxResourceTreeCtrl;
class ewxFrame;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_CALCMGRGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX|wxTAB_TRAVERSAL
#define SYMBOL_CALCMGRGUI_TITLE _("ECCE Organizer")
#define SYMBOL_CALCMGRGUI_IDNAME ID_FRAME
#define SYMBOL_CALCMGRGUI_SIZE wxSize(800, 650)
#define SYMBOL_CALCMGRGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * CalcMgrGUI class declaration
 */

class CalcMgrGUI: public ewxFrame
{    
    DECLARE_CLASS( CalcMgrGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CalcMgrGUI( );
    CalcMgrGUI( wxWindow* parent, wxWindowID id = SYMBOL_CALCMGRGUI_IDNAME, const wxString& caption = SYMBOL_CALCMGRGUI_TITLE, const wxPoint& pos = SYMBOL_CALCMGRGUI_POSITION, const wxSize& size = SYMBOL_CALCMGRGUI_SIZE, long style = SYMBOL_CALCMGRGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_CALCMGRGUI_IDNAME, const wxString& caption = SYMBOL_CALCMGRGUI_TITLE, const wxPoint& pos = SYMBOL_CALCMGRGUI_POSITION, const wxSize& size = SYMBOL_CALCMGRGUI_SIZE, long style = SYMBOL_CALCMGRGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CalcMgrGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_FRAME
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_CONTEXT_MENU event handler for ID_FRAME
    virtual    void OnContextMenu( wxContextMenuEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_IMPORT
    virtual    void OnImportClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXPORT
    virtual    void OnExportClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
    virtual    void OnExitClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE
    virtual    void OnDuplicateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE2
    virtual    void OnDuplicate2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CUT
    virtual    void OnCutClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_COPY
    virtual    void OnCopyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_PASTE
    virtual    void OnPasteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_RENAME
    virtual    void OnRenameClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DELETE
    virtual    void OnDeleteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ANNOTATION
    virtual    void OnAnnotationClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CHANGE_ACCESS
    virtual    void OnChangeAccessClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CONFIGURE_NWFS_ARCHIVE
    virtual    void OnConfigureNwfsArchiveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_TOOLBAR
    virtual    void OnShowToolbarClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_LEGEND
    virtual    void OnShowLegendClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_LINE
    virtual    void OnShowLineClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_RUNSTATES
    virtual    void OnShowRunstatesClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_INTERNAL
    virtual    void OnShowInternalClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HOMEASROOT
    virtual    void OnHomeasrootClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SORT_TYPE
    virtual    void OnSortTreeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ADD_BOOKMARK
    virtual    void OnAddBookmarkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_MANAGE_BOOKMARK
    virtual    void OnManageBookmarkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
    virtual    void OnHelpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_SUPPORT
    virtual    void OnMenuSupportClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_TEST_SUITE
    virtual    void OnTestSuiteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_VIEW_PROPERTY
    virtual    void OnViewPropertyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ADD_SERVER
    virtual    void OnAddServerClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HOME
    virtual    void OnHomeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UP
    virtual    void OnUpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_BACKWARD
    virtual    void OnBackwardClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_FORWARD
    virtual    void OnForwardClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_FIND
    virtual    void OnFindClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_REFRESH
    virtual    void OnRefreshClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UPLOAD
    virtual    void OnUploadClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DOWNLOAD
    virtual    void OnDownloadClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TREE_SEL_CHANGED event handler for ID_TREECTRL
    virtual    void OnTreectrlSelChanged( wxTreeEvent& event );

    /// wxEVT_COMMAND_TREE_SEL_CHANGING event handler for ID_TREECTRL
    virtual    void OnTreectrlSelChanging( wxTreeEvent& event );

    /// wxEVT_COMMAND_TREE_BEGIN_DRAG event handler for ID_TREECTRL
    virtual    void OnTreectrlBeginDrag( wxTreeEvent& event );

    /// wxEVT_COMMAND_TREE_END_DRAG event handler for ID_TREECTRL
    virtual    void OnTreectrlEndDrag( wxTreeEvent& event );

    /// wxEVT_COMMAND_TREE_BEGIN_RDRAG event handler for ID_TREECTRL
    virtual    void OnTreectrlBeginRDrag( wxTreeEvent& event );

    /// wxEVT_COMMAND_TREE_DELETE_ITEM event handler for ID_TREECTRL
    virtual    void OnTreectrlDeleteItem( wxTreeEvent& event );

    /// wxEVT_COMMAND_TREE_ITEM_ACTIVATED event handler for ID_TREECTRL
    virtual    void OnTreectrlItemActivated( wxTreeEvent& event );

    /// wxEVT_COMMAND_TREE_ITEM_COLLAPSED event handler for ID_TREECTRL
    virtual    void OnTreectrlItemCollapsed( wxTreeEvent& event );

    /// wxEVT_COMMAND_TREE_ITEM_COLLAPSING event handler for ID_TREECTRL
    virtual    void OnTreectrlItemCollapsing( wxTreeEvent& event );

    /// wxEVT_COMMAND_TREE_ITEM_EXPANDED event handler for ID_TREECTRL
    virtual    void OnTreectrlItemExpanded( wxTreeEvent& event );

    /// wxEVT_COMMAND_TREE_ITEM_EXPANDING event handler for ID_TREECTRL
    virtual    void OnTreectrlItemExpanding( wxTreeEvent& event );

    /// wxEVT_COMMAND_TREE_KEY_DOWN event handler for ID_TREECTRL
    virtual    void OnTreectrlKeyDown( wxTreeEvent& event );

    /// wxEVT_COMMAND_TREE_ITEM_MENU event handler for ID_TREECTRL
    virtual    void OnTreectrlItemMenu( wxTreeEvent& event );

////@end CalcMgrGUI event handler declarations

////@begin CalcMgrGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CalcMgrGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CalcMgrGUI member variables
    wxBoxSizer* p_topSizer;
    wxToolBar* p_toolBar;
    wxSplitterWindow* p_splitterWindow;
    WxResourceTreeCtrl* p_treeCtrl;
    static const wxWindowID ID_TOOLBAR1;
    static const wxWindowID wxID_SHOW_LEGEND;
    static const wxWindowID wxID_UPLOAD;
    static const wxWindowID wxID_SHOW_INTERNAL;
    static const wxWindowID wxID_DUPLICATE2;
    static const wxWindowID wxID_HOMEASROOT;
    static const wxWindowID ID_FRAME;
    static const wxWindowID wxID_SORT_TYPE;
    static const wxWindowID wxID_ANNOTATION;
    static const wxWindowID wxID_CONFIRM_DELETE;
    static const wxWindowID wxID_DOWNLOAD;
    static const wxWindowID wxID_TEST_SUITE;
    static const wxWindowID wxID_RUNDEFAULTTOOL;
    static const wxWindowID wxID_VIEW_PROPERTY;
    static const wxWindowID wxID_ADD_BOOKMARK;
    static const wxWindowID ID_MENU;
    static const wxWindowID wxID_SHOW_LINE;
    static const wxWindowID ID_SPLITTERWINDOW;
    static const wxWindowID wxID_MANAGE_BOOKMARK;
    static const wxWindowID wxID_SORT_DATE_REV;
    static const wxWindowID ID_MENU_SUPPORT;
    static const wxWindowID wxID_CONFIGURE_NWFS_ARCHIVE;
    static const wxWindowID wxID_SORT_NAME;
    static const wxWindowID ID_TREECTRL;
    static const wxWindowID wxID_ADD_SERVER;
    static const wxWindowID wxID_ASKFORNAME;
    static const wxWindowID wxID_SORT_NAME_REV;
    static const wxWindowID wxID_SORT_DATE;
    static const wxWindowID wxID_IMPORT;
    static const wxWindowID wxID_RENAME;
    static const wxWindowID wxID_SHOW_TOOLBAR;
    static const wxWindowID wxID_CHANGE_ACCESS;
    static const wxWindowID wxID_CONFIRM_RESET;
    static const wxWindowID wxID_EXPORT;
    static const wxWindowID wxID_SORT_TYPE_REV;
    static const wxWindowID wxID_SHOW_RUNSTATES;
////@end CalcMgrGUI member variables
};

#endif
    // _CALCMGRGUI_H_
