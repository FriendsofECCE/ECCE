/////////////////////////////////////////////////////////////////////////////
// Name:        GridGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _GRIDGUI_H_
#define _GRIDGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "GridGUI.C"
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
class ewxDialog;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_GRIDGUI_STYLE wxRESIZE_BORDER|wxCLOSE_BOX|wxSUNKEN_BORDER
#define SYMBOL_GRIDGUI_TITLE _("ECCE 3D Grid Definition")
#define SYMBOL_GRIDGUI_IDNAME ID_DIALOG_GRID
#define SYMBOL_GRIDGUI_SIZE wxDefaultSize
#define SYMBOL_GRIDGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * GridGUI class declaration
 */

class GridGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( GridGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    GridGUI( );
    GridGUI( wxWindow* parent, wxWindowID id = SYMBOL_GRIDGUI_IDNAME, const wxString& caption = SYMBOL_GRIDGUI_TITLE, const wxPoint& pos = SYMBOL_GRIDGUI_POSITION, const wxSize& size = SYMBOL_GRIDGUI_SIZE, long style = SYMBOL_GRIDGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_GRIDGUI_IDNAME, const wxString& caption = SYMBOL_GRIDGUI_TITLE, const wxPoint& pos = SYMBOL_GRIDGUI_POSITION, const wxSize& size = SYMBOL_GRIDGUI_SIZE, long style = SYMBOL_GRIDGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin GridGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_DIALOG_GRID
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_GRID_UNIFORM
    virtual    void OnCheckboxGridUniformClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_GRID_UNIFORM
    virtual    void OnTextctrlGridUniformEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    virtual    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_APPLY
    virtual    void OnApplyClick( wxCommandEvent& event );

////@end GridGUI event handler declarations

////@begin GridGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end GridGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin GridGUI member variables
    static const wxWindowID ID_TEXTCTRL_GRID_TOY;
    static const wxWindowID ID_TEXTCTRL_GRID_ZRES;
    static const wxWindowID ID_STATICTEXT_GRIDZ;
    static const wxWindowID ID_TEXTCTRLGRID_FROMX;
    static const wxWindowID ID_TEXTCTRL_GRID_TOZ;
    static const wxWindowID ID_CHECKBOX_GRID_UNIFORM;
    static const wxWindowID ID_TEXTCTRL_GRID_FROMZ;
    static const wxWindowID ID_DIALOG_GRID;
    static const wxWindowID ID_STATICTEXT_GRIDX;
    static const wxWindowID ID_TEXTCTRL_GRID_UNIFORM;
    static const wxWindowID ID_TEXTCTRL_GRID_TOX;
    static const wxWindowID ID_TEXTCTRL_GRID_XRES;
    static const wxWindowID ID_TEXTCTRL_GRID_YRES;
    static const wxWindowID ID_TEXTCTRL_GRID_FROMY;
    static const wxWindowID ID_STATICTEXT_GRIDY;
////@end GridGUI member variables
};

#endif
    // _GRIDGUI_H_
