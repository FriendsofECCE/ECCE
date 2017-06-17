/////////////////////////////////////////////////////////////////////////////
// Name:        SymmetryPanelGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SYMMETRYPANELGUI_H_
#define _SYMMETRYPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "SymmetryPanelGUI.C"
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
class wxBoxSizer;
class ewxPanel;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_SYMMETRYPANELGUI_STYLE 0
#define SYMBOL_SYMMETRYPANELGUI_TITLE _("Symmetry")
#define SYMBOL_SYMMETRYPANELGUI_IDNAME ID_DIALOG_SYMMETRY
#define SYMBOL_SYMMETRYPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_SYMMETRYPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * SymmetryPanelGUI class declaration
 */

class SymmetryPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( SymmetryPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    SymmetryPanelGUI( );
    SymmetryPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_SYMMETRYPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_SYMMETRYPANELGUI_POSITION, const wxSize& size = SYMBOL_SYMMETRYPANELGUI_SIZE, long style = SYMBOL_SYMMETRYPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_SYMMETRYPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_SYMMETRYPANELGUI_POSITION, const wxSize& size = SYMBOL_SYMMETRYPANELGUI_SIZE, long style = SYMBOL_SYMMETRYPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin SymmetryPanelGUI event handler declarations
    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_FIND
    virtual    void OnButtonSymmFindClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_CLEAN
    virtual    void OnButtonSymmCleanClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_FIND_IRREDUCIBLE
    virtual    void OnButtonSymmFindIrreducibleClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_GENERATE_GHOSTS
    virtual    void OnButtonSymmGenerateGhostsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DEL_GHOSTS
    virtual    void OnButtonDelGhostsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SYMM_GENERATE_MOLECULE
    virtual    void OnButtonSymmGenerateMoleculeClick( wxCommandEvent& event );

////@end SymmetryPanelGUI event handler declarations

////@begin SymmetryPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end SymmetryPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin SymmetryPanelGUI member variables
    wxBoxSizer* p_topSizer;
    static const wxWindowID ID_TEXTCTRL_SYMM_THRESHOLD;
    static const wxWindowID ID_BUTTON_SYMM_FIND_IRREDUCIBLE;
    static const wxWindowID ID_DIALOG_SYMMETRY;
    static const wxWindowID ID_BUTTON_SYMM_GENERATE_MOLECULE;
    static const wxWindowID ID_BUTTON_DEL_GHOSTS;
    static const wxWindowID ID_STATIC_SYMM_THRESHOLD_UNIT;
    static const wxWindowID ID_BUTTON_SYMM_FIND;
    static const wxWindowID ID_BUTTON_SYMM_GENERATE_GHOSTS;
    static const wxWindowID ID_BUTTON_SYMM_CLEAN;
////@end SymmetryPanelGUI member variables
};

#endif
    // _SYMMETRYPANELGUI_H_
