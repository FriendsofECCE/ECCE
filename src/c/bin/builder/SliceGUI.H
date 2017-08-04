/////////////////////////////////////////////////////////////////////////////
// Name:        SliceGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SLICEGUI_H_
#define _SLICEGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "SliceGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/spinctrl.h"
#include "wxgui/ewxPanel.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxPanel;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_SLICEGUI_STYLE wxSUNKEN_BORDER
#define SYMBOL_SLICEGUI_TITLE _("Dialog")
#define SYMBOL_SLICEGUI_IDNAME ID_DIALOG_SLICE
#define SYMBOL_SLICEGUI_SIZE wxDefaultSize
#define SYMBOL_SLICEGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * SliceGUI class declaration
 */

class SliceGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( SliceGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    SliceGUI( );
    SliceGUI( wxWindow* parent, wxWindowID id = SYMBOL_SLICEGUI_IDNAME, const wxPoint& pos = SYMBOL_SLICEGUI_POSITION, const wxSize& size = SYMBOL_SLICEGUI_SIZE, long style = SYMBOL_SLICEGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_SLICEGUI_IDNAME, const wxPoint& pos = SYMBOL_SLICEGUI_POSITION, const wxSize& size = SYMBOL_SLICEGUI_SIZE, long style = SYMBOL_SLICEGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin SliceGUI event handler declarations
    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_H
    virtual    void OnSpinctrlSliceHUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_K
    virtual    void OnSpinctrlSliceKUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_L
    virtual    void OnSpinctrlSliceLUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_SHOWPLANE
    virtual    void OnCheckboxShowplaneClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICE_COLOR_1
    virtual    void OnButtonSliceColor1Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_SLICE_TRANSPARENT_1
    virtual    void OnCheckboxSliceTransparent1Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_OFFSET_1
    virtual    void OnSpinctrlSliceOffset1Updated( wxSpinEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICE_COLOR_2
    virtual    void OnButtonSliceColor2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_SLICE_TRANSPARENT_2
    virtual    void OnCheckboxSliceTransparent2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_SLICE_OFFSET_2
    virtual    void OnSpinctrlSliceOffset2Updated( wxSpinEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICER_XFORM
    virtual    void OnButtonSlicerXformClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SLICER_SLICE
    virtual    void OnButtonSlicerSliceClick( wxCommandEvent& event );

////@end SliceGUI event handler declarations

////@begin SliceGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end SliceGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin SliceGUI member variables
    static const wxWindowID ID_BUTTON_SLICE_COLOR_1;
    static const wxWindowID ID_DIALOG_SLICE;
    static const wxWindowID ID_BUTTON_SLICER_XFORM;
    static const wxWindowID ID_CHECKBOX_SLICE_TRANSPARENT_2;
    static const wxWindowID ID_CHECKBOX_SLICE_TRANSPARENT_1;
    static const wxWindowID ID_SPINCTRL_SLICE_OFFSET_2;
    static const wxWindowID ID_SPINCTRL_SLICE_L;
    static const wxWindowID ID_PANEL_SLICE_FIELDS;
    static const wxWindowID ID_SPINCTRL_SLICE_H;
    static const wxWindowID ID_SPINCTRL_SLICE_OFFSET_1;
    static const wxWindowID ID_BUTTON_SLICER_SLICE;
    static const wxWindowID ID_BUTTON_SLICE_COLOR_2;
    static const wxWindowID ID_SPINCTRL_SLICE_K;
    static const wxWindowID ID_CHECKBOX_SHOWPLANE;
////@end SliceGUI member variables
};

#endif
    // _SLICEGUI_H_
