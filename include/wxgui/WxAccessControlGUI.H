/////////////////////////////////////////////////////////////////////////////
// Name:        WxAccessControlGUI.H
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXACCESSCONTROLGUI_H_
#define _WXACCESSCONTROLGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxAccessControlGUI.C"
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
class wxBoxSizer;
class ewxStaticText;
class ewxTextCtrl;
class ewxStaticText;
class ewxFrame;
class ewxTextCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXACCESSCONTROLGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXACCESSCONTROLGUI_TITLE _("ECCE Access Control")
#define SYMBOL_WXACCESSCONTROLGUI_IDNAME ID_FRAME_ACCESS_CONTROL_TOP
#define SYMBOL_WXACCESSCONTROLGUI_SIZE wxDefaultSize
#define SYMBOL_WXACCESSCONTROLGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxAccessControlGUI class declaration
 */

class WxAccessControlGUI: public ewxFrame
{    
    DECLARE_CLASS( WxAccessControlGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxAccessControlGUI( );
    WxAccessControlGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXACCESSCONTROLGUI_IDNAME, const wxString& caption = SYMBOL_WXACCESSCONTROLGUI_TITLE, const wxPoint& pos = SYMBOL_WXACCESSCONTROLGUI_POSITION, const wxSize& size = SYMBOL_WXACCESSCONTROLGUI_SIZE, long style = SYMBOL_WXACCESSCONTROLGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXACCESSCONTROLGUI_IDNAME, const wxString& caption = SYMBOL_WXACCESSCONTROLGUI_TITLE, const wxPoint& pos = SYMBOL_WXACCESSCONTROLGUI_POSITION, const wxSize& size = SYMBOL_WXACCESSCONTROLGUI_SIZE, long style = SYMBOL_WXACCESSCONTROLGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxAccessControlGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_FRAME_ACCESS_CONTROL_TOP
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CHANGE
    virtual    void OnButtonChangeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CLOSE
    virtual    void OnButtonCloseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP
    virtual    void OnButtonHelpClick( wxCommandEvent& event );

////@end WxAccessControlGUI event handler declarations

////@begin WxAccessControlGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxAccessControlGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxAccessControlGUI member variables
    wxBoxSizer* p_topSizer;
    ewxStaticText* p_projectName;
    ewxTextCtrl* p_readOnly;
    ewxTextCtrl* p_readWrite;
    wxBoxSizer* p_buttonSizer;
    static const wxWindowID ID_BUTTON_HELP;
    static const wxWindowID ID_BUTTON_CLOSE;
    static const wxWindowID ID_TEXTCTRL_READONLY;
    static const wxWindowID ID_BUTTON_CHANGE;
    static const wxWindowID ID_TEXTCTRL_READWRITE;
    static const wxWindowID ID_FRAME_ACCESS_CONTROL_TOP;
////@end WxAccessControlGUI member variables
};

#endif
    // _WXACCESSCONTROLGUI_H_
