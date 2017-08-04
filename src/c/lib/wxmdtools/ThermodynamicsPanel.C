
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ThermodynamicsPanel.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "tdat/ThermodynamicsModel.H"

#include "wxgui/ewxChoice.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxUnitHelper.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/MDPanelHelper.H"

#include "wxgui/ThermodynamicsPanel.H"

/*!
 * ThermodynamicsPanel constructors
 */

ThermodynamicsPanel::ThermodynamicsPanel(MDPanelHelper *helper )
{
   p_helper = helper;
}



ThermodynamicsPanel::ThermodynamicsPanel( MDPanelHelper *helper,
        wxWindow* parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style )
        : ThermodynamicsPanelGUI(parent, id, pos, size, style)
{
   p_helper = helper;
}



ThermodynamicsPanel::~ThermodynamicsPanel( )
{
   // Don't delete this
   p_helper = 0;
}



void ThermodynamicsPanel::setThermodynamicsModel(ThermodynamicsModel *model)
{
  p_model = model;
}



ThermodynamicsModel* ThermodynamicsPanel::getThermodynamicsModel()
{
  return p_model;
}



/**
 * Set validators for all text input fields. Note that this
 * can only be called after the ThermodynamicsModel has been set.
 */
void ThermodynamicsPanel::initializeGUI()
{
  ewxTextCtrl   *text = 0;
  ewxStaticText *label = 0;
  ewxSpinCtrl   *spin = 0;
  ewxNumericValidator validator;
  ThermodynamicsModel *model = getThermodynamicsModel();

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_THERM_PMF_SCALED));
  validator.setHardRange("[0.0,)");
  validator.setValue(model->PMFSCALE);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_THERM_PMF_HARMONIC));
  validator.setHardRange("[0,)");
  validator.setValue(model->PMFEQUILHARM);
  text->SetValidator(validator);

  spin = ((ewxSpinCtrl*)FindWindow(ID_SPINCTRL_THERM_INTEG_ENSEMBLES_NUM));
  spin->SetRange(1, 100);
  spin = ((ewxSpinCtrl*)FindWindow(ID_SPINCTRL_THERM_INTEG_ENSEMBLES_MAX));
  spin->SetRange(1, 100);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_THERM_INTEG_ERROR));
  validator.setHardRange("[0.0,)");
  validator.setValue(model->ERRORMAX);
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_THERM_INTEG_ERROR));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Energy",label));

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_THERM_INTEG_DRIFT));
  validator.setHardRange("[0.0,)");
  validator.setValue(model->DRIFTMAX);
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_THERM_INTEG_DRIFT));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Drift",label));
}



/**
 * Refresh values of GUI based on current values of model
 */
void ThermodynamicsPanel::refreshGUI()
{
  // Change to default units since the model is always in these units
  ewxWindowUtils::setUnitFamily(this, DEFAULT_UNIT_FAMILY);

  wxCheckBox  *checkbox;
  ewxTextCtrl *text;
  ewxSpinCtrl *spin;
  ewxChoice   *choice;
  int radioId;
  bool usePmf;
  bool useThermoIntegration;
  ThermodynamicsModel *model = getThermodynamicsModel();

  // enable/disable pmf options
  usePmf = model->getUsePmf();
  checkbox = (wxCheckBox*) FindWindow(ID_CHECKBOX_THERM_PMF);
  checkbox->SetValue(usePmf);
  enablePmfPanel(usePmf);

  switch(model->getPmfOpt()) {
    case 2:  radioId = ID_RADIOBUTTON_THERM_PMF_HARMONIC; break;
    case 1:  radioId = ID_RADIOBUTTON_THERM_PMF_SCALED; break;
    case 0: 
    default: radioId = ID_RADIOBUTTON_THERM_PMF_STANDARD; break;
  }
  ((wxRadioButton*)FindWindow(radioId))->SetValue(true);

  text = (ewxTextCtrl*) FindWindow(ID_TEXTCTRL_THERM_PMF_SCALED);
  text->setValueAsDouble(model->getPmfScale());

  text = (ewxTextCtrl*) FindWindow(ID_TEXTCTRL_THERM_PMF_HARMONIC);
  text->setValueAsInt(model->getPmfEquilharm());

  // enable/disable thermodynamic integration options
  useThermoIntegration = model->getUseThermoIntegration();
  checkbox = (wxCheckBox*) FindWindow(ID_CHECKBOX_THERM_INTEG);
  checkbox->SetValue(useThermoIntegration);
  enableIntegrationPanel(useThermoIntegration);

  switch(model->getInitialConditionsOpt()) {
    case 2:  radioId = ID_RADIOBUTTON_THERM_INTEG_EXTEND; break;
    case 1:  radioId = ID_RADIOBUTTON_THERM_INTEG_RENEW; break;
    case 0:
    default: radioId = ID_RADIOBUTTON_THERM_INTEG_NEW; break;
  }
  ((wxRadioButton*)FindWindow(radioId))->SetValue(true);

  choice = (ewxChoice*) FindWindow(ID_CHOICE_THERM_INTEG_DIRECTION);
  choice->SetSelection(model->getDirectionOpt());

  spin = (ewxSpinCtrl*) FindWindow(ID_SPINCTRL_THERM_INTEG_ENSEMBLES_NUM);
  spin->SetValue(model->getEnsembleNumber());

  spin = (ewxSpinCtrl*) FindWindow(ID_SPINCTRL_THERM_INTEG_ENSEMBLES_MAX);
  spin->SetValue(model->getEnsembleMax());

  text = (ewxTextCtrl*) FindWindow(ID_TEXTCTRL_THERM_INTEG_ERROR);
  text->setValueAsDouble(model->getErrorMax());

  text = (ewxTextCtrl*) FindWindow(ID_TEXTCTRL_THERM_INTEG_DRIFT);
  text->setValueAsDouble(model->getDriftMax());

  // Now make sure all of the validators contain the forced values above
  TransferDataFromWindow();
}



