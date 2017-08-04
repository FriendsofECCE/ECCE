/////////////////////////////////////////////////////////////////////////////
// Name:        TranslatePanelGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _TRANSLATEPANELGUI_H_
#define _TRANSLATEPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "TranslatePanelGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
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
#define SYMBOL_TRANSLATEPANELGUI_STYLE 0
#define SYMBOL_TRANSLATEPANELGUI_TITLE _("Translate")
#define SYMBOL_TRANSLATEPANELGUI_IDNAME ID_DIALOG_TRANSL
#define SYMBOL_TRANSLATEPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_TRANSLATEPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * TranslatePanelGUI class declaration
 */

class TranslatePanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( TranslatePanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    TranslatePanelGUI( );
    TranslatePanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_TRANSLATEPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_TRANSLATEPANELGUI_POSITION, const wxSize& size = SYMBOL_TRANSLATEPANELGUI_SIZE, long style = SYMBOL_TRANSLATEPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_TRANSLATEPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_TRANSLATEPANELGUI_POSITION, const wxSize& size = SYMBOL_TRANSLATEPANELGUI_SIZE, long style = SYMBOL_TRANSLATEPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin TranslatePanelGUI event handler declarations
    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_TRANSL_GO
    virtual    void OnButtonTranslGoClick( wxCommandEvent& event );

////@end TranslatePanelGUI event handler declarations

////@begin TranslatePanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end TranslatePanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin TranslatePanelGUI member variables
    static const wxWindowID ID_DIALOG_TRANSL;
    static const wxWindowID ID_BUTTON_TRANSL_GO;
    static const wxWindowID ID_TEXTCTRL_TRANSL_X;
    static const wxWindowID ID_TEXTCTRL_TRANSL_Z;
    static const wxWindowID ID_TEXTCTRL_TRANSL_Y;
////@end TranslatePanelGUI member variables
};

#endif
    // _TRANSLATEPANELGUI_H_
