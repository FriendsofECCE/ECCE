/**
 * @file SelectionPanel.C
 */

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "SelectionPanel.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/STLUtil.H"

#include "tdat/TPerTab.H"

#include "viz/SelectCmd.H"
#include "viz/SelectRadiusCmd.H"
#include "viz/SelectBondDisCmd.H"
#include "viz/SelectAllCmd.H"

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxTextCtrl.H"

#include "wxviz/SelectionPanelGUI.H"
#include "wxviz/SelectionPanel.H"


IMPLEMENT_DYNAMIC_CLASS( SelectionPanel, SelectionPanelGUI )


/*!
 * SelectionPanel constructors
 */
SelectionPanel::SelectionPanel( )
  : SelectionPanelGUI(), Listener("SelectionPanel")
{
  subscribe();

  p_oldElement = "C";
  initializeGUI();
}


SelectionPanel::SelectionPanel( wxWindow* parent, wxWindowID id,
                                const wxPoint& pos, const wxSize& size,
                                long style )
  : SelectionPanelGUI(parent, id, pos, size, style), Listener("SelectionPanel")
{
  subscribe();

  p_oldElement = "C";
  initializeGUI();
}

/**
 *  Set validators etc. on panel widgets
 */
void SelectionPanel::initializeGUI()
{
  ewxTextCtrl *text;
  ewxNumericValidator validator(0.,1.,0.);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MINDIS));
  validator.setHardRange("[0.0,)");
  validator.setValue(0.0);
  text->SetValidator(validator);

  text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MAXDIS));
  validator.setHardRange("[0.0,)");
  validator.setValue(5.0);
  text->SetValidator(validator);
}

/**
 * Selection panel utility functions
 */
void SelectionPanel::atomSelection(bool clear, bool reverse)
{
  WxVizToolFW &fw = getFW();
  ewxRadioButton* radio1 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_AT_ELMNT);
  ewxRadioButton* radio2 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_AT_IDX);
  ewxRadioButton* radio3 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_AT_MOL);
  ewxRadioButton* radio4 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_AT_ALL);
  if (reverse) {
    Command *cmd = new SelectCmd("Reverse Selection", &fw.getSceneGraph());
    cmd->getParameter("reverse")->setBoolean(reverse);
    fw.execute(cmd);
  } else if (radio1->GetValue()) {
    Command *cmd = new SelectCmd("Select Element", &fw.getSceneGraph());
    cmd->getParameter("clear")->setBoolean(clear);
    string element = ((ewxComboBox*)FindWindow(ID_COMBOBOX_ELMNT))->GetValue().c_str();
    cmd->getParameter("element")->setString(element);
    fw.execute(cmd);
  } else if (radio2->GetValue()) {
    Command *cmd = new SelectCmd("Select Atom", &fw.getSceneGraph());
    cmd->getParameter("clear")->setBoolean(clear);
    int first = ((ewxSpinCtrl*)FindWindow(ID_SPINCTRL_FRST_AT))->GetValue();
    int last = ((ewxSpinCtrl*)FindWindow(ID_SPINCTRL_LAST_AT))->GetValue();
    cmd->getParameter("atom_range1")->setInteger(first<last?first:last);
    cmd->getParameter("atom_range2")->setInteger(first<last?last:first);
    fw.execute(cmd);
  } else if (radio3->GetValue()) {
    Command *cmd = new SelectCmd("Select Molecule", &fw.getSceneGraph());
    cmd->getParameter("clear")->setBoolean(clear);
    cmd->getParameter("molecule")->setBoolean(true);
    fw.execute(cmd);
  } else if (radio4->GetValue()) {
    Command *cmd = new SelectAllCmd("Select All", &fw.getSceneGraph());
    fw.execute(cmd);
  }
}

