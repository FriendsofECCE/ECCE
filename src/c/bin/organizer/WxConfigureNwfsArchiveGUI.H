/////////////////////////////////////////////////////////////////////////////
// Name:        WxConfigureNwfsArchiveGUI.H
// Purpose:     
// Author:      Jack Chatterton
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXCONFIGURENWFSARCHIVEGUI_H_
#define _WXCONFIGURENWFSARCHIVEGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxConfigureNwfsArchiveGUI.C"
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
#define SYMBOL_WXCONFIGURENWFSARCHIVEGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXCONFIGURENWFSARCHIVEGUI_TITLE _("Configure NWFS Archive")
#define SYMBOL_WXCONFIGURENWFSARCHIVEGUI_IDNAME wxID_CONFIGURE_NWFS_ARCHIVE
#define SYMBOL_WXCONFIGURENWFSARCHIVEGUI_SIZE wxSize(350, 120)
#define SYMBOL_WXCONFIGURENWFSARCHIVEGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxConfigureNwfsArchiveGUI class declaration
 */

class WxConfigureNwfsArchiveGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( WxConfigureNwfsArchiveGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxConfigureNwfsArchiveGUI( );
    WxConfigureNwfsArchiveGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_IDNAME, const wxString& caption = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_TITLE, const wxPoint& pos = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_POSITION, const wxSize& size = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_SIZE, long style = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_IDNAME, const wxString& caption = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_TITLE, const wxPoint& pos = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_POSITION, const wxSize& size = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_SIZE, long style = SYMBOL_WXCONFIGURENWFSARCHIVEGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxConfigureNwfsArchiveGUI event handler declarations
    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_OK
    virtual    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CANCEL
    virtual    void OnCancelClick( wxCommandEvent& event );

////@end WxConfigureNwfsArchiveGUI event handler declarations

////@begin WxConfigureNwfsArchiveGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxConfigureNwfsArchiveGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxConfigureNwfsArchiveGUI member variables
    static const wxWindowID ID_CANCEL;
    static const wxWindowID ID_ROOTDIR;
    static const wxWindowID ID_PASSWORD;
    static const wxWindowID ID_USERNAME;
    static const wxWindowID ID_OK;
    static const wxWindowID wxID_CONFIGURE_NWFS_ARCHIVE;
////@end WxConfigureNwfsArchiveGUI member variables
};

#endif
    // _WXCONFIGURENWFSARCHIVEGUI_H_
