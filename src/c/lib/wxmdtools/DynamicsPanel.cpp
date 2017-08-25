
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "DynamicsPanel.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
  using std::cout;
  using std::endl;

#include "dsm/EDSIFactory.H"
#include "dsm/TaskJob.H"
#include "dsm/ResourceDescriptor.H"

#include "tdat/DynamicsModel.H"

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxUnitHelper.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/MDPanelHelper.H"
#include "wxgui/WxFeedback.H"

#include "wxgui/DynamicsPanel.H"

/**
 * DynamicsPanel constructors and destructors
 */

DynamicsPanel::DynamicsPanel(MDPanelHelper *helper )
{
   p_helper = helper;
}

DynamicsPanel::DynamicsPanel( MDPanelHelper *helper, 
                              wxWindow* parent, wxWindowID id,
                              const wxPoint& pos, const wxSize& size,
                              long style )
{
   Create(parent, id, pos, size, style);
   p_helper = helper;
}

DynamicsPanel::~DynamicsPanel( )
{
   // don't delete
   p_helper = 0;
}

void DynamicsPanel::setDynamicsModel(DynamicsModel *model)
{
  p_model = model;
}

DynamicsModel* DynamicsPanel::getDynamicsModel()
{
  return p_model;
}

void DynamicsPanel::refreshGUI()
{
  // Change to default units since the model is always in these units
  ewxWindowUtils::setUnitFamily(this, DEFAULT_UNIT_FAMILY);

  DynamicsModel *model = getDynamicsModel();
  ewxChoice *choice;
  ewxTextCtrl *text;
  ewxCheckBox *check;
  ewxRadioButton *radio;

  choice = ((ewxChoice*)FindWindow(ID_CHOICE_DYNMC_INTALG));
  choice->SetSelection(model->getIntegrationOpts());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_RESUM));
  if (model->getResumeOpt()) {
    check->SetValue(true);
  } else {
    check->SetValue(false);
  }

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_EQSTP));
  text->setValueAsInt(model->getEquilibrationSteps());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_DATSTP));
  text->setValueAsInt(model->getDataSteps());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_TSTEP));
  text->setValueAsDouble(model->getTimeStep());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_ITIME));
  text->setValueAsDouble(model->getInitialTime());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_CNTSYS));
  check->SetValue(model->getRecenterSystem());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_CNTFRQ));
  text->setValueAsInt(model->getRecenterFreq());

  if (model->getRecenterOpts() == 1) {
    radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DYNMC_ALLCRD));
    radio->SetValue(true);
  }  else if (model->getRecenterOpts() == 2) {
    radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DYNMC_XYONLY));
    radio->SetValue(true);
  } else {
    radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DYNMC_ZONLY));
    radio->SetValue(true);
  }

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_USEFRAC));
  check->SetValue(model->getUseFractions());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC1));
  text->setValueAsInt(model->getFraction1());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC2));
  text->setValueAsInt(model->getFraction2());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC3));
  text->setValueAsInt(model->getFraction3());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC4));
  text->setValueAsInt(model->getFraction4());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC5));
  text->setValueAsInt(model->getFraction5());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_RMCM));
  check->SetValue(model->getRemoveCM());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_CMFRQ));
  text->setValueAsInt(model->getRemoveCMFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_USENVT));
  check->SetValue(model->getUseNVT());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_NVTTMP));
  text->setValueAsDouble(model->getNVTTemperature());
  if (model->getUseDoubleRelaxation()) {
    radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DYNMC_SPLTRX));
    radio->SetValue(true);
  } else {
    radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DYNMC_SNGLRX));
    radio->SetValue(true);
  }
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_SLVTIM));
  text->setValueAsDouble(model->getSolventRxTime());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_SLUTIM));
  text->setValueAsDouble(model->getSoluteRxTime());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_USEANL));
  check->SetValue(model->getUseAnnealing());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_ANLTMP));
  text->setValueAsDouble(model->getFinalAnnealTemp());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_IANL));
  text->setValueAsDouble(model->getInitAnnealTime());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FANL));
  text->setValueAsDouble(model->getFinalAnnealTime());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_USENPT));
  check->SetValue(model->getUseNPT());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_NPTPRS));
  text->setValueAsDouble(model->getNPTPressure());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_PRSTIM));
  text->setValueAsDouble(model->getNPTRxTime());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_CMPRSS));
  text->setValueAsDouble(model->getCompressibility());
  choice = ((ewxChoice*)FindWindow(ID_CHOICE_DYNMC_VOLALG));
  choice->SetSelection(model->getVolumeAdjustAlg());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_RTHRM));
  check->SetValue(model->getUseVelRetherm());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_VELFRQ));
  text->setValueAsInt(model->getRethermFreq());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_RTHTMP));
  text->setValueAsDouble(model->getRethermTemp());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_VFRAC));
  text->setValueAsDouble(model->getRethermFrac());
  choice = ((ewxChoice*)FindWindow(ID_CHOICE_DYNMC_RTHOPT));
  choice->SetSelection(model->getRethermOpts());

  // Now change to user prefered units
  ewxWindowUtils::processGlobalPreferenceChange(this);

  wxCommandEvent event;
  OnCheckboxDynmcCntsysClick(event);
  OnCheckboxDynmcUsefracClick(event);
  OnCheckboxDynmcUsenvtClick(event);
  OnCheckboxDynmcUsenptClick(event);
  OnCheckboxDynmcRmcmClick(event);
  OnCheckboxDynmcUseanlClick(event);
  OnCheckboxDynmcRthrmClick(event);
  OnRadiobuttonDynmcSnglrxSelected(event);
  OnRadiobuttonDynmcSpltrxSelected(event);
  OnCheckboxDynmcResumClick(event);
  OnTextctrlDynmcDatstpEnter(event);

  // Now make sure all of the validators contain the forced values above
  TransferDataFromWindow();
}

