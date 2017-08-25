
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "FilesPanel.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
  using std::cout;
  using std::endl;

#include "util/Ecce.H"

#include "tdat/FilesModel.H"

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/MDPanelHelper.H"

#include "wxgui/FilesPanel.H"

/*!
 * FilesPanel constructors
 */

FilesPanel::FilesPanel(MDPanelHelper *helper)
{
  p_helper = helper;

  // give a special value for data steps so the fields
  // that depend on it work until the user enters a real value
  // for data steps or if they are doing an optimize
  p_dataSteps = 0;
}

FilesPanel::FilesPanel(MDPanelHelper *helper, wxWindow* parent,
                         wxWindowID id, const wxPoint& pos,
                         const wxSize& size, long style )
                         : FilesPanelGUI(parent, id, pos, size, style)
{
  p_helper = helper;

  // give a special value for data steps so the fields
  // that depend on it work until the user enters a real value
  // for data steps or if they are doing an optimize
  p_dataSteps = 0;
}

FilesPanel::~FilesPanel()
{
  // Don't delete this
  p_helper = 0;
}

void FilesPanel::setFilesModel(FilesModel *model)
{
  p_model = model;
}

FilesModel* FilesPanel::getFilesModel() const
{
  return p_model;
}

void FilesPanel::initializeGUI()
{
  ewxTextCtrl *text;
  FilesModel *model = getFilesModel();

  ewxNumericValidator validator;

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_RSTFRQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->RSTFREQ);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_BTCSZ));
  validator.setHardRange("[1,)");
  validator.setValue(model->BATCHSZ);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_CSUFRQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->COORDSLUFRQ);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_CSVFRQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->COORDSLVFRQ);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_VSUFRQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->VELSLUFRQ);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_VSVFRQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->VELSLVFRQ);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_FSUFRQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->FRCSLUFRQ);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_FSVFRQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->FRCSLVFRQ);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_PRPFRQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->PROPFREQ);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_MDFREQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->PRINTMDFREQ);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_STFREQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->STATFREQ);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_SEFREQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->BONDFREQ);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_SFFREQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->FORCEFREQ);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_PMFFREQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->PMFFREQ);
  text->SetValidator(validator);
}

