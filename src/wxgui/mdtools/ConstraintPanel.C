
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ConstraintPanel.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
  using std::cout;
  using std::endl;

#include "util/StringConverter.H"
#include "util/STLUtil.H"
#include "util/JMSPublisher.H"
#include "util/JMSMessage.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxRegexpValidator.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxUnitHelper.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/MDPanelHelper.H"
#include "wxgui/WxFeedback.H"

#include "tdat/ConstraintModel.H"
#include "tdat/FragmentSummary.H"

#include "wxgui/ConstraintPanel.H"

static const wxString ATOMREGEXP = "[0-9]+:_[a-z,A-Z]+";

/*!
 * ConstraintPanel constructors
 */

ConstraintPanel::ConstraintPanel(MDPanelHelper *helper )
{
   p_helper = helper;
}

ConstraintPanel::ConstraintPanel( MDPanelHelper *helper, wxWindow* parent,
                                  wxWindowID id, const wxPoint& pos,
                                  const wxSize& size, long style )
                                  : ConstraintPanelGUI(parent, id, pos, size, style)
{
   p_helper = helper;
   p_fragSummary = (FragmentSummary*)0;
}

ConstraintPanel::~ConstraintPanel()
{
   // don't delete
   p_helper = 0;
}

void ConstraintPanel::setConstraintModel(ConstraintModel *model)
{
  p_model = model;
}

ConstraintModel* ConstraintPanel::getConstraintModel()
{
  return p_model;
}

void ConstraintPanel::setFragmentSummary(FragmentSummary *fragSum)
{
  p_fragSummary = fragSum;
}

void ConstraintPanel::initializeGUI()
{
  ewxTextCtrl *text = 0;
  ConstraintModel *model = getConstraintModel();
  ewxStaticText *label = 0;
  ewxNumericValidator validator;

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_SLVITER));
  validator.setHardRange("[1,)");
  validator.setValue(model->MAXSLVITER);
//  text->SetValidator(ewxNumericValidator(1,10000,model->MAXSLVITER));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_SLVTOL));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->SLVTOL);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000.0,model->SLVTOL));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_CONST_SLVTOL_UNITS));
  text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_SLUITER));
  validator.setHardRange("[0,)");
  validator.setValue(model->MAXSLUITER);
//  text->SetValidator(ewxNumericValidator(1,10000,model->MAXSLUITER));
  text->SetValidator(validator);
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_SLUTOL));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->SLUTOL);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000.0,model->SLUTOL));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_CONST_SLUTOL_UNITS));
  text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_RADIUS));
  validator.setHardRange("(0.0,)");
  validator.setValue(model->RADIUS);
//  text->SetValidator(ewxNumericValidator(1.0e-10,1000.0,model->RADIUS));
  text->SetValidator(validator);
  label = ((ewxStaticText*)FindWindow(wxID_STATIC_CONST_RADIUS_UNITS));
  text->setUnitHelper( new ewxUnitHelper(DEFAULT_UNIT_FAMILY, "Length", label) );
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_ATCENT));
  text->SetValidator( ewxRegexpValidator(ATOMREGEXP, "") );

}

void ConstraintPanel::showConstraintPanel()
{
  ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_CONST_SELOPT));
  if (choice->GetSelection() == 0) {
    p_constraintVSizer->Show(p_constrainResidue, true, true);
    p_constraintVSizer->Show(p_constrainRadius, false, true);
  } else {
    p_constraintVSizer->Show(p_constrainResidue, false, true);
    p_constraintVSizer->Show(p_constrainRadius, true, true);
  }
  p_constraintVSizer->Layout();
}

/**
 * Refresh widgets in ConstrainPanel based on value in ConstraintModel.
 */
