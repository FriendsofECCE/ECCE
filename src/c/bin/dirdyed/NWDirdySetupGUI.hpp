/////////////////////////////////////////////////////////////////////////////
// Name:        NWDirdySetupGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NWDIRDYSETUPGUI_H_
#define _NWDIRDYSETUPGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "NWDirdySetupGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wx/statline.h"
#include "wxgui/ewxFrame.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxFrame;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_NWDIRDYSETUPGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_NWDIRDYSETUPGUI_TITLE _("NWChem DirDyVTST Setup")
#define SYMBOL_NWDIRDYSETUPGUI_IDNAME ID_NWDRDYSET_FRAME
#define SYMBOL_NWDIRDYSETUPGUI_SIZE wxDefaultSize
#define SYMBOL_NWDIRDYSETUPGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * NWDirdySetupGUI class declaration
 */

class NWDirdySetupGUI: public ewxFrame
{    
    DECLARE_CLASS( NWDirdySetupGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NWDirdySetupGUI( );
    NWDirdySetupGUI( wxWindow* parent, wxWindowID id = SYMBOL_NWDIRDYSETUPGUI_IDNAME, const wxString& caption = SYMBOL_NWDIRDYSETUPGUI_TITLE, const wxPoint& pos = SYMBOL_NWDIRDYSETUPGUI_POSITION, const wxSize& size = SYMBOL_NWDIRDYSETUPGUI_SIZE, long style = SYMBOL_NWDIRDYSETUPGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_NWDIRDYSETUPGUI_IDNAME, const wxString& caption = SYMBOL_NWDIRDYSETUPGUI_TITLE, const wxPoint& pos = SYMBOL_NWDIRDYSETUPGUI_POSITION, const wxSize& size = SYMBOL_NWDIRDYSETUPGUI_SIZE, long style = SYMBOL_NWDIRDYSETUPGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NWDirdySetupGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_NWDRDYSET_FRAME
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_SYM
    virtual    void OnCheckboxNwdrdysetSymClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_TOL
    virtual    void OnTextctrlNwdrdysetTolEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_AUTOZ
    virtual    void OnCheckboxNwdrdysetAutozClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SEP
    virtual    void OnTextctrlNwdrdysetSepEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_MEMORY
    virtual    void OnCheckboxNwdrdysetMemoryClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_MEMORY
    virtual    void OnTextctrlNwdrdysetMemoryEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_DISK
    virtual    void OnCheckboxNwdrdysetDiskClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_DISK
    virtual    void OnTextctrlNwdrdysetDiskEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_NWDRDYSET_TH
    virtual    void OnChoiceNwdrdysetThSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_NWDRDYSET_SPTH
    virtual    void OnChoiceNwdrdysetSpthSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_BASIS
    virtual    void OnButtonNwdrdysetBasisClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_SPBASIS
    virtual    void OnButtonNwdrdysetSpbasisClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_EXPCOEFF
    virtual    void OnCheckboxNwdrdysetExpcoeffClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF
    virtual    void OnCheckboxNwdrdysetSpexpcoeffClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_DTLS
    virtual    void OnButtonNwdrdysetDtlsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_SPDTLS
    virtual    void OnButtonNwdrdysetSpdtlsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_TST
    virtual    void OnCheckboxNwdrdysetTstClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SSTEP
    virtual    void OnTextctrlNwdrdysetSstepEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SSV
    virtual    void OnTextctrlNwdrdysetSsvEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SHSS
    virtual    void OnTextctrlNwdrdysetShssEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SMSS
    virtual    void OnTextctrlNwdrdysetSmssEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SLM
    virtual    void OnTextctrlNwdrdysetSlmEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NWDRDYSET_SLP
    virtual    void OnTextctrlNwdrdysetSlpEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_SGN
    virtual    void OnCheckboxNwdrdysetSgnClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_NWDRDYSET_INTALG
    virtual    void OnChoiceNwdrdysetIntalgSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NWDRDYSET_PRT
    virtual    void OnCheckboxNwdrdysetPrtClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_CLOSE
    virtual    void OnButtonNwdrdysetCloseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NWDRDYSET_RESET
    virtual    void OnButtonNwdrdysetResetClick( wxCommandEvent& event );

////@end NWDirdySetupGUI event handler declarations

////@begin NWDirdySetupGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end NWDirdySetupGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NWDirdySetupGUI member variables
    static const wxWindowID ID_CHOICE_NWDRDYSET_TH;
    static const wxWindowID wxID_STATIC_NWDRDYSET_TOL;
    static const wxWindowID wxID_STATIC_NWDRDYSET_BASIS_LABEL;
    static const wxWindowID ID_TEXTCTRL_NWDRDYSET_TOL;
    static const wxWindowID ID_NWDRDYSET_FRAME;
    static const wxWindowID ID_TEXTCTRL_NWDRDYSET_DISK;
    static const wxWindowID ID_CHECKBOX_NWDRDYSET_SGN;
    static const wxWindowID ID_BUTTON_NWDRDYSET_SPBASIS;
    static const wxWindowID ID_CHECKBOX_NWDRDYSET_SYM;
    static const wxWindowID ID_BUTTON_NWDRDYSET_SPDTLS;
    static const wxWindowID wxID_STATIC_NWDRDYSET_SLM;
    static const wxWindowID ID_CHOICE_NWDRDYSET_SPTH;
    static const wxWindowID ID_TEXTCTRL_NWDRDYSET_SLP;
    static const wxWindowID ID_TEXTCTRL_NWDRDYSET_SLM;
    static const wxWindowID ID_CHECKBOX_NWDRDYSET_PRT;
    static const wxWindowID ID_TEXTCTRL_NWDRDYSET_MEMORY;
    static const wxWindowID wxID_STATIC_NWDRDYSET_SSV;
    static const wxWindowID ID_BUTTON_NWDRDYSET_CLOSE;
    static const wxWindowID ID_TEXTCTRL_NWDRDYSET_SMSS;
    static const wxWindowID wxID_STATIC_NWDRDYSET_SLP;
    static const wxWindowID wxID_STATIC_NWDRDYSET_SSTEP;
    static const wxWindowID wxID_STATIC_NWDRDYSET_SHSS;
    static const wxWindowID ID_STATIC_NWDRDYSET_SPBASIS_NAME;
    static const wxWindowID wxID_STATICTEXT_NWDRDYSET_MEMORY;
    static const wxWindowID ID_STATIC_NWDRDYSET_BASIS_NAME;
    static const wxWindowID ID_FEEDBACK_NWDRDYSET;
    static const wxWindowID ID_TEXTCTRL_NWDRDYSET_SSTEP;
    static const wxWindowID ID_CHECKBOX_NWDRDYSET_EXPCOEFF;
    static const wxWindowID ID_BUTTON_NWDRDYSET_BASIS;
    static const wxWindowID ID_BUTTON_NWDRDYSET_DTLS;
    static const wxWindowID ID_CHECKBOX_NWDRDYSET_DISK;
    static const wxWindowID ID_CHECKBOX_NWDRDYSET_TST;
    static const wxWindowID ID_CHOICE_NWDRDYSET_INTALG;
    static const wxWindowID ID_CHECKBOX_NWDRDYSET_MEMORY;
    static const wxWindowID wxID_STATIC_NWDRDYSET_SEP;
    static const wxWindowID ID_BUTTON_NWDRDYSET_RESET;
    static const wxWindowID ID_CHECKBOX_NWDRDYSET_AUTOZ;
    static const wxWindowID ID_CHECKBOX_NWDRDYSET_SPEXPCOEFF;
    static const wxWindowID wxID_STATIC_NWDRDYSET_SMSS;
    static const wxWindowID wxID_STATICTEXT_NWDRDYSET_DISK;
    static const wxWindowID ID_TEXTCTRL_NWDRDYSET_SEP;
    static const wxWindowID wxID_STATIC_NWDRDYSET_SPBASIS_LABEL;
    static const wxWindowID ID_TEXTCTRL_NWDRDYSET_SHSS;
    static const wxWindowID ID_TEXTCTRL_NWDRDYSET_SSV;
////@end NWDirdySetupGUI member variables
};

#endif
    // _NWDIRDYSETUPGUI_H_
