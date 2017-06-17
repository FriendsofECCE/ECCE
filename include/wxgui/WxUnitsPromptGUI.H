/////////////////////////////////////////////////////////////////////////////
// Name:        WxUnitsPromptGUI.H
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXUNITSPROMPTGUI_H_
#define _WXUNITSPROMPTGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxUnitsPromptGUI.C"
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
class ewxChoice;
class ewxCheckBox;
class ewxChoice;
class ewxDialog;
class ewxCheckBox;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXUNITSPROMPTGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXUNITSPROMPTGUI_TITLE _("Units Prompt Dialog")
#define SYMBOL_WXUNITSPROMPTGUI_IDNAME ID_DIALOG
#define SYMBOL_WXUNITSPROMPTGUI_SIZE wxSize(400, 300)
#define SYMBOL_WXUNITSPROMPTGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxUnitsPromptGUI class declaration
 */

class WxUnitsPromptGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( WxUnitsPromptGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxUnitsPromptGUI( );
    WxUnitsPromptGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXUNITSPROMPTGUI_IDNAME, const wxString& caption = SYMBOL_WXUNITSPROMPTGUI_TITLE, const wxPoint& pos = SYMBOL_WXUNITSPROMPTGUI_POSITION, const wxSize& size = SYMBOL_WXUNITSPROMPTGUI_SIZE, long style = SYMBOL_WXUNITSPROMPTGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXUNITSPROMPTGUI_IDNAME, const wxString& caption = SYMBOL_WXUNITSPROMPTGUI_TITLE, const wxPoint& pos = SYMBOL_WXUNITSPROMPTGUI_POSITION, const wxSize& size = SYMBOL_WXUNITSPROMPTGUI_SIZE, long style = SYMBOL_WXUNITSPROMPTGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxUnitsPromptGUI event handler declarations
////@end WxUnitsPromptGUI event handler declarations

////@begin WxUnitsPromptGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxUnitsPromptGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxUnitsPromptGUI member variables
    ewxChoice* p_units;
    ewxCheckBox* p_genBonds;
    static const wxWindowID ID_CHOICE_UNITS;
    static const wxWindowID ID_DIALOG;
    static const wxWindowID ID_CHECKBOX_GENBONDS;
////@end WxUnitsPromptGUI member variables
};

#endif
    // _WXUNITSPROMPTGUI_H_
