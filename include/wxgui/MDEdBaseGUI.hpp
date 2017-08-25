/////////////////////////////////////////////////////////////////////////////
// Name:        MDEdBaseGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _MDEDBASEGUI_H_
#define _MDEDBASEGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "MDEdBaseGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wx/notebook.h"
#include "wx/statline.h"
#include "wxgui/WxFeedback.H"
#include "wxgui/ewxFrame.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxNotebook;
class wxBoxSizer;
class ewxNotebook;
class ewxFrame;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_MDEDBASEGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_MDEDBASEGUI_TITLE _("ECCE NWChem MD Editor")
#define SYMBOL_MDEDBASEGUI_IDNAME ID_FRAME
#define SYMBOL_MDEDBASEGUI_SIZE wxDefaultSize
#define SYMBOL_MDEDBASEGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * MDEdBaseGUI class declaration
 */

class MDEdBaseGUI: public ewxFrame
{    
    DECLARE_CLASS( MDEdBaseGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    MDEdBaseGUI( );
    MDEdBaseGUI( wxWindow* parent, wxWindowID id = SYMBOL_MDEDBASEGUI_IDNAME, const wxString& caption = SYMBOL_MDEDBASEGUI_TITLE, const wxPoint& pos = SYMBOL_MDEDBASEGUI_POSITION, const wxSize& size = SYMBOL_MDEDBASEGUI_SIZE, long style = SYMBOL_MDEDBASEGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_MDEDBASEGUI_IDNAME, const wxString& caption = SYMBOL_MDEDBASEGUI_TITLE, const wxPoint& pos = SYMBOL_MDEDBASEGUI_POSITION, const wxSize& size = SYMBOL_MDEDBASEGUI_SIZE, long style = SYMBOL_MDEDBASEGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin MDEdBaseGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_FRAME
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
    virtual    void OnSaveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
    virtual    void OnExitClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_HELP
    virtual    void OnMenuHelpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_SUPPORT
    virtual    void OnMenuSupportClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_BUILDER
    virtual    void OnButtonBuilderClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FINALEDIT
    virtual    void OnButtonFinaleditClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_LAUNCH
    virtual    void OnButtonLaunchClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RESETW
    virtual    void OnButtonResetwClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RESETA
    virtual    void OnButtonResetaClick( wxCommandEvent& event );

////@end MDEdBaseGUI event handler declarations

////@begin MDEdBaseGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end MDEdBaseGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin MDEdBaseGUI member variables
    ewxNotebook* p_notebook;
    wxBoxSizer* p_feedbackSizer;
    static const wxWindowID ID_BUTTON_BUILDER;
    static const wxWindowID ID_MENU_HELP;
    static const wxWindowID ID_BUTTON_FINALEDIT;
    static const wxWindowID ID_FRAME;
    static const wxWindowID ID_BUTTON_RESETW;
    static const wxWindowID ID_FEEDBACK;
    static const wxWindowID ID_BUTTON_RESETA;
    static const wxWindowID ID_MENU_SUPPORT;
    static const wxWindowID ID_NOTEBOOK_ENERGY;
    static const wxWindowID ID_BUTTON_LAUNCH;
////@end MDEdBaseGUI member variables
};

#endif
    // _MDEDBASEGUI_H_
