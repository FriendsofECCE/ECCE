/////////////////////////////////////////////////////////////////////////////
// Name:        WxBasisSetDetailGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXBASISSETDETAILGUI_H_
#define _WXBASISSETDETAILGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxBasisSetDetailGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/notebook.h"
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
#define SYMBOL_WXBASISSETDETAILGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXBASISSETDETAILGUI_TITLE _("ECCE Basis Set Details")
#define SYMBOL_WXBASISSETDETAILGUI_IDNAME ID_DIALOG_WXBASISSETDETAIL_MAIN
#define SYMBOL_WXBASISSETDETAILGUI_SIZE wxSize(500, 500)
#define SYMBOL_WXBASISSETDETAILGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxBasisSetDetailGUI class declaration
 */

class WxBasisSetDetailGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( WxBasisSetDetailGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxBasisSetDetailGUI( );
    WxBasisSetDetailGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXBASISSETDETAILGUI_IDNAME, const wxString& caption = SYMBOL_WXBASISSETDETAILGUI_TITLE, const wxPoint& pos = SYMBOL_WXBASISSETDETAILGUI_POSITION, const wxSize& size = SYMBOL_WXBASISSETDETAILGUI_SIZE, long style = SYMBOL_WXBASISSETDETAILGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXBASISSETDETAILGUI_IDNAME, const wxString& caption = SYMBOL_WXBASISSETDETAILGUI_TITLE, const wxPoint& pos = SYMBOL_WXBASISSETDETAILGUI_POSITION, const wxSize& size = SYMBOL_WXBASISSETDETAILGUI_SIZE, long style = SYMBOL_WXBASISSETDETAILGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxBasisSetDetailGUI event handler declarations
    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISSETDETAIL_CLOSE
    virtual    void closeButtonClickCB( wxCommandEvent& event );

////@end WxBasisSetDetailGUI event handler declarations

////@begin WxBasisSetDetailGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxBasisSetDetailGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxBasisSetDetailGUI member variables
    static const wxWindowID ID_PANEL_WXBASISSETDETAIL_MAIN;
    static const wxWindowID ID_BUTTON_WXBASISSETDETAIL_HELP;
    static const wxWindowID ID_BUTTON_WXBASISSETDETAIL_CLOSE;
    static const wxWindowID ID_PANEL_WXBASISSETDETAIL_REFERENCES;
    static const wxWindowID ID_DIALOG_WXBASISSETDETAIL_MAIN;
    static const wxWindowID ID_SCROLLEDWINDOW_WXBASISSETDETAIL_GRAPH;
    static const wxWindowID ID_PANEL_WXBASISSETDETAIL_DETAILS;
    static const wxWindowID ID_BITMAP_WXBASISSETDETAIL_GRAPH;
    static const wxWindowID ID_TEXTCTRL_WXBASISSETDETAIL_DESCRIPTION;
    static const wxWindowID ID_PANEL_WXBASISSETDETAIL_DESCRIPTION;
    static const wxWindowID ID_TEXTCTRL_WXBASISSETDETAIL_REFERENCES;
    static const wxWindowID ID_NOTEBOOK_WXBASISSETDETAIL_DETAILS;
////@end WxBasisSetDetailGUI member variables
};

#endif
    // _WXBASISSETDETAILGUI_H_
