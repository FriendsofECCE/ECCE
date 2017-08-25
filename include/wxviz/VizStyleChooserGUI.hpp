/////////////////////////////////////////////////////////////////////////////
// Name:        VizStyleChooserGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _VIZSTYLECHOOSERGUI_H_
#define _VIZSTYLECHOOSERGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "VizStyleChooserGUI.C"
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
class wxBoxSizer;
class ewxFrame;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define SYMBOL_VIZSTYLECHOOSERGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_VIZSTYLECHOOSERGUI_TITLE _("ECCE Display Style")
#define SYMBOL_VIZSTYLECHOOSERGUI_IDNAME ID_VIZSTYLECHOOSER
#define SYMBOL_VIZSTYLECHOOSERGUI_SIZE wxDefaultSize
#define SYMBOL_VIZSTYLECHOOSERGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * VizStyleChooserGUI class declaration
 */

class VizStyleChooserGUI: public ewxFrame
{    
    DECLARE_CLASS( VizStyleChooserGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    VizStyleChooserGUI( );
    VizStyleChooserGUI( wxWindow* parent, wxWindowID id = SYMBOL_VIZSTYLECHOOSERGUI_IDNAME, const wxString& caption = SYMBOL_VIZSTYLECHOOSERGUI_TITLE, const wxPoint& pos = SYMBOL_VIZSTYLECHOOSERGUI_POSITION, const wxSize& size = SYMBOL_VIZSTYLECHOOSERGUI_SIZE, long style = SYMBOL_VIZSTYLECHOOSERGUI_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_VIZSTYLECHOOSERGUI_IDNAME, const wxString& caption = SYMBOL_VIZSTYLECHOOSERGUI_TITLE, const wxPoint& pos = SYMBOL_VIZSTYLECHOOSERGUI_POSITION, const wxSize& size = SYMBOL_VIZSTYLECHOOSERGUI_SIZE, long style = SYMBOL_VIZSTYLECHOOSERGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin VizStyleChooserGUI event handler declarations
    /// wxEVT_CLOSE_WINDOW event handler for ID_VIZSTYLECHOOSER
    virtual    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DSPLY_DSOPT
    virtual    void OnChoiceDsplyDsoptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DSPLY_CSOPT
    virtual    void OnChoiceDsplyCsoptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_BLOCKEDIT
    virtual    void OnButtonDsplyBlockeditClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TOGGLE_DSPLY_SHOW
    virtual    void OnToggleDsplyShowClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_LNWDTH
    virtual    void OnSliderDsplyLnwdthUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CYLRQ
    virtual    void OnSliderDsplyCylrqUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CYLRAD
    virtual    void OnSliderDsplyCylradUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_RESRAD
    virtual    void OnSliderDsplyResradUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_SPHRQ
    virtual    void OnSliderDsplySphrqUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_SPHSCL
    virtual    void OnSliderDsplySphsclUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_COILW
    virtual    void OnSliderDsplyCoilwUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_HLXRAD
    virtual    void OnSliderDsplyHlxradUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_HLXW
    virtual    void OnSliderDsplyHlxwUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_SHTW
    virtual    void OnSliderDsplyShtwUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_RBTHK
    virtual    void OnSliderDsplyRbthkUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CCYLRQ
    virtual    void OnSliderDsplyCcylrqUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CYLR
    virtual    void OnSliderDsplyCylrUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_TUBER
    virtual    void OnSliderDsplyTuberUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_DSPLY_GROUPS
    virtual    void OnListboxDsplyGroupsSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_UPDATE
    virtual    void OnButtonDsplyUpdateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_ADD
    virtual    void OnButtonDsplyAddClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_REMOVE
    virtual    void OnButtonDsplyRemoveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_CLOSE
    virtual    void OnButtonDsplyCloseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_SAVE
    virtual    void OnButtonDsplySaveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_RESET
    virtual    void OnButtonDsplyResetClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_HELP
    virtual    void OnButtonDsplyHelpClick( wxCommandEvent& event );

////@end VizStyleChooserGUI event handler declarations

////@begin VizStyleChooserGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end VizStyleChooserGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin VizStyleChooserGUI member variables
    wxBoxSizer* p_mainSizer;
    wxBoxSizer* p_optionSizer;
    static const wxWindowID ID_RADIOBUTTON_DSPLY_NONE;
    static const wxWindowID ID_CHOICE_DSPLY_CSOPT;
    static const wxWindowID wxID_STATIC_DSPLY_CCYLRQ;
    static const wxWindowID wxID_STATIC_DSPLY_HLXW;
    static const wxWindowID ID_PANEL_DSPLY_BLOCKSCHEME;
    static const wxWindowID ID_BUTTON_DSPLY_SAVE;
    static const wxWindowID ID_SLIDER_DSPLY_HLXRAD;
    static const wxWindowID wxID_STATIC_DSPLY_CYLRAD;
    static const wxWindowID wxID_STATIC_DSPLY_SPHSCLTXT;
    static const wxWindowID wxID_STATIC_DSPLY_SPHRQ;
    static const wxWindowID ID_BUTTON_DSPLY_HELP;
    static const wxWindowID ID_SLIDER_DSPLY_HLXW;
    static const wxWindowID wxID_STATIC_DSPLY_HLXRAD;
    static const wxWindowID ID_STATICTEXT1;
    static const wxWindowID wxID_STATIC_DSPLY_CYLRQ;
    static const wxWindowID wxID_STATIC_DSPLY_CYLRQMIN;
    static const wxWindowID ID_RADIOBUTTON_DSPLY_ROUND;
    static const wxWindowID wxID_STATIC_DSPLY_SPHSCL;
    static const wxWindowID wxID_STATIC_DSPLY_RBTHK;
    static const wxWindowID ID_SLIDER_DSPLY_RBTHK;
    static const wxWindowID wxID_STATIC_DSPLY_CYLRADTXT;
    static const wxWindowID ID_PANEL_DSP_CHOICES;
    static const wxWindowID wxID_STATIC_DSPLY_SPHSCLMAX;
    static const wxWindowID ID_PANEL_DSPLY_BLOCKCOLOR;
    static const wxWindowID wxID_STATIC_DSPLY_COILW;
    static const wxWindowID ID_BUTTON_DSPLY_UPDATE;
    static const wxWindowID ID_BUTTON_DSPLY_REMOVE;
    static const wxWindowID ID_SLIDER_DSPLY_CCYLRQ;
    static const wxWindowID ID_SLIDER_DSPLY_LNWDTH;
    static const wxWindowID ID_BUTTON_DSPLY_CLOSE;
    static const wxWindowID wxID_STATIC_DSPLY_TUBER;
    static const wxWindowID ID_CHOICE_DSPLY_BONDCYLINDER;
    static const wxWindowID ID_PANEL_DSPLY_RIBBON;
    static const wxWindowID ID_CHOICE_DSPLY_ATOMSPHERE;
    static const wxWindowID ID_SLIDER_DSPLY_CYLRQ;
    static const wxWindowID ID_SLIDER_DSPLY_RESRAD;
    static const wxWindowID ID_VIZSTYLECHOOSER;
    static const wxWindowID ID_SLIDER_DSPLY_CYLRAD;
    static const wxWindowID wxID_STATIC_DSPLY_RESRADMAX;
    static const wxWindowID wxID_STATIC_DSPLY_RESRADTXT;
    static const wxWindowID ID_PANEL_DSPLY_LNWDTH;
    static const wxWindowID ID_TOGGLE_DSPLY_SHOW;
    static const wxWindowID ID_SLIDER_DSPLY_SHTW;
    static const wxWindowID ID_SLIDER_DSPLY_CYLR;
    static const wxWindowID wxID_STATIC_DSPLY_RESRAD;
    static const wxWindowID ID_CHECKBOX_DSPLY_BLNDCL;
    static const wxWindowID ID_CHECKBOX_DSPLY_RNDRB;
    static const wxWindowID ID_PANEL_DSPLY_CYLNDR;
    static const wxWindowID ID_CHOICE_DSPLY_DSOPT;
    static const wxWindowID ID_SLIDER_DSPLY_COILW;
    static const wxWindowID wxID_STATIC_DSPLY_BLOCKCOLOR;
    static const wxWindowID ID_SLIDER_DSPLY_SPHSCL;
    static const wxWindowID wxID_STATIC_DSPLY_SPHSCLMIN;
    static const wxWindowID wxID_STATIC_DSPLY_CYLRQMAX;
    static const wxWindowID ID_SLIDER_DSPLY_TUBER;
    static const wxWindowID ID_PANEL_DSPLY_SPECTRUM;
    static const wxWindowID ID_RADIOBUTTON_DSPLY_FLAT;
    static const wxWindowID wxID_STATIC_DSPLY_LNWDTH;
    static const wxWindowID wxID_STATIC_DSPLY_RBTHKMAX;
    static const wxWindowID ID_BUTTON_DSPLY_BLOCKEDIT;
    static const wxWindowID wxID_STATIC_DSPLY_CYLRADMIN;
    static const wxWindowID ID_BUTTON_DSPLY_ADD;
    static const wxWindowID wxID_STATIC_DSPLY_RBTHKTXT;
    static const wxWindowID wxID_STATIC_DSPLY_RBTHKMIN;
    static const wxWindowID wxID_STATIC_DSPLY_CYLRQTXT;
    static const wxWindowID ID_SLIDER_DSPLY_SPHRQ;
    static const wxWindowID ID_BUTTON_DSPLY_RESET;
    static const wxWindowID ID_PANEL_DSPLY_CNTRLS;
    static const wxWindowID wxID_STATIC_DSPLY_SHTW;
    static const wxWindowID ID_PANEL_DSPLY_CRTN;
    static const wxWindowID wxID_STATIC_DSPLY_CYLR;
    static const wxWindowID ID_PANEL_DSPLY_SWOTH;
    static const wxWindowID wxID_STATIC_DSPLY_RESRADMIN;
    static const wxWindowID ID_PANEL_DSPLY_SPHERE;
    static const wxWindowID ID_LISTBOX_DSPLY_GROUPS;
    static const wxWindowID wxID_STATIC_DSPLY_CYLRADMAX;
////@end VizStyleChooserGUI member variables
};

#endif
    // _VIZSTYLECHOOSERGUI_H_