void SelectionPanel::residueSelection(bool clear, bool reverse)
{
  WxVizToolFW &fw = getFW();
  ewxRadioButton* radio1 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_ATYP);
  ewxRadioButton* radio2 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_ANAM);
  ewxRadioButton* radio3 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_RESNAM);
  ewxRadioButton* radio4 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_CHAIN);
  ewxRadioButton* radio5 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_FLRES);
  ewxRadioButton* radio6 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_SLVNT);
  ewxRadioButton* radio7 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_BBONE);
  ewxRadioButton* radio8 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_SCHAIN);
  if (reverse) {
    Command *cmd = new SelectCmd("Reverse Selection", &fw.getSceneGraph());
    cmd->getParameter("reverse")->setBoolean(reverse);
    fw.execute(cmd);
  } else if (radio1->GetValue()) {
    Command *cmd = new SelectCmd("Select Atom Type", &fw.getSceneGraph());
    cmd->getParameter("clear")->setBoolean(clear);
    string type =
      ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_RES_ATYP))->GetValue().c_str();
    cmd->getParameter("atom_type")->setString(type);
    fw.execute(cmd);
  } else if (radio2->GetValue()) {
    Command *cmd = new SelectCmd("Select Atom Name", &fw.getSceneGraph());
    cmd->getParameter("clear")->setBoolean(clear);
    string name =
      ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_RES_ANAM))->GetValue().c_str();
    cmd->getParameter("atom_name")->setString(name);
    fw.execute(cmd);
  } else if (radio3->GetValue()) {
    Command *cmd = new SelectCmd("Select Residue Name", &fw.getSceneGraph());
    cmd->getParameter("clear")->setBoolean(clear);
    string name =
      ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_RESNAM))->GetValue().c_str();
    cmd->getParameter("residue_name")->setString(name);
    fw.execute(cmd);
  } else if (radio4->GetValue()) {
    Command *cmd = new SelectCmd("Select Chain", &fw.getSceneGraph());
    cmd->getParameter("clear")->setBoolean(clear);
    string chains =
      ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_CHAIN))->GetValue().c_str();
    STLUtil::stripLeadingAndTrailingWhiteSpace(chains);
    vector<string> chainList;
    char buf[2];
    buf[1] = '\0';
    if (chains.length()>0) {
      for (size_t i=0; i<chains.length(); i++) {
        buf[0] = chains[i];
        chainList.push_back(buf);
      }
    }
    cmd->getParameter("chains")->setStringList(chainList);
    fw.execute(cmd);
  } else if (radio5->GetValue()) {
    Command *cmd = new SelectCmd("Select Full Residue", &fw.getSceneGraph());
    SGFragment *frag = fw.getSceneGraph().getFragment();
    cmd->getParameter("clear")->setBoolean(clear);
    if (frag->m_atomHighLight.size()>0) {
      cmd->getParameter("full_residue")->setInteger(frag->m_atomHighLight[0]);
    }
    fw.execute(cmd);
  } else if (radio6->GetValue()) {
    Command *cmd = new SelectCmd("Select Solvent", &fw.getSceneGraph());
    cmd->getParameter("clear")->setBoolean(clear);
    cmd->getParameter("solvent")->setBoolean(true);
    fw.execute(cmd);
  } else if (radio7->GetValue()) {
    Command *cmd = new SelectCmd("Select Backbone", &fw.getSceneGraph());
    cmd->getParameter("clear")->setBoolean(clear);
    cmd->getParameter("backbone")->setBoolean(true);
    fw.execute(cmd);
  } else if (radio8->GetValue()) {
    Command *cmd = new SelectCmd("Select Sidechains", &fw.getSceneGraph());
    cmd->getParameter("clear")->setBoolean(clear);
    cmd->getParameter("sidechains")->setBoolean(true);
    fw.execute(cmd);
  }
}

/**
 * Selection panel event handlers
 */
void SelectionPanel::OnComboboxElmntSelected( wxCommandEvent& event )
{
  OnComboboxElmntEnter( event );
}

