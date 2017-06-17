/////////////////////////////////////////////////////////////////////////////
// Name:        SolvateEdGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SOLVATEEDGUI_H_
#define _SOLVATEEDGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "SolvateEdGUI.C"
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
class ewxFrame;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_SOLVATEEDGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_SOLVATEEDGUI_TITLE _("ECCE Solvation Editor")
#define SYMBOL_SOLVATEEDGUI_IDNAME ID_SOLVATEED_FRAME
#define SYMBOL_SOLVATEEDGUI_SIZE wxDefaultSize
#define SYMBOL_SOLVATEEDGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * SolvateEdGUI class declaration
 */

class SolvateEdGUI: public ewxFrame
{    
    DECLARE_CLASS( SolvateEdGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    SolvateEdGUI( );
    SolvateEdGUI( wxWindow* parent, wxWindowID id = SYMBOL_SOLVATEEDGUI_IDNAME, const wxString& caption = SYMBOL_SOLVATEEDGUI_TITLE, const wxPoint& pos = SYMBOL_SOLVATEEDGUI_POSITION, const wxSize& size = SYMBOL_SOLVATEEDGUI_SIZE, long style = SYMBOL_SOLVATEEDGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_SOLVATEEDGUI_IDNAME, const wxString& caption = SYMBOL_SOLVATEEDGUI_TITLE, const wxPoint& pos = SYMBOL_SOLVATEEDGUI_POSITION, const wxSize& size = SYMBOL_SOLVATEEDGUI_SIZE, long style = SYMBOL_SOLVATEEDGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin SolvateEdGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_SOLVATEED_FRAME
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
    virtual    void OnSaveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
    virtual    void OnExitClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
    virtual    void OnHelpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK
    virtual    void OnMenuFeedbackClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_RAD
    virtual    void OnTextctrlSlvRadEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_MIN_NX
    virtual    void OnTextctrlSlvMinNxEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_MIN_NY
    virtual    void OnTextctrlSlvMinNyEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_MIN_NZ
    virtual    void OnTextctrlSlvMinNzEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SOLVATEED_SOLVATE
    virtual    void OnButtonSolvateEdSolvateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SOLVATEED_CLEAR_SLV
    virtual    void OnButtonSolvateEdClearSlvClick( wxCommandEvent& event );

////@end SolvateEdGUI event handler declarations

////@begin SolvateEdGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end SolvateEdGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin SolvateEdGUI member variables
    static const wxWindowID ID_COMBOBOX_SOLVATEED_SOLVENT;
    static const wxWindowID ID_STATIC_SLV_X_DIM;
    static const wxWindowID ID_STATIC_SLV_DIM_HEADER;
    static const wxWindowID ID_STATIC_SLV_TATOMS;
    static const wxWindowID ID_BUTTON_SOLVATEED_SOLVATE;
    static const wxWindowID ID_TEXTCTRL_SLV_MIN_NX;
    static const wxWindowID ID_STATIC_SLV_Y_LABEL;
    static const wxWindowID ID_TEXTCTRL_SLV_MIN_NZ;
    static const wxWindowID ID_STATIC_SLV_X_LABEL;
    static const wxWindowID ID_STATIC_SLV_Y_DIM;
    static const wxWindowID ID_TEXTCTRL_SLV_MIN_NY;
    static const wxWindowID ID_BUTTON_SOLVATEED_BUILDER;
    static const wxWindowID ID_BUTTON_SOLVATEED_CLEAR_SLV;
    static const wxWindowID ID_STATIC_SLV_Z_LABEL;
    static const wxWindowID ID_TEXTCTRL_SLV_RAD;
    static const wxWindowID ID_MENU_FEEDBACK;
    static const wxWindowID ID_SOLVATEED_FRAME;
    static const wxWindowID ID_STATIC_SLV_Z_DIM;
////@end SolvateEdGUI member variables
};

#endif
    // _SOLVATEEDGUI_H_
