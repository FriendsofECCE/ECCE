/////////////////////////////////////////////////////////////////////////////
// Name:        BuilderGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _BUILDERGUI_H_
#define _BUILDERGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "BuilderGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wxgui/ewxTool.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxTool;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_BUILDERGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_BUILDERGUI_TITLE _("ECCE Builder")
#define SYMBOL_BUILDERGUI_IDNAME ID_FRAME_Builder
#define SYMBOL_BUILDERGUI_SIZE wxSize(800, 600)
#define SYMBOL_BUILDERGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * BuilderGUI class declaration
 */

class BuilderGUI: public ewxTool
{    
    DECLARE_CLASS( BuilderGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    BuilderGUI( );
    BuilderGUI( wxWindow* parent, wxWindowID id = SYMBOL_BUILDERGUI_IDNAME, const wxString& caption = SYMBOL_BUILDERGUI_TITLE, const wxPoint& pos = SYMBOL_BUILDERGUI_POSITION, const wxSize& size = SYMBOL_BUILDERGUI_SIZE, long style = SYMBOL_BUILDERGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_BUILDERGUI_IDNAME, const wxString& caption = SYMBOL_BUILDERGUI_TITLE, const wxPoint& pos = SYMBOL_BUILDERGUI_POSITION, const wxSize& size = SYMBOL_BUILDERGUI_SIZE, long style = SYMBOL_BUILDERGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin BuilderGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_FRAME_Builder
    virtual    void mainWindowCloseCB( wxCloseEvent& event );

    /// wxEVT_SIZE event handler for ID_FRAME_Builder
    virtual    void OnSize( wxSizeEvent& event );

    /// wxEVT_LEFT_UP event handler for ID_FRAME_Builder
    virtual    void OnLeftUp( wxMouseEvent& event );

    /// wxEVT_KEY_DOWN event handler for ID_FRAME_Builder
    virtual    void OnKeyDown( wxKeyEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_IMPORT
    virtual    void OnImportChemsysClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_IMPORTCALC
    virtual    void OnImportCalcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_NEW
    virtual    void OnNewClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_OPEN
    virtual    void OnOpenClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CLOSE
    virtual    void OnCloseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
    virtual    void OnSaveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVEAS
    virtual    void OnSaveasClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_SAVE_THUMB
    virtual    void OnSaveThumbClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_DUMPSG
    virtual    void OnDumpsgClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
    virtual    void quitMenuItemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UNDO
    virtual    void OnUndoClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_REDO
    virtual    void OnRedoClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CUT
    virtual    void OnCutClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_COPY
    virtual    void OnCopyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_PASTE
    virtual    void OnPasteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CLEAR
    virtual    void OnClearClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DELETE
    virtual    void OnDeleteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SELECTALL
    virtual    void OnSelectallClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_MOLECULE
    virtual    void OnSelectMoleculeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_SOLVENT
    virtual    void OnSelectSolventClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_BACKBONE
    virtual    void OnSelectBackboneClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_SIDECHAINS
    virtual    void OnSelectSidechainsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_REVERSE
    virtual    void OnSelectReverseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_UNSELECT
    virtual    void OnUnselectClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MODE_PREV
    virtual    void OnModePrevClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MODE_NEXT
    virtual    void OnModeNextClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
    virtual    void helpBuilderMenuitemClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_SUPPORT
    virtual    void helpSupportMenuitemClick( wxCommandEvent& event );

////@end BuilderGUI event handler declarations

////@begin BuilderGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end BuilderGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin BuilderGUI member variables
    static const wxWindowID ID_SELECT_SIDECHAINS;
    static const wxWindowID ID_IMPORTCALC;
    static const wxWindowID ID_MODE_NEXT;
    static const wxWindowID ID_SELECT_BACKBONE;
    static const wxWindowID ID_FRAME_Builder;
    static const wxWindowID ID_UNSELECT;
    static const wxWindowID ID_IMPORT;
    static const wxWindowID ID_SELECT_REVERSE;
    static const wxWindowID ID_SELECT_MOLECULE;
    static const wxWindowID ID_DUMPSG;
    static const wxWindowID ID_SELECT_SOLVENT;
    static const wxWindowID ID_SAVE_THUMB;
    static const wxWindowID ID_SUPPORT;
    static const wxWindowID ID_MODE_PREV;
////@end BuilderGUI member variables
};

#endif
    // _BUILDERGUI_H_