void SelectionPanel::OnComboboxElmntEnter( wxCommandEvent& event )
{
  ewxComboBox *combo = (ewxComboBox*)FindWindow(ID_COMBOBOX_ELMNT);
  TPerTab pertab;
  if (pertab.isValid(combo->GetValue().c_str())) {
    p_oldElement = combo->GetValue().c_str();
  } else {
    combo->SetValue(p_oldElement.c_str());
  }
  OnComboboxElmntUpdated(event);
}

void SelectionPanel::OnComboboxElmntUpdated( wxCommandEvent& event )
{
  ewxRadioButton* radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_AT_ELMNT);
  radio->SetValue(true);
  event.Skip();
}

void SelectionPanel::OnSpinctrlFrstAtUpdated( wxSpinEvent& event )
{
  ewxRadioButton* radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_AT_IDX);
  radio->SetValue(true);
  ewxSpinCtrl *spin1 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_FRST_AT);
  ewxSpinCtrl *spin2 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_LAST_AT);
  int first = spin1->GetValue();
  int last = spin2->GetValue();
  if (last < first) {
    spin2->SetValue(first);
  }
  event.Skip();
}

void SelectionPanel::OnSpinctrlFrstAtTextUpdated( wxCommandEvent& event )
{
  ewxRadioButton* radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_AT_IDX);
  radio->SetValue(true);
  ewxSpinCtrl *spin1 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_FRST_AT);
  ewxSpinCtrl *spin2 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_LAST_AT);
  int first = spin1->GetValue();
  int last = spin2->GetValue();
  if (last < first) {
    spin2->SetValue(first);
  }
  event.Skip();
}

void SelectionPanel::OnSpinctrlLastAtUpdated( wxSpinEvent& event )
{
  ewxRadioButton* radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_AT_IDX);
  radio->SetValue(true);
  ewxSpinCtrl *spin1 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_FRST_AT);
  ewxSpinCtrl *spin2 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_LAST_AT);
  int first = spin1->GetValue();
  int last = spin2->GetValue();
  if (last < first) {
    spin1->SetValue(last);
  }
  event.Skip();
}

void SelectionPanel::OnSpinctrlLastAtTextUpdated( wxCommandEvent& event )
{
  ewxRadioButton* radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_AT_IDX);
  radio->SetValue(true);
  ewxSpinCtrl *spin1 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_FRST_AT);
  ewxSpinCtrl *spin2 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_LAST_AT);
  int first = spin1->GetValue();
  int last = spin2->GetValue();
  if (last < first) {
    spin1->SetValue(last);
  }
  event.Skip();
}

void SelectionPanel::OnAtomSelectClick( wxCommandEvent& event )
{
  atomSelection(true,false);
  event.Skip();
}

void SelectionPanel::OnAtomXselectClick( wxCommandEvent& event )
{
  atomSelection(false,false);
  event.Skip();
}

void SelectionPanel::OnAtomInverseClick( wxCommandEvent& event )
{
  atomSelection(false,true);
  event.Skip();
}

void SelectionPanel::OnAtomClearClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();
  Command *cmd = new SelectCmd("Clear Selection", &fw.getSceneGraph());
  cmd->getParameter("clear")->setBoolean(true);
  fw.execute(cmd);
  event.Skip();
}

void SelectionPanel::OnAtomAddGrpClick( wxCommandEvent& event )
{
  event.Skip();
}

void SelectionPanel::OnTextctrlResAtypUpdated( wxCommandEvent& event )
{
  ewxRadioButton* radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_ATYP);
  radio->SetValue(true);
  event.Skip();
}

void SelectionPanel::OnTextctrlResAnamUpdated( wxCommandEvent& event )
{
  ewxRadioButton* radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_ANAM);
  radio->SetValue(true);
  event.Skip();
}

void SelectionPanel::OnTextctrlResnamUpdated( wxCommandEvent& event )
{
  ewxRadioButton* radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_RESNAM);
  radio->SetValue(true);
  event.Skip();
}

void SelectionPanel::OnTextctrlChainUpdated( wxCommandEvent& event )
{
  ewxRadioButton* radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_CHAIN);
  radio->SetValue(true);
  event.Skip();
}

