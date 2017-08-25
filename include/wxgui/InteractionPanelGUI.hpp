/////////////////////////////////////////////////////////////////////////////
// Name:        InteractionPanelGUI.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _INTERACTIONPANELGUI_H_
#define _INTERACTIONPANELGUI_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "InteractionPanelGUI.C"
#endif

/*!
 * Includes
 */

////@begin includes
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
#define SYMBOL_INTERACTIONPANELGUI_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxSUNKEN_BORDER
#define SYMBOL_INTERACTIONPANELGUI_TITLE _("InteractionPanelGUI")
#define SYMBOL_INTERACTIONPANELGUI_IDNAME ID_DIALOG_INTRCT
#define SYMBOL_INTERACTIONPANELGUI_SIZE wxSize(400, 300)
#define SYMBOL_INTERACTIONPANELGUI_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * InteractionPanelGUI class declaration
 */

class InteractionPanelGUI: public ewxPanel
{    
    DECLARE_DYNAMIC_CLASS( InteractionPanelGUI )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    InteractionPanelGUI( );
    InteractionPanelGUI( wxWindow* parent, wxWindowID id = SYMBOL_INTERACTIONPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_INTERACTIONPANELGUI_POSITION, const wxSize& size = SYMBOL_INTERACTIONPANELGUI_SIZE, long style = SYMBOL_INTERACTIONPANELGUI_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_INTERACTIONPANELGUI_IDNAME, const wxPoint& pos = SYMBOL_INTERACTIONPANELGUI_POSITION, const wxSize& size = SYMBOL_INTERACTIONPANELGUI_SIZE, long style = SYMBOL_INTERACTIONPANELGUI_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin InteractionPanelGUI event handler declarations
    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SET1
    virtual    void OnRadiobuttonIntrctSet1Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SET2
    virtual    void OnRadiobuttonIntrctSet2Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SET3
    virtual    void OnRadiobuttonIntrctSet3Selected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_PERT1
    virtual    void OnCheckboxIntrctPert1Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_PERT2
    virtual    void OnCheckboxIntrctPert2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_INTRCT_ES_OPT
    virtual    void OnChoiceIntrctEsOptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_GRID
    virtual    void OnTextctrlIntrctGridEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_EWTOL
    virtual    void OnTextctrlIntrctEwtolEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_ESPLINE
    virtual    void OnTextctrlIntrctEsplineEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_QGRID
    virtual    void OnCheckboxIntrctQgridClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TOGGLE_INTRCT_ADVFFT
    virtual    void OnToggleIntrctAdvfftClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_NFFT
    virtual    void OnTextctrlIntrctNfftEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_INTRCT_FFT_SRC
    virtual    void OnChoiceIntrctFftSrcSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_MAXIT
    virtual    void OnTextctrlIntrctMaxitEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_PTOL
    virtual    void OnTextctrlIntrctPtolEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_FSTORD
    virtual    void OnCheckboxIntrctFstordClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SROPT
    virtual    void OnRadiobuttonIntrctSroptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_DROPT
    virtual    void OnRadiobuttonIntrctDroptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_SCUT
    virtual    void OnTextctrlIntrctScutEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_SFREQ
    virtual    void OnTextctrlIntrctSfreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_LCUT
    virtual    void OnTextctrlIntrctLcutEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_LFREQ
    virtual    void OnTextctrlIntrctLfreqEnter( wxCommandEvent& event );

////@end InteractionPanelGUI event handler declarations

////@begin InteractionPanelGUI member function declarations
    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end InteractionPanelGUI member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin InteractionPanelGUI member variables
    wxBoxSizer* p_mainVSizer;
    wxPanel* p_parameterPanel;
    wxPanel* p_ewaldPanel;
    wxBoxSizer* p_EwaldSizer;
    wxBoxSizer* p_FFTVSizer;
    wxPanel* p_fftPanel;
    wxPanel* p_polarizationPanel;
    wxPanel* p_cutoffPanel;
    wxBoxSizer* p_cutoffVSizer;
    wxBoxSizer* p_cutoffHSizer;
    wxPanel* p_lcutoffPanel;
    static const wxWindowID ID_TEXTCTRL_INTRCT_SCUT;
    static const wxWindowID ID_PANEL_INTRCT_PARAMETER;
    static const wxWindowID ID_CHECKBOX_INTRCT_FSTORD;
    static const wxWindowID ID_TOGGLE_INTRCT_ADVFFT;
    static const wxWindowID ID_CHOICE_INTRCT_FFT_SRC;
    static const wxWindowID ID_CHOICE_INTRCT_ES_OPT;
    static const wxWindowID ID_DIALOG_INTRCT;
    static const wxWindowID ID_CHECKBOX_INTRCT_PERT2;
    static const wxWindowID ID_RADIOBUTTON_INTRCT_SET2;
    static const wxWindowID ID_RADIOBUTTON_INTRCT_SET1;
    static const wxWindowID wxID_STATIC_INTRCT_ULCUT;
    static const wxWindowID wxID_STATIC_INTRCT_ESPLINE;
    static const wxWindowID ID_PANEL_INTRCT_FFTOPT;
    static const wxWindowID ID_CHECKBOX_INTRCT_QGRID;
    static const wxWindowID ID_TEXTCTRL_INTRCT_GRID;
    static const wxWindowID ID_STATICTEXT1;
    static const wxWindowID ID_TEXTCTRL_INTRCT_LCUT;
    static const wxWindowID wxID_STATIC_INTRCT_NFFT;
    static const wxWindowID ID_TEXTCTRL_INTRCT_EWTOL;
    static const wxWindowID ID_TEXTCTRL_INTRCT_ESPLINE;
    static const wxWindowID wxID_STATIC_INTRCT_LFREQ;
    static const wxWindowID ID_TEXTCTRL_INTRCT_NFFT;
    static const wxWindowID ID_CHECKBOX_INTRCT_PERT1;
    static const wxWindowID ID_RADIOBUTTON_INTRCT_SROPT;
    static const wxWindowID ID_TEXTCTRL_INTRCT_LFREQ;
    static const wxWindowID ID_PANEL_INTRCT_CUTOFF;
    static const wxWindowID ID_RADIOBUTTON_INTRCT_DROPT;
    static const wxWindowID wxID_STATIC_INTRCT_ECNV;
    static const wxWindowID ID_PANEL_INTRCT_POLARIZATION;
    static const wxWindowID wxID_STATIC_INTRCT_LCUT;
    static const wxWindowID ID_PANEL_INTRCT_EWALD;
    static const wxWindowID ID_TEXTCTRL_INTRCT_PTOL;
    static const wxWindowID wxID_STATIC_INTRCT_SCUT;
    static const wxWindowID ID_TEXTCTRL_INTRCT_SFREQ;
    static const wxWindowID ID_PANEL_INTRCT_LCUT;
    static const wxWindowID ID_RADIOBUTTON_INTRCT_SET3;
    static const wxWindowID ID_TEXTCTRL_INTRCT_MAXIT;
    static const wxWindowID wxID_STATIC_INTRCT_GRID;
    static const wxWindowID wxID_STATIC_INTRCT_SFREQ;
////@end InteractionPanelGUI member variables
};

#endif
    // _INTERACTIONPANELGUI_H_
