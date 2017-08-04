/////////////////////////////////////////////////////////////////////////////
// Name:        CoordPanelGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _COORDPANELGUI_H_
#define _COORDPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "CoordPanelGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/notebook.h"
#include "wx/statline.h"
#include "wx/spinctrl.h"
#include "wxgui/ewxPanel.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxFlexGridSizer;
class ewxPanel;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_COORDPANELGUI_STYLE wxNO_BORDER
#define SYMBOL_COORDPANELGUI_TITLE _("CoordPanelGUI")
#define SYMBOL_COORDPANELGUI_IDNAME ID_COORDINATE_PANEL
#define SYMBOL_COORDPANELGUI_SIZE wxDefaultSize
#define SYMBOL_COORDPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * CoordPanelGUI class declaration
 */

class CoordPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( CoordPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CoordPanelGUI( );
    CoordPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_COORDPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_COORDPANELGUI_POSITION, const wxSize& size = SYMBOL_COORDPANELGUI_SIZE, long style = SYMBOL_COORDPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_COORDPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_COORDPANELGUI_POSITION, const wxSize& size = SYMBOL_COORDPANELGUI_SIZE, long style = SYMBOL_COORDPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CoordPanelGUI event handler declarations
    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIO_CENTER_ORIGIN
    virtual    void OnCenterSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CENTER_APPLY_TO
    virtual    void OnCenterApplyToSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CENTER
    virtual    void OnCenterClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_BY_X
    virtual    void OnTransByXEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_BY_Y
    virtual    void OnTransByYEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_BY_Z
    virtual    void OnTransByZEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_TRANS_EXPAND
    virtual    void OnTransExpandClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_FROM_X
    virtual    void OnTransFromXEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_FROM_Y
    virtual    void OnTransFromYEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_FROM_Z
    virtual    void OnTransFromZEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANS_FROM_PICK
    virtual    void OnTransFromPickClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_TO_X
    virtual    void OnTransToXEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_TO_Y
    virtual    void OnTransToYEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_TO_Z
    virtual    void OnTransToZEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANS_TO_PICK
    virtual    void OnTransToPickClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANSLATE
    virtual    void OnTranslateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_X
    virtual    void OnDuplicateDim1XEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_Y
    virtual    void OnDuplicateDim1YEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_Z
    virtual    void OnDuplicateDim1ZEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_X
    virtual    void OnDuplicateDim2XEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_Y
    virtual    void OnDuplicateDim2YEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_Z
    virtual    void OnDuplicateDim2ZEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_X
    virtual    void OnDuplicateDim3XEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_Y
    virtual    void OnDuplicateDim3YEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_Z
    virtual    void OnDuplicateDim3ZEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DUPLICATE
    virtual    void OnDuplicateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_ROTATE_START_X
    virtual    void OnRotateStartUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE_START_PICK
    virtual    void OnRotateStartPickClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_ROTATE_END_X
    virtual    void OnRotateEndUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE_END_PICK
    virtual    void OnRotateEndPickClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE
    virtual    void OnRotateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_ALIGN_TO_AXIS
    virtual    void OnAlignToAxisSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_ALIGN_TO_PLANE
    virtual    void OnAlignToPlaneSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_X_YZ
    virtual    void OnButtonAlignX_YZClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_Y_ZX
    virtual    void OnButtonAlignY_ZXClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_Z_XY
    virtual    void OnButtonAlignZ_XYClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ALIGN
    virtual    void OnAlignClick( wxCommandEvent& event );

////@end CoordPanelGUI event handler declarations

////@begin CoordPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CoordPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CoordPanelGUI member variables
    wxFlexGridSizer* p_translateSizer;
    static const wxWindowID ID_DUPLICATE_DIM1_Z;
    static const wxWindowID ID_RADIO_CENTER_Z;
    static const wxWindowID ID_RADIO_CENTER_X;
    static const wxWindowID ID_BUTTON_ALIGN_Z_XY;
    static const wxWindowID ID_ROTATE_END_X;
    static const wxWindowID ID_TRANS_FROM_X;
    static const wxWindowID ID_TRANS_APPLY_TO;
    static const wxWindowID ID_DUPLICATE_DIM2_COUNT;
    static const wxWindowID ID_TRANS_FROM_PICK;
    static const wxWindowID ID_BUTTON_ALIGN_Y_ZX;
    static const wxWindowID ID_CENTER_PANEL;
    static const wxWindowID ID_TRANS_BY_Y;
    static const wxWindowID ID_ROTATE_START_Z;
    static const wxWindowID ID_ALIGN_PANEL;
    static const wxWindowID ID_DUPLICATE_DIM2_Y;
    static const wxWindowID ID_ALIGN_TO_AXIS;
    static const wxWindowID ID_ROTATE;
    static const wxWindowID ID_TRANSLATE_PANEL;
    static const wxWindowID ID_DUPLICATE_DIM2_X;
    static const wxWindowID ID_RANDOM_ORIENT;
    static const wxWindowID ID_TRANS_TO_Z;
    static const wxWindowID ID_ALIGN_NZ;
    static const wxWindowID ID_STATIC_PLAX;
    static const wxWindowID ID_ROTATE_END_Y;
    static const wxWindowID ID_TRANS_TO_PICK;
    static const wxWindowID ID_TRANS_EXPAND;
    static const wxWindowID ID_ROTATE_DEGREE;
    static const wxWindowID ID_ROT_APPLY_TO;
    static const wxWindowID ID_DUPLICATE_DIM3_Y;
    static const wxWindowID ID_TRANS_TO_X;
    static const wxWindowID ID_DUPLICATE_PANEL;
    static const wxWindowID ID_DUPLICATE_DIM3_Z;
    static const wxWindowID ID_COORDINATE_PANEL;
    static const wxWindowID ID_ALIGN;
    static const wxWindowID ID_ALIGN_NX;
    static const wxWindowID ID_ALIGN_APPLY_TO;
    static const wxWindowID ID_ROTATE_PANEL;
    static const wxWindowID ID_DUPLICATE_DIM1_Y;
    static const wxWindowID ID_DUPLICATE_DIM1_COUNT;
    static const wxWindowID ID_TRANS_FROM_Z;
    static const wxWindowID ID_TRANSLATE;
    static const wxWindowID ID_NOTEBOOK;
    static const wxWindowID ID_ROTATE_START_X;
    static const wxWindowID ID_RADIO_CENTER_Y;
    static const wxWindowID ID_ALIGN_NY;
    static const wxWindowID ID_ROTATE_START_Y;
    static const wxWindowID ID_DUPLICATE_DIM2_Z;
    static const wxWindowID ID_CENTER;
    static const wxWindowID ID_TRANS_FROM_Y;
    static const wxWindowID ID_ALIGN_TO_PLANE;
    static const wxWindowID ID_RADIO_CENTER_ORIGIN;
    static const wxWindowID ID_DUPLICATE;
    static const wxWindowID ID_DUPLICATE_DIM3_COUNT;
    static const wxWindowID ID_ROTATE_END_PICK;
    static const wxWindowID ID_ROTATE_START_PICK;
    static const wxWindowID ID_CENTER_APPLY_TO;
    static const wxWindowID ID_ROTATE_END_Z;
    static const wxWindowID ID_TRANS_BY_Z;
    static const wxWindowID ID_TRANS_BY_X;
    static const wxWindowID ID_BUTTON_ALIGN_X_YZ;
    static const wxWindowID ID_TRANS_TO_Y;
    static const wxWindowID ID_DUPLICATE_DIM3_X;
    static const wxWindowID ID_DUPLICATE_DIM1_X;
////@end CoordPanelGUI member variables
};

#endif
    // _COORDPANELGUI_H_