void DynamicsPanel::checkResume()
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_RESUM));
  string url = getDynamicsModel()->getURL();
  TaskJob *taskJob = dynamic_cast<TaskJob*>(EDSIFactory::getResource(EcceURL(url)));
  if (taskJob != (TaskJob*)0 &&
      taskJob->getInputProviderType() != ResourceDescriptor::CT_MDDYNAMICS) {
    check->Disable();
  } else {
    check->Enable(true);
  }
}

void DynamicsPanel::initializeGUI()
{
  ewxTextCtrl *text = 0;
  ewxStaticText *label = 0;
  DynamicsModel *model = getDynamicsModel();
  ewxNumericValidator validator;

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_EQSTP));
  validator.setHardRange("[0,)");
  validator.setValue(model->EQSTEPS);
//  text->SetValidator(ewxNumericValidator(0,10000000,model->EQSTEPS));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_DATSTP));
  //validator.setHardRange("[0,)");
  //validator.setValue(model->DATASTEPS);
  text->SetValidator(ewxNumericValidator(0,INT_MAX,model->DATASTEPS));
  //text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_TSTEP));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->TIMESTEP);
//  text->SetValidator(ewxNumericValidator(1.0e-10,100.0,model->TIMESTEP));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_DYNMC_TSTEP));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Time",label));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_ITIME));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->INITTIME);
//  text->SetValidator(ewxNumericValidator(1.0e-10,100000.0,model->INITTIME));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_DYNMC_ITIME));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Time",label));

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_CNTFRQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->RECENTERFREQ);
//  text->SetValidator(ewxNumericValidator(1,10000,model->RECENTERFREQ));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC1));
  validator.setHardRange("[0,)");
  validator.setValue(model->FRAC1);
//  text->SetValidator(ewxNumericValidator(0,10000,model->FRAC1));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC2));
  validator.setHardRange("[0,)");
  validator.setValue(model->FRAC2);
//  text->SetValidator(ewxNumericValidator(0,10000,model->FRAC2));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC3));
  validator.setHardRange("[0,)");
  validator.setValue(model->FRAC3);
//  text->SetValidator(ewxNumericValidator(0,10000,model->FRAC3));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC4));
  validator.setHardRange("[0,)");
  validator.setValue(model->FRAC4);
//  text->SetValidator(ewxNumericValidator(0,10000,model->FRAC4));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC5));
  validator.setHardRange("[0,)");
  validator.setValue(model->FRAC5);
//  text->SetValidator(ewxNumericValidator(0,10000,model->FRAC5));
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_CMFRQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->REMOVECMFREQ);
//  text->SetValidator(ewxNumericValidator(1,10000,model->REMOVECMFREQ));
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_NVTTMP));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->NVTTEMP);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000000.0,model->NVTTEMP));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_DYNMC_NVTTMP));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Temperature",label));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_SLVTIM));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->SLVRXTIME);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000000.0,model->SLVRXTIME));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_DYNMC_SLVTIM));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Time",label));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_SLUTIM));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->SLURXTIME);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000000.0,model->SLURXTIME));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_DYNMC_USLUTIM));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Time",label));

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_ANLTMP));
  validator.setHardRange("[0.0,)");
  validator.setValue(model->FINALTEMP);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000000.0,model->FINALTEMP));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_DYNMC_ANLTMP));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Temperature",label));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_IANL));
  validator.setHardRange("[0.0,)");
  validator.setValue(model->INITANNEALTIME);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000000.0,model->INITANNEALTIME));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_DYNMC_IANL));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Time",label));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FANL));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->FINALANNEALTIME);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000000.0,model->FINALANNEALTIME));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_DYNMC_FANL));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Time",label));

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_NPTPRS));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->NPTPRSSR);
//  text->SetValidator(ewxNumericValidator(1.0e-10,100000000.0,model->NPTPRSSR));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_DYNMC_NPTPRS));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Pressure",label));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_PRSTIM));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->NPTRXTIME);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000000.0,model->NPTRXTIME));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_DYNMC_PRSTIM));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Time",label));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_CMPRSS));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->COMPRESSIBILITY);
//  text->SetValidator(ewxNumericValidator(1.0e-12,1000000.0,model->COMPRESSIBILITY));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_DYNMC_CMPRSS));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Compressibility",label));

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_VELFRQ));
  validator.setHardRange("[0,)");
  validator.setValue(model->RETHERMFREQ);
