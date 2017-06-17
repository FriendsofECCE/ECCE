/////////////////////////////////////////////////////////////////////////////
// Name:        ResidueIndexPromptGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _RESIDUEINDEXPROMPTGUI_H_
#define _RESIDUEINDEXPROMPTGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "ResidueIndexPromptGUI.C"
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
class wxBoxSizer;
class ewxDialog;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_RESIDUEINDEXPROMPTGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_RESIDUEINDEXPROMPTGUI_TITLE _("Ecce Residue Selection Prompt")
#define SYMBOL_RESIDUEINDEXPROMPTGUI_IDNAME ID_DIALOG_RESPROMPT
#define SYMBOL_RESIDUEINDEXPROMPTGUI_SIZE wxSize(400, 300)
#define SYMBOL_RESIDUEINDEXPROMPTGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ResidueIndexPromptGUI class declaration
 */

class ResidueIndexPromptGUI: public ewxDialog
{    
    DECLARE_DYNAMIC_CLASS( ResidueIndexPromptGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ResidueIndexPromptGUI( );
    ResidueIndexPromptGUI( wxWindow* parent, wxWindowID id = SYMBOL_RESIDUEINDEXPROMPTGUI_IDNAME, const wxString& caption = SYMBOL_RESIDUEINDEXPROMPTGUI_TITLE, const wxPoint& pos = SYMBOL_RESIDUEINDEXPROMPTGUI_POSITION, const wxSize& size = SYMBOL_RESIDUEINDEXPROMPTGUI_SIZE, long style = SYMBOL_RESIDUEINDEXPROMPTGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_RESIDUEINDEXPROMPTGUI_IDNAME, const wxString& caption = SYMBOL_RESIDUEINDEXPROMPTGUI_TITLE, const wxPoint& pos = SYMBOL_RESIDUEINDEXPROMPTGUI_POSITION, const wxSize& size = SYMBOL_RESIDUEINDEXPROMPTGUI_SIZE, long style = SYMBOL_RESIDUEINDEXPROMPTGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ResidueIndexPromptGUI event handler declarations
    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_RESPROMPT_SELECT
    virtual    void OnRadiobuttonRespromptSelectSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_RESPROMPT_NEW
    virtual    void OnRadiobuttonRespromptNewSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_RESPROMPT_DELETE
    virtual    void OnRadiobuttonRespromptDeleteSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    virtual    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    virtual    void OnCancelClick( wxCommandEvent& event );

////@end ResidueIndexPromptGUI event handler declarations

////@begin ResidueIndexPromptGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ResidueIndexPromptGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ResidueIndexPromptGUI member variables
    wxBoxSizer* p_masterSizer;
    wxBoxSizer* p_sizerForSelectPopup;
    wxBoxSizer* p_sizerForInsertPopup;
    static const wxWindowID ID_CHOICE_RES_SELECT;
    static const wxWindowID ID_RADIOBUTTON_RESPROMPT_SELECT;
    static const wxWindowID ID_RADIOBUTTON_RESPROMPT_NEW;
    static const wxWindowID ID_CHOICE_RES_NEW;
    static const wxWindowID ID_RADIOBUTTON_RESPROMPT_DELETE;
    static const wxWindowID wxID_STATIC_NAME;
    static const wxWindowID ID_TEXTCTRL_RESPROMPT_NAME;
    static const wxWindowID ID_DIALOG_RESPROMPT;
////@end ResidueIndexPromptGUI member variables
};

#endif
    // _RESIDUEINDEXPROMPTGUI_H_
