#ifndef _CONTROLPANEL_H_
#define _CONTROLPANEL_H_

/*!
 * Includes
 */

#include "wxgui/ControlPanelGUI.H"

class ControlModel;
class MDPanelHelper;

/**
 * GUI implementation of ControlPanel in MD task editors
 */
class ControlPanel: public ControlPanelGUI
{    

public:
    /// Constructors
    ControlPanel(MDPanelHelper *helper );
    ControlPanel( MDPanelHelper *helper, wxWindow* parent,
	          wxWindowID id = SYMBOL_CONTROLPANELGUI_IDNAME,
		  const wxPoint& pos = SYMBOL_CONTROLPANELGUI_POSITION,
		  const wxSize& size = SYMBOL_CONTROLPANELGUI_SIZE,
		  long style = SYMBOL_CONTROLPANELGUI_STYLE );

    virtual ~ControlPanel();

    /**
     * Attach a ControlModel object to the GUI. This is used to manage
     * storing and restoring of settings
     */
    void setControlModel(ControlModel *model);
    ControlModel* getControlModel() const;
    /**
     * This method restores GUI settings based on values stored in the
     * ControlModel
     */
    void refreshGUI();
    /**
     * Attach validators etc. to GUI fields
     */
    void initializeGUI();
    /**
     * Redisplay panel if any subpanels are hidden or exposed.
     */
    void showControlPanel();

    /**
     * Control Panel event handlers
     */
    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_USELOAD
    virtual void OnCheckboxCntrlUseloadClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_CELLSZ
    virtual void OnCheckboxCntrlCellszClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_STRNGTH
    virtual void OnTextctrlCntrlStrngthEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_ZONLY
    virtual void OnCheckboxCntrlZonlyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_RDSTRB
    virtual void OnCheckboxCntrlRdstrbClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CFREQ
    virtual void OnTextctrlCntrlCfreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CNTRL_TSYNCH
    virtual void OnChoiceCntrlTsynchSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TOGGLE_CLICKED event handler for ID_TOGGLE_CNTRL_CNFG
    virtual void OnToggleCntrlCnfgClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_PCNFG
    virtual void OnCheckboxCntrlPcnfgClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_PX
    virtual void OnTextctrlCntrlPxEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRLCNTRL_PY
    virtual void OnTextctrlCntrlPyEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRLCNTRL_PZ
    virtual void OnTextctrlCntrlPzEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_CCNFG
    virtual void OnCheckboxCntrlCcnfgClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CX
    virtual void OnTextctrlCntrlCxEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CY
    virtual void OnTextctrlCntrlCyEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_CZ
    virtual void OnTextctrlCntrlCzEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_MAXSLV
    virtual void OnCheckboxCntrlMaxslvClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_MAXSLV
    virtual void OnTextctrlCntrlMaxslvEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_MAXSLU
    virtual void OnCheckboxCntrlMaxsluClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CNTRL_MAXSLU
    virtual void OnTextctrlCntrlMaxsluEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CNTRL_PACK
    virtual void OnCheckboxCntrlPackClick( wxCommandEvent& event );

  protected:
    MDPanelHelper *p_helper;
    ControlModel *p_model;
};

#endif
    // _CONTROLPANEL_H_