void SelectionPanel::OnButtonResSelectClick( wxCommandEvent& event )
{
  residueSelection(true,false);
  event.Skip();
}

void SelectionPanel::OnButtonResXselectClick( wxCommandEvent& event )
{
  residueSelection(false,false);
  event.Skip();
}

void SelectionPanel::OnButtonResInverseClick( wxCommandEvent& event )
{
  residueSelection(false,true);
  event.Skip();
}

void SelectionPanel::OnButtonResClearClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();
  Command *cmd = new SelectCmd("Clear Selection", &fw.getSceneGraph());
  cmd->getParameter("clear")->setBoolean(true);
  fw.execute(cmd);
  event.Skip();
}

void SelectionPanel::OnButtonResAddGrpClick( wxCommandEvent& event )
{
  event.Skip();
}

void SelectionPanel::OnTextctrlMindisUpdated( wxCommandEvent& event )
{
  ewxRadioButton *radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DISTANCE);
  radio->SetValue(true);
  event.Skip();
}

void SelectionPanel::OnTextctrlMindisEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text1 = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MINDIS);
  ewxTextCtrl *text2 = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MAXDIS);
  double mindis = text1->getValueAsDouble();
  double maxdis = text2->getValueAsDouble();
  if (maxdis < mindis) {
    text2->setValueAsDouble(mindis);
  }
  event.Skip();
}

void SelectionPanel::OnTextctrlMaxdisUpdated( wxCommandEvent& event )
{
  ewxRadioButton *radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DISTANCE);
  radio->SetValue(true);
  event.Skip();
}

void SelectionPanel::OnTextctrlMaxdisEnter( wxCommandEvent& event )
{
  ewxTextCtrl *text1 = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MINDIS);
  ewxTextCtrl *text2 = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MAXDIS);
  double mindis = text1->getValueAsDouble();
  double maxdis = text2->getValueAsDouble();
  if (maxdis < mindis) {
    text1->setValueAsDouble(maxdis);
  }
  event.Skip();
}

void SelectionPanel::OnSpinctrlMinbndUpdated( wxSpinEvent& event )
{
  ewxRadioButton *radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_BONDS);
  radio->SetValue(true);
  ewxSpinCtrl *spin1 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_MINBND);
  ewxSpinCtrl *spin2 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_MAXBND);
  int minbnd = spin1->GetValue();
  int maxbnd = spin2->GetValue();
  if (maxbnd < minbnd) {
    spin2->SetValue(minbnd);
  }
  event.Skip();
}

void SelectionPanel::OnSpinctrlMinbndTextUpdated( wxCommandEvent& event )
{
  ewxRadioButton *radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_BONDS);
  radio->SetValue(true);
  ewxSpinCtrl *spin1 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_MINBND);
  ewxSpinCtrl *spin2 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_MAXBND);
  int minbnd = spin1->GetValue();
  int maxbnd = spin2->GetValue();
  if (maxbnd < minbnd) {
    spin2->SetValue(minbnd);
  }
  event.Skip();
}

void SelectionPanel::OnSpinctrlMaxbndUpdated( wxSpinEvent& event )
{
  ewxRadioButton *radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_BONDS);
  radio->SetValue(true);
  ewxSpinCtrl *spin1 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_MINBND);
  ewxSpinCtrl *spin2 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_MAXBND);
  int minbnd = spin1->GetValue();
  int maxbnd = spin2->GetValue();
  if (maxbnd < minbnd) {
    spin1->SetValue(maxbnd);
  }
  event.Skip();
}

void SelectionPanel::OnSpinctrlMaxbndTextUpdated( wxCommandEvent& event )
{
  ewxRadioButton *radio = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_BONDS);
  radio->SetValue(true);
  ewxSpinCtrl *spin1 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_MINBND);
  ewxSpinCtrl *spin2 = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_MAXBND);
  int minbnd = spin1->GetValue();
  int maxbnd = spin2->GetValue();
  if (maxbnd < minbnd) {
    spin1->SetValue(maxbnd);
  }
  event.Skip();
}

