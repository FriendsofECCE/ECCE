#include <float.h> // for DBL_MAX
#include <map>
  using std::map;
#include <math.h>
#include <set>
  using std::set;
  using std::less;

#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"
#include "dsm/ResourceType.H"

#include "tdat/ESPConstraint.H"
#include "tdat/ESPConstraintModel.H"
#include "tdat/Fragment.H"

#include "util/BrowserHelp.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/NumericValidator.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/WxJMSMessageDispatch.H"

#include "CalcEd.H"
#include "PartialCharge.H"

/** Used to subscribe to messaging, publish, identify */
static const string WHOAMI = PARTIALCHARGE;


IMPLEMENT_CLASS(PartialCharge, PartialChargeGUI)



PartialCharge::PartialCharge()
  : PartialChargeGUI(),
    WxJMSListener(),
    p_msgDispatcher(NULL),
    p_group(),
    p_calcEd(NULL),
    p_atomCharge(0.0),
    p_totalCharge(0.0)
{ }



PartialCharge::PartialCharge(wxWindow* parent, wxWindowID id,
        const wxString& caption, const wxPoint& pos, const wxSize& size,
        long style)
  : PartialChargeGUI(),
    WxJMSListener(),
    p_msgDispatcher(NULL),
    p_group(),
    p_calcEd((CalcEd*)parent),
    p_atomCharge(0.0),
    p_totalCharge(0.0)
{
  Create(parent, id, caption, pos, size, style);
}



/**
 * PartialCharge destructor
 */
PartialCharge::~PartialCharge()
{
}



bool PartialCharge::Create(wxWindow* parent, wxWindowID id,
        const wxString& caption, const wxPoint& pos, const wxSize& size,
        long style)
{
  if (!PartialChargeGUI::Create(parent, id, caption, pos, size, style)) {
    wxFAIL_MSG( wxT("PartialCharge creation failed") );
    return false;
  }
  
  initializeMessaging();
  initializeGUI();

  // set desktop icon
  SetIcon(wxIcon(ewxBitmap::pixmapFile(
          ResourceDescriptor::getResourceDescriptor().getTool(CALCED)
          ->getIcon()), wxBITMAP_TYPE_XPM));

  ewxTextCtrl *text;
  //ewxNumericValidator validator;
  text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FIX_VALUE);
  //text->SetValidator(ewxNumericValidator(-DBL_MAX,DBL_MAX,0.0));
  NumericValidator<double> acval(&p_atomCharge,-DBL_MAX,DBL_MAX);
  text->SetValidator(acval);
  //validator.setHardRange("(,)");
  //validator.setValue(0.0);
  //text->SetValidator(validator);
   
  text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_TOTAL_CHRG);
  //text->SetValidator(ewxNumericValidator(-DBL_MAX,DBL_MAX,0.0));
  NumericValidator<double> tcval(&p_totalCharge,-DBL_MAX,DBL_MAX);
  text->SetValidator(tcval);
  //validator.setHardRange("(,)");
  //validator.setValue(0.0);
  //text->SetValidator(validator);
   
  wxCommandEvent event;
  OnChoiceConstraintsSelected(event); // sizing occurs within

  return true;
}



/**
 * Clears the constraint listbox and loads values from model.
 */
void PartialCharge::initializeGUI()
{
  // Load existing model
  ewxListBox *list = (ewxListBox*)FindWindow(ID_LISTBOX_CMDLIST);
  list->Clear();
  ESPConstraintModel *model = p_calcEd->getESPConstraintModel();
  if (model && model->size() > 0) {
    // Add code here to make list box entries for each constraint
    int nsize = model->size();
    for (int idx=0; idx<nsize; idx++) {
      ESPConstraint *constraint = model->get(idx);
      string entry = constraintToString(constraint);
      list->Append(entry.c_str());
    }
  }
}



void PartialCharge::OnButtonConstrainAllMethylGroupsClick(wxCommandEvent& event)
{
  constrainAllMethylGroups();
}



void PartialCharge::OnButtonConstrainAllHydrogensClick(wxCommandEvent& event)
{
  constrainAllMethylHydrogen();
}



