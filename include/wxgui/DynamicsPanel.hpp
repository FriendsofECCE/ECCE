#ifndef _DYNAMICSPANEL_H_
#define _DYNAMICSPANEL_H_

/*!
 * Includes
 */

#include "wxgui/DynamicsPanelGUI.H"

class DynamicsModel;
class MDPanelHelper;

/**
 * GUI implementation of DynamicsPanel in MD editors
 */
class DynamicsPanel: public DynamicsPanelGUI
{    

public:
    /// Constructors
    DynamicsPanel(MDPanelHelper *helper );
    DynamicsPanel( MDPanelHelper *helper, wxWindow* parent,
	           wxWindowID id = SYMBOL_DYNAMICSPANELGUI_IDNAME,
		   const wxPoint& pos = SYMBOL_DYNAMICSPANELGUI_POSITION,
		   const wxSize& size = SYMBOL_DYNAMICSPANELGUI_SIZE,
		   long style = SYMBOL_DYNAMICSPANELGUI_STYLE );

    virtual ~DynamicsPanel( );

    void setDynamicsModel(DynamicsModel *model);
    DynamicsModel* getDynamicsModel();
    void refreshGUI();
    void initializeGUI();
    void showDynamicsPanel();
    void checkResume();

/**
 * DynamicsPanel event handlers
 */
    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_DATSTP
    virtual void OnTextctrlDynmcDatstpEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RESUM
    virtual    void OnCheckboxDynmcResumClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_INTALG
    virtual void OnChoiceDynmcIntalgSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_EQSTP
    virtual void OnTextctrlDynmcEqstpEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_TSTEP
    virtual void OnTextctrlDynmcTstepEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_ITIME
    virtual void OnTextctrlDynmcItimeEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_CNTSYS
    virtual void OnCheckboxDynmcCntsysClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CNTFRQ
    virtual void OnTextctrlDynmcCntfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_ALLCRD
    virtual void OnRadiobuttonDynmcAllcrdSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_XYONLY
    virtual void OnRadiobuttonDynmcXyonlySelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_ZONLY
    virtual void OnRadiobuttonDynmcZonlySelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOXDYNMC_USEFRAC
    virtual void OnCheckboxDynmcUsefracClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC1
    virtual void OnTextctrlDynmcFrac1Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC2
    virtual void OnTextctrlDynmcFrac2Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC3
    virtual void OnTextctrlDynmcFrac3Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC4
    virtual void OnTextctrlDynmcFrac4Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FRAC5
    virtual void OnTextctrlDynmcFrac5Enter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RMCM
    virtual void OnCheckboxDynmcRmcmClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CMFRQ
    virtual void OnTextctrlDynmcCmfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USENVT
    virtual void OnCheckboxDynmcUsenvtClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_NVTTMP
    virtual void OnTextctrlDynmcNvttmpEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_SNGLRX
    virtual void OnRadiobuttonDynmcSnglrxSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DYNMC_SPLTRX
    virtual void OnRadiobuttonDynmcSpltrxSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_SLVTIM
    virtual void OnTextctrlDynmcSlvtimEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_SLUTIM
    virtual void OnTextctrlDynmcSlutimEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USEANL
    virtual void OnCheckboxDynmcUseanlClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_ANLTMP
    virtual void OnTextctrlDynmcAnltmpEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_IANL
    virtual void OnTextctrlDynmcIanlEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_FANL
    virtual void OnTextctrlDynmcFanlEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_USENPT
    virtual void OnCheckboxDynmcUsenptClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_NPTPRS
    virtual void OnTextctrlDynmcNptprsEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_PRSTIM
    virtual void OnTextctrlDynmcPrstimEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_CMPRSS
    virtual void OnTextctrlDynmcCmprssEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_VOLALG
    virtual void OnChoiceDynmcVolalgSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_DYNMC_RTHRM
    virtual void OnCheckboxDynmcRthrmClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_VELFRQ
    virtual void OnTextctrlDynmcVelfrqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_RTHTMP
    virtual void OnTextctrlDynmcRthtmpEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_DYNMC_VFRAC
    virtual void OnTextctrlDynmcVfracEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DYNMC_RTHOPT
    virtual void OnChoiceDynmcRthoptSelected( wxCommandEvent& event );
  protected:
    DynamicsModel *p_model;
    MDPanelHelper *p_helper;

};

#endif
    // _DYNAMICSPANEL_H_
