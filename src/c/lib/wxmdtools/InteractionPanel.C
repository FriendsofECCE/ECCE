
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "InteractionPanel.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
  using std::cout;
  using std::endl;

#include "tdat/InteractionModel.H"

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxOpenCloseToggle.H"
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxUnitHelper.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/MDPanelHelper.H"

#include "wxgui/InteractionPanel.H"

/**
 * InteractionPanel constructors
 */

InteractionPanel::InteractionPanel(MDPanelHelper *helper ) 
      : InteractionPanelGUI()
{
   p_helper = helper;
}

InteractionPanel::InteractionPanel( MDPanelHelper *helper,
                                    wxWindow* parent, wxWindowID id,
                                    const wxPoint& pos, const wxSize& size,
                                    long style ) : InteractionPanelGUI(parent,
                                    id, pos, size, style)
{
   p_helper = helper;
}

InteractionPanel::~InteractionPanel( )
{
   // Don't delete
   p_helper = 0;
}

void InteractionPanel::setInteractionModel(InteractionModel *model)
{
  p_model = model;
}

InteractionModel* InteractionPanel::getInteractionModel()
{
  return p_model;
}

/**
 *  Set validatiors on all text input fields. Note that this can
 *  only be called after the InteractionModel has been set.
 */
void InteractionPanel::initializeGUI()
{
  ewxTextCtrl *text = 0;
  ewxStaticText *label = 0;
  InteractionModel *model = getInteractionModel();

  ewxNumericValidator validator;

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_GRID));
  validator.setHardRange("[1,)");
  validator.setValue(model->GRID);
//  text->SetValidator(ewxNumericValidator(1,100,model->GRID));
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_EWTOL));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->EWALDCONVRG);
//  text->SetValidator(ewxNumericValidator(1.0e-10,0.1,model->EWALDCONVRG));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_ESPLINE));
  validator.setHardRange("[1,)");
  validator.setValue(model->BSPLINEORDER);
//  text->SetValidator(ewxNumericValidator(1,100,model->BSPLINEORDER));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_NFFT));
  validator.setHardRange("[1,)");
  validator.setValue(model->NFFT);
//  text->SetValidator(ewxNumericValidator(1,10000,model->NFFT));
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_MAXIT));
  validator.setHardRange("[1,)");
  validator.setValue(model->MAXPOLITERATION);
//  text->SetValidator(ewxNumericValidator(1,10000,model->MAXPOLITERATION));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_PTOL));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->POLTOL);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1.0,model->POLTOL));
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_SCUT));
  validator.setHardRange("(0.0,)");
//  validator.setValue(model->SHORTCUT);
  text->SetValidator(ewxNumericValidator(0.001,1000.0,model->SHORTCUT));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_INTRCT_SCUT));
  text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_LCUT));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->LONGCUT);
//  text->SetValidator(ewxNumericValidator(0.001,1000.0,model->LONGCUT));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_INTRCT_ULCUT));
  text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_SFREQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->SHORTFREQ);
//  text->SetValidator(ewxNumericValidator(1,10000,model->SHORTFREQ));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_LFREQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->LONGFREQ);
//  text->SetValidator(ewxNumericValidator(1,10000,model->LONGFREQ));
  text->SetValidator(validator);

  showElectrostaticPanel();
}


/**
 * Based on the value of the choice menu, show only the correct panel(s).
 * Cutoff needs to be available for all options.
 */
void InteractionPanel::showElectrostaticPanel()
{
   ewxOpenCloseToggle *toggle
     = ((ewxOpenCloseToggle*)FindWindow(ID_TOGGLE_INTRCT_ADVFFT));

   ewxPanel *panel = ((ewxPanel*)FindWindow(ID_PANEL_INTRCT_FFTOPT));
   if (toggle->isOpen()) {
     panel->Enable(true);
   } else {
     panel->Disable();
   }
   p_EwaldSizer->Layout();
   ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_INTRCT_ES_OPT));
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_INTRCT_ES_OPT));
   ewxRadioButton *radio
     = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_INTRCT_SROPT));
   if (radio->GetValue()){
     p_cutoffHSizer->Show(p_lcutoffPanel,false,true);
   } else {
     p_cutoffHSizer->Show(p_lcutoffPanel,true,true);
   }
   p_cutoffHSizer->Layout();
   p_cutoffVSizer->Layout();
   p_mainVSizer->Show(p_parameterPanel, false, true);
   p_mainVSizer->Show(p_polarizationPanel, false, true);
   if (choice->GetStringSelection() == "Particle-Mesh Ewald") {
      p_mainVSizer->Show(p_ewaldPanel, true, true);
      p_mainVSizer->Show(p_polarizationPanel, false, true);
   } else if (choice->GetStringSelection() == "Polarization") {
      p_mainVSizer->Show(p_ewaldPanel, false, true);
      p_mainVSizer->Show(p_polarizationPanel, true, true);
   } else {
      p_mainVSizer->Show(p_ewaldPanel, false, true);
      p_mainVSizer->Show(p_polarizationPanel, false, true);
   }
   p_mainVSizer->Show(p_cutoffPanel, true, true);
   p_mainVSizer->Layout();
}