void PartialCharge::OnChoiceConstraintsSelected( wxCommandEvent& event )
{
  ewxChoice *menu = (ewxChoice*)FindWindow(ID_CHOICE_CONSTRAINTS);
  string text;
  if (menu->GetStringSelection() == "Fix Atom Charge") {
    text = "The charge of the selected atoms is constrained"
            " to have a specific value. The atoms in the group"
            " must be contained in a block of consecutive atoms.";
    FindWindow(ID_FIXED_CHARGE)->Show();
    FindWindow(ID_TOTAL_CHARGE)->Hide();
    FindWindow(ID_GROUP1)->Show();
    FindWindow(ID_GROUP2)->Hide();
  } else if (menu->GetStringSelection() == "All Charges Equal") {
    text = "All selected atoms are required to have equal charge.";
    FindWindow(ID_FIXED_CHARGE)->Hide();
    FindWindow(ID_TOTAL_CHARGE)->Hide();
    FindWindow(ID_GROUP1)->Show();
    FindWindow(ID_GROUP2)->Hide();
  } else if (menu->GetStringSelection() == "Total Charge Fixed") {
    text = "The net charge of the selected atoms is constrained"
            " to have a specific value. The atoms in the group"
            " must be contained in a block of consecutive atoms.";
    FindWindow(ID_FIXED_CHARGE)->Hide();
    FindWindow(ID_TOTAL_CHARGE)->Show();
    FindWindow(ID_GROUP1)->Show();
    FindWindow(ID_GROUP2)->Hide();
  } else if (menu->GetStringSelection() == "Constrain Group 1 to 2") {
    text = "The constraints in Group 2 are equal to the constraints in"
            " Group 1. Group 1 and Group 2 must have the same number"
            " of atoms.";
    FindWindow(ID_FIXED_CHARGE)->Hide();
    FindWindow(ID_TOTAL_CHARGE)->Hide();
    FindWindow(ID_GROUP1)->Show();
    FindWindow(ID_GROUP2)->Show();
  } else if (menu->GetStringSelection() == "Constrain Methyl Group") {
    text = "The net charge of the methyl group is zero and all attached"
            " hydrogens are constrained to have the same charge."
            " Select only the central carbon atom for this constraint.";
    FindWindow(ID_FIXED_CHARGE)->Hide();
    FindWindow(ID_TOTAL_CHARGE)->Hide();
    FindWindow(ID_GROUP1)->Show();
    FindWindow(ID_GROUP2)->Hide();
  } else if (menu->GetStringSelection() == "Methyl-Type Constraint") {
    text = "The net charge of the entire group is zero and all atoms"
            " in Group 2 are constrained to have the same charge."
            " Only one atom is in Group 1.";
    FindWindow(ID_FIXED_CHARGE)->Hide();
    FindWindow(ID_TOTAL_CHARGE)->Hide();
    FindWindow(ID_GROUP1)->Show();
    FindWindow(ID_GROUP2)->Show();
  }

  menu->SetToolTip(text);

  Refresh();
  Update();
  GetSizer()->Layout();
  GetSizer()->SetSizeHints(this);
  Refresh();
  Update();
  Fit();
  event.Skip();
}



void PartialCharge::OnButtonGroup1Click( wxCommandEvent& event )
{
  p_group = 1;
  requestBuilder("get_atomlist");
  event.Skip();
}



void PartialCharge::OnButtonGroup2Click( wxCommandEvent& event )
{
  p_group = 2;
  requestBuilder("get_atomlist");
  event.Skip();
}



