/////////////////////////////////////////////////////////////////////////////
// Name:        WxConfigureAccessGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXCONFIGUREACCESSGUI_H_
#define _WXCONFIGUREACCESSGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxConfigureAccessGUI.C"
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
class ewxDialog;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXCONFIGUREACCESSGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXCONFIGUREACCESSGUI_TITLE _("Setup Remote Access")
#define SYMBOL_WXCONFIGUREACCESSGUI_IDNAME ID_WXCONFIGURACCESS_FRAME
#define SYMBOL_WXCONFIGUREACCESSGUI_SIZE wxDefaultSize
#define SYMBOL_WXCONFIGUREACCESSGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxConfigureAccessGUI class declaration
 */

class WxConfigureAccessGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( WxConfigureAccessGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxConfigureAccessGUI( );
    WxConfigureAccessGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXCONFIGUREACCESSGUI_IDNAME, const wxString& caption = SYMBOL_WXCONFIGUREACCESSGUI_TITLE, const wxPoint& pos = SYMBOL_WXCONFIGUREACCESSGUI_POSITION, const wxSize& size = SYMBOL_WXCONFIGUREACCESSGUI_SIZE, long style = SYMBOL_WXCONFIGUREACCESSGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXCONFIGUREACCESSGUI_IDNAME, const wxString& caption = SYMBOL_WXCONFIGUREACCESSGUI_TITLE, const wxPoint& pos = SYMBOL_WXCONFIGUREACCESSGUI_POSITION, const wxSize& size = SYMBOL_WXCONFIGUREACCESSGUI_SIZE, long style = SYMBOL_WXCONFIGUREACCESSGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxConfigureAccessGUI event handler declarations
    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXCONFIGUREACCESS_REMOTESHELL
    virtual    void remoteShellChoiceSelectedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXCONFIGUREACCESS_USERNAME
    virtual    void usernameTextCtrlUpdatedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD1
    virtual    void password1TextCtrlUpdatedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD2
    virtual    void password2TextCtrlUpdatedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXCONFIGUREACCESS_OKAY
    virtual    void okayButtonClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXCONFIGUREACCESS_CANCEL
    virtual    void cancelButtonClickCB( wxCommandEvent& event );

////@end WxConfigureAccessGUI event handler declarations

////@begin WxConfigureAccessGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxConfigureAccessGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxConfigureAccessGUI member variables
    static const wxWindowID ID_STATIC_WXCONFIGUREACCESS_PASSWORD2;
    static const wxWindowID ID_PANEL_WXCONFIGUREACCESS_PASSWORD1;
    static const wxWindowID ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD2;
    static const wxWindowID ID_TEXTCTRL_WXCONFIGUREACCESS_USERNAME;
    static const wxWindowID ID_PANEL;
    static const wxWindowID ID_PANEL_WXCONFIGUREACCESS_PASSWORD2;
    static const wxWindowID ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD1;
    static const wxWindowID ID_STATIC_WXCONFIGUREACCESS_PASSWORD1;
    static const wxWindowID ID_CHOICE_WXCONFIGUREACCESS_REMOTESHELL;
    static const wxWindowID ID_BUTTON_WXCONFIGUREACCESS_CANCEL;
    static const wxWindowID ID_WXCONFIGURACCESS_FRAME;
    static const wxWindowID ID_BUTTON_WXCONFIGUREACCESS_OKAY;
    static const wxWindowID ID_PANEL_WXCONFIGUREACCESS_USERNAME;
    static const wxWindowID ID_PANEL1;
    static const wxWindowID ID_PANEL_WXCONFIGUREACCESS_REMOTESHELL;
////@end WxConfigureAccessGUI member variables
};

#endif
    // _WXCONFIGUREACCESSGUI_H_