void SelectionPanel::OnButtonDisSelectClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();
  ewxRadioButton *radio1 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DISTANCE);
  ewxRadioButton *radio2 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_BONDS);
  if (radio1->GetValue()) {
    Command *cmd = new SelectRadiusCmd("Distance Selection", &fw.getSceneGraph());
    ewxTextCtrl *text;
    ewxCheckBox *check;
    text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MINDIS);
    double min_dis = text->getValueAsDouble();
    text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MAXDIS);
    double max_dis = text->getValueAsDouble();
    check = (ewxCheckBox*)FindWindow(ID_CHECKBOX_FULL_RES);
    bool fullRes = check->GetValue();
    cmd->getParameter("min_radius")->setDouble(min_dis);
    cmd->getParameter("max_radius")->setDouble(max_dis);
    cmd->getParameter("residues")->setBoolean(fullRes);
    fw.execute(cmd);
  } else if (radio2->GetValue()) {
    Command *cmd = new SelectBondDisCmd("Bond Distance Selection", &fw.getSceneGraph());
    ewxSpinCtrl *spin;
    spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_MINBND);
    int imin = spin->GetValue();
    spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_MAXBND);
    int imax = spin->GetValue();
    cmd->getParameter("min_bond")->setInteger(imin);
    cmd->getParameter("max_bond")->setInteger(imax);
    fw.execute(cmd);
  }
  event.Skip();
}

void SelectionPanel::OnButtonDisXselectClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();
  ewxRadioButton *radio1 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DISTANCE);
  ewxRadioButton *radio2 = (ewxRadioButton*)FindWindow(ID_RADIOBUTTON_BONDS);
  if (radio1->GetValue()) {
    Command *cmd = new SelectRadiusCmd("Distance Selection", &fw.getSceneGraph());
    ewxTextCtrl *text;
    ewxCheckBox *check;
    text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MINDIS);
    double min_dis = text->getValueAsDouble();
    text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MAXDIS);
    double max_dis = text->getValueAsDouble();
    check = (ewxCheckBox*)FindWindow(ID_CHECKBOX_FULL_RES);
    bool fullRes = check->GetValue();
    cmd->getParameter("min_radius")->setDouble(min_dis);
    cmd->getParameter("max_radius")->setDouble(max_dis);
    cmd->getParameter("clear")->setBoolean(false);
    cmd->getParameter("residues")->setBoolean(fullRes);
    fw.execute(cmd);
  } else if (radio2->GetValue()) {
    Command *cmd = new SelectBondDisCmd("Bond Distance Selection", &fw.getSceneGraph());
    ewxSpinCtrl *spin;
    spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_MINBND);
    int imin = spin->GetValue();
    spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_MAXBND);
    int imax = spin->GetValue();
    cmd->getParameter("min_bond")->setInteger(imin);
    cmd->getParameter("max_bond")->setInteger(imax);
    cmd->getParameter("clear")->setBoolean(false);
    fw.execute(cmd);
  }
  event.Skip();
}


void SelectionPanel::OnButtonDisInverseClick( wxCommandEvent& event )
{
  atomSelection(false,true);
  event.Skip();
}


void SelectionPanel::OnButtonDisClearClick( wxCommandEvent& event )
{
  WxVizToolFW &fw = getFW();
  Command *cmd = new SelectCmd("Clear Selection", &fw.getSceneGraph());
  cmd->getParameter("clear")->setBoolean(true);
  fw.execute(cmd);
  event.Skip();
}


void SelectionPanel::eventMCB(const Event& event)
{
  
  string name = event.getName();
  string value = event.getValue();
  
  if (name == "ResidueChange" || name == "GeomChange") {
    SGFragment *frag = getFW().getSceneGraph().getFragment();
    p_residuePanel->Enable(frag->numResidues() != 0);
  }
}