void PartialCharge::OnButtonAddCmdClick( wxCommandEvent& event )
{
  //ewxButton *button = (ewxButton*)FindWindow(ID_BUTTON_ADD_CMD);
  ewxListBox *list = (ewxListBox*)FindWindow(ID_LISTBOX_CMDLIST);
  ewxChoice *menu = (ewxChoice*)FindWindow(ID_CHOICE_CONSTRAINTS);
  ewxTextCtrl *text;
  size_t nsize = list->GetCount();
  string cmd;
  bool error = false;
  string atoms;
  vector<int> indices;
  vector<string> elements;
  double rval;
  char buf[120];
  size_t i;
  if (menu->GetStringSelection() == "Fix Atom Charge") {
    cmd = "constrain ";
    text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GROUP1);
    atoms = text->GetValue().c_str();
    parseAtomList(atoms, indices, elements);
    if (indices.size() == 1) {
      sprintf(buf,"%d",indices[0]);
      cmd.append(buf);
      rval = p_atomCharge;
      sprintf(buf," %lf",rval);
      cmd.append(buf);
    } else {
      error = true;
      p_calcEd->getFeedback()->setMessage(
              "Must select only one atom", WxFeedback::ERROR);
    }
  } else if (menu->GetStringSelection() == "All Charges Equal") {
    cmd = "constrain equal ";
    text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GROUP1);
    atoms = text->GetValue().c_str();
    parseAtomList(atoms, indices, elements);
    if (indices.size() > 0) {
      for (i=0; i<indices.size(); i++) {
        sprintf(buf," %d",indices[i]);
        cmd.append(buf);
      }
    } else {
      error = true;
      p_calcEd->getFeedback()->setMessage(
              "No atoms selected", WxFeedback::ERROR);
    }
  } else if (menu->GetStringSelection() == "Total Charge Fixed") {
    cmd = "constrain";
    text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GROUP1);
    atoms = text->GetValue().c_str();
    parseAtomList(atoms, indices, elements);
    if (indices.size() >= 1) {
      rval = p_totalCharge;
      int imin = indices[0];
      int imax = indices[0];
      for (i=1; i<indices.size(); i++) {
        if (indices[i] < imin) imin = indices[i];
        if (indices[i] > imax) imax = indices[i];
      }
      if (imax - imin + 1 == indices.size()) {
        vector<int> reindex(indices.size());
        for (i=0; i<indices.size(); i++) {
          reindex[indices[i]-imin] = indices[i];
        }
        if (indices.size() > 1) {
          sprintf(buf," %lf",rval);
          cmd.append(buf);
          sprintf(buf," %d",reindex[0]);
          cmd.append(buf);
          cmd.append(" through");
          sprintf(buf," %d",reindex[reindex.size()-1]);
          cmd.append(buf);
        } else {
          sprintf(buf," %d",indices[0]);
          cmd.append(buf);
          sprintf(buf," %lf",rval);
          cmd.append(buf);
        }
      } else {
        error = true;
        p_calcEd->getFeedback()->setMessage(
                "Must be a consecutive block of atoms", WxFeedback::ERROR);
      }
    } else {
      error = true;
      p_calcEd->getFeedback()->setMessage(
              "No atoms selected", WxFeedback::ERROR);
    }
  } else if (menu->GetStringSelection() == "Constrain Group 1 to 2") {
    vector<int> idx1, idx2;
    vector<string> elem1, elem2;
    text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GROUP1);
    atoms = text->GetValue().c_str();
    parseAtomList(atoms, idx1, elem1);
    text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GROUP2);
    atoms = text->GetValue().c_str();
    parseAtomList(atoms, idx2, elem2);
    // Check to see that groups 1 and 2 are compatible
    int natom1, natom2, i;
    natom1 = idx1.size();
    natom2 = idx2.size();
    cmd = "constrain";
    if (idx1.size() <= 0) {
      error = true;
      p_calcEd->getFeedback()->setMessage(
              "No atoms selected for Group 1", WxFeedback::ERROR);
    } else if (idx2.size() <= 0) {
      error = true;
      p_calcEd->getFeedback()->setMessage(
              "No atoms selected for Group 2", WxFeedback::ERROR);
    } else if (natom1 == natom2) {
      int imax, imin;
      imin = idx1[0];
      imax = idx1[0];
      bool check = true;
      for (i=1; i<natom1; i++) {
        if (imin > idx1[i]) imin = idx1[i];
        if (imax < idx1[i]) imax = idx1[i];
      }
      if (imax - imin + 1 != natom1) {
        check = false;
      }
      sprintf(buf," %d %d",imin,imax);
      cmd.append(buf);
      imin = idx2[0];
      imax = idx2[0];
      for (i=1; i<natom2; i++) {
        if (imin > idx2[i]) imin = idx2[i];
        if (imax < idx2[i]) imax = idx2[i];
      }
      if (imax - imin + 1 != natom2) {
        check = false;
      }
      sprintf(buf," to %d %d",imin,imax);
      cmd.append(buf);
      if (!check) {
        error = true;
        p_calcEd->getFeedback()->setMessage(
                "Groups 1 and 2 must have corresponding elements."
                " Are atoms selected in the same order?", WxFeedback::ERROR);
      }
    } else {
      error = true;
      p_calcEd->getFeedback()->setMessage(
              "Groups 1 and 2 must have the same number of atoms",
              WxFeedback::ERROR);
    }
  } else if (menu->GetStringSelection() == "Constrain Methyl Group") {
    text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GROUP1);
    atoms = text->GetValue().c_str();
    parseAtomList(atoms, indices, elements);
    size_t i;
    if (indices.size() > 1) {
      size_t iheavy, hcnt;
      hcnt = 0;
      for (i=0; i<indices.size(); i++) {
        if (elements[i] != "H") {
          iheavy = i;
          hcnt++;
        }
      }
      if (hcnt == 1) {
        cmd = "constrain xhn";
        sprintf(buf," %d",indices[iheavy]);
        cmd.append(buf);
        for (i=0; i<indices.size(); i++) {
          if (i != iheavy) {
            sprintf(buf," %d",indices[i]);
            cmd.append(buf);
          }
        }
      } else if (hcnt == 0) {
        error = true;
        p_calcEd->getFeedback()->setMessage(
                "No heavy atom in group 1", WxFeedback::ERROR);
      } else {
        error = true;
        p_calcEd->getFeedback()->setMessage(
                "Too many heavy atoms in group 1", WxFeedback::ERROR);
      }
    } else {
      error = true;
      p_calcEd->getFeedback()->setMessage(
              "Group 1 must have more than atom", WxFeedback::ERROR);
    }
  } else if (menu->GetStringSelection() == "Methyl-Type Constraint") {
    text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GROUP1);
    string grp1 = text->GetValue().c_str();
    vector<int> idx1, idx2;
    vector<string> elem1, elem2;
    parseAtomList(grp1, idx1, elem1);
    text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GROUP2);
    string grp2 = text->GetValue().c_str();
    parseAtomList(grp2, idx2, elem2);
    if (idx1.size() == 1) {
      cmd = "constrain xhn";
      sprintf(buf," %d",idx1[0]);
      cmd.append(buf);
      if (idx2.size() > 0) {
        size_t i;
        for (i=0; i<idx2.size(); i++) {
          sprintf(buf," %d",idx2[i]);
          cmd.append(buf);
        }
      } else {
        error = true;
        p_calcEd->getFeedback()->setMessage(
                "Group 2 must have at least one atom", WxFeedback::ERROR);
      }
    } else {
      error = true;
      p_calcEd->getFeedback()->setMessage(
              "Group 1 must have only 1 atom", WxFeedback::ERROR);
    }
  }

  if (!error) {
    if (list->FindString(cmd) == wxNOT_FOUND) {
      list->Append(cmd.c_str());
    }
  }

  if (nsize != list->GetCount()) {
    updateConstraints();
  }

  event.Skip();
}


