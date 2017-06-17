#ifndef _CONSTRAINTPANEL_H_
#define _CONSTRAINTPANEL_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "ConstraintPanel.C"
#endif

/*!
 * Includes
 */

#include "wxgui/ConstraintPanelGUI.H"

class ConstraintModel;
class FragmentSummary;
class MDPanelHelper;

/**
 * GUI implementation of ConstraintPanel
 */
class ConstraintPanel: public ConstraintPanelGUI
{    

public:
    /// Constructors
    ConstraintPanel( MDPanelHelper *helper );
    ConstraintPanel( MDPanelHelper *helper, wxWindow* parent,
	             wxWindowID id = SYMBOL_CONSTRAINTPANELGUI_IDNAME,
		     const wxPoint& pos = SYMBOL_CONSTRAINTPANELGUI_POSITION,
		     const wxSize& size = SYMBOL_CONSTRAINTPANELGUI_SIZE,
		     long style = SYMBOL_CONSTRAINTPANELGUI_STYLE );

    virtual ~ConstraintPanel( );

    void setConstraintModel(ConstraintModel *model);
    ConstraintModel* getConstraintModel();
    void refreshGUI();
    void initializeGUI();
    void showConstraintPanel();
    void setFragmentSummary(FragmentSummary *fragSum);
    void requestBuilder();

    /**
     *  Constraint panel even handlers
     */
    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_USESHK
    virtual    void OnCheckboxConstUseshkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLVITER
    virtual    void OnTextctrlConstSlviterEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLVTOL
    virtual    void OnTextctrlConstSlvtolEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLUITER
    virtual    void OnTextctrlConstSluiterEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_CONST_SLUTOL
    virtual    void OnTextctrlConstSlutolEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_SHKOPT
    virtual    void OnChoiceConstShkoptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_HONLY
    virtual    void OnCheckboxConstHonlyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_CONST_PERM
    virtual    void OnCheckboxConstPermClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_FIXADD
    virtual    void OnButtonConstFixaddClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_FIXDEL
    virtual    void OnButtonConstFixdelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_FIXOPT
    virtual    void OnChoiceConstFixoptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_CONST_FIXOPT
    virtual    void OnChoiceConstSeloptSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_GETSEL
    virtual    void OnButtonConstGetselClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CONST_CLEAR
    virtual    void OnButtonConstClearClick( wxCommandEvent& event );
protected:
    ConstraintModel *p_model;
    MDPanelHelper *p_helper;
    /**
     * Shared pointers with the higher level task GUI
     */
    FragmentSummary *p_fragSummary;
};

#endif // _CONSTRAINTPANEL_H_
