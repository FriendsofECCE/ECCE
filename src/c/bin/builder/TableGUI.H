/////////////////////////////////////////////////////////////////////////////
// Name:        TableGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _TABLEGUI_H_
#define _TABLEGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "TableGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/grid.h"
#include "PropertyPanel.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class IPropCalculation;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_TABLEGUI_STYLE wxSUNKEN_BORDER
#define SYMBOL_TABLEGUI_TITLE _("MO Panel")
#define SYMBOL_TABLEGUI_IDNAME ID_PANEL_TABLE
#define SYMBOL_TABLEGUI_SIZE wxDefaultSize
#define SYMBOL_TABLEGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * TableGUI class declaration
 */

class TableGUI: public PropertyPanel
{    
    DECLARE_DYNAMIC_CLASS( TableGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    TableGUI( );
    TableGUI( IPropCalculation* calculation, wxWindow* parent, wxWindowID id = SYMBOL_TABLEGUI_IDNAME, const wxPoint& pos = SYMBOL_TABLEGUI_POSITION, const wxSize& size = SYMBOL_TABLEGUI_SIZE, long style = SYMBOL_TABLEGUI_STYLE );

    /// Creation
    bool Create( IPropCalculation* calculation, wxWindow* parent, wxWindowID id = SYMBOL_TABLEGUI_IDNAME, const wxPoint& pos = SYMBOL_TABLEGUI_POSITION, const wxSize& size = SYMBOL_TABLEGUI_SIZE, long style = SYMBOL_TABLEGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin TableGUI event handler declarations
////@end TableGUI event handler declarations

////@begin TableGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end TableGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin TableGUI member variables
    static const wxWindowID ID_GRID_TABLE;
    static const wxWindowID ID_PANEL_TABLE;
////@end TableGUI member variables
};

#endif
    // _TABLEGUI_H_
