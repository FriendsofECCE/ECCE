#include "viz/SelectWithinCmd.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "tdat/DistanceSelection.H"
#include "tdat/TBond.H"
#include "util/Event.H"
#include "util/EventDispatcher.H"

SelectWithinCmd::SelectWithinCmd(const string& name, Receiver *receiver)
  : Command(name, receiver)
{
  init();
}

void SelectWithinCmd::init()
{
  addParameter(new CommandParameter("radius", 3.5));
  addParameter(new CommandParameter("molecules", false));
  addParameter(new CommandParameter("keep", true));
}

bool SelectWithinCmd::execute()
{
  SGFragment *frag = dynamic_cast<SGContainer*>(getReceiver())->getFragment();
  if (!frag || frag->m_atomHighLight.empty()) return false;
  const vector<int> atoms = selectWithinDistance(*frag, frag->m_atomHighLight,
      getParameter("radius")->getDouble(),
      getParameter("molecules")->getBoolean(), getParameter("keep")->getBoolean());
  vector<int> bonds;
  vector<bool> selected(frag->numAtoms(), false);
  for (int i : atoms) selected[i] = true;
  vector<TBond*> *all = frag->bonds();
  for (size_t i = 0; i < all->size(); ++i)
    if (selected[(*all)[i]->atom1()->index()] &&
        selected[(*all)[i]->atom2()->index()]) bonds.push_back(i);
  delete all;
  if (atoms == frag->m_atomHighLight && bonds == frag->m_bondHighLight) return false;
  if (!p_saved) {
    p_atoms = frag->m_atomHighLight;
    p_bonds = frag->m_bondHighLight;
    p_saved = true;
  }
  frag->m_atomHighLight = atoms;
  frag->m_bondHighLight = bonds;
  ecce::EventDispatcher::getDispatcher().publish(ecce::Event("SelectionChanged"));
  return true;
}

void SelectWithinCmd::undo()
{
  SGFragment *frag = dynamic_cast<SGContainer*>(getReceiver())->getFragment();
  frag->m_atomHighLight = p_atoms;
  frag->m_bondHighLight = p_bonds;
  ecce::EventDispatcher::getDispatcher().publish(ecce::Event("SelectionChanged"));
}