void PartialCharge::OnButtonDeleteCmdClick( wxCommandEvent& event )
{
  wxArrayInt selections;
  ewxListBox *list = (ewxListBox*)FindWindow(ID_LISTBOX_CMDLIST);
  list->GetSelections(selections);
  int nsize = selections.GetCount();
  size_t nstart = list->GetCount();
  int i;
  for (i=nsize-1; i >= 0; i--) {
    list->Delete(selections[i]);
  }
  if (nstart != list->GetCount()) {
    updateConstraints();
  }
  event.Skip();
}



/**
 * Set selection in builder if it is up and any of the constraint
 * commands is selected
 */
void PartialCharge::OnListboxCmdlistSelected( wxCommandEvent& event )
{
  wxArrayInt selections;
  vector<int> indices;
  ewxListBox *list = (ewxListBox*)FindWindow(ID_LISTBOX_CMDLIST);
  list->GetSelections(selections);
  int nsize = selections.GetCount();
  int i;
  string item;
  for (i=0; i < nsize; i++) {
    item = list->GetString(selections[i]).c_str();
    parseConstraint(item, indices);
  }
  char buf[120];
  nsize = indices.size();
  if (nsize > 0) {
    sprintf(buf,"%d",indices[0]);
    string selection = buf;
    for (i=1; i<nsize; i++) {
      sprintf(buf," %d",indices[i]);
      selection.append(buf);
    }
    JMSPublisher publisher(WHOAMI);
    JMSMessage *msg = publisher.newMessage();
    msg->addProperty("selection",selection);
    publisher.publish("ecce_set_selection",*msg);
    delete msg;
  }
  event.Skip();
}



void PartialCharge::OnButtonShowUnconstrainedAtomsClick(wxCommandEvent& event)
{
  wxArrayInt selections;
  ewxListBox *list = (ewxListBox*)FindWindow(ID_LISTBOX_CMDLIST);
  vector<int> tmp;
  set<int> atoms;
  set<int> allAtoms;
  size_t nsize = list->GetCount();
  size_t i,j;
  string item;
  for (i=0; i < nsize; i++) {
    item = list->GetString(i).c_str();
    tmp.clear();
    parseConstraint(item, tmp);
    for (j=0; j<tmp.size(); j++) {
      atoms.insert(tmp[j]);
    }
  }

  // we now have a set of atom indices, check against atoms
  int numAtoms = p_calcEd->getFrag()->numAtoms();
  vector<int> atomDiff;
  for (int i = 1; i <= numAtoms; i++) {
    if (atoms.find(i) == atoms.end()) {
      atomDiff.push_back(i);
    }
  }

  string selection = "";
  nsize = atomDiff.size();
  char buf[120];
  if (nsize > 0) {
    sprintf(buf,"%d",atomDiff[0]);
    selection = buf;
  }
  for (i=1; i<nsize; i++) {
    sprintf(buf," %d",atomDiff[i]);
    selection.append(buf);
  }
  map<string,string> properties;
  properties["selection"] = selection;
  requestBuilder("ecce_set_selection", &properties);
}



