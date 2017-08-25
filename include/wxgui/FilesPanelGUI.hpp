/////////////////////////////////////////////////////////////////////////////
// Name:        FilesPanelGUI.H
// Purpose:     
// Author:      FilesPanelGUI
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _FILESPANELGUI_H_
#define _FILESPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "FilesPanelGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/notebook.h"
#include "wx/statline.h"
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
#define SYMBOL_FILESPANELGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxSUNKEN_BORDER
#define SYMBOL_FILESPANELGUI_TITLE _("FilesPanelGUI")
#define SYMBOL_FILESPANELGUI_IDNAME ID_PANEL_FILE
#define SYMBOL_FILESPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_FILESPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * FilesPanelGUI class declaration
 */

class FilesPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( FilesPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    FilesPanelGUI( );
    FilesPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_FILESPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_FILESPANELGUI_POSITION, const wxSize& size = SYMBOL_FILESPANELGUI_SIZE, long style = SYMBOL_FILESPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_FILESPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_FILESPANELGUI_POSITION, const wxSize& size = SYMBOL_FILESPANELGUI_SIZE, long style = SYMBOL_FILESPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin FilesPanelGUI event handler declarations
    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_FILE_TRJSTR
    virtual    void OnChoiceFileTrjstrSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_USEBTC
    virtual    void OnCheckboxFileUsebtcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_BTCSZ
    virtual    void OnTextctrlFileBtcszEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTCRD
    virtual    void OnCheckboxFileSltcrdClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_CSUFRQ
    virtual    void OnTextctrlFileCsufrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVCRD
    virtual    void OnCheckboxFileSlvcrdClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_CSVFRQ
    virtual    void OnTextctrlFileCsvfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTVEL
    virtual    void OnCheckboxFileSltvelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_VSUFRQ
    virtual    void OnTextctrlFileVsufrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVVEL
    virtual    void OnCheckboxFileSlvvelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_VSVFRQ
    virtual    void OnTextctrlFileVsvfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLTFRC
    virtual    void OnCheckboxFileSltfrcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_FSUFRQ
    virtual    void OnTextctrlFileFsufrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVFRC
    virtual    void OnCheckboxFileSlvfrcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_FSVFRQ
    virtual    void OnTextctrlFileFsvfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_PRPFRQ
    virtual    void OnTextctrlFilePrpfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_USEAVG
    virtual    void OnCheckboxFileUseavgClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_RSTFRQ
    virtual    void OnTextctrlFileRstfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_KEEP
    virtual    void OnCheckboxFileKeepClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_PRTMD
    virtual    void OnCheckboxFilePrtmdClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_MDFREQ
    virtual    void OnTextctrlFileMdfreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_ESOL
    virtual    void OnCheckboxFileEsolClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_EBOND
    virtual    void OnCheckboxFileEbondClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_STAT
    virtual    void OnCheckboxFileStatClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_STFREQ
    virtual    void OnTextctrlFileStfreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SUBOND
    virtual    void OnCheckboxFileSubondClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_SEFREQ
    virtual    void OnTextctrlFileSefreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_FRC
    virtual    void OnCheckboxFileFrcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_SFFREQ
    virtual    void OnTextctrlFileSffreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_PMF
    virtual    void OnCheckboxFilePmfClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_FILE_PMFFREQ
    virtual    void OnTextctrlFilePmffreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_TOP
    virtual    void OnCheckboxFileTopClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_NBOND
    virtual    void OnCheckboxFileNbondClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLVPAR
    virtual    void OnCheckboxFileSlvparClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_FILE_SLUPAR
    virtual    void OnCheckboxFileSluparClick( wxCommandEvent& event );

////@end FilesPanelGUI event handler declarations

////@begin FilesPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end FilesPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin FilesPanelGUI member variables
    wxPanel* p_trajectoryPanel;
    wxBoxSizer* p_trajectoryVSizer;
    wxPanel* p_coordPanel;
    wxPanel* p_velocPanel;
    wxPanel* p_forcePanel;
    wxPanel* p_propPanel;
    wxPanel* p_restartPanel;
    static const wxWindowID ID_TEXTCTRL_FILE_PRPFRQ;
    static const wxWindowID ID_CHECKBOX_FILE_SLVCRD;
    static const wxWindowID ID_PANEL_FILE_TRJOPT;
    static const wxWindowID ID_TEXTCTRL_FILE_RSTFRQ;
    static const wxWindowID ID_CHECKBOX_FILE_STAT;
    static const wxWindowID wxID_STATIC_FILE_FSUFRQ;
    static const wxWindowID ID_CHECKBOX_FILE_SLTCRD;
    static const wxWindowID wxID_STATIC_FILE_VSUFRQ;
    static const wxWindowID ID_TEXTCTRL_FILE_VSUFRQ;
    static const wxWindowID ID_CHECKBOX_FILE_SUBOND;
    static const wxWindowID ID_PANEL_FILE;
    static const wxWindowID ID_CHECKBOX_FILE_USEAVG;
    static const wxWindowID ID_CHECKBOX_FILE_TOP;
    static const wxWindowID ID_TEXTCTRL_FILE_STFREQ;
    static const wxWindowID ID_PANEL_FILE_WRTPRP;
    static const wxWindowID wxID_STATIC_FILE_VSVFRQ;
    static const wxWindowID ID_CHECKBOX_FILE_NBOND;
    static const wxWindowID ID_PANEL_FILE_WRTFRC;
    static const wxWindowID wxID_STATIC_FILE_SFFREQ;
    static const wxWindowID ID_CHECKBOX_FILE_SLVVEL;
    static const wxWindowID ID_TEXTCTRL_FILE_CSVFRQ;
    static const wxWindowID ID_TEXTCTRL_FILE_BTCSZ;
    static const wxWindowID ID_TEXTCTRL_FILE_CSUFRQ;
    static const wxWindowID ID_PANEL_FILE_WRTVEL;
    static const wxWindowID ID_CHECKBOX_FILE_SLVPAR;
    static const wxWindowID ID_TEXTCTRL_FILE_MDFREQ;
    static const wxWindowID ID_PANEL_FILE_WRTRST;
    static const wxWindowID ID_CHECKBOX_FILE_KEEP;
    static const wxWindowID ID_CHECKBOX_FILE_SLTVEL;
    static const wxWindowID ID_CHECKBOX_FILE_EBOND;
    static const wxWindowID ID_CHECKBOX_FILE_SLTFRC;
    static const wxWindowID ID_PANEL_FILE_MDOPT;
    static const wxWindowID ID_TEXTCTRL_FILE_FSUFRQ;
    static const wxWindowID ID_CHECKBOX_FILE_SLVFRC;
    static const wxWindowID wxID_STATIC_FILE_SEFREQ;
    static const wxWindowID ID_NOTEBOOK;
    static const wxWindowID wxID_STATIC_FILE_FSVFRQ;
    static const wxWindowID ID_TEXTCTRL_FILE_SEFREQ;
    static const wxWindowID ID_CHECKBOX_FILE_USEBTC;
    static const wxWindowID ID_TEXTCTRL_FILE_FSVFRQ;
    static const wxWindowID ID_PANEL_FILE_WRTOUT;
    static const wxWindowID ID_CHECKBOX_FILE_ESOL;
    static const wxWindowID ID_PANEL_FILE_WRTTRJ;
    static const wxWindowID ID_PANEL_FILE_TOPOPT;
    static const wxWindowID wxID_STATIC_FILE_CSUFRQ;
    static const wxWindowID ID_TEXTCTRL_FILE_VSVFRQ;
    static const wxWindowID wxID_STATIC_FILE_MDFREQ;
    static const wxWindowID ID_CHECKBOX_FILE_SLUPAR;
    static const wxWindowID ID_TEXTCTRL_FILE_PMFFREQ;
    static const wxWindowID ID_CHECKBOX_FILE_PRTMD;
    static const wxWindowID ID_TEXTCTRL_FILE_SFFREQ;
    static const wxWindowID wxID_STATIC_FILE_CSVFRQ;
    static const wxWindowID ID_CHOICE_FILE_TRJSTR;
    static const wxWindowID wxID_STATIC_FILE_BTCSZ;
    static const wxWindowID wxID_STATIC_FILE_PMFFREQ;
    static const wxWindowID ID_CHECKBOX_FILE_PMF;
    static const wxWindowID ID_CHECKBOX_FILE_FRC;
    static const wxWindowID wxID_STATIC_FILE_STFREQ;
    static const wxWindowID ID_PANEL_FILE_WRTCRD;
////@end FilesPanelGUI member variables
};

#endif
    // _FILESPANELGUI_H_
