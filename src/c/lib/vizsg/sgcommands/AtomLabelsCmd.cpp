/**
 * @file
 *
 *
 */

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "tdat/TAtm.H"

#include "wxgui/WindowEvent.H"

#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/AtomLabelsCmd.H"


static const char *NUB_TAG = "Nub";


AtomLabelsCmd::AtomLabelsCmd(const string& name, Receiver * receiver, wxWindow *sender)
  : Command(name, receiver)
{
  p_sender = sender;
  init();
}


/**
 * Destructor.
 * Deletes undo related data.
 */
AtomLabelsCmd::~AtomLabelsCmd()
{
}


void AtomLabelsCmd::init()
{
  addParameter(new CommandParameter("type", AtomLabelsCmd::ELEMENT));
  setHelpMessage("Atom Labels");
}



bool AtomLabelsCmd::execute() throw(EcceException)
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  bool applytoall = false;
  vector<int> targets = frag->m_atomHighLight;
  if (targets.empty()) {
    applytoall = true;
    int numAtoms = frag->numAtoms();
    for (int idx=0; idx<numAtoms; ++idx)
      targets.push_back(idx);
  }

  TAtm *atom;
  char buf[32];
  string name;

  int type = getParameter("type")->getInteger();
  vector<TAtm*> *atoms = frag->atoms();

  p_targets.clear();
  p_labels.clear();

  int targetSize = targets.size();
  for (int idx=0; idx<targetSize; ++idx) {
    atom = (*atoms)[targets[idx]];
    name = atom->atomicSymbol();
    if (atom->displayStyle().isDisplayed() && name != NUB_TAG) {
      p_targets.push_back(targets[idx]);
      p_labels.push_back(atom->displayLabel());
      switch (type) {
      case AtomLabelsCmd::NONE:
        atom->displayLabel("");
        break;
      case AtomLabelsCmd::ELEMENT:
        sprintf(buf,"%s%d",name.c_str(),targets[idx]+1);
        atom->displayLabel(buf);
        break;
      case AtomLabelsCmd::CHARGE2:
        sprintf(buf,"%.2f",atom->partialCharge());
        atom->displayLabel(buf);
        break;
      case AtomLabelsCmd::CHARGE3:
        sprintf(buf,"%.3f",atom->partialCharge());
        atom->displayLabel(buf);
        break;
      case AtomLabelsCmd::CHARGE4:
        sprintf(buf,"%.4f",atom->partialCharge());
        atom->displayLabel(buf);
        break;
      case AtomLabelsCmd::TYPE:
        atom->displayLabel(atom->atomType());
        break;
      case AtomLabelsCmd::ATOMNAME:
        atom->displayLabel(atom->atomName());
        break;
      }
    }
  }

  // tells MI to turn them on or off per display style
  sg->setAtomLabels((type!=AtomLabelsCmd::NONE) || (!applytoall));

  return true;
}


void AtomLabelsCmd::undo()
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();
  vector<TAtm*> *atoms = frag->atoms();
  int targetSize = p_targets.size();
  TAtm *atom;
  for (int idx=0; idx<targetSize; ++idx) {
    atom = (*atoms)[p_targets[idx]];
    atom->displayLabel(p_labels[idx]);
  }
  sg->setAtomLabels(targetSize);
}


bool AtomLabelsCmd::isUndoable() const
{
  return true;
}