void PartialCharge::OnButtonShowMultiplyConstrainedAtomsClick(wxCommandEvent& event)
{
  wxArrayInt selections;
  ewxListBox *list = (ewxListBox*)FindWindow(ID_LISTBOX_CMDLIST);
  vector<int> indices;
  vector<int> tmp;
  set<int> atoms;
  size_t nsize = list->GetCount();
  size_t i,j;
  string item;
  for (i=0; i < nsize; i++) {
    item = list->GetString(i).c_str();
    tmp.clear();
    parseConstraint(item, tmp);
    for (j=0; j<tmp.size(); j++) {
      if (atoms.find(tmp[j]) != atoms.end()) {
        indices.push_back(tmp[j]);
      } else {
        atoms.insert(tmp[j]);
      }
    }
  }

  string selection = "";
  nsize = indices.size();
  char buf[120];
  if (nsize > 0) {
    sprintf(buf,"%d",indices[0]);
    selection = buf;
    for (i=1; i<nsize; i++) {
      sprintf(buf," %d",indices[i]);
      selection.append(buf);
    }
  }
  map<string,string> properties;
  properties["selection"] = selection;
  requestBuilder("ecce_set_selection", &properties);
}



void PartialCharge::OnCloseClick(wxCommandEvent& event)
{
  if (IsModal()) {
    EndModal(GetReturnCode());
  } else {
    Show(false);
  }
  event.Skip();
}



void PartialCharge::OnHelpClick(wxCommandEvent& event)
{
  BrowserHelp help;
  help.showPage(help.URL("HomeFallback"));
}



void PartialCharge::initializeMessaging()
{
  p_msgDispatcher = new WxJMSMessageDispatch(WHOAMI, true);

  p_msgDispatcher->subscribe("ecce_get_atomlist",
       this, (wxJmsCBFunc)&PartialCharge::selectionMCB);

  p_msgDispatcher->startSubscriber();
  p_msgDispatcher->registerTopShell(this);
  p_msgDispatcher->notifyReady();
}



void PartialCharge::selectionMCB(JMSMessage& msg)
{
  int natoms = msg.getIntProperty("numatoms");
  string atom_list = msg.getProperty("selection");
  if (natoms > 0) {
    if (p_group == 1) {
      ewxTextCtrl *text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GROUP1);
      text->SetValue(atom_list.c_str());
    } else {
      ewxTextCtrl *text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GROUP2);
      text->SetValue(atom_list.c_str());
    }
  }
}



/**
 * Send message(s) to request startup of builder with this context.
 * It allows for arbitrary properties to be added to the message rather than
 * just the actionTopic message.
 */
void PartialCharge::requestBuilder(const string& actionTopic,
                                   map<string, string> *properties)
{
  JMSPublisher publisher(WHOAMI);
  JMSMessage *msg = publisher.newMessage();
  msg->addProperty("action","start");
  publisher.publish("ecce_activity",*msg);
  delete msg;

  // Send the start app request to gateway:
  Target gateway(GATEWAY, "");
  msg = publisher.newMessage(gateway);
  // Ensure that this message will carry original sender info for 
  // sending point to point responses after startup
  msg->markAsPointToPoint();

  // Add arbitrary number of extra properties, if available
  if (properties) {
    map<string,string>::iterator props;
    for (props = properties->begin(); props != properties->end(); props++) {
       msg->addProperty(props->first, props->second);
    }
  }

  // which builder do we invoke?  find out now!
  Resource *resource = EDSIFactory::getResource(p_calcEd->getContext());
  if (resource) {
    ResourceType *resourceType = resource->getDescriptor();
    if (resourceType) {
      vector<ResourceTool*> tools = resourceType->getTools();
      for (size_t i = 0; i < tools.size(); i++) {
        string name = tools[i]->getName();
        if (name == BUILDER) {
          publisher.invoke(*msg, name, 0, p_calcEd->getContext(), actionTopic);
          break;
        }
      }
    }
  }

  delete msg;
}



