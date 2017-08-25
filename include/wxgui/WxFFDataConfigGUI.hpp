/////////////////////////////////////////////////////////////////////////////
// Name:        WxFFDataConfigGUI.H
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXFFDATACONFIGGUI_H_
#define _WXFFDATACONFIGGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxFFDataConfigGUI.C"
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
#define SYMBOL_WXFFDATACONFIGGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXFFDATACONFIGGUI_TITLE _("ECCE MD Source Selection")
#define SYMBOL_WXFFDATACONFIGGUI_IDNAME ID_DIALOG
#define SYMBOL_WXFFDATACONFIGGUI_SIZE wxSize(400, 300)
#define SYMBOL_WXFFDATACONFIGGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxFFDataConfigGUI class declaration
 */

class WxFFDataConfigGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( WxFFDataConfigGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxFFDataConfigGUI( );
    WxFFDataConfigGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXFFDATACONFIGGUI_IDNAME, const wxString& caption = SYMBOL_WXFFDATACONFIGGUI_TITLE, const wxPoint& pos = SYMBOL_WXFFDATACONFIGGUI_POSITION, const wxSize& size = SYMBOL_WXFFDATACONFIGGUI_SIZE, long style = SYMBOL_WXFFDATACONFIGGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXFFDATACONFIGGUI_IDNAME, const wxString& caption = SYMBOL_WXFFDATACONFIGGUI_TITLE, const wxPoint& pos = SYMBOL_WXFFDATACONFIGGUI_POSITION, const wxSize& size = SYMBOL_WXFFDATACONFIGGUI_SIZE, long style = SYMBOL_WXFFDATACONFIGGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxFFDataConfigGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_DIALOG
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_FORCE_FIELD
    virtual    void OnChoiceForceFieldSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_BROWSE
    virtual    void OnButtonFfBrowseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_REMOVE
    virtual    void OnButtonFfRemoveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_UP
    virtual    void OnButtonFfUpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FF_DOWN
    virtual    void OnButtonFfDownClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_BROWSE
    virtual    void OnButtonSegBrowseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_REMOVE
    virtual    void OnButtonSegRemoveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_UP
    virtual    void OnButtonSegUpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SEG_DOWN
    virtual    void OnButtonSegDownClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DEFAULTS
    virtual    void OnButtonDefaultsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
    virtual    void OnCloseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    virtual    void OnHelpClick( wxCommandEvent& event );

////@end WxFFDataConfigGUI event handler declarations

////@begin WxFFDataConfigGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxFFDataConfigGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxFFDataConfigGUI member variables
    static const wxWindowID ID_BUTTON_SEG_UP;
    static const wxWindowID ID_CHOICE_FORCE_FIELD;
    static const wxWindowID ID_BUTTON_SEG_DOWN;
    static const wxWindowID ID_BUTTON_FF_BROWSE;
    static const wxWindowID ID_LISTBOX_SEG;
    static const wxWindowID ID_BUTTON_SEG_REMOVE;
    static const wxWindowID ID_DIALOG;
    static const wxWindowID ID_BUTTON_FF_DOWN;
    static const wxWindowID ID_LISTBOX_FF;
    static const wxWindowID ID_BUTTON_DEFAULTS;
    static const wxWindowID ID_BUTTON_FF_REMOVE;
    static const wxWindowID ID_BUTTON_FF_UP;
    static const wxWindowID ID_BUTTON_SEG_BROWSE;
////@end WxFFDataConfigGUI member variables
};

#endif
    // _WXFFDATACONFIGGUI_H_
