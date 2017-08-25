#include "wxgui/ewxTextCtrl.H"

#include "wxviz/ResidueIndexPrompt.H"

ResidueIndexPrompt::ResidueIndexPrompt()
  : ResidueIndexPromptGUI(),
    WxVizTool()
{
} 


ResidueIndexPrompt::ResidueIndexPrompt( wxWindow* parent, wxWindowID id,
        const wxString& caption, const wxPoint& pos, const wxSize& size,
        long style )
  : ResidueIndexPromptGUI(),
    WxVizTool()
{
  Create(parent, id, caption, pos, size, style);
}


bool ResidueIndexPrompt::Create( wxWindow* parent, wxWindowID id,
        const wxString& caption, const wxPoint& pos, const wxSize& size,
        long style )
{
  if (!ResidueIndexPromptGUI::Create(parent, id, caption, pos, size, style)) {
    wxFAIL_MSG( wxT("ResidueIndexPrompt creation failed") );
    return false;
  }

  return true;
}


ResidueIndexPrompt::~ResidueIndexPrompt()
{
}


bool ResidueIndexPrompt::isSelectResidue() const
{
  return isRadioSelected(ID_RADIOBUTTON_RESPROMPT_SELECT);
}


bool ResidueIndexPrompt::isDeleteResidue() const
{
  return isRadioSelected(ID_RADIOBUTTON_RESPROMPT_DELETE);
}


bool ResidueIndexPrompt::isInsertResidue() const
{
  return isRadioSelected(ID_RADIOBUTTON_RESPROMPT_NEW);
}


long ResidueIndexPrompt::getResidueIndex() const
{
  long ret = -1;
  if (isSelectResidue()) {
    wxChoice *select =
            dynamic_cast<wxChoice*>(FindWindow(ID_CHOICE_RES_SELECT));
    ret = select->GetSelection();
  } else if (isInsertResidue()) {
    wxChoice *insert =
            dynamic_cast<wxChoice*>(FindWindow(ID_CHOICE_RES_NEW));
    wxString selection = insert->GetStringSelection();
    if (selection == "beginning") {
      ret = 0;
    } else if (selection == "end") {
      ret = insert->GetCount() - 2; // beginning, end
    } else {
       // "   12 GLU" --> "   12" --> "12"
       selection = selection.BeforeLast(' ').AfterLast(' ');
       selection.ToLong(&ret);
    }
  }
  return ret;
}


string ResidueIndexPrompt::getResidueName() const
{
  ewxTextCtrl *txt =
          dynamic_cast<ewxTextCtrl*>(FindWindow(ID_TEXTCTRL_RESPROMPT_NAME));
  return txt->GetValue().c_str();
}


void ResidueIndexPrompt::setSelectResidue()
{
  ((wxRadioButton*)FindWindow(ID_RADIOBUTTON_RESPROMPT_SELECT))->SetValue(true);
  doRadioSelected(ID_RADIOBUTTON_RESPROMPT_SELECT);
}


void ResidueIndexPrompt::setInsertResidue()
{
  ((wxRadioButton*)FindWindow(ID_RADIOBUTTON_RESPROMPT_NEW))->SetValue(true);
  doRadioSelected(ID_RADIOBUTTON_RESPROMPT_NEW);
}


bool ResidueIndexPrompt::Show(const bool show)
{
  if (show) loadChoices();
  return ResidueIndexPromptGUI::Show(show);
}


int ResidueIndexPrompt::ShowModal()
{
  loadChoices();
  return ResidueIndexPromptGUI::ShowModal();
}


bool ResidueIndexPrompt::isRadioSelected(wxWindowID id) const
{
  wxRadioButton *radio = dynamic_cast<wxRadioButton*>(FindWindow(id));
  if (radio) {
    return radio->GetValue();
  }
  return false;
}


void ResidueIndexPrompt::loadChoices()
{
  wxChoice *select;
  wxChoice *insert;

  WxVizToolFW &fw = getFW();
  SGFragment *frag = fw.getSceneGraph().getFragment();

  int selectedIndex = -1;
  if (frag->m_atomHighLight.size() > 0) {
    Residue *res = frag->atomRef(frag->m_atomHighLight[0])->getResidue();
    selectedIndex = res->index();
  }

  wxArrayString choices;
  int numResidues = frag->numResidues();
  for (int idx=0; idx<numResidues; ++idx) {
    wxString buf;
    buf.Printf("%5d %s", frag->residue(idx)->number(),
                          frag->residue(idx)->name().c_str());
    choices.Add(buf);
  }
  choices.Sort();

  select = dynamic_cast<wxChoice*>(FindWindow(ID_CHOICE_RES_SELECT));
  select->Clear();
  select->Append(choices);
  int selectCount = select->GetCount();
  if (selectedIndex != wxNOT_FOUND && selectedIndex < selectCount) {
    select->Select(selectedIndex);
  } else {
    select->Select(0);
  }

  choices.Insert("beginning",0);
  choices.Insert("end",0);
  insert = dynamic_cast<wxChoice*>(FindWindow(ID_CHOICE_RES_NEW));
  insert->Clear();
  insert->Append(choices);
  insert->Select(0); // "end"
}


void ResidueIndexPrompt::OnRadiobuttonRespromptSelectSelected( wxCommandEvent& event )
{
  doRadioSelected(ID_RADIOBUTTON_RESPROMPT_SELECT);
}


void ResidueIndexPrompt::OnChoiceResSelectSelected( wxCommandEvent& event )
{
}


void ResidueIndexPrompt::OnRadiobuttonRespromptNewSelected( wxCommandEvent& event )
{
  doRadioSelected(ID_RADIOBUTTON_RESPROMPT_NEW);
}


void ResidueIndexPrompt::OnChoiceResInsertSelected( wxCommandEvent& event )
{
}


void ResidueIndexPrompt::OnRadiobuttonRespromptDeleteSelected( wxCommandEvent& event )
{
  doRadioSelected(ID_RADIOBUTTON_RESPROMPT_DELETE);
}


void ResidueIndexPrompt::OnOkClick( wxCommandEvent& event )
{
  doClose(wxID_OK);
}


void ResidueIndexPrompt::OnCancelClick( wxCommandEvent& event )
{
  doClose(wxID_CANCEL);
}


void ResidueIndexPrompt::doRadioSelected(wxWindowID id)
{
  // hide all radio-specific items
  FindWindow(ID_CHOICE_RES_SELECT)->Show(false);
  FindWindow(ID_CHOICE_RES_NEW)->Show(false);
  FindWindow(ID_TEXTCTRL_RESPROMPT_NAME)->Show(false);
  FindWindow(wxID_STATIC_NAME)->Show(false);

  if (id == ID_RADIOBUTTON_RESPROMPT_SELECT) {
    FindWindow(ID_CHOICE_RES_SELECT)->Show(true);
  } else if (id == ID_RADIOBUTTON_RESPROMPT_NEW) {
    FindWindow(ID_CHOICE_RES_NEW)->Show(true);
    FindWindow(ID_TEXTCTRL_RESPROMPT_NAME)->Show(true);
    FindWindow(wxID_STATIC_NAME)->Show(true);
  }

  Layout();
}


void ResidueIndexPrompt::doClose(int retCode)
{
  if (IsModal()) {
    EndModal(retCode);
  } else {
    SetReturnCode(retCode);
    Show(false);
  }

}

