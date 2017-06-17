#ifndef _INTERACTIONPANEL_H_
#define _INTERACTIONPANEL_H_

/*!
 * Includes
 */

#include "wxgui/InteractionPanelGUI.H"

class InteractionModel;
class MDPanelHelper;

/**
 * GUI implementation of InteractionPanel in MD Task editors
 */
class InteractionPanel:  public InteractionPanelGUI
{    

  public:
    /// Constructors
    InteractionPanel(MDPanelHelper *helper);
    InteractionPanel(MDPanelHelper *helper, wxWindow* parent,
	                wxWindowID id = SYMBOL_INTERACTIONPANELGUI_IDNAME,
			const wxPoint& pos = SYMBOL_INTERACTIONPANELGUI_POSITION,
			const wxSize& size = SYMBOL_INTERACTIONPANELGUI_SIZE,
			long style = SYMBOL_INTERACTIONPANELGUI_STYLE );

    virtual ~InteractionPanel();

    void setInteractionModel(InteractionModel *model);
    InteractionModel* getInteractionModel();
    void refreshGUI();
    void initializeGUI();

    /**
     *  Interaction panel event handlers
     */
    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_INTRCT_ES_OPT
    virtual void OnChoiceIntrctEsOptSelected( wxCommandEvent& event );

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

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_GRIDX
    virtual    void OnTextctrlIntrctGridEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_EWTOL
    virtual    void OnTextctrlIntrctEwtolEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_ESPLINE
    virtual    void OnTextctrlIntrctEsplineEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TOGGLE_CLICKED event handler for ID_TOGGLE_INTRCT_ADVFFT
    virtual    void OnToggleIntrctAdvfftClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_NFFT
    virtual    void OnTextctrlIntrctNfftEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_INTRCT_FFT_SRC
    virtual    void OnChoiceIntrctFftSrcSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_QGRID
    virtual    void OnCheckboxIntrctQgridClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_MAXIT
    virtual    void OnTextctrlIntrctMaxitEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_PTOL
    virtual    void OnTextctrlIntrctPtolEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_INTRCT_FSTORD
    virtual    void OnCheckboxIntrctFstordClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_INTRCT_SROPT
    virtual    void OnRadiobuttonIntrctSroptSelected( wxCommandEvent& event);

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_DROPT
    virtual    void OnRadiobuttonIntrctDroptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_SCUT
    virtual    void OnTextctrlIntrctScutEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_LCUT
    virtual    void OnTextctrlIntrctLcutEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_SFREQ
    virtual    void OnTextctrlIntrctSfreqEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_INTRCT_LFREQ
    virtual    void OnTextctrlIntrctLfreqEnter( wxCommandEvent& event );

  protected:

    void showElectrostaticPanel();

    InteractionModel *p_model;
    MDPanelHelper *p_helper;
};

#endif
    // _INTERACTIONPANEL_H_