void ConstraintPanel::refreshGUI()
{
  // Change to default units since the model is always in these units
  ewxWindowUtils::setUnitFamily(this, DEFAULT_UNIT_FAMILY);

  ConstraintModel *model = getConstraintModel();
  ewxCheckBox *check;
  ewxChoice *choice;
  ewxTextCtrl *text;

  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CONST_USESHK));
  check->SetValue(model->getUseSHAKE());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_SLVITER));
  text->setValueAsInt(model->getMaxSolventIter());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_SLVTOL));
  text->setValueAsDouble(model->getSolventTol());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_SLUITER));
  text->setValueAsInt(model->getMaxSoluteIter());
  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_SLUTOL));
  text->setValueAsDouble(model->getSoluteTol());
  choice = ((ewxChoice*)FindWindow(ID_CHOICE_CONST_SHKOPT));
  choice->SetSelection(model->getSHAKEOpt());
  choice = ((ewxChoice*)FindWindow(ID_CHOICE_CONST_FIXOPT));
  choice->SetSelection(model->getFixOption());
  choice = ((ewxChoice*)FindWindow(ID_CHOICE_CONST_SELOPT));
  choice->SetSelection(model->getSelectOption());
  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CONST_HONLY));
  check->SetValue(model->getHeavyAtomsOnly());
  check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CONST_PERM));
  check->SetValue(model->getPermanentOption());

  //Restore fix atom commands
  vector<string> cmds;
  cmds = model->getFixAtomCmds();
  if (cmds.size() > 0) {
    ewxListBox *box = ((ewxListBox*)FindWindow(ID_LISTBOX_CONST_FIXAT));
    box->Clear();
    for (size_t i=0; i<cmds.size(); i++) {
      box->Append(cmds[i].c_str());
    }
  }

  wxCommandEvent event;
  OnCheckboxConstUseshkClick( event );
  OnChoiceConstFixoptSelected( event );
  OnChoiceConstSeloptSelected( event );

  // Now change to user prefered units
  ewxWindowUtils::processGlobalPreferenceChange(this);

  // Now make sure all of the validators contain the forced values above
  TransferDataFromWindow();
}

/**
 * Send message(s) to request startup of builder
 */
void ConstraintPanel::requestBuilder()
{
  JMSPublisher publisher("MDConstraints");
  JMSMessage *msg = publisher.newMessage();
  msg->addProperty("action","start");
  publisher.publish("ecce_activity",*msg);
  delete msg;

  string url = getConstraintModel()->getURL();
  Target gateway(GATEWAY, "");
  msg = publisher.newMessage(gateway);

  publisher.invoke(*msg, BUILDER, 0, url, "noop");

  delete msg;
}

/**
 * Event handlers for SHAKE parameters
 */

void ConstraintPanel::OnCheckboxConstUseshkClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CONST_USESHK));
    wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_CONST_SHAKE));
    getConstraintModel()->setUseSHAKE(check->GetValue());
    p_helper->setSaveState(true);
    // enable or disable remaining shake options
    if (check->GetValue()) {
      panel->Enable(true);
    } else {
      panel->Disable();
    }
    event.Skip();
}

void ConstraintPanel::OnTextctrlConstSlviterEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_SLVITER));
    getConstraintModel()->setMaxSolventIter(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void ConstraintPanel::OnTextctrlConstSlvtolEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_SLVTOL));
    getConstraintModel()->setSolventTol(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void ConstraintPanel::OnTextctrlConstSluiterEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_SLUITER));
    getConstraintModel()->setMaxSoluteIter(text->getValueAsInt());
    p_helper->setSaveState(true);
    event.Skip();
}

void ConstraintPanel::OnTextctrlConstSlutolEnter( wxCommandEvent& event )
{
    ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_SLUTOL));
    getConstraintModel()->setSoluteTol(text->getBaseValueAsDouble());
    p_helper->setSaveState(true);
    event.Skip();
}

void ConstraintPanel::OnChoiceConstShkoptSelected( wxCommandEvent& event )
{
    ewxChoice *choice = ((ewxChoice*)FindWindow(ID_CHOICE_CONST_SHKOPT));
    getConstraintModel()->setSHAKEOpt(choice->GetSelection());
    p_helper->setSaveState(true);
    event.Skip();
}