/**
 * Remove GUI parts that aren't needed for an Optimize calculation.
 */
void ThermodynamicsPanel::configOptimizeCalc()
{
  FindWindow(ID_CHECKBOX_THERM_INTEG)->Hide();
  FindWindow(ID_PANEL_THERM_INTEG)->Hide();
  // Although the GUI parts are hidden, the model inherits from the input
  // provider. This could cause the input deck to be incorrect.
  // Force model values to false to correct this.
  getThermodynamicsModel()->setUseThermoIntegration(false);
}



/**
 */
void ThermodynamicsPanel::enablePmfPanel(bool enable)
{
  FindWindow(ID_PANEL_THERM_PMF)->Enable(enable);
  FindWindow(ID_CHECKBOX_THERM_INTEG)->Enable(enable);
  FindWindow(ID_PANEL_THERM_INTEG)->Enable(enable);

  if (enable) {
    switch(getThermodynamicsModel()->getPmfOpt()) {
      case 2:  FindWindow(ID_TEXTCTRL_THERM_PMF_SCALED)->Enable(false);
               FindWindow(ID_TEXTCTRL_THERM_PMF_HARMONIC)->Enable(true); break;
      case 1:  FindWindow(ID_TEXTCTRL_THERM_PMF_SCALED)->Enable(true);
               FindWindow(ID_TEXTCTRL_THERM_PMF_HARMONIC)->Enable(false); break;
      case 0: 
      default: FindWindow(ID_TEXTCTRL_THERM_PMF_SCALED)->Enable(false);
               FindWindow(ID_TEXTCTRL_THERM_PMF_HARMONIC)->Enable(false); break;
    }

    enableIntegrationPanel(getThermodynamicsModel()->getUseThermoIntegration());
  }
}



/**
 */
void ThermodynamicsPanel::enableIntegrationPanel(bool enable)
{
  FindWindow(ID_PANEL_THERM_INTEG)->Enable(enable);
}



/**
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_THERM_PMF     
 */
void ThermodynamicsPanel::OnCheckboxThermPmfClick( wxCommandEvent& event )
{
  // update model
  getThermodynamicsModel()->setUsePmf(event.IsChecked());

  // update GUI
  enablePmfPanel(event.IsChecked());

  // update save state
  p_helper->setSaveState(true);
}


/**
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for
 * ID_RADIOBUTTON_THERM_PMF_STANDARD
 */
void ThermodynamicsPanel::OnRadiobuttonThermPmfStandardSelected( wxCommandEvent& event )
{
  // update model
  getThermodynamicsModel()->setPmfOpt(0);

  // update GUI
  enablePmfPanel(true);

  // update save state
  p_helper->setSaveState(true);
}


/**
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for
 * ID_RADIOBUTTON_THERM_PMF_SCALED
 */
void ThermodynamicsPanel::OnRadiobuttonThermPmfScaledSelected( wxCommandEvent& event )
{
  // update model
  getThermodynamicsModel()->setPmfOpt(1);

  // update GUI
  enablePmfPanel(true);

  // update save state
  p_helper->setSaveState(true);

}


/**
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_PMF_SCALED
 */
void ThermodynamicsPanel::OnTextctrlThermPmfScaledEnter( wxCommandEvent& event )
{
  // update model
  getThermodynamicsModel()->setPmfScale(
          ((ewxTextCtrl*)FindWindow(event.GetId()))->getValueAsDouble());

  // update save state
  p_helper->setSaveState(true);
}


/**
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for
 * ID_RADIOBUTTON_THERM_PMF_HARMONIC
 */
