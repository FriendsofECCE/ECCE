/////////////////////////////////////////////////////////////////////////////
// Name:        ImageContextPanelGUI.H
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _IMAGECONTEXTPANELGUI_H_
#define _IMAGECONTEXTPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "ImageContextPanelGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/toolbar.h"
#include "wxgui/ewxPanel.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class ewxScrolledWindow;
class ewxPanel;
class ewxScrolledWindow;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_IMAGECONTEXTPANELGUI_STYLE 0
#define SYMBOL_IMAGECONTEXTPANELGUI_TITLE _("ImageContextPanelGUI")
#define SYMBOL_IMAGECONTEXTPANELGUI_IDNAME wxID_ANY
#define SYMBOL_IMAGECONTEXTPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_IMAGECONTEXTPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ImageContextPanelGUI class declaration
 */

class ImageContextPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( ImageContextPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ImageContextPanelGUI( );
    ImageContextPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_IMAGECONTEXTPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_IMAGECONTEXTPANELGUI_POSITION, const wxSize& size = SYMBOL_IMAGECONTEXTPANELGUI_SIZE, long style = SYMBOL_IMAGECONTEXTPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_IMAGECONTEXTPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_IMAGECONTEXTPANELGUI_POSITION, const wxSize& size = SYMBOL_IMAGECONTEXTPANELGUI_SIZE, long style = SYMBOL_IMAGECONTEXTPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ImageContextPanelGUI event handler declarations
    /// wxEVT_PAINT event handler for ID_SCROLLEDWINDOW
    virtual    void OnPaint( wxPaintEvent& event );

    /// wxEVT_LEFT_DOWN event handler for ID_SCROLLEDWINDOW
    virtual    void OnLeftDown( wxMouseEvent& event );

    /// wxEVT_LEFT_UP event handler for ID_SCROLLEDWINDOW
    virtual    void OnLeftUp( wxMouseEvent& event );

    /// wxEVT_MOTION event handler for ID_SCROLLEDWINDOW
    virtual    void OnMotion( wxMouseEvent& event );

    /// wxEVT_MOUSEWHEEL event handler for ID_SCROLLEDWINDOW
    virtual    void OnMouseWheel( wxMouseEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_FIT
    virtual    void OnZoomFitClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_100
    virtual    void OnZoom100Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_IN
    virtual    void OnZoomInClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_OUT
    virtual    void OnZoomOutClick( wxCommandEvent& event );

////@end ImageContextPanelGUI event handler declarations

////@begin ImageContextPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ImageContextPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ImageContextPanelGUI member variables
    wxBoxSizer* p_topSizer;
    ewxScrolledWindow* p_imageWindow;
    wxStaticBitmap* p_staticBitmap;
    wxToolBar* p_toolBar;
    static const wxWindowID ID_TOOLBAR;
    static const wxWindowID ID_SCROLLEDWINDOW;
////@end ImageContextPanelGUI member variables
};

#endif
    // _IMAGECONTEXTPANELGUI_H_