void PartialCharge::constrainAllMethylGroups()
{
  Fragment *frag = p_calcEd->getFrag();
  int natoms = frag->numAtoms();

  for (int idx=0; idx<natoms; idx++) {
    string symbol = frag->atomRef(idx)->atomicSymbol();

    if (symbol == "C") {
      vector<TAtm*> *atoms = frag->connectedAtoms(idx);
      int nconnected = atoms->size();
      char buf[120];
      sprintf(buf,"C%d",frag->atomRef(idx)->index()+1);
      string atomList = buf;
      int icnt = 0;
      for (int i=0; i<nconnected; i++) {
        if ((*atoms)[i]->atomicSymbol() == "H") {
          icnt++;
          sprintf(buf," H%d",(*atoms)[i]->index()+1);
          atomList.append(buf);
        }
      }
      if (icnt > 0) {
        methylConstraint(icnt+1, atomList);
      }
      delete atoms;
    }
  }
}



void PartialCharge::constrainAllMethylHydrogen()
{
  Fragment *frag = p_calcEd->getFrag();
  int natoms = frag->numAtoms();
  for (int idx=0; idx<natoms; idx++) {
    string symbol = frag->atomRef(idx)->atomicSymbol();

    if (symbol == "C") {
      vector<TAtm*> *atoms = frag->connectedAtoms(idx);
      int nconnected = atoms->size();
      char buf[120];
      string atomList = buf;
      int icnt = 0;
      for (int i=0; i<nconnected; i++) {
        if ((*atoms)[i]->atomicSymbol() == "H") {
          if (icnt == 0) {
            sprintf(buf,"H%d",(*atoms)[i]->index()+1);
            atomList = buf;
          } else {
            sprintf(buf," H%d",(*atoms)[i]->index()+1);
            atomList.append(buf);
          }
          icnt++;
        }
      }
      if (icnt > 1) {
        methylConstraint(icnt, atomList);
      }
      delete atoms;
    }
  }
}



void PartialCharge::methylConstraint(const int& numAtoms,
                                     const string& atomList)
{
  if (numAtoms > 1) {
    vector<int> indices;
    vector<string> elements;
    string cmd;
    char buf[120];
    ewxListBox *list = (ewxListBox*)FindWindow(ID_LISTBOX_CMDLIST);
    parseAtomList(atomList,indices,elements);
    if (elements[0] != "H") {
      cmd = "constrain xhn";
      int i;
      for (i=0; i<numAtoms; i++) {
        sprintf(buf," %d",indices[i]);
        cmd.append(buf);
      }
    } else {
      cmd = "constrain equal";
      int i;
      for (i=0; i<numAtoms; i++) {
        sprintf(buf," %d",indices[i]);
        cmd.append(buf);
      }
    }
    if (list->FindString(cmd) == wxNOT_FOUND) {
      list->Append(cmd.c_str());
      updateConstraints();
    }
  }
}



/**
 * Utility function to convert text string to a list of atoms and element types.
 */
void PartialCharge::parseAtomList(string atom_list, vector<int> &indices,
                                  vector<string> &symbols)
{
  StringTokenizer tkn(atom_list);
  vector<string> tokens = tkn.tokenize(" ");
  int natoms = tokens.size();
  indices.clear();
  symbols.clear();
  string index;
  string symbol;
  int idx, len;
  for (idx = 0; idx < natoms; idx++) {
    len = tokens[idx].length();
    if (len > 1) {
      if (isdigit(tokens[idx][1])) {
        symbol = tokens[idx].substr(0,1);
        index = tokens[idx].substr(1,len-1);
      } else {
        symbol = tokens[idx].substr(0,2);
        index = tokens[idx].substr(2,len-2);
      }
    }
    symbols.push_back(symbol);
    indices.push_back(atoi(index.c_str()));
  }
}



/**
 * Utility function to convert constraint to a list of atoms.
 */