//  text->SetValidator(ewxNumericValidator(1,1000000,model->RETHERMFREQ));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_RTHTMP));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->RETHERMTEMP);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000000.0,model->RETHERMTEMP));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_DYNMC_RTHTMP));
  text->setUnitHelper(new ewxUnitHelper(DEFAULT_UNIT_FAMILY,"Temperature",label));
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_VFRAC));
  validator.setHardRange("(0.0,1.0]");
  validator.setValue(model->RETHERMFRAC);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1.0,model->RETHERMFRAC));
  text->SetValidator(validator);
}

/**
 * DynamicsPanel event handlers
 */
void DynamicsPanel::OnCheckboxDynmcResumClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_RESUM));
    ewxPanel *panel = ((ewxPanel*)FindWindow(ID_PANEL_DYNMC_ALL));
    getDynamicsModel()->setResumeOpt(check->GetValue());
    if (check->GetValue()) {
      panel->Disable();
      ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_EQSTP));
      text->setValueAsInt(0);
    } else {
      panel->Enable(true);
    }
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnChoiceDynmcIntalgSelected( wxCommandEvent& event )
{
    ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_DYNMC_INTALG));
    getDynamicsModel()->setIntegrationOpts(choice->GetSelection());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcEqstpEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_EQSTP));
    getDynamicsModel()->setEquilibrationSteps(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcDatstpEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_DATSTP));
    getDynamicsModel()->setDataSteps(text->getValueAsInt());

    // data collection steps is a required value
    if (p_helper->getFeedback().getRunState()==
                ResourceDescriptor::STATE_CREATED && text->getValueAsInt()>0) {
      p_helper->setRunState(ResourceDescriptor::STATE_READY);
    }

    // invoke inter-panel constraint logic
    p_helper->notifyChange();

    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcTstepEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_TSTEP));
    getDynamicsModel()->setTimeStep(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcItimeEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_ITIME));
    getDynamicsModel()->setInitialTime(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnCheckboxDynmcCntsysClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_CNTSYS));
    ewxPanel *panel = ((ewxPanel*)FindWindow(ID_PANEL_DYNMC_RCNTR));
    if (check->GetValue()) {
      panel->Enable(true);
      getDynamicsModel()->setRecenterSystem(true);
    } else {
      panel->Disable();
      getDynamicsModel()->setRecenterSystem(false);
    }
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcCntfrqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_CNTFRQ));
    getDynamicsModel()->setRecenterFreq(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnRadiobuttonDynmcAllcrdSelected( wxCommandEvent& event )
{
    ewxRadioButton *radio
      = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DYNMC_ALLCRD));
    if (radio->GetValue()) {
      getDynamicsModel()->setRecenterOpts(1);
      p_helper->setSaveState(true);
    }
    event.Skip();
}

void DynamicsPanel::OnRadiobuttonDynmcXyonlySelected( wxCommandEvent& event )
{
    ewxRadioButton *radio
      = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DYNMC_XYONLY));
    if (radio->GetValue()) {
      getDynamicsModel()->setRecenterOpts(2);
      p_helper->setSaveState(true);
    }
    event.Skip();
}

void DynamicsPanel::OnRadiobuttonDynmcZonlySelected( wxCommandEvent& event )
{
    ewxRadioButton *radio
      = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DYNMC_ZONLY));
    if (radio->GetValue()) {
      getDynamicsModel()->setRecenterOpts(3);
      p_helper->setSaveState(true);
    }
    event.Skip();
}

