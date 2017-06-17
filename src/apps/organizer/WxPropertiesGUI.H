/////////////////////////////////////////////////////////////////////////////
// Name:        WxPropertiesGUI.H
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXPROPERTIESGUI_H_
#define _WXPROPERTIESGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxPropertiesGUI.C"
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
class ewxFrame;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXPROPERTIESGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXPROPERTIESGUI_TITLE _("Resource Properties")
#define SYMBOL_WXPROPERTIESGUI_IDNAME ID_FRAME_PROPERTIES_DIALOG
#define SYMBOL_WXPROPERTIESGUI_SIZE wxSize(550, 340)
#define SYMBOL_WXPROPERTIESGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxPropertiesGUI class declaration
 */

class WxPropertiesGUI: public ewxFrame
{    
    DECLARE_CLASS( WxPropertiesGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxPropertiesGUI( );
    WxPropertiesGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXPROPERTIESGUI_IDNAME, const wxString& caption = SYMBOL_WXPROPERTIESGUI_TITLE, const wxPoint& pos = SYMBOL_WXPROPERTIESGUI_POSITION, const wxSize& size = SYMBOL_WXPROPERTIESGUI_SIZE, long style = SYMBOL_WXPROPERTIESGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXPROPERTIESGUI_IDNAME, const wxString& caption = SYMBOL_WXPROPERTIESGUI_TITLE, const wxPoint& pos = SYMBOL_WXPROPERTIESGUI_POSITION, const wxSize& size = SYMBOL_WXPROPERTIESGUI_SIZE, long style = SYMBOL_WXPROPERTIESGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxPropertiesGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_FRAME_PROPERTIES_DIALOG
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CLOSE_PROPERTIES
    virtual    void OnButtonClosePropertiesClick( wxCommandEvent& event );

////@end WxPropertiesGUI event handler declarations

////@begin WxPropertiesGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxPropertiesGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxPropertiesGUI member variables
    wxBoxSizer* p_topSizer;
    wxBoxSizer* p_propSizer;
    static const wxWindowID ID_BUTTON_CLOSE_PROPERTIES;
    static const wxWindowID ID_TEXTCTRL_URL;
    static const wxWindowID ID_FRAME_PROPERTIES_DIALOG;
////@end WxPropertiesGUI member variables
};

#endif
    // _WXPROPERTIESGUI_H_
