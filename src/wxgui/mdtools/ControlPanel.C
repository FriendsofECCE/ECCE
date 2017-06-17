#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ControlPanel.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
  using std::cout;
  using std::endl;

#include "tdat/ControlModel.H"

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxOpenCloseToggle.H"
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/MDPanelHelper.H"

#include "wxgui/ControlPanel.H"

/*!
 * ControlPanel constructors
 */

ControlPanel::ControlPanel(MDPanelHelper *helper ) : ControlPanelGUI()
{
   p_helper = helper;
}

ControlPanel::ControlPanel( MDPanelHelper *helper,
                            wxWindow* parent, wxWindowID id,
                            const wxPoint& pos, const wxSize& size,
                            long style ) : ControlPanelGUI(parent,
                            id, pos, size, style)
{
   p_helper = helper;
}

ControlPanel::~ControlPanel( )
{
   // don't delete
   p_helper = 0;
}

void ControlPanel::setControlModel(ControlModel *model)
{
  p_model = model;
}

ControlModel* ControlPanel::getControlModel() const
{
  return p_model;
}

void ControlPanel::initializeGUI()
{
  ewxTextCtrl *text;
  ControlModel *model = getControlModel();
  ewxNumericValidator validator;

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_STRNGTH));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->COUPLING);
//  text->SetValidator(ewxNumericValidator(1.0e-10,100.0,model->COUPLING));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_CFREQ));
  validator.setHardRange("[1,)");
  validator.setValue(model->BALANCEFREQ);
//  text->SetValidator(ewxNumericValidator(1,10000,model->BALANCEFREQ));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_PX));
  validator.setHardRange("[1,)");
  validator.setValue(model->XPROC);
//  text->SetValidator(ewxNumericValidator(1,10000,model->XPROC));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_PY));
  validator.setHardRange("[1,)");
  validator.setValue(model->YPROC);
//  text->SetValidator(ewxNumericValidator(1,10000,model->YPROC));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_PZ));
  validator.setHardRange("[1,)");
  validator.setValue(model->ZPROC);
//  text->SetValidator(ewxNumericValidator(1,10000,model->ZPROC));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_CX));
  validator.setHardRange("[1,)");
  validator.setValue(model->XCELL);
//  text->SetValidator(ewxNumericValidator(1,10000,model->XCELL));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_CY));
  validator.setHardRange("[1,)");
  validator.setValue(model->YCELL);
//  text->SetValidator(ewxNumericValidator(1,10000,model->YCELL));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_CZ));
  validator.setHardRange("[1,)");
  validator.setValue(model->ZCELL);
//  text->SetValidator(ewxNumericValidator(1,10000,model->ZCELL));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_MAXSLV));
  validator.setHardRange("[1,)");
  validator.setValue(model->MAXSLV);
//  text->SetValidator(ewxNumericValidator(1,1000000,model->MAXSLV));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_MAXSLU));
  validator.setHardRange("[1,)");
  validator.setValue(model->MAXSLU);
//  text->SetValidator(ewxNumericValidator(1,1000000,model->MAXSLU));
  text->SetValidator(validator);
}

void ControlPanel::refreshGUI()
{
  // Change to default units since the model is always in these units
  ewxWindowUtils::setUnitFamily(this, DEFAULT_UNIT_FAMILY);

  ewxTextCtrl *text;
  ewxCheckBox *check;
  ewxChoice *choice;
  ControlModel *model = getControlModel();

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_USELOAD));
  check->SetValue(model->getLoadBalance());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_CELLSZ));
  check->SetValue(model->getBalanceCellSize());
  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_ZONLY));
  check->SetValue(model->getBalanceZOnly());
  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_RDSTRB));
  check->SetValue(model->getRedistributePairs());

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_STRNGTH));
  text->setValueAsDouble(model->getCouplingStrength());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_CFREQ));
  text->setValueAsInt(model->getAdjustFrequency());

  choice = ((ewxChoice*)FindWindow(ID_CHOICE_CNTRL_TSYNCH));
  choice->SetSelection(model->getSynchOption());

  ewxOpenCloseToggle *toggle
    = ((ewxOpenCloseToggle*)FindWindow(ID_TOGGLE_CNTRL_CNFG));
  toggle->setState(model->getAdvancedConfigOpt());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_PCNFG));
  check->SetValue(model->getProcessorConfig());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_PX));
  text->setValueAsInt(model->getXProc());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_PY));
  text->setValueAsInt(model->getYProc());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_PZ));
  text->setValueAsInt(model->getZProc());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_CCNFG));
  check->SetValue(model->getCellConfig());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_CX));
  text->setValueAsInt(model->getXCell());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_CY));
  text->setValueAsInt(model->getYCell());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_CZ));
  text->setValueAsInt(model->getZCell());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_MAXSLV));
  check->SetValue(model->getUseMaxSolvent());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_MAXSLV));
  text->setValueAsInt(model->getMaxSolvent());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_MAXSLU));
  check->SetValue(model->getUseMaxSolute());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_MAXSLU));
  text->setValueAsInt(model->getMaxSolute());

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_PACK));
  check->SetValue(model->getPackOption());

  // Now change to user prefered units
  ewxWindowUtils::processGlobalPreferenceChange(this);

  wxCommandEvent event;
  OnCheckboxCntrlUseloadClick( event );
  OnCheckboxCntrlCellszClick( event );
  OnCheckboxCntrlRdstrbClick( event );
  OnToggleCntrlCnfgClick( event );
  OnCheckboxCntrlPcnfgClick( event );
  OnCheckboxCntrlCcnfgClick( event );
  OnCheckboxCntrlMaxslvClick( event );
  OnCheckboxCntrlMaxsluClick( event );

  // Now make sure all of the validators contain the forced values above
  TransferDataFromWindow();
}