void FilesPanel::refreshGUI()
{
  // Change to default units since the model is always in these units
  ewxWindowUtils::setUnitFamily(this, DEFAULT_UNIT_FAMILY);

  ewxTextCtrl *text;
  ewxCheckBox *check;
  ewxChoice *choice;
  FilesModel *model = getFilesModel();

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_RSTFRQ));
  text->setValueAsInt(model->getRestartFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_KEEP));
  check->SetValue(model->getKeepRestart());

  choice = ((ewxChoice*)FindWindow(ID_CHOICE_FILE_TRJSTR));
  // Don't let user choose whether to store trajectories if the site admin
  // has set a policy never to allow it via the site_runtime file
  choice->Clear();
  choice->Append("Compute Resource Only");
  choice->Append("NWFS Archive");
  if (Ecce::ecceStoreTrajectories()) {
    choice->Append("Data Server");
  }
  choice->SetSelection(model->getTrajectoryStorage());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_USEBTC));
  check->SetValue(model->getUseBatches());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_BTCSZ));
  text->setValueAsInt(model->getBatchSize());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLTCRD));
  check->SetValue(model->getCoordsSolutePrint());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_CSUFRQ));
  text->setValueAsInt(model->getCoordsSoluteFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLVCRD));
  check->SetValue(model->getCoordsSolventPrint());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_CSVFRQ));
  text->setValueAsInt(model->getCoordsSolventFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLTVEL));
  check->SetValue(model->getVelocitySolutePrint());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_VSUFRQ));
  text->setValueAsInt(model->getVelocitySoluteFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLVVEL));
  check->SetValue(model->getVelocitySolventPrint());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_VSVFRQ));
  text->setValueAsInt(model->getVelocitySolventFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLTFRC));
  check->SetValue(model->getForcesSolutePrint());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_FSUFRQ));
  text->setValueAsInt(model->getForcesSoluteFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLVFRC));
  check->SetValue(model->getForcesSolventPrint());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_FSVFRQ));
  text->setValueAsInt(model->getForcesSolventFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_USEAVG));
  check->SetValue(model->getAvgProperties());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_PRPFRQ));
  text->setValueAsInt(model->getPropertyFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_PRTMD));
  check->SetValue(p_model->getPrintMD());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_MDFREQ));
  text->setValueAsInt(p_model->getPrintMDFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_ESOL));
  check->SetValue(p_model->getSolventEnergy());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_EBOND));
  check->SetValue(p_model->getSoluteEnergy());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_STAT));
  check->SetValue(p_model->getPrintStat());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_STFREQ));
  text->setValueAsInt(p_model->getStatFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SUBOND));
  check->SetValue(p_model->getSoluteBonds());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_SEFREQ));
  text->setValueAsInt(p_model->getBondFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_FRC));
  check->SetValue(p_model->getSoluteForces());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_SFFREQ));
  text->setValueAsInt(p_model->getForceFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_PMF));
  check->SetValue(p_model->getPmfData());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_PMFFREQ));
  text->setValueAsInt(p_model->getPmfFreq());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_TOP));
  check->SetValue(p_model->getPrintTop());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_NBOND));
  check->SetValue(p_model->getNbondPar());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLVPAR));
  check->SetValue(p_model->getSolventBondPar());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLUPAR));
  check->SetValue(p_model->getSoluteBondPar());

  // Now change to user prefered units
  ewxWindowUtils::processGlobalPreferenceChange(this);

  wxCommandEvent event;
  OnCheckboxFileSltcrdClick(event);
  OnCheckboxFileSlvcrdClick(event);
  OnCheckboxFileUsebtcClick(event);
  OnCheckboxFileSltvelClick(event);
  OnCheckboxFileSlvvelClick(event);
  OnCheckboxFileSltfrcClick(event);
  OnCheckboxFileSlvfrcClick(event);
  OnCheckboxFilePrtmdClick( event );
  OnCheckboxFileStatClick( event );
  OnCheckboxFileSubondClick( event );
  OnCheckboxFileFrcClick( event );
  OnCheckboxFileTopClick( event );
  OnCheckboxFilePmfClick( event );

  // Now make sure all of the validators contain the forced values above
  TransferDataFromWindow();
}


/**
 * Remove panels and options not needed by Energy calc.
 */
void FilesPanel::configEnergyCalc()
{
   ewxNotebook *notebook = ((ewxNotebook*)FindWindow(ID_NOTEBOOK));
   notebook->RemovePage(2);
   notebook->RemovePage(1);
   notebook->RemovePage(0);

   // remove PMF options not needed by energy calc
   ewxCheckBox* check = (ewxCheckBox*) FindWindow(ID_CHECKBOX_FILE_PMF);
   ewxStaticText* label = (ewxStaticText*) FindWindow(wxID_STATIC_FILE_PMFFREQ);
   ewxTextCtrl* text = (ewxTextCtrl*) FindWindow(ID_TEXTCTRL_FILE_PMFFREQ);
   check->Hide();
   label->Hide();
   text->Hide();
   // Although the GUI checkbox is hidden, the model inherits from the input
   // provider.  This could cause input deck to be incorrect.
   // Force model value to false to correct this.
   getFilesModel()->setPmfData(false);
}

void FilesPanel::disableBatchAndKeepOptions()
{
  getFilesModel()->setUseBatches(false);
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_USEBTC));
  check->SetValue(false);
  wxCommandEvent event;
  OnCheckboxFileUsebtcClick(event);
  check->Disable();

  getFilesModel()->setKeepRestart(false);
  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_KEEP));
  check->SetValue(false);
  OnCheckboxFileKeepClick(event);
  check->Disable();
}

