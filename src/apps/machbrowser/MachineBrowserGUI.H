/////////////////////////////////////////////////////////////////////////////
// Name:        MachineBrowserGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _MACHINEBROWSERGUI_H_
#define _MACHINEBROWSERGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "MachineBrowserGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wx/statline.h"
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
#define SYMBOL_MACHINEBROWSERGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_MACHINEBROWSERGUI_TITLE _("ECCE Machine Browser")
#define SYMBOL_MACHINEBROWSERGUI_IDNAME ID_MACHINEBROWSER_FRAME
#define SYMBOL_MACHINEBROWSERGUI_SIZE wxSize(300, 200)
#define SYMBOL_MACHINEBROWSERGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * MachineBrowserGUI class declaration
 */

class MachineBrowserGUI: public ewxFrame
{    
    DECLARE_CLASS( MachineBrowserGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    MachineBrowserGUI( );
    MachineBrowserGUI( wxWindow* parent, wxWindowID id = SYMBOL_MACHINEBROWSERGUI_IDNAME, const wxString& caption = SYMBOL_MACHINEBROWSERGUI_TITLE, const wxPoint& pos = SYMBOL_MACHINEBROWSERGUI_POSITION, const wxSize& size = SYMBOL_MACHINEBROWSERGUI_SIZE, long style = SYMBOL_MACHINEBROWSERGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_MACHINEBROWSERGUI_IDNAME, const wxString& caption = SYMBOL_MACHINEBROWSERGUI_TITLE, const wxPoint& pos = SYMBOL_MACHINEBROWSERGUI_POSITION, const wxSize& size = SYMBOL_MACHINEBROWSERGUI_SIZE, long style = SYMBOL_MACHINEBROWSERGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin MachineBrowserGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_MACHINEBROWSER_FRAME
    virtual    void mainWindowCloseCB( wxCloseEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_QUERYMACHINE
    virtual    void machineQueryButtonClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_QUERYDISK
    virtual    void diskQueryButtonClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_QUERYPROCESS
    virtual    void processQueryButtonClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_CONFIGURE
    virtual    void configureButtonClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINEBROWSER_VIEWDETACH
    virtual    void viewDetachButtonClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_MACHINEBROWSER_MACHINEREGISTER
    virtual    void machineRegisterMenuItemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
    virtual    void exitMenuitemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
    virtual    void helpAppMenuitemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_MACHINEBROWSER_HELPSUPPORT
    virtual    void helpSupportMenuitemClickCB( wxCommandEvent& event );

////@end MachineBrowserGUI event handler declarations

////@begin MachineBrowserGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end MachineBrowserGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin MachineBrowserGUI member variables
    static const wxWindowID ID_STATICTEXT_MACHINEBROWSER_MACHINENAME;
    static const wxWindowID ID_BUTTON_MACHINEBROWSER_VIEWDETACH;
    static const wxWindowID ID_STATIC_MACHINEBROWSER_MODE;
    static const wxWindowID ID_MENUITEM_MACHINEBROWSER_HELPSUPPORT;
    static const wxWindowID ID_MACHINEBROWSER_FRAME;
    static const wxWindowID ID_MENU_MACHINEBROWSER_MACHINEREGISTER;
    static const wxWindowID ID_BITMAP_MACHINEBROWSER_CHECKMARK;
    static const wxWindowID ID_PANEL_MACHINEBROWSER_MACHINES;
    static const wxWindowID ID_BUTTON_MACHINEBROWSER_QUERYPROCESS;
    static const wxWindowID ID_TEXTCTRL_MACHINEBROWSER_STATUS;
    static const wxWindowID ID_BUTTON_MACHINEBROWSER_QUERYDISK;
    static const wxWindowID ID_PANEL_MACHINEBROWSER_MAIN;
    static const wxWindowID ID_BUTTON_MACHINEBROWSER_QUERYMACHINE;
    static const wxWindowID ID_BUTTON_MACHINEBROWSER_CONFIGURE;
////@end MachineBrowserGUI member variables
};

#endif
    // _MACHINEBROWSERGUI_H_