void ControlPanel::showControlPanel()
{
  ewxOpenCloseToggle *toggle
    = ((ewxOpenCloseToggle*)FindWindow(ID_TOGGLE_CNTRL_CNFG));
  if (toggle->isOpen()) {
    p_configVSizer->Show(p_configPanel,true,true);
  } else {
    p_configVSizer->Show(p_configPanel,false,true);
  }
  p_configVSizer->Layout();

  ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_USELOAD));
  if (check->GetValue()) {
    p_controlVSizer->Show(p_balancePanel,true,true);
  } else {
    p_controlVSizer->Show(p_balancePanel,false,true);
  }
  p_controlVSizer->Layout();
}

/**
 * Control panel event handlers
 */

void ControlPanel::OnCheckboxCntrlUseloadClick( wxCommandEvent& event )
{
    wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_CNTRL_ADJUST));
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_USELOAD));
    if (check->GetValue()) {
      panel->Enable(true);
    } else {
      panel->Disable();
    }
    getControlModel()->setLoadBalance(check->GetValue());
    showControlPanel();
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnCheckboxCntrlCellszClick( wxCommandEvent& event )
{
    ewxCheckBox *check
      = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_CELLSZ));
    getControlModel()->setBalanceCellSize(check->GetValue());
    if (check->GetValue()) {
      ewxCheckBox *check2
        = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_ZONLY));
      check2->Enable(true);
      wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_CNTRL_STRNGTH));
      panel->Enable(true);
    } else {
      ewxCheckBox *check2
        = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_ZONLY));
      check2->Disable();
      wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_CNTRL_STRNGTH));
      panel->Disable();
    }
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnCheckboxCntrlZonlyClick( wxCommandEvent& event )
{
    ewxCheckBox *check
      = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_ZONLY));
    getControlModel()->setBalanceZOnly(check->GetValue());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnCheckboxCntrlRdstrbClick( wxCommandEvent& event )
{
    ewxCheckBox *check
      = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_RDSTRB));
    getControlModel()->setRedistributePairs(check->GetValue());
    wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_CNTRL_RDPAR));
    if (check->GetValue()) {
      panel->Enable(true);
    } else {
      panel->Disable();
    }
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnTextctrlCntrlStrngthEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_STRNGTH));
    getControlModel()->setCouplingStrength(text->getValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnTextctrlCntrlCfreqEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_CFREQ));
    getControlModel()->setAdjustFrequency(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnChoiceCntrlTsynchSelected( wxCommandEvent& event )
{
    ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_CNTRL_TSYNCH));
    getControlModel()->setSynchOption(choice->GetSelection());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnToggleCntrlCnfgClick( wxCommandEvent& event )
{
    ewxOpenCloseToggle* toggle
      = ((ewxOpenCloseToggle*)FindWindow(ID_TOGGLE_CNTRL_CNFG));
    getControlModel()->setAdvancedConfigOpt(toggle->isOpen());
    showControlPanel();
    p_helper->setSaveState(true);
    event.Skip();
}
void ControlPanel::OnCheckboxCntrlPcnfgClick( wxCommandEvent& event )
{
    wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_CNTRL_NPROC));
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_PCNFG));
    if (check->GetValue()) {
      panel->Enable(true);
    } else {
      panel->Disable();
    }
    getControlModel()->setProcessorConfig(check->GetValue());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnTextctrlCntrlPxEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_PX));
    getControlModel()->setXProc(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnTextctrlCntrlPyEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_PY));
    getControlModel()->setYProc(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnTextctrlCntrlPzEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_PZ));
    getControlModel()->setZProc(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnCheckboxCntrlCcnfgClick( wxCommandEvent& event )
{
    wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_CNTRL_NCELL));
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_CCNFG));
    if (check->GetValue()) {
      panel->Enable(true);
    } else {
      panel->Disable();
    }
    getControlModel()->setCellConfig(check->GetValue());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnTextctrlCntrlCxEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_CX));
    getControlModel()->setXCell(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnTextctrlCntrlCyEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_CY));
    getControlModel()->setYCell(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnTextctrlCntrlCzEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_CZ));
    getControlModel()->setZCell(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnCheckboxCntrlMaxslvClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_MAXSLV));
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_MAXSLV));
    if (check->GetValue()) {
      text->Enable(true);
    } else {
      text->Disable();
    }
    getControlModel()->setUseMaxSolvent(check->GetValue());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnTextctrlCntrlMaxslvEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_MAXSLV));
    getControlModel()->setMaxSolvent(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnCheckboxCntrlMaxsluClick( wxCommandEvent& event )
{
    // Insert custom code here
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_MAXSLU));
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_MAXSLU));
    if (check->GetValue()) {
      text->Enable(true);
    } else {
      text->Disable();
    }
    getControlModel()->setUseMaxSolute(check->GetValue());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnTextctrlCntrlMaxsluEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CNTRL_MAXSLU));
    getControlModel()->setMaxSolute(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void ControlPanel::OnCheckboxCntrlPackClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CNTRL_PACK));
    getControlModel()->setPackOption(check->GetValue());
    p_helper->setSaveState(true);
    event.Skip();
}