void FilesPanel::constrainSteps(const int& steps)
{
  // save this value for use later
  p_dataSteps = steps;

  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_BTCSZ));

  // pretend that files are being batched even if the user hasn't chosen
  // to do that (yet).  That way if they do enable batching, all the
  // necessary constraints will be setup already
  ewxNumericValidator validator;
  if (steps == 0) {
    validator.setHardRange("[1,)");
  } else {
    char buf[16];
    sprintf(buf, "[1,%d]", steps);
    validator.setHardRange(buf);
  }

  if (steps>0 && getFilesModel()->BATCHSZ>steps)
    validator.setValue(steps);
  else
    validator.setValue(getFilesModel()->BATCHSZ);
  text->SetValidator(validator);

  if (steps>0 && getFilesModel()->getBatchSize()>steps)
    text->setValueAsInt(steps);
  else
    text->setValueAsInt(getFilesModel()->getBatchSize());
    
  if (getFilesModel()->getUseBatches()) {
    wxCommandEvent event;
    OnTextctrlFileBtcszEnter(event);
  } else {
    constrainFreqs(steps);
  }

  TransferDataFromWindow();
}

void FilesPanel::constrainFreqs(const int& steps)
{
  FilesModel *model = getFilesModel();

  // Solute Coordinates
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_CSUFRQ));

  ewxNumericValidator validator;

  char buf[16];
  if (steps == 0)
    strcpy(buf, "[1,)");
  else
    sprintf(buf, "[1,%d]", steps);
  validator.setHardRange(buf);

  if (steps>0 && model->COORDSLUFRQ>steps)
    validator.setValue(steps);
  else
    validator.setValue(model->COORDSLUFRQ);
  text->SetValidator(validator);

  if (steps>0 && model->getCoordsSoluteFreq()>steps)
    text->setValueAsInt(steps);
  else
    text->setValueAsInt(model->getCoordsSoluteFreq());

  wxCommandEvent event;
  // special flag to indicate callback was invoked by code and not
  // by the user
  event.SetId(0);
  OnTextctrlFileCsufrqEnter(event);
    

  // Solvent Coordinates
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_CSVFRQ));

  validator.setHardRange(buf);

  if (steps>0 && model->COORDSLVFRQ>steps)
    validator.setValue(steps);
  else
    validator.setValue(model->COORDSLVFRQ);
  text->SetValidator(validator);

  if (steps>0 && model->getCoordsSolventFreq()>steps)
    text->setValueAsInt(steps);
  else
    text->setValueAsInt(model->getCoordsSolventFreq());

  OnTextctrlFileCsvfrqEnter(event);


  // Solute Velocities
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_VSUFRQ));

  validator.setHardRange(buf);

  if (steps>0 && model->VELSLUFRQ>steps)
    validator.setValue(steps);
  else
    validator.setValue(model->VELSLUFRQ);
  text->SetValidator(validator);

  if (steps>0 && model->getVelocitySoluteFreq()>steps)
    text->setValueAsInt(steps);
  else
    text->setValueAsInt(model->getVelocitySoluteFreq());

  OnTextctrlFileVsufrqEnter(event);


  // Solvent Velocities
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_VSVFRQ));

  validator.setHardRange(buf);

  if (steps>0 && model->VELSLVFRQ>steps)
    validator.setValue(steps);
  else
    validator.setValue(model->VELSLVFRQ);
  text->SetValidator(validator);

  if (steps>0 && model->getVelocitySolventFreq()>steps)
    text->setValueAsInt(steps);
  else
    text->setValueAsInt(model->getVelocitySolventFreq());

  OnTextctrlFileVsvfrqEnter(event);


  // Solute Forces
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_FSUFRQ));

  validator.setHardRange(buf);

  if (steps>0 && model->FRCSLUFRQ>steps)
    validator.setValue(steps);
  else
    validator.setValue(model->FRCSLUFRQ);
  text->SetValidator(validator);

  if (steps>0 && model->getForcesSoluteFreq()>steps)
    text->setValueAsInt(steps);
  else
    text->setValueAsInt(model->getForcesSoluteFreq());

  OnTextctrlFileFsufrqEnter(event);


  // Solvent Forces
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_FSVFRQ));

  validator.setHardRange(buf);

  if (steps>0 && model->FRCSLVFRQ>steps)
    validator.setValue(steps);
  else
    validator.setValue(model->FRCSLVFRQ);
  text->SetValidator(validator);

  if (steps>0 && model->getForcesSolventFreq()>steps)
    text->setValueAsInt(steps);
  else
    text->setValueAsInt(model->getForcesSolventFreq());

  OnTextctrlFileFsvfrqEnter(event);


  // Property Files
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_PRPFRQ));

  validator.setHardRange(buf);

  if (steps>0 && model->PROPFREQ>steps)
    validator.setValue(steps);
  else
    validator.setValue(model->PROPFREQ);
  text->SetValidator(validator);

  if (steps>0 && model->getPropertyFreq()>steps)
    text->setValueAsInt(steps);
  else
    text->setValueAsInt(model->getPropertyFreq());

  OnTextctrlFilePrpfrqEnter(event);


  // Restart Files
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_RSTFRQ));

  validator.setHardRange(buf);

  if (steps>0 && model->RSTFREQ>steps)
    validator.setValue(steps);
  else
    validator.setValue(model->RSTFREQ);
  text->SetValidator(validator);

  if (steps>0 && model->getRestartFreq()>steps)
    text->setValueAsInt(steps);
  else
    text->setValueAsInt(model->getRestartFreq());

  OnTextctrlFileRstfrqEnter(event);

  TransferDataFromWindow();
}


