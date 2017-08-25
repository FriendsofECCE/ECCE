
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "OptimizePanel.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "tdat/OptimizeModel.H"

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxUnitHelper.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/MDPanelHelper.H"

#include "wxgui/OptimizePanel.H"

/*!
 * OptimizePanel constructors
 */

OptimizePanel::OptimizePanel(MDPanelHelper *helper )
{
   p_helper = helper;
}

OptimizePanel::OptimizePanel( MDPanelHelper *helper,
                              wxWindow* parent,
                              wxWindowID id, const wxPoint& pos,
                              const wxSize& size, long style )
                              : OptimizePanelGUI(parent, id, pos, size, style)
{
   p_helper = helper;
}

OptimizePanel::~OptimizePanel( )
{
   // Don't delete this
   p_helper = 0;
}

void OptimizePanel::setOptimizeModel(OptimizeModel *model)
{
  p_model = model;
}

OptimizeModel* OptimizePanel::getOptimizeModel()
{
  return p_model;
}
/**
 * Set validators for all text input fields. Note that this
 * can only be called after the OptimizeModel has been set.
 */
void OptimizePanel::initializeGUI()
{
  ewxTextCtrl *text = 0;
  ewxStaticText *label = 0;
  OptimizeModel *model = getOptimizeModel();

  ewxNumericValidator validator;

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_SD_MAXSTP));
  validator.setHardRange("[1,)");
  validator.setValue(model->SDMAXITER);
//  text->SetValidator(ewxNumericValidator(1,10000,model->SDMAXITER));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_ISTEP));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->SDISTEPSIZE);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000.0,model->SDISTEPSIZE));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_OPT_ISTEP));
  text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Length",label));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_TOL));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->SDTOLERANCE);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000.0,model->SDTOLERANCE));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_OPT_TOL));
  text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Length",label));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_MAXSZ));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->SDMAXSTEPSIZE);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000.0,model->SDMAXSTEPSIZE));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_OPT_MAXSZ));
  text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Length",label));

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_CG_MAXSTP));
  validator.setHardRange("[1,)");
  validator.setValue(model->CGMAXITER);
//  text->SetValidator(ewxNumericValidator(1,10000,model->CGMAXITER));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_ISRCH));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->CGSEARCHINT);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000.0,model->CGSEARCHINT));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_OPT_ISRCH));
  text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Length",label));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_THRESH));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->CGTHRESHOLD);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000.0,model->CGTHRESHOLD));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_OPT_THRESH));
  text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Length",label));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_REFRESH));
  validator.setHardRange("[1,)");
  validator.setValue(model->CGREFRESHINT);
//  text->SetValidator(ewxNumericValidator(1,10000,model->CGREFRESHINT));
  text->SetValidator(validator);
}
/**
 * Refresh values of GUI based on current values of model
 */
void OptimizePanel::refreshGUI()
{
  // Change to default units since the model is always in these units
  ewxWindowUtils::setUnitFamily(this, DEFAULT_UNIT_FAMILY);

  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_OPT_USESD));
  check->SetValue(p_model->getUseSD());

  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_SD_MAXSTP));
  text->setValueAsInt(p_model->getSDMaxIterations());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_ISTEP));
  text->setValueAsDouble(p_model->getSDInitialStepSize());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_TOL));
  text->setValueAsDouble(p_model->getSDTolerance());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_MAXSZ));
  text->setValueAsDouble(p_model->getSDMaxStepSize());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_OPT_USECG));
  check->SetValue(p_model->getUseCG());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_CG_MAXSTP));
  text->setValueAsInt(p_model->getCGMaxIterations());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_ISRCH));
  text->setValueAsDouble(p_model->getCGInitialSearchInterval());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_THRESH));
  text->setValueAsDouble(p_model->getCGThresholdSize());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_REFRESH));
  text->setValueAsInt(p_model->getCGRefreshCycle());

  // Now change to user prefered units
  ewxWindowUtils::processGlobalPreferenceChange(this);

  wxCommandEvent event;
  OnCheckboxOptUsesdClick(event);
  OnCheckboxOptUsecgClick(event);

  // Now make sure all of the validators contain the forced values above
  TransferDataFromWindow();
}

