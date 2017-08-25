/////////////////////////////////////////////////////////////////////////////
// Name:        WxMachineStatusGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXMACHINESTATUSGUI_H_
#define _WXMACHINESTATUSGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxMachineStatusGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wxgui/ewxDialog.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxDialog;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXMACHINESTATUSGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxCLOSE_BOX|wxFRAME_TOOL_WINDOW|wxFRAME_FLOAT_ON_PARENT
#define SYMBOL_WXMACHINESTATUSGUI_TITLE _("ECCE Machine Browser")
#define SYMBOL_WXMACHINESTATUSGUI_IDNAME ID_WXMACHINESTATUS_FRAME
#define SYMBOL_WXMACHINESTATUSGUI_SIZE wxSize(600, 400)
#define SYMBOL_WXMACHINESTATUSGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxMachineStatusGUI class declaration
 */

class WxMachineStatusGUI: public ewxDialog
{    
    DECLARE_CLASS( WxMachineStatusGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxMachineStatusGUI( );
    WxMachineStatusGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXMACHINESTATUSGUI_IDNAME, const wxString& caption = SYMBOL_WXMACHINESTATUSGUI_TITLE, const wxPoint& pos = SYMBOL_WXMACHINESTATUSGUI_POSITION, const wxSize& size = SYMBOL_WXMACHINESTATUSGUI_SIZE, long style = SYMBOL_WXMACHINESTATUSGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXMACHINESTATUSGUI_IDNAME, const wxString& caption = SYMBOL_WXMACHINESTATUSGUI_TITLE, const wxPoint& pos = SYMBOL_WXMACHINESTATUSGUI_POSITION, const wxSize& size = SYMBOL_WXMACHINESTATUSGUI_SIZE, long style = SYMBOL_WXMACHINESTATUSGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxMachineStatusGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_WXMACHINESTATUS_FRAME
    virtual    void mainWindowCloseCB( wxCloseEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXMACHINESTATUS_CLOSE
    virtual    void closeButtonClickCB( wxCommandEvent& event );

////@end WxMachineStatusGUI event handler declarations

////@begin WxMachineStatusGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxMachineStatusGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxMachineStatusGUI member variables
    static const wxWindowID ID_STATIC_WXMACHINESTATUS_MODE;
    static const wxWindowID ID_TEXTCTRL_WXMACHINESTATUS_STATUS;
    static const wxWindowID ID_PANEL_WXMACHINESTATUS_MAIN;
    static const wxWindowID ID_WXMACHINESTATUS_FRAME;
    static const wxWindowID ID_BUTTON_WXMACHINESTATUS_CLOSE;
////@end WxMachineStatusGUI member variables
};

#endif
    // _WXMACHINESTATUSGUI_H_
