/////////////////////////////////////////////////////////////////////////////
// Name:        WxAuthGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXAUTHGUI_H_
#define _WXAUTHGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxAuthGUI.C"
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
#define SYMBOL_WXAUTHGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXAUTHGUI_TITLE _("ECCE Authentication")
#define SYMBOL_WXAUTHGUI_IDNAME ID_DIALOG
#define SYMBOL_WXAUTHGUI_SIZE wxSize(380, 260)
#define SYMBOL_WXAUTHGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxAuthGUI class declaration
 */

class WxAuthGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( WxAuthGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxAuthGUI( );
    WxAuthGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXAUTHGUI_IDNAME, const wxString& caption = SYMBOL_WXAUTHGUI_TITLE, const wxPoint& pos = SYMBOL_WXAUTHGUI_POSITION, const wxSize& size = SYMBOL_WXAUTHGUI_SIZE, long style = SYMBOL_WXAUTHGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXAUTHGUI_IDNAME, const wxString& caption = SYMBOL_WXAUTHGUI_TITLE, const wxPoint& pos = SYMBOL_WXAUTHGUI_POSITION, const wxSize& size = SYMBOL_WXAUTHGUI_SIZE, long style = SYMBOL_WXAUTHGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxAuthGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_AUTH_USER
    virtual    void OnTextctrlAuthUserEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_AUTH_PASSWORD
    virtual    void OnTextctrlAuthPasswordEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_AUTH_NEWPASSWORD
    virtual    void OnTextctrlAuthNewpasswordEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CHANGE
    virtual    void OnChange( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_AUTH_SAVEPASSWORDS
    virtual    void OnSavePasswordsClick( wxCommandEvent& event );

////@end WxAuthGUI event handler declarations

////@begin WxAuthGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxAuthGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxAuthGUI member variables
    static const wxWindowID wxID_STATIC_AUTH_PASSWD;
    static const wxWindowID ID_TEXTCTRL_AUTH_NEWPASSWORD;
    static const wxWindowID ID_DIALOG;
    static const wxWindowID wxID_STATIC_AUTH_NEWPASSWD;
    static const wxWindowID ID_TEXTCTRL_AUTH_USER;
    static const wxWindowID wxID_STATIC_AUTH_PROMPT_LABEL;
    static const wxWindowID wxID_CHANGE;
    static const wxWindowID wxID_STATIC_AUTH_SERVER_LABEL;
    static const wxWindowID wxID_STATIC_AUTH_SERVER_VALUE;
    static const wxWindowID wxID_STATIC_AUTH_USER_LABEL;
    static const wxWindowID ID_CHECKBOX_AUTH_SAVEPASSWORDS;
    static const wxWindowID ID_TEXTCTRL_AUTH_PASSWORD;
////@end WxAuthGUI member variables
};

#endif
    // _WXAUTHGUI_H_
