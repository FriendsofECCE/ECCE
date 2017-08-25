/////////////////////////////////////////////////////////////////////////////
// Name:        SelectionPanelGUI.H
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SELECTIONPANELGUI_H_
#define _SELECTIONPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "SelectionPanelGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/notebook.h"
#include "wx/spinctrl.h"
#include "wx/statline.h"
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
#define SYMBOL_SELECTIONPANELGUI_STYLE 0
#define SYMBOL_SELECTIONPANELGUI_TITLE _("SelectionPanelGUI")
#define SYMBOL_SELECTIONPANELGUI_IDNAME ID_DIALOG_SEL
#define SYMBOL_SELECTIONPANELGUI_SIZE wxSize(350, 180)
#define SYMBOL_SELECTIONPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * SelectionPanelGUI class declaration
 */

class SelectionPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( SelectionPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    SelectionPanelGUI( );
    SelectionPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_SELECTIONPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_SELECTIONPANELGUI_POSITION, const wxSize& size = SYMBOL_SELECTIONPANELGUI_SIZE, long style = SYMBOL_SELECTIONPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_SELECTIONPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_SELECTIONPANELGUI_POSITION, const wxSize& size = SYMBOL_SELECTIONPANELGUI_SIZE, long style = SYMBOL_SELECTIONPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin SelectionPanelGUI event handler declarations
    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_ELMNT
    virtual    void OnComboboxElmntSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_COMBOBOX_ELMNT
    virtual    void OnComboboxElmntUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_ELMNT
    virtual    void OnComboboxElmntEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_FRST_AT
    virtual    void OnSpinctrlFrstAtUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_FRST_AT
    virtual    void OnSpinctrlFrstAtTextUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_LAST_AT
    virtual    void OnSpinctrlLastAtUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_LAST_AT
    virtual    void OnSpinctrlLastAtTextUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_SELECT
    virtual    void OnAtomSelectClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_XSELECT
    virtual    void OnAtomXselectClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_INVERSE
    virtual    void OnAtomInverseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_CLEAR
    virtual    void OnAtomClearClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RES_ATYP
    virtual    void OnTextctrlResAtypUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RES_ANAM
    virtual    void OnTextctrlResAnamUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RESNAM
    virtual    void OnTextctrlResnamUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_CHAIN
    virtual    void OnTextctrlChainUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_SELECT
    virtual    void OnButtonResSelectClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_XSELECT
    virtual    void OnButtonResXselectClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_INVERSE
    virtual    void OnButtonResInverseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_CLEAR
    virtual    void OnButtonResClearClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_MINDIS
    virtual    void OnTextctrlMindisUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MINDIS
    virtual    void OnTextctrlMindisEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_MAXDIS
    virtual    void OnTextctrlMaxdisUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MAXDIS
    virtual    void OnTextctrlMaxdisEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_MINBND
    virtual    void OnSpinctrlMinbndUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_MINBND
    virtual    void OnSpinctrlMinbndTextUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_MAXBND
    virtual    void OnSpinctrlMaxbndUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_MAXBND
    virtual    void OnSpinctrlMaxbndTextUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_SELECT
    virtual    void OnButtonDisSelectClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_XSELECT
    virtual    void OnButtonDisXselectClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_INVERSE
    virtual    void OnButtonDisInverseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_CLEAR
    virtual    void OnButtonDisClearClick( wxCommandEvent& event );

////@end SelectionPanelGUI event handler declarations

////@begin SelectionPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end SelectionPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin SelectionPanelGUI member variables
    wxPanel* p_residuePanel;
    static const wxWindowID ID_RADIOBUTTON_RESNAM;
    static const wxWindowID ID_BUTTON_DIS_SELECT;
    static const wxWindowID ID_BUTTON_RES_INVERSE;
    static const wxWindowID ID_RADIOBUTTON_FLRES;
    static const wxWindowID ID_RADIOBUTTON_DISTANCE;
    static const wxWindowID ID_TEXTCTRL_RES_ATYP;
    static const wxWindowID ID_BUTTON_RES_CLEAR;
    static const wxWindowID ID_RADIOBUTTON_AT_ALL;
    static const wxWindowID ID_RADIOBUTTON_ANAM;
    static const wxWindowID ID_RADIOBUTTON_CHAIN;
    static const wxWindowID ID_TEXTCTRL_RES_ANAM;
    static const wxWindowID ID_RADIOBUTTON_AT_MOL;
    static const wxWindowID ID_RADIOBUTTON_AT_ELMNT;
    static const wxWindowID ID_RADIOBUTTON_BBONE;
    static const wxWindowID ID_COMBOBOX_ELMNT;
    static const wxWindowID ID_BUTTON_DIS_INVERSE;
    static const wxWindowID ID_BUTTON_DIS_CLEAR;
    static const wxWindowID ID_RADIOBUTTON_BONDS;
    static const wxWindowID ID_BUTTON_DIS_XSELECT;
    static const wxWindowID ID_SPINCTRL_LAST_AT;
    static const wxWindowID ID_SPINCTRL_FRST_AT;
    static const wxWindowID ID_RADIOBUTTON_SCHAIN;
    static const wxWindowID ID_CHECKBOX_FULL_RES;
    static const wxWindowID ID_ATOM_INVERSE;
    static const wxWindowID ID_RADIOBUTTON_SLVNT;
    static const wxWindowID ID_SPINCTRL_MAXBND;
    static const wxWindowID ID_ATOM_SELECT;
    static const wxWindowID ID_RADIOBUTTON_AT_IDX;
    static const wxWindowID ID_ATOMS;
    static const wxWindowID ID_TEXTCTRL_CHAIN;
    static const wxWindowID ID_ATOM_CLEAR;
    static const wxWindowID ID_PANEL2;
    static const wxWindowID ID_TEXTCTRL_MINDIS;
    static const wxWindowID ID_BUTTON_RES_XSELECT;
    static const wxWindowID ID_TEXTCTRL_MAXDIS;
    static const wxWindowID ID_RESIDUES;
    static const wxWindowID ID_TEXTCTRL_RESNAM;
    static const wxWindowID ID_ATOM_XSELECT;
    static const wxWindowID ID_DIALOG_SEL;
    static const wxWindowID ID_BUTTON_RES_SELECT;
    static const wxWindowID ID_SPINCTRL_MINBND;
    static const wxWindowID ID_RADIOBUTTON_ATYP;
////@end SelectionPanelGUI member variables
};

#endif
    // _SELECTIONPANELGUI_H_
