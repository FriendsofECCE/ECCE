/////////////////////////////////////////////////////////////////////////////
// Name:        WxContractionEditorGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXCONTRACTIONEDITORGUI_H_
#define _WXCONTRACTIONEDITORGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxContractionEditorGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/statline.h"
#include "wx/grid.h"
#include "wxgui/WxParameterEdit.H"
#include "wxgui/ewxDialog.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxDialog;
class ewxEVT_PARAMETER_VALUE_CHANGED;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXCONTRACTIONEDITORGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXCONTRACTIONEDITORGUI_TITLE _("ECCE Contraction Editor")
#define SYMBOL_WXCONTRACTIONEDITORGUI_IDNAME ID_DIALOG
#define SYMBOL_WXCONTRACTIONEDITORGUI_SIZE wxDefaultSize
#define SYMBOL_WXCONTRACTIONEDITORGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxContractionEditorGUI class declaration
 */

class WxContractionEditorGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( WxContractionEditorGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxContractionEditorGUI( );
    WxContractionEditorGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXCONTRACTIONEDITORGUI_IDNAME, const wxString& caption = SYMBOL_WXCONTRACTIONEDITORGUI_TITLE, const wxPoint& pos = SYMBOL_WXCONTRACTIONEDITORGUI_POSITION, const wxSize& size = SYMBOL_WXCONTRACTIONEDITORGUI_SIZE, long style = SYMBOL_WXCONTRACTIONEDITORGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXCONTRACTIONEDITORGUI_IDNAME, const wxString& caption = SYMBOL_WXCONTRACTIONEDITORGUI_TITLE, const wxPoint& pos = SYMBOL_WXCONTRACTIONEDITORGUI_POSITION, const wxSize& size = SYMBOL_WXCONTRACTIONEDITORGUI_SIZE, long style = SYMBOL_WXCONTRACTIONEDITORGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxContractionEditorGUI event handler declarations
    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXCONTRACTIONEDITOR_ELEMENTS
    virtual    void elementsChoiceSelectedCB( wxCommandEvent& event );

    /// wxEVT_GRID_CMD_CELL_CHANGE event handler for ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS
    virtual    void contractGridCellChangedCB( wxGridEvent& event );

    /// wxEVT_GRID_CMD_SELECT_CELL event handler for ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS
    virtual    void contractGridCellSelectedCB( wxGridEvent& event );

    /// wxEVT_GRID_CMD_EDITOR_HIDDEN event handler for ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS
    virtual    void contractGridEditorHiddenCB( wxGridEvent& event );

    /// wxEVT_GRID_CMD_CELL_CHANGE event handler for ID_GRID_WXCONTRACTIONEDITOR_SUMMARY
    virtual    void summaryGridCellChangedCB( wxGridEvent& event );

    /// wxEVT_GRID_CMD_SELECT_CELL event handler for ID_GRID_WXCONTRACTIONEDITOR_SUMMARY
    virtual    void summaryGridCellSelectedCB( wxGridEvent& event );

    /// wxEVT_GRID_CMD_EDITOR_HIDDEN event handler for ID_GRID_WXCONTRACTIONEDITOR_SUMMARY
    virtual    void summaryGridCellEditorHiddenCB( wxGridEvent& event );

    /// ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXCONTRACTIONEDITOR_DIFFUSEFUNCTIONS
    virtual    void diffuseParamEditValueChangedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXCONTRACTIONEDIT_CLOSE
    virtual    void closeButtonClickCB( wxCommandEvent& event );

////@end WxContractionEditorGUI event handler declarations

////@begin WxContractionEditorGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxContractionEditorGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxContractionEditorGUI member variables
    static const wxWindowID ID_GRID_WXCONTRACTIONEDITOR_SUMMARY;
    static const wxWindowID ID_BUTTON_WXCONTRACTIONEDITOR_HELP;
    static const wxWindowID ID_PANEL;
    static const wxWindowID ID_GRID_WXCONTRACTIONEDITOR_CONTRACTIONS;
    static const wxWindowID ID_BUTTON_WXCONTRACTIONEDIT_CLOSE;
    static const wxWindowID ID_PARAMEDIT_WXCONTRACTIONEDITOR_DIFFUSEFUNCTIONS;
    static const wxWindowID ID_DIALOG;
    static const wxWindowID ID_CHOICE_WXCONTRACTIONEDITOR_ELEMENTS;
////@end WxContractionEditorGUI member variables
};

#endif
    // _WXCONTRACTIONEDITORGUI_H_
