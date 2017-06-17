/////////////////////////////////////////////////////////////////////////////
// Name:        WxLauncherGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXLAUNCHERGUI_H_
#define _WXLAUNCHERGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxLauncherGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wxgui/WxParameterEdit.H"
#include "wxgui/WxTimeSpanEdit.H"
#include "wx/statline.h"
#include "wxgui/WxFeedback.H"
#include "wxgui/ewxFrame.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class ewxEVT_TIMESPAN_VALUE_CHANGED;
class ewxFrame;
class ewxEVT_PARAMETER_VALUE_CHANGED;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_WXLAUNCHERGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_WXLAUNCHERGUI_TITLE _("ECCE Launcher")
#define SYMBOL_WXLAUNCHERGUI_IDNAME ID_FRAME_WXLAUNCHER
#define SYMBOL_WXLAUNCHERGUI_SIZE wxDefaultSize
#define SYMBOL_WXLAUNCHERGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WxLauncherGUI class declaration
 */

class WxLauncherGUI: public ewxFrame
{    
    DECLARE_CLASS( WxLauncherGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WxLauncherGUI( );
    WxLauncherGUI( wxWindow* parent, wxWindowID id = SYMBOL_WXLAUNCHERGUI_IDNAME, const wxString& caption = SYMBOL_WXLAUNCHERGUI_TITLE, const wxPoint& pos = SYMBOL_WXLAUNCHERGUI_POSITION, const wxSize& size = SYMBOL_WXLAUNCHERGUI_SIZE, long style = SYMBOL_WXLAUNCHERGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_WXLAUNCHERGUI_IDNAME, const wxString& caption = SYMBOL_WXLAUNCHERGUI_TITLE, const wxPoint& pos = SYMBOL_WXLAUNCHERGUI_POSITION, const wxSize& size = SYMBOL_WXLAUNCHERGUI_SIZE, long style = SYMBOL_WXLAUNCHERGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WxLauncherGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_FRAME_WXLAUNCHER
    virtual    void mainWindowCloseCB( wxCloseEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_MACHINES
    virtual    void machinesChoiceSelectedCB( wxCommandEvent& event );

    /// ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_WKSTNPROCS
    virtual    void wkstnProcsParamValueChangedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_PRIORITY
    virtual    void priorityChoiceSelectedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_ALLOCACCT
    virtual    void allocationAccountTextCtrlUpdateCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_QUEUE
    virtual    void queueChoiceSelectedCB( wxCommandEvent& event );

    /// ewxEVT_TIMESPAN_VALUE_CHANGED event handler for ID_TIMESPANEDIT_WXLAUNCHER_WALLTIME
    virtual    void wallTimeLimitChangedCB( wxCommandEvent& event );

    /// ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_BATCHPROCS
    virtual    void batchProcsParamValueChangedCB( wxCommandEvent& event );

    /// ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_BATCHNODES
    virtual    void batchNodesParamValueChangedCB( wxCommandEvent& event );

    /// ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_MINSCRATCH
    virtual    void minScratchParamValueChangedCB( wxCommandEvent& event );

    /// ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_MAXMEMORY
    virtual    void maxMemoryParamValueChangedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_USERNAME
    virtual    void usernameTextCtrlUpdateCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_PASSWORD1
    virtual    void password1TextCtrlUpdateCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_PASSWORD2
    virtual    void password2TextCtrlUpdateCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_REMSHELL
    virtual    void remshellChoiceSelectedCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXLAUNCHER_SHELLOPEN
    virtual    void shellOpenButtonClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_CALCDIR
    virtual    void calculationDirectoryTextCtrlUpdateCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_SCRATCHDIR
    virtual    void scratchDirectoryTextCtrlUpdateCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXLAUNCHER_LAUNCH
    virtual    void launchButtonClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_MACHREGISTER
    virtual    void machRegisterMenuitemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_STAGELAUNCH
    virtual    void stageLaunchMenuItemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH
    virtual    void finishLaunchMenuItemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
    virtual    void quitMenuItemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
    virtual    void helpLauncherMenuitemClickCB( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_HELPSUPPORT
    virtual    void helpSupportMenuitemClickCB( wxCommandEvent& event );

////@end WxLauncherGUI event handler declarations

////@begin WxLauncherGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WxLauncherGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WxLauncherGUI member variables
    static const wxWindowID ID_PANEL_WXLAUNCHER_MAXWALLTIME;
    static const wxWindowID ID_PANEL_WXLAUNCHER_MACHINE;
    static const wxWindowID ID_TEXTCTRL_WXLAUNCHER_PASSWORD2;
    static const wxWindowID ID_PANEL_WXLAUNCHER_BATCHNODES;
    static const wxWindowID ID_STATIC_WXLAUNCHER_MACHINENAME;
    static const wxWindowID ID_BUTTON_WXLAUNCHER_SHELLOPEN;
    static const wxWindowID ID_STATIC_WXLAUNCHER_MAXWALLTIME;
    static const wxWindowID ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH;
    static const wxWindowID ID_MENUITEM_WXLAUNCHER_MACHREGISTER;
    static const wxWindowID ID_PANEL_WXLAUNCHER_6;
    static const wxWindowID ID_PANEL_WXLAUNCHER_MINSCRATCH;
    static const wxWindowID ID_PANEL_WXLAUNCHER_BATCH3;
    static const wxWindowID ID_PANEL_WXLAUNCHER_BATCHPROCSLABELS;
    static const wxWindowID ID_PANEL_WXLAUNCHER_PASSWORD1;
    static const wxWindowID ID_PANEL_WXLAUNCHER_BATCHPROCS;
    static const wxWindowID ID_STATIC_WXLAUNCHER_QUEUELABELLEFT;
    static const wxWindowID ID_TEXTCTRL_WXLAUNCHER_USERNAME;
    static const wxWindowID ID_TIMESPANEDIT_WXLAUNCHER_WALLTIME;
    static const wxWindowID ID_PANEL_WXLAUNCHER_QUEUE;
    static const wxWindowID ID_FRAME_WXLAUNCHER;
    static const wxWindowID ID_PANEL_WXLAUNCHER_SCRATCHDIR;
    static const wxWindowID ID_CHOICE_WXLAUNCHER_MACHINES;
    static const wxWindowID ID_STATIC_WXLAUNCHER_NOMACHINE;
    static const wxWindowID ID_PANEL_WXLAUNCHER_ALLOCACCT;
    static const wxWindowID ID_PARAMEDIT_WXLAUNCHER_BATCHPROCS;
    static const wxWindowID ID_PARAMEDIT_WXLAUNCHER_WKSTNPROCS;
    static const wxWindowID ID_TEXTCTRL_WXLAUNCHER_CALCDIR;
    static const wxWindowID ID_PANEL;
    static const wxWindowID ID_PANEL_WXLAUNCHER_WRKSTSPCFC;
    static const wxWindowID ID_TEXTCTRL_WXLAUNCHER_SCRATCHDIR;
    static const wxWindowID ID_STATICTEXT;
    static const wxWindowID ID_CHOICE_WXLAUNCHER_PRIORITY;
    static const wxWindowID ID_PANEL_WXLAUNCHER_BATCH0;
    static const wxWindowID ID_PANEL_WXLAUNCHER_BATCHPROCSMAX;
    static const wxWindowID ID_CHOICE_WXLAUNCHER_REMSHELL;
    static const wxWindowID ID_PANEL_WXLAUNCHER_DIRECTORIES;
    static const wxWindowID ID_TEXTCTRL_WXLAUNCHER_ALLOCACCT;
    static const wxWindowID ID_MENUITEM_WXLAUNCHER_STAGELAUNCH;
    static const wxWindowID ID_FEEDBACK_WXLAUNCHER_MESSAGES;
    static const wxWindowID ID_PANEL_WXLAUNCHER_BATCHSPCFC;
    static const wxWindowID ID_PANEL_WXLAUNCHER_WKSTNPROCS;
    static const wxWindowID ID_PARAMEDIT_WXLAUNCHER_BATCHNODES;
    static const wxWindowID ID_STATIC_WXLAUNCHER_QUEUELABELRIGHT;
    static const wxWindowID ID_PANEL_WXLAUNCHER_WALLTIME;
    static const wxWindowID ID_STATIC_WXLAUNCHER_USERNAMELABEL;
    static const wxWindowID ID_PANEL_WXLAUNCHER_PRIORITY;
    static const wxWindowID ID_TEXTCTRL_WXLAUNCHER_PASSWORD1;
    static const wxWindowID ID_STATIC_WXLAUNCHER_REQUIREDFIELDSKEY;
    static const wxWindowID ID_STATIC_WXLAUNCHER_PASSWORD1LABEL;
    static const wxWindowID ID_PARAMEDIT_WXLAUNCHER_MAXMEMORY;
    static const wxWindowID ID_PANEL_WXLAUNCHER_BATCH2;
    static const wxWindowID ID_PANEL_WXLAUNCHER_USERNAME;
    static const wxWindowID ID_PANEL_WXLAUNCHER_PASSWORD2;
    static const wxWindowID ID_STATIC_WXLAUNCHER_PASSWORD2LABEL;
    static const wxWindowID ID_PANEL_WXLAUNCHER_CALCDIR;
    static const wxWindowID ID_PARAMEDIT_WXLAUNCHER_MINSCRATCH;
    static const wxWindowID ID_CHOICE_WXLAUNCHER_QUEUE;
    static const wxWindowID ID_BUTTON_WXLAUNCHER_LAUNCH;
    static const wxWindowID ID_PANEL_WXLAUNCHER_MAXMEMORY;
    static const wxWindowID ID_MENUITEM_WXLAUNCHER_HELPSUPPORT;
    static const wxWindowID ID_PANEL_WXLAUNCHER_REMSHELL;
    static const wxWindowID ID_PANEL1;
    static const wxWindowID ID_STATIC_WXLAUNCHER_BATCHPROCSMAX;
////@end WxLauncherGUI member variables
};

#endif
    // _WXLAUNCHERGUI_H_