/**
 *  Reset all GUI fields based on contents of NWChemMDModel
 */
void InteractionPanel::refreshGUI()
{
   
  // Change to default units since the model is always in these units
  ewxWindowUtils::setUnitFamily(this, DEFAULT_UNIT_FAMILY);

  InteractionModel *imodel = getInteractionModel();

  int paramSet = imodel->getParameterSet();
  if (paramSet == 1) {
    ewxRadioButton
      *radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_INTRCT_SET1));
    radio->SetValue(true);
  } else if (paramSet == 2) {
    ewxRadioButton
      *radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_INTRCT_SET2));
    radio->SetValue(true);
  } else if (paramSet == 3) {
    ewxRadioButton
      *radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_INTRCT_SET3));
    radio->SetValue(true);
  }

  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_INTRCT_PERT1));
  check->SetValue(imodel->getFirstPerturbationSet());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_INTRCT_PERT2));
  check->SetValue(imodel->getSecondPerturbationSet());

  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_INTRCT_ES_OPT));
  choice->SetSelection(imodel->getInteractionOption());

  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_GRID));
  text->setValueAsInt(imodel->getGridDimensions());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_EWTOL));
  text->setValueAsDouble(imodel->getEwaldConvergence());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_ESPLINE));
  text->setValueAsInt(imodel->getBSplineOrder());

  ewxOpenCloseToggle *toggle
    = ((ewxOpenCloseToggle*)FindWindow(ID_TOGGLE_INTRCT_ADVFFT));
  toggle->setState(imodel->getAdvancedFFT());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_NFFT));
  text->setValueAsInt(imodel->getFFTNodes());

  choice = ((ewxChoice*)FindWindow(ID_CHOICE_INTRCT_FFT_SRC));
  choice->SetSelection(imodel->getFFTOption());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_INTRCT_QGRID));
  check->SetValue(imodel->getChargeGridOption());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_MAXIT));
  text->setValueAsInt(imodel->getMaximumPolarIter());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_PTOL));
  text->setValueAsDouble(imodel->getPolarizationTolerance());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_INTRCT_FSTORD));
  check->SetValue(imodel->getFirstOrderOption());

  int cutoffOpt = imodel->getCutoffOption();
  if (cutoffOpt == 1) {
    ewxRadioButton
      *radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_INTRCT_SROPT));
    radio->SetValue(true);
  } else if (cutoffOpt == 2) {
    ewxRadioButton
      *radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_INTRCT_DROPT));
    radio->SetValue(true);
  }

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_SCUT));
  text->setValueAsDouble(imodel->getSRCutoff());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_LCUT));
  text->setValueAsDouble(imodel->getLRCutoff());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_SFREQ));
  text->setValueAsInt(imodel->getShortListUpdateFreq());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_LFREQ));
  text->setValueAsInt(imodel->getLongListUpdateFreq());

  // Now change to user prefered units
  ewxWindowUtils::processGlobalPreferenceChange(this);

  wxCommandEvent event;
  OnChoiceIntrctEsOptSelected( event );
  OnRadiobuttonIntrctSroptSelected( event );
  OnToggleIntrctAdvfftClick( event );

  // Now make sure all of the validators contain the forced values above
  TransferDataFromWindow();
}

/**
 * Interaction panel event handlers
 */

void InteractionPanel::OnChoiceIntrctEsOptSelected( wxCommandEvent& event )
{
    ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_INTRCT_ES_OPT));
    ewxRadioButton *radio1 = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_INTRCT_SROPT));
    ewxRadioButton *radio2 = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_INTRCT_DROPT));
    if (choice->GetStringSelection() == "Particle-Mesh Ewald") {
      radio1->SetValue(true);
      radio2->Disable();
    } else if (choice->GetStringSelection() == "Polarization") {
      radio2->Enable(true);
    } else {
      radio2->Enable(true);
    }
    getInteractionModel()->setInteractionOption(choice->GetSelection());
    showElectrostaticPanel();
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnRadiobuttonIntrctSroptSelected( wxCommandEvent& event )
{
    ewxRadioButton *radio1 = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_INTRCT_SROPT));
    if (radio1->GetValue()) {
      p_lcutoffPanel->Disable();
      getInteractionModel()->setCutoffOption(1);
    } else {
      p_lcutoffPanel->Enable(true);
      getInteractionModel()->setCutoffOption(2);
    }
    showElectrostaticPanel();
    p_helper->setSaveState(true);
    event.Skip();

}

