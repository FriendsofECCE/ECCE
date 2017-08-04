/////////////////////////////////////////////////////////////////////////////
// Name:        WxMachineRegisterGUI.H
// Purpose:     
// Author:      Kenneth R. Swanson
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXMACHINEREGISTERGUI_H_
#define _WXMACHINEREGISTERGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxMachineRegisterGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wx/spinctrl.h"
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
#define SYMBOL_WXMACHINEREGISTERGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_WXMACHINEREGISTERGUI_TITLE _("ECCE Machine Registration")
#define SYMBOL_WXMACHINEREGISTERGUI_IDNAME ID_FRAME_MAIN
#define SYMBOL_WXMACHINEREGISTERGUI_SIZE wxDefaultSize
#define SYMBOL_WXMACHINEREGISTERGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxMachineRegisterGUI class declaration
 */

class WxMachineRegisterGUI: public ewxFrame
{    
    DECLARE_CLASS( WxMachineRegisterGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxMachineRegisterGUI( );
    WxMachineRegisterGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXMACHINEREGISTERGUI_IDNAME, const wxString& caption = SYMBOL_WXMACHINEREGISTERGUI_TITLE, const wxPoint& pos = SYMBOL_WXMACHINEREGISTERGUI_POSITION, const wxSize& size = SYMBOL_WXMACHINEREGISTERGUI_SIZE, long style = SYMBOL_WXMACHINEREGISTERGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXMACHINEREGISTERGUI_IDNAME, const wxString& caption = SYMBOL_WXMACHINEREGISTERGUI_TITLE, const wxPoint& pos = SYMBOL_WXMACHINEREGISTERGUI_POSITION, const wxSize& size = SYMBOL_WXMACHINEREGISTERGUI_SIZE, long style = SYMBOL_WXMACHINEREGISTERGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxMachineRegisterGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_FRAME_MAIN
    virtual    void mainWindowCloseCB( wxCloseEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_MACHINES
    virtual    void machinesListBoxSelectedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_FULLNAME
    virtual    void machineFullNameUpdatedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_REFNAME
    virtual    void machineRefNameUpdatedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_VENDOR
    virtual    void machineVendorTextUpdatedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_MODEL
    virtual    void machineModelTextUpdatedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_PROC
    virtual    void machineProcTextUpdatedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICEBOX_QUEUE
    virtual    void queueChoiceboxSelectedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_QUEUE_CHANGE
    virtual    void queueChangeButtonClickedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_QUEUE_REMOVE
    virtual    void queueRemoveButtonClickedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_QUEUE_CLEAR
    virtual    void queueClearButtonClickedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINE_CHANGE
    virtual    void machineChangeButtonClickedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINE_DELETE
    virtual    void machineDeleteButtonClickedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FORM_CLEAR
    virtual    void formClearButtonClickedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FORM_CLOSE
    virtual    void formCloseButtonClickedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP
    virtual    void helpButtonClickedCB( wxCommandEvent& event );

////@end WxMachineRegisterGUI event handler declarations

////@begin WxMachineRegisterGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxMachineRegisterGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxMachineRegisterGUI member variables
    static const wxWindowID ID_CHECKBOX_QUEUE_ALLOCATION;
    static const wxWindowID ID_TEXT_MACHINE_FULLNAME;
    static const wxWindowID ID_BUTTON_MACHINE_CHANGE;
    static const wxWindowID ID_SPIN_QUEUE_MAXMEMORY;
    static const wxWindowID ID_PANEL_WXMACHINEREGISTER_APPLICATIONS;
    static const wxWindowID ID_CHECKBOX_REMSHELL_SSH_PASS;
    static const wxWindowID ID_TEXT_MACHINE_REFNAME;
    static const wxWindowID ID_TEXT_QUEUE_NAME;
    static const wxWindowID ID_CHOICEBOX_QUEUE;
    static const wxWindowID ID_CHOICEBOX_QUEUE_MANAGER;
    static const wxWindowID ID_SPIN_MACHINE_NUMNODES;
    static const wxWindowID ID_PANEL;
    static const wxWindowID ID_TEXT_MACHINE_PROC;
    static const wxWindowID ID_CHECKBOX_REMSHELL_RSH;
    static const wxWindowID ID_SPIN_QUEUE_MAXWALL;
    static const wxWindowID ID_CHECKBOX_REMSHELL_SSH;
    static const wxWindowID ID_BUTTON_FORM_CLOSE;
    static const wxWindowID ID_CHECKBOX_REMSHELL_SSH_FTP;
    static const wxWindowID ID_SPIN_QUEUE_MINSCRATCH;
    static const wxWindowID ID_SPIN_MACHINE_NUMPROCS;
    static const wxWindowID ID_BUTTON_FORM_CLEAR;
    static const wxWindowID ID_PANEL_QUEUES;
    static const wxWindowID ID_PANEL_WXMACHINEREGISTER_MISCPATHS;
    static const wxWindowID ID_CHECKBOX_REMSHELL_GLOBUS;
    static const wxWindowID ID_BUTTON_QUEUE_CHANGE;
    static const wxWindowID ID_LISTBOX_MACHINES;
    static const wxWindowID ID_BUTTON_QUEUE_REMOVE;
    static const wxWindowID ID_BUTTON_QUEUE_CLEAR;
    static const wxWindowID ID_BUTTON_MACHINE_DELETE;
    static const wxWindowID ID_CHECKBOX_REMSHELL_TELNET;
    static const wxWindowID ID_TEXT_MACHINE_VENDOR;
    static const wxWindowID ID_SPIN_QUEUE_MINPROCS;
    static const wxWindowID ID_BUTTON_HELP;
    static const wxWindowID ID_TEXT_MACHINE_MODEL;
    static const wxWindowID ID_CHECKBOX_REMSHELL_GLOBUS_SSH;
    static const wxWindowID ID_SPIN_QUEUE_MAXPROCS;
    static const wxWindowID ID_FRAME_MAIN;
////@end WxMachineRegisterGUI member variables
};

#endif
    // _WXMACHINEREGISTERGUI_H_
