/////////////////////////////////////////////////////////////////////////////
// Name:        WxFindGUI.H
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXFINDGUI_H_
#define _WXFINDGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxFindGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/statline.h"
#include "wxgui/ewxDialog.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class ewxTextCtrl;
class ewxListBox;
class ewxDialog;
class ewxListBox;
class ewxTextCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXFINDGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXFINDGUI_TITLE _("ECCE Find")
#define SYMBOL_WXFINDGUI_IDNAME ID_DIALOG
#define SYMBOL_WXFINDGUI_SIZE wxSize(500, 500)
#define SYMBOL_WXFINDGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxFindGUI class declaration
 */

class WxFindGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( WxFindGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxFindGUI( );
    WxFindGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXFINDGUI_IDNAME, const wxString& caption = SYMBOL_WXFINDGUI_TITLE, const wxPoint& pos = SYMBOL_WXFINDGUI_POSITION, const wxSize& size = SYMBOL_WXFINDGUI_SIZE, long style = SYMBOL_WXFINDGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXFINDGUI_IDNAME, const wxString& caption = SYMBOL_WXFINDGUI_TITLE, const wxPoint& pos = SYMBOL_WXFINDGUI_POSITION, const wxSize& size = SYMBOL_WXFINDGUI_SIZE, long style = SYMBOL_WXFINDGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxFindGUI event handler declarations
    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FIND
    virtual    void OnFindButtonFindClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_RESULTS
    virtual    void OnFindResultsSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX_RESULTS
    virtual    void OnFindButtonGoClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_BUTTON_CLOSE
    virtual    void OnFindButtonCloseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_BUTTON_HELP
    virtual    void OnFindButtonHelpClick( wxCommandEvent& event );

////@end WxFindGUI event handler declarations

////@begin WxFindGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxFindGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxFindGUI member variables
    wxBoxSizer* p_topSizer;
    ewxTextCtrl* p_find;
    ewxTextCtrl* p_from;
    ewxListBox* p_results;
    static const wxWindowID ID_FIND_BUTTON_HELP;
    static const wxWindowID ID_FIND_BUTTON_FIND;
    static const wxWindowID ID_FIND_BUTTON_CLOSE;
    static const wxWindowID ID_LISTBOX_RESULTS;
    static const wxWindowID ID_TEXTCTRL_FROM;
    static const wxWindowID ID_TEXTCTRL_FIND;
    static const wxWindowID ID_DIALOG;
    static const wxWindowID ID_FIND_BUTTON_GO;
////@end WxFindGUI member variables
};

#endif
    // _WXFINDGUI_H_
