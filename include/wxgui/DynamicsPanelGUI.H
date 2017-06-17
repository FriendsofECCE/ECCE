/////////////////////////////////////////////////////////////////////////////
// Name:        DynamicsPanelGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _DYNAMICSPANELGUI_H_
#define _DYNAMICSPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "DynamicsPanelGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/notebook.h"
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
#define SYMBOL_DYNAMICSPANELGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxSUNKEN_BORDER
#define SYMBOL_DYNAMICSPANELGUI_TITLE _("DynamicsPanelGUI")
#define SYMBOL_DYNAMICSPANELGUI_IDNAME ID_DIALOG_DYNMC
#define SYMBOL_DYNAMICSPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_DYNAMICSPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * DynamicsPanelGUI class declaration
 */

class DynamicsPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( DynamicsPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    DynamicsPanelGUI( );
    DynamicsPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_DYNAMICSPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_DYNAMICSPANELGUI_POSITION, const wxSize& size = SYMBOL_DYNAMICSPANELGUI_SIZE, long style = SYMBOL_DYNAMICSPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_DYNAMICSPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_DYNAMICSPANELGUI_POSITION, const wxSize& size = SYMBOL_DYNAMICSPANELGUI_SIZE, long style = SYMBOL_DYNAMICSPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin DynamicsPanelGUI event handler declarations
    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_DATSTP
    virtual    void OnTextctrlDynmcDatstpEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RESUM
    virtual    void OnCheckboxDynmcResumClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_INTALG
    virtual    void OnChoiceDynmcIntalgSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_EQSTP
    virtual    void OnTextctrlDynmcEqstpEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_TSTEP
    virtual    void OnTextctrlDynmcTstepEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_ITIME
    virtual    void OnTextctrlDynmcItimeEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_CNTSYS
    virtual    void OnCheckboxDynmcCntsysClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CNTFRQ
    virtual    void OnTextctrlDynmcCntfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_ALLCRD
    virtual    void OnRadiobuttonDynmcAllcrdSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_XYONLY
    virtual    void OnRadiobuttonDynmcXyonlySelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_ZONLY
    virtual    void OnRadiobuttonDynmcZonlySelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USEFRAC
    virtual    void OnCheckboxDynmcUsefracClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC1
    virtual    void OnTextctrlDynmcFrac1Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC2
    virtual    void OnTextctrlDynmcFrac2Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC3
    virtual    void OnTextctrlDynmcFrac3Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC4
    virtual    void OnTextctrlDynmcFrac4Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC5
    virtual    void OnTextctrlDynmcFrac5Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RMCM
    virtual    void OnCheckboxDynmcRmcmClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CMFRQ
    virtual    void OnTextctrlDynmcCmfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USENVT
    virtual    void OnCheckboxDynmcUsenvtClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_NVTTMP
    virtual    void OnTextctrlDynmcNvttmpEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_SNGLRX
    virtual    void OnRadiobuttonDynmcSnglrxSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_SPLTRX
    virtual    void OnRadiobuttonDynmcSpltrxSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_SLVTIM
    virtual    void OnTextctrlDynmcSlvtimEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_SLUTIM
    virtual    void OnTextctrlDynmcSlutimEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USENPT
    virtual    void OnCheckboxDynmcUsenptClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_NPTPRS
    virtual    void OnTextctrlDynmcNptprsEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_PRSTIM
    virtual    void OnTextctrlDynmcPrstimEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CMPRSS
    virtual    void OnTextctrlDynmcCmprssEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_VOLALG
    virtual    void OnChoiceDynmcVolalgSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RTHRM
    virtual    void OnCheckboxDynmcRthrmClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_VELFRQ
    virtual    void OnTextctrlDynmcVelfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_RTHTMP
    virtual    void OnTextctrlDynmcRthtmpEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_VFRAC
    virtual    void OnTextctrlDynmcVfracEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_RTHOPT
    virtual    void OnChoiceDynmcRthoptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USEANL
    virtual    void OnCheckboxDynmcUseanlClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_ANLTMP
    virtual    void OnTextctrlDynmcAnltmpEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_IANL
    virtual    void OnTextctrlDynmcIanlEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FANL
    virtual    void OnTextctrlDynmcFanlEnter( wxCommandEvent& event );

////@end DynamicsPanelGUI event handler declarations

////@begin DynamicsPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end DynamicsPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin DynamicsPanelGUI member variables
    wxBoxSizer* p_dynamicsVSizer;
    wxPanel* p_recenterPanel;
    wxPanel* p_NVTPanel;
    wxPanel* p_NPTPanel;
    wxPanel* p_rethermalizationPanel;
    wxPanel* p_annealingPanel;
    static const wxWindowID ID_TEXTCTRL_DYNMC_FRAC3;
    static const wxWindowID ID_CHOICE_DYNMC_VOLALG;
    static const wxWindowID ID_TEXTCTRL_DYNMC_SLVTIM;
    static const wxWindowID ID_PANEL_DYNMC_NPT;
    static const wxWindowID ID_PANEL_DYNMC_RCNTR;
    static const wxWindowID ID_RADIOBUTTON_DYNMC_ZONLY;
    static const wxWindowID ID_RADIOBUTTON_DYNMC_ALLCRD;
    static const wxWindowID ID_CHECKBOX_DYNMC_RTHRM;
    static const wxWindowID wxID_STATIC_DYNMC_PRSTIM;
    static const wxWindowID ID_PANEL_DYNMC_ALL;
    static const wxWindowID ID_TEXTCTRL_DYNMC_DATSTP;
    static const wxWindowID ID_DIALOG_DYNMC;
    static const wxWindowID ID_CHECKBOX_DYNMC_RMCM;
    static const wxWindowID ID_TEXTCTRL_DYNMC_IANL;
    static const wxWindowID ID_PANEL_DYNMC_RMCM;
    static const wxWindowID wxID_STATIC_DYNMC_SLVTIM;
    static const wxWindowID ID_TEXTCTRL_DYNMC_FANL;
    static const wxWindowID ID_TEXTCTRL_DYNMC_ITIME;
    static const wxWindowID ID_PANEL_DYNMC_TEMP;
    static const wxWindowID wxID_STATIC_DYNMC_SLUTXT;
    static const wxWindowID wxID_STATIC_DYNMC_IANL;
    static const wxWindowID wxID_STATIC_DYNMC_ANLTMP;
    static const wxWindowID ID_TEXTCTRL_DYNMC_FRAC4;
    static const wxWindowID wxID_STATIC_DYNMC_RTHTMP;
    static const wxWindowID ID_TEXTCTRL_DYNMC_SLUTIM;
    static const wxWindowID ID_RADIOBUTTON_DYNMC_SPLTRX;
    static const wxWindowID ID_TEXTCTRL_DYNMC_CNTFRQ;
    static const wxWindowID ID_RADIOBUTTON_DYNMC_XYONLY;
    static const wxWindowID wxID_STATIC_DYNMC_ITIME;
    static const wxWindowID ID_PANEL_DYNMC_SLUTIM;
    static const wxWindowID ID_TEXTCTRL_DYNMC_EQSTP;
    static const wxWindowID wxID_STATIC_DYNMC_NVTTMP;
    static const wxWindowID wxID_STATIC_DYNMC_CMPRSS;
    static const wxWindowID ID_CHOICE_DYNMC_INTALG;
    static const wxWindowID ID_CHECKBOX_DYNMC_USENVT;
    static const wxWindowID ID_PANEL_DYNMC_FRACS;
    static const wxWindowID ID_CHECKBOX_DYNMC_CNTSYS;
    static const wxWindowID ID_PANEL_DYNMC_CNTR;
    static const wxWindowID ID_TEXTCTRL_DYNMC_FRAC5;
    static const wxWindowID ID_PANEL_DYNMC_RTHRM;
    static const wxWindowID wxID_STATIC_DYNMC_TSTEP;
    static const wxWindowID ID_PANEL_DYNMC_RETHRM;
    static const wxWindowID ID_TEXTCTRL_DYNMC_NVTTMP;
    static const wxWindowID wxID_STATIC_DYNMC_NPTPRS;
    static const wxWindowID ID_TEXTCTRL_DYNMC_TSTEP;
    static const wxWindowID ID_PANEL_DYNMC_ANLOPT;
    static const wxWindowID ID_CHECKBOX_DYNMC_USEFRAC;
    static const wxWindowID ID_NOTEBOOK_DYNMC;
    static const wxWindowID ID_TEXTCTRL_DYNMC_FRAC2;
    static const wxWindowID ID_TEXTCTRL_DYNMC_NPTPRS;
    static const wxWindowID ID_TEXTCTRL_DYNMC_CMPRSS;
    static const wxWindowID ID_TEXTCTRL_DYNMC_VELFRQ;
    static const wxWindowID ID_PANEL_DYNMC_NVT;
    static const wxWindowID ID_TEXTCTRL_DYNMC_PRSTIM;
    static const wxWindowID ID_TEXTCTRL_DYNMC_RTHTMP;
    static const wxWindowID ID_TEXTCTRL_DYNMC_VFRAC;
    static const wxWindowID ID_TEXTCTRL_DYNMC_CMFRQ;
    static const wxWindowID wxID_STATIC_DYNMC_USLUTIM;
    static const wxWindowID ID_CHECKBOX_DYNMC_USENPT;
    static const wxWindowID ID_CHECKBOX_DYNMC_USEANL;
    static const wxWindowID ID_PANEL_DYNMC_PRSSR;
    static const wxWindowID ID_RADIOBUTTON_DYNMC_SNGLRX;
    static const wxWindowID ID_CHOICE_DYNMC_RTHOPT;
    static const wxWindowID wxID_STATIC_DYNMC_FANL;
    static const wxWindowID ID_PANEL_DYNMC_ANNEAL;
    static const wxWindowID ID_TEXTCTRL_DYNMC_ANLTMP;
    static const wxWindowID ID_CHECKBOX_DYNMC_RESUM;
    static const wxWindowID ID_TEXTCTRL_DYNMC_FRAC1;
////@end DynamicsPanelGUI member variables
};

#endif
    // _DYNAMICSPANELGUI_H_
