/////////////////////////////////////////////////////////////////////////////
// Name:        PlaneGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PLANEGUI_H_
#define _PLANEGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "PlaneGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/notebook.h"
#include "wx/spinctrl.h"
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
#define SYMBOL_PLANEGUI_STYLE wxCAPTION|wxSUNKEN_BORDER
#define SYMBOL_PLANEGUI_TITLE _("ECCE Define Plane")
#define SYMBOL_PLANEGUI_IDNAME ID_DIALOG_PLANE
#define SYMBOL_PLANEGUI_SIZE wxDefaultSize
#define SYMBOL_PLANEGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * PlaneGUI class declaration
 */

class PlaneGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( PlaneGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    PlaneGUI( );
    PlaneGUI( wxWindow* parent, wxWindowID id = SYMBOL_PLANEGUI_IDNAME, const wxString& caption = SYMBOL_PLANEGUI_TITLE, const wxPoint& pos = SYMBOL_PLANEGUI_POSITION, const wxSize& size = SYMBOL_PLANEGUI_SIZE, long style = SYMBOL_PLANEGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_PLANEGUI_IDNAME, const wxString& caption = SYMBOL_PLANEGUI_TITLE, const wxPoint& pos = SYMBOL_PLANEGUI_POSITION, const wxSize& size = SYMBOL_PLANEGUI_SIZE, long style = SYMBOL_PLANEGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin PlaneGUI event handler declarations
    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PLANE_H
    virtual    void OnSpinctrlPlaneHUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PLANE_K
    virtual    void OnSpinctrlPlaneKUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PLANE_L
    virtual    void OnSpinctrlPlaneLUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLANE_NAME
    virtual    void OnTextctrlPlaneNameEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLANE_COLOR
    virtual    void OnButtonPlaneColorClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLANE_TRANSPARENT
    virtual    void OnCheckboxPlaneTransparentClick( wxCommandEvent& event );

////@end PlaneGUI event handler declarations

////@begin PlaneGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end PlaneGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin PlaneGUI member variables
    static const wxWindowID ID_PANEL_TAB_SELECTION;
    static const wxWindowID ID_SPINCTRL_PLANE_H;
    static const wxWindowID ID_SPINCTRL_PLANE_L;
    static const wxWindowID ID_BUTTON_PLANE_COLOR;
    static const wxWindowID ID_NOTEBOOK_PLANE;
    static const wxWindowID ID_TEXTCTRL_PLANE_NAME;
    static const wxWindowID ID_SPINCTRL_PLANE_K;
    static const wxWindowID ID_DIALOG_PLANE;
    static const wxWindowID ID_PANEL_TAB_MILLER;
    static const wxWindowID ID_CHECKBOX_PLANE_TRANSPARENT;
////@end PlaneGUI member variables
};

#endif
    // _PLANEGUI_H_