void OptimizePanel::showOptimizePanel()
{
  ewxCheckBox *check;
  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_OPT_USESD));
  if (check->GetValue()) {
    p_optimizeVSizer->Show(p_SDPanel, true, true);
  } else {
    p_optimizeVSizer->Show(p_SDPanel, false, true);
  }
  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_OPT_USECG));
  if (check->GetValue()) {
    p_optimizeVSizer->Show(p_CGPanel, true, true);
  } else {
    p_optimizeVSizer->Show(p_CGPanel, false, true);
  }
  p_optimizeVSizer->Layout();
}

/**
 * Event handlers
 */

void OptimizePanel::OnCheckboxOptUsesdClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_OPT_USESD));
    wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_OPT_SDPAR));
    if (check->GetValue()) {
      panel->Enable(true);
    } else {
      panel->Disable();
    }
    // Don't allow user to turn off both Steepest Descents and Conjugate
    // Gradient optimizations.
    if (!check->GetValue()) {
      ewxCheckBox *check2 = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_OPT_USECG));
      if (!check2->GetValue()) {
        check2->SetValue(true);
        OnCheckboxOptUsecgClick(event);
      }
    }
    getOptimizeModel()->setUseSD(check->GetValue());
    showOptimizePanel();
    p_helper->setSaveState(true);
    event.Skip();
}

void OptimizePanel::OnTextctrlOptSdMaxstpEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_SD_MAXSTP));
    int ival = atoi(text->GetValue().c_str());
    getOptimizeModel()->setSDMaxIterations(ival);
    p_helper->setSaveState(true);
    event.Skip();
}

void OptimizePanel::OnTextctrlOptIstepEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_ISTEP));
    double rval = atof(text->GetBaseValue().c_str());
    getOptimizeModel()->setSDInitialStepSize(rval);
    p_helper->setSaveState(true);
    event.Skip();
}

void OptimizePanel::OnTextctrlOptTolEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_TOL));
    double rval = atof(text->GetBaseValue().c_str());
    getOptimizeModel()->setSDInitialStepSize(rval);
    p_helper->setSaveState(true);
    event.Skip();
}

void OptimizePanel::OnTextctrlOptMaxszEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_MAXSZ));
    double rval = atof(text->GetBaseValue().c_str());
    getOptimizeModel()->setSDMaxStepSize(rval);
    p_helper->setSaveState(true);
    event.Skip();
}

void OptimizePanel::OnCheckboxOptUsecgClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_OPT_USECG));
    wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_OPT_CGPAR));
    if (check->GetValue()) {
      panel->Enable(true);
    } else {
      panel->Disable();
    }
    // Don't allow user to turn off both Steepest Descents and Conjugate
    // Gradient optimizations.
    if (!check->GetValue()) {
      ewxCheckBox *check2 = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_OPT_USESD));
      if (!check2->GetValue()) {
        check2->SetValue(true);
        OnCheckboxOptUsesdClick(event);
      }
    }
    getOptimizeModel()->setUseCG(check->GetValue());
    showOptimizePanel();
    p_helper->setSaveState(true);
    event.Skip();
}

void OptimizePanel::OnTextctrlOptCgMaxstpEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_CG_MAXSTP));
    int ival = atoi(text->GetValue().c_str());
    getOptimizeModel()->setCGMaxIterations(ival);
    p_helper->setSaveState(true);
    event.Skip();
}

void OptimizePanel::OnTextctrlOptIsrchEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_ISRCH));
    double rval = atof(text->GetBaseValue().c_str());
    getOptimizeModel()->setCGInitialSearchInterval(rval);
    p_helper->setSaveState(true);
    event.Skip();
}

void OptimizePanel::OnTextctrlOptThreshEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_THRESH));
    double rval = atof(text->GetBaseValue().c_str());
    getOptimizeModel()->setCGThresholdSize(rval);
    p_helper->setSaveState(true);
    event.Skip();
}

void OptimizePanel::OnTextctrlOptRefreshEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_OPT_REFRESH));
    int ival = atoi(text->GetValue().c_str());
    getOptimizeModel()->setCGRefreshCycle(ival);
    p_helper->setSaveState(true);
    event.Skip();
}