/**
 * Event handlers
 */

void FilesPanel::OnTextctrlFileRstfrqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_RSTFRQ));
    getFilesModel()->setRestartFreq(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void FilesPanel::OnCheckboxFileKeepClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_KEEP));
    getFilesModel()->setKeepRestart(check->GetValue());
    p_helper->setSaveState(true);
    event.Skip();
}

void FilesPanel::OnChoiceFileTrjstrSelected( wxCommandEvent& event )
{
    ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_FILE_TRJSTR));
    getFilesModel()->setTrajectoryStorage(choice->GetSelection());
    p_helper->setSaveState(true);
    event.Skip();
}

void FilesPanel::OnCheckboxFileUsebtcClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_USEBTC));
    ewxStaticText *label = ((ewxStaticText*)FindWindow(wxID_STATIC_FILE_BTCSZ));
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_BTCSZ));
    if (check->GetValue()) {
      label->Enable(true);
      text->Enable(true);
    } else {
      label->Disable();
      text->Disable();
    }
    getFilesModel()->setUseBatches(check->GetValue());
    p_helper->setSaveState(true);

    if (check->GetValue())
      constrainFreqs(text->getValueAsInt());
    else
      constrainFreqs(p_dataSteps);

    event.Skip();
}

void FilesPanel::OnTextctrlFileBtcszEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_BTCSZ));
    getFilesModel()->setBatchSize(text->getValueAsInt());
    p_helper->setSaveState(true);

    constrainFreqs(text->getValueAsInt());

    event.Skip();
}

void FilesPanel::OnCheckboxFileSltcrdClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLTCRD));
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_CSUFRQ));
  ewxStaticText *label = ((ewxStaticText*)FindWindow(wxID_STATIC_FILE_CSUFRQ));
  if (check->GetValue()) {
    input->Enable(true);
    label->Enable(true);
  } else {
    input->Disable();
    label->Disable();
  }
  getFilesModel()->setCoordsSolutePrint(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnTextctrlFileCsufrqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_CSUFRQ));
    int soluteFrq = text->getValueAsInt();
    FilesModel *model = getFilesModel();
    int oldFrq = model->getCoordsSoluteFreq();
    model->setCoordsSoluteFreq(soluteFrq);

    if (event.GetId()!=0 && soluteFrq!=oldFrq) {
      string msg = "Would you like to set all trajectory file frequencies"
                   " and the property file frequency to this value?";
      ewxMessageDialog prompt(this, msg.c_str(), "File Frequency Settings",
                              wxYES_NO);
      if (prompt.ShowModal() == wxID_YES) {
        text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_CSVFRQ));
        text->setValueAsInt(soluteFrq);
        model->setCoordsSolventFreq(soluteFrq);

        text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_VSUFRQ));
        text->setValueAsInt(soluteFrq);
        model->setVelocitySoluteFreq(soluteFrq);

        text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_VSVFRQ));
        text->setValueAsInt(soluteFrq);
        model->setVelocitySolventFreq(soluteFrq);

        text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_FSUFRQ));
        text->setValueAsInt(soluteFrq);
        model->setForcesSoluteFreq(soluteFrq);

        text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_FSVFRQ));
        text->setValueAsInt(soluteFrq);
        model->setForcesSolventFreq(soluteFrq);

        text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_PRPFRQ));
        text->setValueAsInt(soluteFrq);
        model->setPropertyFreq(soluteFrq);

        text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_RSTFRQ));
        text->setValueAsInt(soluteFrq);
        model->setRestartFreq(soluteFrq);

        // This is pretty much a hack since we don't know why it works, but
        // without it there are "false positives" with changes being made
        // when the user was just mousing around and didn't change any values.
        TransferDataFromWindow();
      }
    }
    p_helper->setSaveState(true);
    event.Skip();
}

