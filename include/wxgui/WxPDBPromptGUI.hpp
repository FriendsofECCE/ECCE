/////////////////////////////////////////////////////////////////////////////
// Name:        WxPDBPromptGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXPDBPROMPTGUI_H_
#define _WXPDBPROMPTGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxPDBPromptGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/spinctrl.h"
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
#define SYMBOL_WXPDBPROMPTGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXPDBPROMPTGUI_TITLE _("PDB Prompt Dialog")
#define SYMBOL_WXPDBPROMPTGUI_IDNAME ID_PDBPROMPT_DIALOG
#define SYMBOL_WXPDBPROMPTGUI_SIZE wxSize(400, 300)
#define SYMBOL_WXPDBPROMPTGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxPDBPromptGUI class declaration
 */

class WxPDBPromptGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( WxPDBPromptGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxPDBPromptGUI( );
    WxPDBPromptGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXPDBPROMPTGUI_IDNAME, const wxString& caption = SYMBOL_WXPDBPROMPTGUI_TITLE, const wxPoint& pos = SYMBOL_WXPDBPROMPTGUI_POSITION, const wxSize& size = SYMBOL_WXPDBPROMPTGUI_SIZE, long style = SYMBOL_WXPDBPROMPTGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXPDBPROMPTGUI_IDNAME, const wxString& caption = SYMBOL_WXPDBPROMPTGUI_TITLE, const wxPoint& pos = SYMBOL_WXPDBPROMPTGUI_POSITION, const wxSize& size = SYMBOL_WXPDBPROMPTGUI_SIZE, long style = SYMBOL_WXPDBPROMPTGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxPDBPromptGUI event handler declarations
    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PDBMODEL
    virtual    void OnSpinctrlPdbmodelUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_ALT_LOC_CHOICE
    virtual    void OnAltLocChoiceSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHAIN_CHOICE
    virtual    void OnChainChoiceSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_OK
    virtual    void OnButtonOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CANCEL
    virtual    void OnButtonCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP
    virtual    void OnButtonHelpClick( wxCommandEvent& event );

////@end WxPDBPromptGUI event handler declarations

////@begin WxPDBPromptGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxPDBPromptGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxPDBPromptGUI member variables
    static const wxWindowID ID_CHAIN_CHOICE;
    static const wxWindowID ID_BUTTON_HELP;
    static const wxWindowID ID_PDBPROMPT_DIALOG;
    static const wxWindowID ID_BUTTON_CANCEL;
    static const wxWindowID ID_ALT_LOC_CHOICE;
    static const wxWindowID ID_BUTTON_OK;
    static const wxWindowID ID_SPINCTRL_PDBMODEL;
    static const wxWindowID ID_PROMPT_MODEL_OPTIONS_TEXT;
////@end WxPDBPromptGUI member variables
};

#endif
    // _WXPDBPROMPTGUI_H_
