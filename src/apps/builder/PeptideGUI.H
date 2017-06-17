/////////////////////////////////////////////////////////////////////////////
// Name:        PeptideGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PEPTIDEGUI_H_
#define _PEPTIDEGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "PeptideGUI.C"
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
class wxGridSizer;
class ewxPanel;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_PEPTIDEGUI_STYLE wxSUNKEN_BORDER
#define SYMBOL_PEPTIDEGUI_TITLE _("Dialog")
#define SYMBOL_PEPTIDEGUI_IDNAME ID_DIALOG_PEPTIDE
#define SYMBOL_PEPTIDEGUI_SIZE wxDefaultSize
#define SYMBOL_PEPTIDEGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * PeptideGUI class declaration
 */

class PeptideGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( PeptideGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    PeptideGUI( );
    PeptideGUI( wxWindow* parent, wxWindowID id = SYMBOL_PEPTIDEGUI_IDNAME, const wxPoint& pos = SYMBOL_PEPTIDEGUI_POSITION, const wxSize& size = SYMBOL_PEPTIDEGUI_SIZE, long style = SYMBOL_PEPTIDEGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_PEPTIDEGUI_IDNAME, const wxPoint& pos = SYMBOL_PEPTIDEGUI_POSITION, const wxSize& size = SYMBOL_PEPTIDEGUI_SIZE, long style = SYMBOL_PEPTIDEGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin PeptideGUI event handler declarations
    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_PEPTIDE_SEQUENCE
    virtual    void OnTextctrlPeptideSequenceUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PEPTIDE_SEQUENCE
    virtual    void OnTextctrlPeptideSequenceEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PEPTIDE_CLEAR
    virtual    void OnButtonPeptideClearClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PEPTIDE_GENERATE
    virtual    void OnButtonPeptideGenerateClick( wxCommandEvent& event );

////@end PeptideGUI event handler declarations

////@begin PeptideGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end PeptideGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin PeptideGUI member variables
    wxGridSizer* p_buttonSizer;
    static const wxWindowID ID_TEXTCTRL_PEPTIDE_SEQUENCE;
    static const wxWindowID ID_BUTTON_PEPTIDE_GENERATE;
    static const wxWindowID ID_BUTTON_PEPTIDE_CLEAR;
    static const wxWindowID ID_DIALOG_PEPTIDE;
////@end PeptideGUI member variables
};

#endif
    // _PEPTIDEGUI_H_