void FilesPanel::OnCheckboxFileSlvcrdClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLVCRD));
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_CSVFRQ));
  ewxStaticText *label = ((ewxStaticText*)FindWindow(wxID_STATIC_FILE_CSVFRQ));
  if (check->GetValue()) {
    input->Enable(true);
    label->Enable(true);
  } else {
    input->Disable();
    label->Disable();
  }
  getFilesModel()->setCoordsSolventPrint(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnTextctrlFileCsvfrqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_CSVFRQ));
    getFilesModel()->setCoordsSolventFreq(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void FilesPanel::OnCheckboxFileSltvelClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLTVEL));
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_VSUFRQ));
  ewxStaticText *label = ((ewxStaticText*)FindWindow(wxID_STATIC_FILE_VSUFRQ));
  if (check->GetValue()) {
    input->Enable(true);
    label->Enable(true);
  } else {
    input->Disable();
    label->Disable();
  }
  getFilesModel()->setVelocitySolutePrint(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnTextctrlFileVsufrqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_VSUFRQ));
    getFilesModel()->setVelocitySoluteFreq(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void FilesPanel::OnCheckboxFileSlvvelClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLVVEL));
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_VSVFRQ));
  ewxStaticText *label = ((ewxStaticText*)FindWindow(wxID_STATIC_FILE_VSVFRQ));
  if (check->GetValue()) {
    input->Enable(true);
    label->Enable(true);
  } else {
    input->Disable();
    label->Disable();
  }
  getFilesModel()->setVelocitySolventPrint(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnTextctrlFileVsvfrqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_VSVFRQ));
    getFilesModel()->setVelocitySolventFreq(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void FilesPanel::OnCheckboxFileSltfrcClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLTFRC));
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_FSUFRQ));
  ewxStaticText *label = ((ewxStaticText*)FindWindow(wxID_STATIC_FILE_FSUFRQ));
  if (check->GetValue()) {
    input->Enable(true);
    label->Enable(true);
  } else {
    input->Disable();
    label->Disable();
  }
  getFilesModel()->setForcesSolutePrint(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnTextctrlFileFsufrqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_FSUFRQ));
    getFilesModel()->setForcesSoluteFreq(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void FilesPanel::OnCheckboxFileSlvfrcClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLVFRC));
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_FSVFRQ));
  ewxStaticText *label = ((ewxStaticText*)FindWindow(wxID_STATIC_FILE_FSVFRQ));
  if (check->GetValue()) {
    input->Enable(true);
    label->Enable(true);
  } else {
    input->Disable();
    label->Disable();
  }
  getFilesModel()->setForcesSolventPrint(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnTextctrlFileFsvfrqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_FSVFRQ));
    getFilesModel()->setForcesSolventFreq(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void FilesPanel::OnCheckboxFileUseavgClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_USEAVG));
    getFilesModel()->setAvgProperties(check->GetValue());
    p_helper->setSaveState(true);
    event.Skip();
}