void PartialCharge::parseConstraint(string constraint, vector<int> &indices)
{
  StringTokenizer tkn(constraint);
  vector<string> tokens = tkn.tokenize(" ");
  set<int> list;
  size_t ntok = tokens.size();
  int ival;
  size_t i;
  double rval;
  for (i=0; i<indices.size(); i++) {
    if (list.find(indices[i]) == list.end()) {
      list.insert(indices[i]);
    }
  }
  if (ntok == 0) return;
  if (tokens[0] == "constrain") {
    // All atoms equal or methyl-type constraints
    if (ntok > 1 && (tokens[1] == "equal" || tokens[1] == "xhn")) {
      for (i=2; i<ntok; i++) {
        ival = atoi(tokens[i].c_str());
        if (list.find(ival) == list.end()) {
           indices.push_back(ival);
           list.insert(ival);
        }
      }
    } else {
      // Fixed atom charge
      if (ntok == 3 && StringConverter::toInt(tokens[1],ival) &&
          StringConverter::toDouble(tokens[2],rval)) {
        ival = atoi(tokens[1].c_str());
        if (list.find(ival) == list.end()) {
          indices.push_back(ival);
          list.insert(ival);
        }
      } else if (ntok == 5 && StringConverter::toDouble(tokens[1],rval) &&
          tokens[3] == "through") {
        int imin, imax;
        imin = atoi(tokens[2].c_str());
        imax = atoi(tokens[4].c_str());
        for (i=imin; i<=imax; i++) {
          if (list.find(i) == list.end()) {
            indices.push_back(i);
            list.insert(i);
          }
        }
      } else if (ntok == 6 && tokens[3] == "to") {
        int imin, imax;
        imin = atoi(tokens[1].c_str());
        imax = atoi(tokens[2].c_str());
        for (i=imin; i<=imax; i++) {
          if (list.find(i) == list.end()) {
            indices.push_back(i);
            list.insert(i);
          }
        }
        imin = atoi(tokens[4].c_str());
        imax = atoi(tokens[5].c_str());
        for (i=imin; i<=imax; i++) {
          if (list.find(i) == list.end()) {
            indices.push_back(i);
            list.insert(i);
          }
        }
      }
    }
  }
}



/**
 * Utility function to convert constraint string to an ESPConstraint.
 * Caller takes ownership of pointer.
 */
ESPConstraint* PartialCharge::stringToConstraint(string constraint)
{
  ESPConstraint *esp = new ESPConstraint;

  StringTokenizer tkn(constraint);
  vector<string> tokens = tkn.tokenize(" ");
  set<int> list;
  int ntok = tokens.size();
  int ival, i;
  double rval;
  vector<TAtm*> atoms;
  Fragment *frag = p_calcEd->getFrag();
  if (ntok == 0) return esp;
  if (tokens[0] == "constrain") {
    // All atoms equal or methyl-type constraints
    if (ntok > 1 && (tokens[1] == "equal" || tokens[1] == "xhn")) {
      if (tokens[1] == "equal") esp->type(ESPConstraint::EQUAL);
      if (tokens[1] == "xhn") esp->type(ESPConstraint::SPECIAL);
      for (i=2; i<ntok; i++) {
        ival = atoi(tokens[i].c_str());
        if (list.find(ival) == list.end()) {
           atoms.push_back(frag->atomRef(ival-1));
           list.insert(ival);
        }
      }
      esp->group1(atoms);
    } else {
      // Fixed atom charge
      if (ntok == 3 && StringConverter::toInt(tokens[1],ival) &&
          StringConverter::toDouble(tokens[2],rval)) {
        esp->type(ESPConstraint::FIXED);
        ival = atoi(tokens[1].c_str());
        if (list.find(ival) == list.end()) {
          atoms.push_back(frag->atomRef(ival-1));
          list.insert(ival);
        }
        esp->charge(rval);
        esp->group1(atoms);
      } else if (ntok == 5 && StringConverter::toDouble(tokens[1],rval) &&
          tokens[3] == "through") {
        esp->type(ESPConstraint::NET);
        int imin, imax;
        imin = atoi(tokens[2].c_str());
        imax = atoi(tokens[4].c_str());
        for (i=imin; i<=imax; i++) {
          if (list.find(i) == list.end()) {
            atoms.push_back(frag->atomRef(i-1));
            list.insert(i);
          }
        }
        esp->charge(rval);
        esp->group1(atoms);
      } else if (ntok == 6 && tokens[3] == "to") {
        esp->type(ESPConstraint::GROUP);
        int imin, imax;
        imin = atoi(tokens[1].c_str());
        imax = atoi(tokens[2].c_str());
        for (i=imin; i<=imax; i++) {
          if (list.find(i) == list.end()) {
            atoms.push_back(frag->atomRef(i-1));
            list.insert(i);
          }
        }
        esp->group1(atoms);
        atoms.clear();
        imin = atoi(tokens[4].c_str());
        imax = atoi(tokens[5].c_str());
        for (i=imin; i<=imax; i++) {
          if (list.find(i) == list.end()) {
            atoms.push_back(frag->atomRef(i-1));
            list.insert(i);
          }
        }
        esp->group2(atoms);
      }
    }
  }

  return esp;
}