/**
 * Event handlers for fixed atom parameters
 */

void ConstraintPanel::OnCheckboxConstHonlyClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CONST_HONLY));
    getConstraintModel()->setHeavyAtomsOnly(check->GetValue());
    p_helper->setSaveState(true);
    event.Skip();
} 

void ConstraintPanel::OnCheckboxConstPermClick( wxCommandEvent& event )
{
    ewxCheckBox *check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CONST_PERM));
    getConstraintModel()->setPermanentOption(check->GetValue());
    p_helper->setSaveState(true);
    event.Skip();
} 

void ConstraintPanel::OnButtonConstFixaddClick( wxCommandEvent& event )
{
    string cmd;
    ewxTextCtrl *text;
    ewxChoice *choice;
    ewxRadioButton *radio;
    ewxCheckBox *check;
    choice = ((ewxChoice*)FindWindow(ID_CHOICE_CONST_FIXOPT));
    int opt = choice->GetSelection();
    bool constrainSolute;
    bool validCmd = true;
    if (opt == 0) {
      cmd = "fix solvent";
      constrainSolute = false;
    } else if (opt == 1) {
      cmd = "fix solute";
      constrainSolute = true;
    } else if (opt == 2) {
      cmd = "free solvent";
      constrainSolute = false;
    } else if (opt == 3) {
      cmd = "free solute";
      constrainSolute = true;
    }

    choice = ((ewxChoice*)FindWindow(ID_CHOICE_CONST_SELOPT));
    opt = choice->GetSelection();
    if (opt == 0) {
      int lower;
      bool validInt;
      text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_MINRES));
      validInt = StringConverter::toInt(text->GetValue().c_str(),lower);
      if (validInt) {
        cmd.append(" ");
        cmd.append(text->GetValue().c_str());
        text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_MAXRES));
        cmd.append(" ");
        cmd.append(text->GetValue().c_str());
      }
      if (constrainSolute) {
        check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CONST_HONLY));
        if (check->GetValue()) {
          //cout<<"Current value of validInt "<<validInt<<endl;
          if (!validInt) {
            //cout<<"Add zero to heavy"<<endl;
            cmd.append(" 0");
          }
          cmd.append(" heavy");
        }
      }
    } else {
      radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_CONST_RADIN));
      if (radio->GetValue()) {
        cmd.append(" within");
      } else {
        cmd.append(" beyond");
      }
      text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_RADIUS));
      cmd.append(" ");
      cmd.append(text->GetBaseValue().c_str());
      text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_ATCENT));
      cmd.append(" ");
      string atm = text->GetValue().c_str();
      STLUtil::stripLeadingAndTrailingWhiteSpace(atm);
      if (atm.size() < 1) {
        validCmd = false;
      }
      cmd.append(atm.c_str());
    }
    check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CONST_PERM));
    if (check->GetValue()) {
      cmd.append(" permanent");
    }
    if (validCmd) {
      ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_CONST_FIXAT));
      list->Append(cmd.c_str());
      getConstraintModel()->appendFixAtomCmd(cmd);
      p_helper->setSaveState(true);
      p_helper->getFeedback().setMessage("Atom center not specified",
                                         WxFeedback::ERROR);
    }
    event.Skip();
}

void ConstraintPanel::OnButtonConstFixdelClick( wxCommandEvent& event )
{
    ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_CONST_FIXAT));
    int index = list->GetSelection();
    if (index != wxNOT_FOUND) {
      list->Delete(index);
      getConstraintModel()->deleteFixAtomCmd(index);
      if (index > 0) {
        list->SetSelection(index-1);
      }
      p_helper->setSaveState(true);
    }
    event.Skip();
}

