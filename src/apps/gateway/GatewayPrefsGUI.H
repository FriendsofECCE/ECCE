/////////////////////////////////////////////////////////////////////////////
// Name:        GatewayPrefsGUI.H
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _GATEWAYPREFSGUI_H_
#define _GATEWAYPREFSGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "GatewayPrefsGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wxgui/ewxFrame.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxChoice;
class ewxCheckBox;
class wxGridSizer;
class ewxButton;
class ewxChoice;
class ewxButton;
class ewxCheckBox;
class ewxFrame;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_GATEWAYPREFSGUI_STYLE wxCAPTION|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_GATEWAYPREFSGUI_TITLE _("ECCE Gateway and Global Preferences")
#define SYMBOL_GATEWAYPREFSGUI_IDNAME ID_GATEWAYPREF_FRAME
#define SYMBOL_GATEWAYPREFSGUI_SIZE wxDefaultSize
#define SYMBOL_GATEWAYPREFSGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * GatewayPrefsGUI class declaration
 */

class GatewayPrefsGUI: public ewxFrame
{    
    DECLARE_CLASS( GatewayPrefsGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    GatewayPrefsGUI( );
    GatewayPrefsGUI( wxWindow* parent, wxWindowID id = SYMBOL_GATEWAYPREFSGUI_IDNAME, const wxString& caption = SYMBOL_GATEWAYPREFSGUI_TITLE, const wxPoint& pos = SYMBOL_GATEWAYPREFSGUI_POSITION, const wxSize& size = SYMBOL_GATEWAYPREFSGUI_SIZE, long style = SYMBOL_GATEWAYPREFSGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_GATEWAYPREFSGUI_IDNAME, const wxString& caption = SYMBOL_GATEWAYPREFSGUI_TITLE, const wxPoint& pos = SYMBOL_GATEWAYPREFSGUI_POSITION, const wxSize& size = SYMBOL_GATEWAYPREFSGUI_SIZE, long style = SYMBOL_GATEWAYPREFSGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin GatewayPrefsGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_GATEWAYPREF_FRAME
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_THEME_CHOICE
    virtual    void OnThemeChoiceSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_FONTSIZE
    virtual    void OnGlobalPrefChange( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_SHOWCONFIRM_CHECKBOX
    virtual    void OnGatewayPrefChange( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CLOSE_BUTTON
    virtual    void OnCloseButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RESETALL_BUTTON
    virtual    void OnResetAllButtonClick( wxCommandEvent& event );

////@end GatewayPrefsGUI event handler declarations

////@begin GatewayPrefsGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end GatewayPrefsGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin GatewayPrefsGUI member variables
    ewxChoice* p_colorTheme;
    ewxChoice* p_fontSize;
    ewxChoice* p_dateFormat;
    ewxChoice* p_timeFormat;
    ewxChoice* p_unit;
    ewxCheckBox* p_beepError;
    ewxCheckBox* p_beepWarn;
    ewxCheckBox* p_focus;
    ewxCheckBox* p_confirmExit;
    ewxCheckBox* p_showBusy;
    ewxCheckBox* p_alwaysOnTop;
    ewxCheckBox* p_leftClickNewApp;
    ewxCheckBox* p_closeShells;
    ewxCheckBox* p_savePasswords;
    ewxChoice* p_orientation;
    wxGridSizer* p_stateIconSizer;
    ewxButton* p_resetAll;
    static const wxWindowID ID_SHOWBUSY_CHECKBOX;
    static const wxWindowID ID_BEEPWARN_CHECKBOX;
    static const wxWindowID ID_ALWAYS_ON_TOP;
    static const wxWindowID ID_UNIT_CHOICE;
    static const wxWindowID ID_TIMEFORMAT_CHOICE;
    static const wxWindowID ID_ORIENTATION;
    static const wxWindowID ID_FONTSIZE;
    static const wxWindowID ID_CLOSE_BUTTON;
    static const wxWindowID ID_BEEPERROR_CHECKBOX;
    static const wxWindowID ID_SHOWCONFIRM_CHECKBOX;
    static const wxWindowID ID_THEME_CHOICE;
    static const wxWindowID ID_RESETALL_BUTTON;
    static const wxWindowID ID_GATEWAYPREF_FRAME;
    static const wxWindowID ID_LEFTCLICK_NEWAPP;
    static const wxWindowID ID_HELP_BUTTON;
    static const wxWindowID ID_SAVEPASSWORDS_CHECKBOX;
    static const wxWindowID ID_DATEFORMAT_CHOICE;
    static const wxWindowID ID_FOCUS_FOLLOW_MOUSE;
    static const wxWindowID ID_CLOSESHELLS_CHECKBOX;
////@end GatewayPrefsGUI member variables
};

#endif
    // _GATEWAYPREFSGUI_H_