/**
 *
 */
string PartialCharge::constraintToString(const ESPConstraint *constraint)
{
  string cmd = "constrain";
  vector<TAtm*> grp1 = constraint->group1();
  char buf[120];
  bool okay = true;
  if (constraint->type() == ESPConstraint::FIXED) {
    if (grp1.size() > 0) {
      int idx = grp1[0]->index()+1;
      sprintf(buf," %d", idx);
      cmd.append(buf);
      double rval = constraint->charge();
      sprintf(buf," %lf", rval);
      cmd.append(buf);
    } else {
      okay = false;
    }
  } else if (constraint->type() == ESPConstraint::EQUAL) {
    int nsize = grp1.size();
    if (nsize > 1) {
      int i, idx;
      cmd.append(" equal");
      for (i=0; i<nsize; i++) {
        idx = grp1[i]->index()+1;
        sprintf(buf," %d", idx);
        cmd.append(buf);
      }
    } else {
      okay = false;
    }
  } else if (constraint->type() == ESPConstraint::NET) {
    int nsize = grp1.size();
    if (nsize > 1) {
      double rval = constraint->charge();
      sprintf(buf," %lf", rval);
      cmd.append(buf);
      int i, idx, imin, imax;
      imin = grp1[0]->index();
      imax = grp1[0]->index();
      for (i=1; i<nsize; i++) {
        idx = grp1[i]->index();
        if (idx > imax) {
          imax = idx;
        }
        if (idx < imin) {
          imin = idx;
        }
      }
      if (imax-imin+1 == nsize) {
        sprintf(buf," %d", imin+1);
        cmd.append(buf);
        cmd.append(" through");
        sprintf(buf," %d", imax+1);
        cmd.append(buf);
      } else {
        okay = false;
      }
    } else {
      okay = false;
    }
  } else if (constraint->type() == ESPConstraint::GROUP) {
    vector<TAtm*> grp2 = constraint->group2();
    int nsize1 = grp1.size();
    int nsize2 = grp2.size();
    if (nsize1 == nsize2) {
      okay = true;
      int i, idx, imin, imax;
      imin = grp1[0]->index();
      imax = grp1[0]->index();
      for (i=1; i<nsize1; i++) {
        idx = grp1[i]->index();
        if (idx > imax) {
          imax = idx;
        }
        if (idx < imin) {
          imin = idx;
        }
      }
      if (imax-imin+1 == nsize1) {
        sprintf(buf," %d", imin+1);
        cmd.append(buf);
        sprintf(buf," %d", imax+1);
        cmd.append(buf);
      } else {
        okay = false;
      }
      cmd.append(" to");
      imin = grp2[0]->index();
      imax = grp2[0]->index();
      for (i=1; i<nsize2; i++) {
        idx = grp2[i]->index();
        if (idx > imax) {
          imax = idx;
        }
        if (idx < imin) {
          imin = idx;
        }
      }
      if (imax-imin+1 == nsize2) {
        sprintf(buf," %d", imin+1);
        cmd.append(buf);
        sprintf(buf," %d", imax+1);
        cmd.append(buf);
      } else {
        okay = false;
      }
    } else {
      okay = false;
    }
  } else if (constraint->type() == ESPConstraint::METHYL ||
             constraint->type() == ESPConstraint::SPECIAL) {
    int nsize = grp1.size();
    if (nsize > 1) {
      int i, idx;
      cmd.append(" xhn");
      for (i=0; i<nsize; i++) {
        idx = grp1[i]->index()+1;
        sprintf(buf," %d", idx);
        cmd.append(buf);
      }
    } else {
      okay = false;
    }
  }
  if (!okay) {
    cmd.clear();
  }
  return cmd;
}

/**
 * Completely rebuild the model based on the contents of the list box.
 */
void PartialCharge::updateConstraints()
{
   ESPConstraintModel *model = p_calcEd->getESPConstraintModel();
   ewxListBox *list = (ewxListBox*)FindWindow(ID_LISTBOX_CMDLIST);
   int nsize = list->GetCount();
   model->clear();
   for (int i=0; i < nsize; i++) {
      model->add(stringToConstraint(list->GetString(i).c_str()));
   }
   // User made change so show save state icon
   p_calcEd->enableSave();
}