void DynamicsPanel::OnCheckboxDynmcUsefracClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_USEFRAC));
    ewxPanel *panel = ((ewxPanel*)FindWindow(ID_PANEL_DYNMC_FRACS));
    if (check->GetValue()) {
      panel->Enable(true);
      getDynamicsModel()->setUseFractions(true);
    } else {
      panel->Disable();
      getDynamicsModel()->setUseFractions(false);
    }
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcFrac1Enter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC1));
    getDynamicsModel()->setFraction1(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcFrac2Enter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC2));
    getDynamicsModel()->setFraction2(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcFrac3Enter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC3));
    getDynamicsModel()->setFraction3(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcFrac4Enter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC4));
    getDynamicsModel()->setFraction4(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcFrac5Enter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FRAC5));
    getDynamicsModel()->setFraction5(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnCheckboxDynmcRmcmClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_RMCM));
    ewxPanel *panel = ((ewxPanel*)FindWindow(ID_PANEL_DYNMC_RMCM));
    if (check->GetValue()) {
      panel->Enable(true);
      getDynamicsModel()->setRemoveCM(true);
    } else {
      panel->Disable();
      getDynamicsModel()->setRemoveCM(false);
    }
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcCmfrqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_CMFRQ));
    getDynamicsModel()->setRemoveCMFreq(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnCheckboxDynmcUsenvtClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_USENVT));
    ewxPanel *panel = ((ewxPanel*)FindWindow(ID_PANEL_DYNMC_NVT));
    if (check->GetValue()) {
      panel->Enable(true);
      getDynamicsModel()->setUseNVT(true);
    } else {
      panel->Disable();
      getDynamicsModel()->setUseNVT(false);
    }
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcNvttmpEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_NVTTMP));
    getDynamicsModel()->setNVTTemperature(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnRadiobuttonDynmcSnglrxSelected( wxCommandEvent& event )
{
    ewxRadioButton *radio
      = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DYNMC_SNGLRX));
    if (radio->GetValue()) {
      ewxPanel *panel
        = ((ewxPanel*)FindWindow(ID_PANEL_DYNMC_SLUTIM));
      panel->Disable();
      getDynamicsModel()->setUseDoubleRelaxation(false);
      p_helper->setSaveState(true);
    }
    event.Skip();
}

void DynamicsPanel::OnRadiobuttonDynmcSpltrxSelected( wxCommandEvent& event )
{
    ewxRadioButton *radio
      = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DYNMC_SPLTRX));
    if (radio->GetValue()) {
      ewxPanel *panel
        = ((ewxPanel*)FindWindow(ID_PANEL_DYNMC_SLUTIM));
      panel->Enable(true);
      getDynamicsModel()->setUseDoubleRelaxation(true);
      p_helper->setSaveState(true);
    }
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcSlvtimEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_SLVTIM));
    getDynamicsModel()->setSolventRxTime(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcSlutimEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_SLUTIM));
    getDynamicsModel()->setSoluteRxTime(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnCheckboxDynmcUseanlClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_USEANL));
    ewxPanel *panel = ((ewxPanel*)FindWindow(ID_PANEL_DYNMC_ANNEAL));
    if (check->GetValue()) {
      panel->Enable(true);
      getDynamicsModel()->setUseAnnealing(true);
    } else {
      panel->Disable();
      getDynamicsModel()->setUseAnnealing(false);
    }
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcAnltmpEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_ANLTMP));
    getDynamicsModel()->setFinalAnnealTemp(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcIanlEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_IANL));
    getDynamicsModel()->setInitAnnealTime(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcFanlEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_FANL));
    getDynamicsModel()->setFinalAnnealTime(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnCheckboxDynmcUsenptClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_USENPT));
    ewxPanel *panel = ((ewxPanel*)FindWindow(ID_PANEL_DYNMC_NPT));
    if (check->GetValue()) {
      panel->Enable(true);
      getDynamicsModel()->setUseNPT(true);
    } else {
      panel->Disable();
      getDynamicsModel()->setUseNPT(false);
    }
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcNptprsEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_NPTPRS));
    getDynamicsModel()->setNPTPressure(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcPrstimEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_PRSTIM));
    getDynamicsModel()->setNPTRxTime(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcCmprssEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_CMPRSS));
    getDynamicsModel()->setCompressibility(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnChoiceDynmcVolalgSelected( wxCommandEvent& event )
{
    ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_DYNMC_VOLALG));
    getDynamicsModel()->setVolumeAdjustAlg(choice->GetSelection());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnCheckboxDynmcRthrmClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DYNMC_RTHRM));
    ewxPanel *panel = ((ewxPanel*)FindWindow(ID_PANEL_DYNMC_RTHRM));
    if (check->GetValue()) {
      panel->Enable(true);
      getDynamicsModel()->setUseVelRetherm(true);
    } else {
      panel->Disable();
      getDynamicsModel()->setUseVelRetherm(false);
    }
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcVelfrqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_VELFRQ));
    getDynamicsModel()->setRethermFreq(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcRthtmpEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_RTHTMP));
    getDynamicsModel()->setRethermTemp(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnTextctrlDynmcVfracEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_DYNMC_VFRAC));
    getDynamicsModel()->setRethermFrac(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void DynamicsPanel::OnChoiceDynmcRthoptSelected( wxCommandEvent& event )
{
    ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_DYNMC_RTHOPT));
    getDynamicsModel()->setRethermOpts(choice->GetSelection());
    p_helper->setSaveState(true);
    event.Skip();
}