void ConstraintPanel::OnChoiceConstFixoptSelected( wxCommandEvent& event )
{
    ewxChoice *choice;
    ewxCheckBox *check1;
    ewxCheckBox *check2;
    choice = ((ewxChoice*)FindWindow(ID_CHOICE_CONST_FIXOPT));
    int opt = choice->GetSelection();
    getConstraintModel()->setFixOption(opt);
    check1 = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CONST_HONLY));
    check2 = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CONST_PERM));
    if (opt == 0 || opt == 2) {
      check1->Disable();
      check2->Disable();
    } else {
      check1->Enable(true);
      check2->Enable(true);
    }
    p_helper->setSaveState(true);
    event.Skip();
}

void ConstraintPanel::OnChoiceConstSeloptSelected( wxCommandEvent& event )
{
    ewxChoice *choice;
    ewxButton *button;
    
    choice = ((ewxChoice*)FindWindow(ID_CHOICE_CONST_SELOPT));
    button = ((ewxButton*)FindWindow(ID_BUTTON_CONST_GETSEL));
    getConstraintModel()->setSelectOption(choice->GetSelection());
    string tip;
    if (choice->GetSelection() == 0) {
      p_constrainResidue->Enable(true);
      p_constrainRadius->Disable();
      tip = "Click this button to use selected range in Builder";
      tip.append(" to populate residue fields");
    } else { 
      p_constrainResidue->Disable();
      p_constrainRadius->Enable(true);
      tip = "Click this button to use selected atom in Builder";
      tip.append(" as central atom");
    }
    button->SetToolTip(tip.c_str());
    showConstraintPanel();
    p_helper->setSaveState(true);
    event.Skip();
}

void ConstraintPanel::OnButtonConstGetselClick( wxCommandEvent& event )
{
    ewxChoice *choice;
    ewxTextCtrl *text;
    choice = ((ewxChoice*)FindWindow(ID_CHOICE_CONST_SELOPT));
    requestBuilder();
    if (p_fragSummary == (FragmentSummary*)0) return;
    if (choice->GetSelection() == 0) {
      int ires1, ires2,ires3;
      if (p_fragSummary->getNumSelected() == 1) {
        ires1 = p_fragSummary->getParent(0);
        ires2 = ires1;
      } else if (p_fragSummary->getNumSelected() == 2 ||
                 p_fragSummary->getNumSelected() > 3) {
        ires1 = p_fragSummary->getParent(0);
        ires2 = p_fragSummary->getParent(1);
        if (ires1 > ires2) {
          int itmp = ires1;
          ires1 = ires2;
          ires2 = itmp;
        }
      } else if (p_fragSummary->getNumSelected() == 3) {
        ires1 = p_fragSummary->getParent(0);
        ires2 = p_fragSummary->getParent(1);
        ires3 = p_fragSummary->getParent(2);
        int imin = ires1;
        int imax = ires1;
        imin = ires1;
        if (ires2 < imin) imin = ires2;
        if (ires3 < imin) imin = ires3;
        if (ires2 > imax) imax = ires2;
        if (ires3 > imax) imax = ires3;
        ires1 = imin;
        ires2 = imax;
      } else {
        ires1 = 0;
        ires2 = 0;
        p_helper->getFeedback().setMessage("No atoms selected",
                                            WxFeedback::ERROR);
      }
      if (ires1 > 0 && ires2 > 0) {
        text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_MINRES));
        text->setValueAsInt(ires1);
        text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_MAXRES));
        text->setValueAsInt(ires2);
      }
    } else {
      if (p_fragSummary->getNumSelected() == 1) {
        text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_ATCENT));
        text->SetValue(p_fragSummary->getAtomString(0));
      } else {
        p_helper->getFeedback().setMessage("Incorrect number of atoms selected",
                                            WxFeedback::ERROR);
      }
    }
    event.Skip();
}

void ConstraintPanel::OnButtonConstClearClick( wxCommandEvent& event )
{
    ewxTextCtrl *text;
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_MINRES));
    text->Clear();
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_MAXRES));
    text->Clear();
    text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CONST_ATCENT));
    text->Clear();
    event.Skip();
}