void FilesPanel::OnTextctrlFilePrpfrqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_PRPFRQ));
    getFilesModel()->setPropertyFreq(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void FilesPanel::OnCheckboxFilePrtmdClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_PRTMD));
  ewxStaticText *label = ((ewxStaticText*)FindWindow(wxID_STATIC_FILE_MDFREQ));
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_MDFREQ));
  wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_FILE_MDOPT));
  if (check->GetValue()) {
    label->Enable(true);
    text->Enable(true);
    panel->Enable(true);
  } else {
    label->Disable();
    text->Disable();
    panel->Disable();
  }
  getFilesModel()->setPrintMD(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnTextctrlFileMdfreqEnter( wxCommandEvent& event )
{
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_MDFREQ));
  int ival = atoi(input->GetValue().c_str());
  getFilesModel()->setPrintMDFreq(ival);
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnCheckboxFileEsolClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_ESOL));
  getFilesModel()->setSolventEnergy(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnCheckboxFileEbondClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_EBOND));
  getFilesModel()->setSoluteEnergy(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnCheckboxFileStatClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_STAT));
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_STFREQ));
  ewxStaticText *label = ((ewxStaticText*)FindWindow(wxID_STATIC_FILE_STFREQ));
  if (check->GetValue()) {
    input->Enable(true);
    label->Enable(true);
  } else {
    input->Disable();
    label->Disable();
  }
  getFilesModel()->setPrintStat(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnTextctrlFileStfreqEnter( wxCommandEvent& event )
{
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_STFREQ));
  int ival = atoi(input->GetValue().c_str());
  getFilesModel()->setStatFreq(ival);
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnCheckboxFilePmfClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_PMF));
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_PMFFREQ));
  ewxStaticText *label = ((ewxStaticText*)FindWindow(wxID_STATIC_FILE_PMFFREQ));
  if (check->GetValue()) {
    input->Enable(true);
    label->Enable(true);
  } else {
    input->Disable();
    label->Disable();
  }
  getFilesModel()->setPmfData(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnTextctrlFilePmffreqEnter( wxCommandEvent& event )
{
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_PMFFREQ));
  int ival = atoi(input->GetValue().c_str());
  getFilesModel()->setPmfFreq(ival);
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnCheckboxFileSubondClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SUBOND));
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_SEFREQ));
  ewxStaticText *label = ((ewxStaticText*)FindWindow(wxID_STATIC_FILE_SEFREQ));
  if (check->GetValue()) {
    input->Enable(true);
    label->Enable(true);
  } else {
    input->Disable();
    label->Disable();
  }
  getFilesModel()->setSoluteBonds(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnTextctrlFileSefreqEnter( wxCommandEvent& event )
{
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_SEFREQ));
  int ival = atoi(input->GetValue().c_str());
  getFilesModel()->setBondFreq(ival);
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnCheckboxFileFrcClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_FRC));
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_SFFREQ));
  ewxStaticText *label = ((ewxStaticText*)FindWindow(wxID_STATIC_FILE_SFFREQ));
  if (check->GetValue()) {
    input->Enable(true);
    label->Enable(true);
  } else {
    input->Disable();
    label->Disable();
  }
  getFilesModel()->setSoluteForces(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnTextctrlFileSffreqEnter( wxCommandEvent& event )
{
  ewxTextCtrl *input = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FILE_SFFREQ));
  int ival = atoi(input->GetValue().c_str());
  getFilesModel()->setForceFreq(ival);
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnCheckboxFileTopClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_TOP));
  wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_FILE_TOPOPT));
  if (check->GetValue()) {
    panel->Enable(true);
  } else {
    panel->Disable();
  }
  getFilesModel()->setPrintTop(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnCheckboxFileNbondClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_NBOND));
  getFilesModel()->setNbondPar(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnCheckboxFileSlvparClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLVPAR));
  getFilesModel()->setSolventBondPar(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}

void FilesPanel::OnCheckboxFileSluparClick( wxCommandEvent& event )
{
  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_FILE_SLUPAR));
  getFilesModel()->setSoluteBondPar(check->GetValue());
  p_helper->setSaveState(true);
  event.Skip();
}