void InteractionPanel::OnRadiobuttonIntrctDroptSelected( wxCommandEvent& event )
{
    ewxRadioButton *radio1 = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_INTRCT_SROPT));
    if (radio1->GetValue()) {
      p_lcutoffPanel->Disable();
      getInteractionModel()->setCutoffOption(1);
    } else {
      p_lcutoffPanel->Enable(true);
      getInteractionModel()->setCutoffOption(2);
    }
    showElectrostaticPanel();
    p_helper->setSaveState(true);
    event.Skip();

}

void InteractionPanel::OnRadiobuttonIntrctSet1Selected( wxCommandEvent& event )
{
    ewxRadioButton
      *button = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_INTRCT_SET1));
    if (button->GetValue()) {
      getInteractionModel()->setParameterSet(1);
      p_helper->setSaveState(true);
    }
    event.Skip();
}

void InteractionPanel::OnRadiobuttonIntrctSet2Selected( wxCommandEvent& event )
{
    ewxRadioButton
      *button = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_INTRCT_SET2));
    if (button->GetValue()) {
      getInteractionModel()->setParameterSet(2);
      p_helper->setSaveState(true);
    }
    event.Skip();
}

void InteractionPanel::OnRadiobuttonIntrctSet3Selected( wxCommandEvent& event )
{
    ewxRadioButton
      *button = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_INTRCT_SET3));
    if (button->GetValue()) {
      getInteractionModel()->setParameterSet(3);
      p_helper->setSaveState(true);
    }
    event.Skip();
}

void InteractionPanel::OnCheckboxIntrctPert1Click( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_INTRCT_PERT1));
    getInteractionModel()->setFirstPerturbationSet(check->GetValue());
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnCheckboxIntrctPert2Click( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_INTRCT_PERT2));
    getInteractionModel()->setSecondPerturbationSet(check->GetValue());
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnTextctrlIntrctGridEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_GRID));
    int gridDim;
    gridDim = atoi(text->GetValue().c_str());
    getInteractionModel()->setGridDimensions(gridDim);
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnTextctrlIntrctEwtolEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_EWTOL));
    double rval = atof(text->GetValue().c_str());
    getInteractionModel()->setEwaldConvergence(rval);
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnTextctrlIntrctEsplineEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_ESPLINE));
    int ival = atoi(text->GetValue().c_str());
    getInteractionModel()->setBSplineOrder(ival);
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnToggleIntrctAdvfftClick( wxCommandEvent& event )
{
    ewxOpenCloseToggle *toggle
      = ((ewxOpenCloseToggle*)FindWindow(ID_TOGGLE_INTRCT_ADVFFT));

    if (toggle->isOpen()) {
      p_EwaldSizer->Show(p_fftPanel, true, true);
    } else {
      p_EwaldSizer->Show(p_fftPanel, false, true);
    }
    p_EwaldSizer->Layout();
    getInteractionModel()->setAdvancedFFT(toggle->isOpen());
    showElectrostaticPanel();
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnTextctrlIntrctNfftEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_NFFT));
    int ival = atoi(text->GetValue().c_str());
    getInteractionModel()->setFFTNodes(ival);
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnChoiceIntrctFftSrcSelected( wxCommandEvent& event )
{
    ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_INTRCT_FFT_SRC));
    getInteractionModel()->setFFTOption(choice->GetSelection());
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnCheckboxIntrctQgridClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_INTRCT_QGRID));
    if (check->GetValue()) {
      getInteractionModel()->setChargeGridOption(true);
    } else {
      getInteractionModel()->setChargeGridOption(false);
    }
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnTextctrlIntrctMaxitEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_MAXIT));
    int ival = atoi(text->GetValue().c_str());
    getInteractionModel()->setMaximumPolarIter(ival);
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnTextctrlIntrctPtolEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_PTOL));
    double rval = atof(text->GetValue().c_str());
    getInteractionModel()->setPolarizationTolerance(rval);
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnCheckboxIntrctFstordClick( wxCommandEvent& event )
{
    ewxCheckBox* check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_INTRCT_FSTORD));
    getInteractionModel()->setFirstOrderOption(check->GetValue());
    p_helper->setSaveState(true);
}

void InteractionPanel::OnTextctrlIntrctScutEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_SCUT));
    double rval = atof(text->GetBaseValue().c_str());
    getInteractionModel()->setSRCutoff(rval);
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnTextctrlIntrctLcutEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_LCUT));
    double rval = atof(text->GetBaseValue().c_str());
    getInteractionModel()->setLRCutoff(rval);
    p_helper->setSaveState(true);
    event.Skip();
}

void InteractionPanel::OnTextctrlIntrctSfreqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_SFREQ));
    int ival = atoi(text->GetValue().c_str());
    getInteractionModel()->setShortListUpdateFreq(ival);
    event.Skip();
}

void InteractionPanel::OnTextctrlIntrctLfreqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_INTRCT_LFREQ));
    int ival = atoi(text->GetValue().c_str());
    getInteractionModel()->setLongListUpdateFreq(ival);
    p_helper->setSaveState(true);
    event.Skip();
}
