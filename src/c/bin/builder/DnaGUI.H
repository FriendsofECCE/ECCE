/////////////////////////////////////////////////////////////////////////////
// Name:        DnaGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _DNAGUI_H_
#define _DNAGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "DnaGUI.C"
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
#define SYMBOL_DNAGUI_STYLE wxSUNKEN_BORDER
#define SYMBOL_DNAGUI_TITLE _("Dialog")
#define SYMBOL_DNAGUI_IDNAME ID_DIALOG_DNA
#define SYMBOL_DNAGUI_SIZE wxDefaultSize
#define SYMBOL_DNAGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * DnaGUI class declaration
 */

class DnaGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( DnaGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    DnaGUI( );
    DnaGUI( wxWindow* parent, wxWindowID id = SYMBOL_DNAGUI_IDNAME, const wxPoint& pos = SYMBOL_DNAGUI_POSITION, const wxSize& size = SYMBOL_DNAGUI_SIZE, long style = SYMBOL_DNAGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_DNAGUI_IDNAME, const wxPoint& pos = SYMBOL_DNAGUI_POSITION, const wxSize& size = SYMBOL_DNAGUI_SIZE, long style = SYMBOL_DNAGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin DnaGUI event handler declarations
    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_DNA_SEQUENCE
    virtual    void OnTextctrlDnaSequenceUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DNA_SEQUENCE
    virtual    void OnTextctrlDnaSequenceEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_AT
    virtual    void OnButtonDnaAtClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_TA
    virtual    void OnButtonDnaTaClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_GC
    virtual    void OnButtonDnaGcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_CG
    virtual    void OnButtonDnaCgClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DNA_GENERATE
    virtual    void OnButtonDnaGenerateClick( wxCommandEvent& event );

////@end DnaGUI event handler declarations

////@begin DnaGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end DnaGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin DnaGUI member variables
    static const wxWindowID ID_BUTTON_DNA_GENERATE;
    static const wxWindowID ID_BUTTON_DNA_TA;
    static const wxWindowID ID_TEXTCTRL_DNA_SEQUENCE;
    static const wxWindowID ID_DIALOG_DNA;
    static const wxWindowID ID_BUTTON_DNA_AT;
    static const wxWindowID ID_BUTTON_DNA_CG;
    static const wxWindowID ID_BUTTON_DNA_GC;
////@end DnaGUI member variables
};

#endif
    // _DNAGUI_H_
