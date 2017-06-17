/////////////////////////////////////////////////////////////////////////////
// Name:        WxBasisToolGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXBASISTOOLGUI_H_
#define _WXBASISTOOLGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxBasisToolGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wx/splitter.h"
#include "wx/notebook.h"
#include "wx/statline.h"
#include "wx/grid.h"
#include "wxgui/ewxFrame.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxFrame;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXBASISTOOLGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_WXBASISTOOLGUI_TITLE _("ECCE Basis Set Tool")
#define SYMBOL_WXBASISTOOLGUI_IDNAME ID_FRAME
#define SYMBOL_WXBASISTOOLGUI_SIZE wxSize(900, 625)
#define SYMBOL_WXBASISTOOLGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxBasisToolGUI class declaration
 */

class WxBasisToolGUI: public ewxFrame
{    
    DECLARE_CLASS( WxBasisToolGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxBasisToolGUI( );
    WxBasisToolGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXBASISTOOLGUI_IDNAME, const wxString& caption = SYMBOL_WXBASISTOOLGUI_TITLE, const wxPoint& pos = SYMBOL_WXBASISTOOLGUI_POSITION, const wxSize& size = SYMBOL_WXBASISTOOLGUI_SIZE, long style = SYMBOL_WXBASISTOOLGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXBASISTOOLGUI_IDNAME, const wxString& caption = SYMBOL_WXBASISTOOLGUI_TITLE, const wxPoint& pos = SYMBOL_WXBASISTOOLGUI_POSITION, const wxSize& size = SYMBOL_WXBASISTOOLGUI_SIZE, long style = SYMBOL_WXBASISTOOLGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxBasisToolGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_FRAME
    virtual    void windowCloseCB( wxCloseEvent& event );

    /// wxEVT_SIZE event handler for ID_FRAME
    virtual    void windowSizeCB( wxSizeEvent& event );

    /// wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED event handler for ID_SPLITTER_WXBASISTOOL_MAIN
    virtual    void windowSplitterSashPosChangedCB( wxSplitterEvent& event );

    /// wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING event handler for ID_SPLITTER_WXBASISTOOL_MAIN
    virtual    void windowSplitterSashPosChangingCB( wxSplitterEvent& event );

    /// wxEVT_SIZE event handler for ID_SPLITTER_WXBASISTOOL_MAIN
    virtual    void windowSplitterSizeCB( wxSizeEvent& event );

    /// wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSETS
    virtual    void basisSetsNotebookPageChangedCB( wxNotebookEvent& event );

    /// wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSETS
    virtual    void basisSetsNotebookPageChangingCB( wxNotebookEvent& event );

    /// wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0
    virtual    void basisSetCategoryNotebookPageChangedCB( wxNotebookEvent& event );

    /// wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0
    virtual    void basisSetCategoryNotebookPageChanging( wxNotebookEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00
    virtual    void basisSetTypeListboxSelectedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00
    virtual    void basisSetTypeListboxDoubleClickedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISTOOL_DETAILS
    virtual    void detailsButtonClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISTOOL_CONTEXT_ADD
    virtual    void contextAddButtonClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS
    virtual    void contextBasisSetsNotebookPageChangedCB( wxNotebookEvent& event );

    /// wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS
    virtual    void contextBasisSetsNotebookPageChangingCB( wxNotebookEvent& event );

    /// wxEVT_GRID_CMD_SELECT_CELL event handler for ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0
    virtual    void contextBasisSetsGridCellSelectCB( wxGridEvent& event );

    /// wxEVT_GRID_CMD_RANGE_SELECT event handler for ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0
    virtual    void contextBasisSetsGridRangeSelectCB( wxGridRangeSelectEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_WXBASISTOOL_CONTEXT_ALLOWMULTIPLE
    virtual    void contextMultipleAllowedCheckboxClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISTOOL_SHOWEDITOR
    virtual    void showEditorButtonClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_1
    virtual    void contextPolarizationRadioButtonSelectedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_WXBASISTOOL_OPTIMIZE
    virtual    void contextOptimizeCheckboxClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
    virtual    void programSaveMenuItemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_PROGRAM_COEFFICIENTS
    virtual    void programCoefficientsMenuItemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
    virtual    void programQuitMenuItemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_EDIT_CLEARALL
    virtual    void editClearAllMenuItemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_EDIT_CLEARSELECTION
    virtual    void editClearSelectionMenuItemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
    virtual    void helpProgramMenuItemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_HELP_SUPPORT
    virtual    void helpSupportMenuItemClickCB( wxCommandEvent& event );

////@end WxBasisToolGUI event handler declarations

////@begin WxBasisToolGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxBasisToolGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxBasisToolGUI member variables
    static const wxWindowID ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_TYPE_04;
    static const wxWindowID ID_SPLITTER_WXBASISTOOL_MAIN;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BOTTOM;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_0;
    static const wxWindowID ID_NOTEBOOK_WXBASISTOOL_BASISSETS;
    static const wxWindowID ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_08;
    static const wxWindowID ID_MENUITEM_WXBASISTOOL_HELP_SUPPORT;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_1;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_TYPE_10;
    static const wxWindowID ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_03;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_TYPE_11;
    static const wxWindowID ID_BUTTON_WXBASISTOOL_SHOWEDITOR;
    static const wxWindowID ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_07;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_2;
    static const wxWindowID ID_MENUITEM_WXBASISTOOL_PROGRAM_COEFFICIENTS;
    static const wxWindowID ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_1;
    static const wxWindowID ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_06;
    static const wxWindowID ID_CHECKBOX_WXBASISTOOL_OPTIMIZE;
    static const wxWindowID ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_04;
    static const wxWindowID ID_PANEL_WXBASISTOOL_CONTEXT_OPERATIONS;
    static const wxWindowID ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_2;
    static const wxWindowID ID_PANEL_WXBASISTOOL_MAIN;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_TYPE_02;
    static const wxWindowID ID_STATIC_WXBASISTOOL_FUNCTIONS;
    static const wxWindowID ID_PANEL_WXBASISTOOL_DIVIDER;
    static const wxWindowID ID_PANEL_WXBASISTOOL_CONTEXT_BASISSETS_1;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSETS;
    static const wxWindowID ID_PANEL17;
    static const wxWindowID ID_STATIC_WXBASISTOOL_CONTEXT_FORMULA;
    static const wxWindowID ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_09;
    static const wxWindowID ID_PANEL_WXBASISTOOL_CONTEXT_BASISSETS_0;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_TYPE_01;
    static const wxWindowID ID_FRAME;
    static const wxWindowID ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_05;
    static const wxWindowID ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_0;
    static const wxWindowID ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_11;
    static const wxWindowID ID_STATIC_WXBASISTOOL_BASISSET_CATEGORY_1;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_TYPE_03;
    static const wxWindowID ID_PANEL_WXBASISTOOL_CONTEXT_BASISSETS_2;
    static const wxWindowID ID_MENUITEM_WXBASISTOOL_EDIT_CLEARSELECTION;
    static const wxWindowID ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_10;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_TYPE_08;
    static const wxWindowID ID_STATIC_WXBASISTOOL_EFFPRIMATIVES;
    static const wxWindowID ID_STATIC_WXBASISTOOL_PRIMATIVES;
    static const wxWindowID ID_BUTTON_WXBASISTOOL_CONTEXT_ADD;
    static const wxWindowID ID_PANEL_WXBASISTOOL_PERTABLE;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_TYPE_07;
    static const wxWindowID ID_BUTTON_WXBASISTOOL_DETAILS;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_TYPE_09;
    static const wxWindowID ID_PANEL_WXBASISTOOL_CONTEXTMOLECULE;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_TYPE_00;
    static const wxWindowID ID_FEEDBACK_WXBASISTOOL_MESSAGES;
    static const wxWindowID ID_PANEL_WXBASISTOOL_TOP;
    static const wxWindowID ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_02;
    static const wxWindowID ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_1;
    static const wxWindowID ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_1;
    static const wxWindowID ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0;
    static const wxWindowID ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_01;
    static const wxWindowID ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00;
    static const wxWindowID ID_MENUITEM_WXBASISTOOL_EDIT_CLEARALL;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_3;
    static const wxWindowID ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_TYPE_06;
    static const wxWindowID ID_PANEL_WXBASISTOOL_BASISSET_TYPE_05;
    static const wxWindowID ID_CHECKBOX_WXBASISTOOL_CONTEXT_ALLOWMULTIPLE;
////@end WxBasisToolGUI member variables
};

#endif
    // _WXBASISTOOLGUI_H_