void ThermodynamicsPanel::OnRadiobuttonThermPmfHarmonicSelected( wxCommandEvent& event )
{
  // update model
  getThermodynamicsModel()->setPmfOpt(2);

  // update GUI
  enablePmfPanel(true);

  // update save state
  p_helper->setSaveState(true);
}


/**
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_PMF_HARMONIC
 */
void ThermodynamicsPanel::OnTextctrlThermPmfHarmonicEnter(wxCommandEvent& event)
{
  // update model
  getThermodynamicsModel()->setPmfEquilharm(
          ((ewxTextCtrl*)FindWindow(event.GetId()))->getValueAsInt());

  // update save state
  p_helper->setSaveState(true);
}


/**
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_THERM_INTEG    
 */
void ThermodynamicsPanel::OnCheckboxThermIntegClick( wxCommandEvent& event )
{
  // update model
  getThermodynamicsModel()->setUseThermoIntegration(event.IsChecked());

  // update GUI
  enableIntegrationPanel(event.IsChecked());

  // update save state
  p_helper->setSaveState(true);
}


/**
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for 
 * ID_RADIOBUTTON_THERM_INTEG_NEW
 */
void ThermodynamicsPanel::OnRadiobuttonThermIntegNewSelected( wxCommandEvent& event )
{
  // update model
  getThermodynamicsModel()->setInitialConditionsOpt(0);

  // update save state
  p_helper->setSaveState(true);
}


/**
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for 
 * ID_RADIOBUTTON_THERM_INTEG_RENEW
 */
void ThermodynamicsPanel::OnRadiobuttonThermIntegRenewSelected( wxCommandEvent& event )
{
  // update model
  getThermodynamicsModel()->setInitialConditionsOpt(1);

  // update save state
  p_helper->setSaveState(true);
}


/**
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for 
 * ID_RADIOBUTTON_THERM_INTEG_EXTEND
 */
void ThermodynamicsPanel::OnRadiobuttonThermIntegExtendSelected( wxCommandEvent& event )
{
  // update model
  getThermodynamicsModel()->setInitialConditionsOpt(2);

  // update save state
  p_helper->setSaveState(true);
}


/**
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for 
 * ID_CHOICE_THERM_INTEG_DIRECTION
 */
void ThermodynamicsPanel::OnChoiceThermIntegDirectionSelected( wxCommandEvent& event )
{
  // update model
  getThermodynamicsModel()->setDirectionOpt(event.GetInt());

  // update save state
  p_helper->setSaveState(true);
}


/**
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for 
 * ID_SPINCTRL_THERM_INTEG_ENSEMBLES_NUM
 */
void ThermodynamicsPanel::OnSpinctrlThermIntegEnsemblesNumUpdated( wxSpinEvent& event )
{
  // update model
  getThermodynamicsModel()->setEnsembleNumber(event.GetPosition());

  // there can only be as many ensembles as the max, so increase the max if
  // we have too many here
  ewxSpinCtrl *spin = 
          (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_THERM_INTEG_ENSEMBLES_MAX);
  if (event.GetPosition() > spin->GetValue()) {
    spin->SetValue(event.GetPosition());
    spin->TransferDataFromWindow();
    getThermodynamicsModel()->setEnsembleMax(event.GetPosition());
  }

  // update save state
  p_helper->setSaveState(true);
}


/**
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for 
 * ID_SPINCTRL_THERM_INTEG_ENSEMBLES_MAX
 */
void ThermodynamicsPanel::OnSpinctrlThermIntegEnsemblesMaxUpdated( wxSpinEvent& event )
{
  // update model
  getThermodynamicsModel()->setEnsembleMax(event.GetPosition());

  // there can only be as many ensembles as the max, so decrease the num if
  // we have few many here
  ewxSpinCtrl *spin = 
          (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_THERM_INTEG_ENSEMBLES_NUM);
  if (event.GetPosition() < spin->GetValue()) {
    spin->SetValue(event.GetPosition());
    spin->TransferDataFromWindow();
    getThermodynamicsModel()->setEnsembleNumber(event.GetPosition());
  }

  // update save state
  p_helper->setSaveState(true);
}


/**
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_INTEG_ERROR
 */
void ThermodynamicsPanel::OnTextctrlThermIntegErrorEnter( wxCommandEvent& event )
{
  // update model
  getThermodynamicsModel()->setErrorMax(
          ((ewxTextCtrl*)FindWindow(event.GetId()))->getValueAsDouble());

  // update save state
  p_helper->setSaveState(true);
}


/**
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_THERM_INTEG_DRIFT
 */
void ThermodynamicsPanel::OnTextctrlThermIntegDriftEnter( wxCommandEvent& event )
{
  // update model
  getThermodynamicsModel()->setDriftMax(
          ((ewxTextCtrl*)FindWindow(event.GetId()))->getValueAsDouble());

  // update save state
  p_helper->setSaveState(true);
}

