/////////////////////////////////////////////////////////////////////////////
// Name:        PolyrateGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _POLYRATEGUI_H_
#define _POLYRATEGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "PolyrateGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wx/notebook.h"
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
#define SYMBOL_POLYRATEGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_POLYRATEGUI_TITLE _("ECCE POLYRATE Editor")
#define SYMBOL_POLYRATEGUI_IDNAME ID_PLYRT_FRAME
#define SYMBOL_POLYRATEGUI_SIZE wxDefaultSize
#define SYMBOL_POLYRATEGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * PolyrateGUI class declaration
 */

class PolyrateGUI: public ewxFrame
{    
    DECLARE_CLASS( PolyrateGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    PolyrateGUI( );
    PolyrateGUI( wxWindow* parent, wxWindowID id = SYMBOL_POLYRATEGUI_IDNAME, const wxString& caption = SYMBOL_POLYRATEGUI_TITLE, const wxPoint& pos = SYMBOL_POLYRATEGUI_POSITION, const wxSize& size = SYMBOL_POLYRATEGUI_SIZE, long style = SYMBOL_POLYRATEGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_POLYRATEGUI_IDNAME, const wxString& caption = SYMBOL_POLYRATEGUI_TITLE, const wxPoint& pos = SYMBOL_POLYRATEGUI_POSITION, const wxSize& size = SYMBOL_POLYRATEGUI_SIZE, long style = SYMBOL_POLYRATEGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin PolyrateGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_PLYRT_FRAME
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLYRT_FROPT
    virtual    void OnCheckboxPlyrtFroptClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_SCLRV
    virtual    void OnTextctrlPlyrtSclrvEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_SGMF
    virtual    void OnTextctrlPlyrtSgmfEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_SGMR
    virtual    void OnTextctrlPlyrtSgmrEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_FSCL
    virtual    void OnTextctrlPlyrtFsclEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLYRT_SDDL
    virtual    void OnCheckboxPlyrtSddlClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_ADD
    virtual    void OnButtonPlyrtAddClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_DELETE
    virtual    void OnButtonPlyrtDeleteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLYRT_ANLYS
    virtual    void OnCheckboxPlyrtAnlysClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_CMPST
    virtual    void OnChoicePlyrtCmpstSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DEG1
    virtual    void OnTextctrlPlyrtDeg1Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_NRG1
    virtual    void OnTextctrlPlyrtNrg1Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DEG2
    virtual    void OnTextctrlPlyrtDeg2Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_NRG2
    virtual    void OnTextctrlPlyrtNrg2Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DEG3
    virtual    void OnTextctrlPlyrtDeg3Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_NRG3
    virtual    void OnTextctrlPlyrtNrg3Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_CLRST
    virtual    void OnButtonPlyrtClrstClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_CMPMD
    virtual    void OnChoicePlyrtCmpmdSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_MRSADD
    virtual    void OnButtonPlyrtMrsaddClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_MRSDEL
    virtual    void OnButtonPlyrtMrsdelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PLYRT_MRS
    virtual    void OnListboxPlyrtMrsSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_TRSNAD
    virtual    void OnButtonPlyrtTrsnadClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_TRSNDL
    virtual    void OnButtonPlyrtTrsndlClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PLYRT_TRSN
    virtual    void OnListboxPlyrtTrsnSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_MODE
    virtual    void OnChoicePlyrtModeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_DTYP
    virtual    void OnChoicePlyrtDtypSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DVALS
    virtual    void OnTextctrlPlyrtDvalsEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_OPTADD
    virtual    void OnButtonPlyrtOptaddClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_OPTDEL
    virtual    void OnButtonPlyrtOptdelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PLYRT_OPTLST
    virtual    void OnListboxPlyrtOptlstSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_CMPAX
    virtual    void OnChoicePlyrtCmpaxSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_AXIS
    virtual    void OnChoicePlyrtAxisSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_FINAL_EDIT
    virtual    void OnButtonPlyrtFinalEditClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_LAUNCH
    virtual    void OnButtonPlyrtLaunchClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_PLYRT_SAVE
    virtual    void OnMenuPlyrtSaveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_PLYRT_CLOSE
    virtual    void OnMenuPlyrtCloseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK
    virtual    void OnMenuFeedbackClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_PLYRT_HELP
    virtual    void OnMenuPlyrtHelpClick( wxCommandEvent& event );

////@end PolyrateGUI event handler declarations

////@begin PolyrateGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end PolyrateGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin PolyrateGUI member variables
    static const wxWindowID ID_LISTBOX_PLYRT_TRSN;
    static const wxWindowID ID_CHOICE_PLYRT_CMPST;
    static const wxWindowID ID_TEXTCTRL_PLYRT_SYMNUM;
    static const wxWindowID ID_TEXTCTRL_PLYRT_MINS;
    static const wxWindowID ID_TEXTCTRL_PLYRT_TMPRTR;
    static const wxWindowID ID_TEXTCTRL_PLYRT_DVALS;
    static const wxWindowID ID_BUTTON_PLYRT_TRSNDL;
    static const wxWindowID ID_CHOICE_PLYRT_SCHM;
    static const wxWindowID ID_PANEL_PLYRT_TORID;
    static const wxWindowID ID_STATICTEXT;
    static const wxWindowID ID_TEXTCTRL_PLYRT_DEG3;
    static const wxWindowID ID_TEXTCTRL_PLYRT_DEG2;
    static const wxWindowID ID_PANEL_PLYRT_MORSE;
    static const wxWindowID ID_BUTTON_PLYRT_MRSADD;
    static const wxWindowID ID_CHECKBOX_PLYRT_ANLYS;
    static const wxWindowID ID_TEXTCTRL_PLYRT_NRG1;
    static const wxWindowID ID_TEXTCTRL_PLYRT_DEG1;
    static const wxWindowID ID_BUTTON_PLYRT_ADDRNG;
    static const wxWindowID wxID_STATIC_PLYRT_NRG1;
    static const wxWindowID ID_NOTEBOOK;
    static const wxWindowID wxID_STATIC_PLYRT_MINS;
    static const wxWindowID ID_CHECKBOX_PLYRT_SDDL;
    static const wxWindowID ID_CHOICE_PLYRT_AMOD;
    static const wxWindowID ID_CHOICE_PLYRT_AXIS;
    static const wxWindowID ID_LISTBOX_PLYRT_TMPLST;
    static const wxWindowID ID_BUTTON_PLYRT_OPTADD;
    static const wxWindowID wxID_STATIC_PLYRT_MAXS;
    static const wxWindowID ID_CHOICE_PLYRT_APLVL;
    static const wxWindowID ID_BUTTON_PLYRT_DELETE;
    static const wxWindowID ID_BUTTON_PLYRT_CLRST;
    static const wxWindowID ID_TEXTCTRL_PLYRT_TRSNMD;
    static const wxWindowID ID_MENU_FEEDBACK;
    static const wxWindowID ID_TEXTCTRL_PLYRT_SBGRP;
    static const wxWindowID ID_LISTBOX_PLYRT_RNGLST;
    static const wxWindowID ID_TEXTCTRL_PLYRT_SCLRV;
    static const wxWindowID ID_BUTTON_PLYRT_FINAL_EDIT;
    static const wxWindowID ID_NOTEBOOK_PLYRT_ANHRM;
    static const wxWindowID ID_LISTBOX_PLYRT_OPTLST;
    static const wxWindowID ID_PANEL_PLYRT_ANHRM;
    static const wxWindowID ID_BUTTON_PLYRT_TRSNAD;
    static const wxWindowID wxID_STATIC_PLYRT_DVALS;
    static const wxWindowID wxID_STATIC_PLYRT_NRG2;
    static const wxWindowID ID_BUTTON_PLYRT_ADD;
    static const wxWindowID ID_NOTEBOOK1;
    static const wxWindowID ID_BUTTON_PLYRT_DELRNG;
    static const wxWindowID ID_PANEL_PLYRT_TRSNAH;
    static const wxWindowID ID_TEXTCTRL_PLYRT_SGMR;
    static const wxWindowID ID_TEXTCTRL_PLYRT_SGMF;
    static const wxWindowID ID_BUTTON_PLYRT_LAUNCH;
    static const wxWindowID ID_TEXTCTRL_PLYRT_MAXS;
    static const wxWindowID ID_LISTBOX_PLYRT_MRS;
    static const wxWindowID ID_TEXTCTRL_PLYRT_NRG2;
    static const wxWindowID ID_MENU_PLYRT_CLOSE;
    static const wxWindowID ID_MENU_PLYRT_SAVE;
    static const wxWindowID ID_TEXTCTRL_PLYRT_DMIN;
    static const wxWindowID ID_CHOICE_PLYRT_DTYP;
    static const wxWindowID ID_PANEL_PLYRT_STATES;
    static const wxWindowID ID_CHECKBOX_PLYRT_FROPT;
    static const wxWindowID ID_FEEDBACK_PLYRT;
    static const wxWindowID ID_CHOICE_PLYRT_CMPAX;
    static const wxWindowID wxID_STATIC_PLYRT_CHKOPT;
    static const wxWindowID ID_PLYRT_FRAME;
    static const wxWindowID ID_MENU_PLYRT_HELP;
    static const wxWindowID ID_BUTTON_PLYRT_MRSDEL;
    static const wxWindowID ID_BUTTON_PLYRT_OPTDEL;
    static const wxWindowID wxID_STATIC_PLYRT_NRG3;
    static const wxWindowID ID_PANEL_PLYRT_ALGNMNT;
    static const wxWindowID wxID_STATIC_PLYRT_DATTYP;
    static const wxWindowID ID_PANEL_PLYRT_TRSNOPT;
    static const wxWindowID ID_CHOICE_PLYRT_CMPMD;
    static const wxWindowID ID_CHOICE_PLYRT_MODE;
    static const wxWindowID ID_PANEL_PLYRT_TMPRTR;
    static const wxWindowID ID_TEXTCTRL_PLYRT_MRSMD;
    static const wxWindowID ID_PANEL_PLYRT_TRSNS;
    static const wxWindowID ID_TEXTCTRL_PLYRT_NRG3;
    static const wxWindowID ID_TEXTCTRL_PLYRT_FSCL;
////@end PolyrateGUI member variables
};

#endif
    // _POLYRATEGUI_H_
