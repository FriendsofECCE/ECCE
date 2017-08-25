/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;

#include "wxgui/WindowEvent.H"
  using namespace ecce;

#include "tdat/TBond.H"
#include "tdat/LinkCellList.H"

#include "viz/SelectBondDisCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

SelectBondDisCmd::SelectBondDisCmd(const string& name, Receiver * receiver, const string& sender)
    : Command(name, receiver)
{
   init();
   p_sender = sender;
}



/**
 * Destructor.
 * Deletes undo related data.
 */
SelectBondDisCmd::~SelectBondDisCmd()
{
}



void SelectBondDisCmd::init()
{
   vector<string> new_list;
   addParameter(new CommandParameter("clear", true));
   addParameter(new CommandParameter("min_bond", 1));
   addParameter(new CommandParameter("max_bond", 1));
   setHelpMessage("Selects atoms and bonds based on bond distance from current selection");
}



// @todo chain list
// @todo radius
bool SelectBondDisCmd::execute() throw(EcceException)
{

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  // For undo
  if (p_selAtoms.size() == 0 && p_selBonds.size() == 0) {
    p_selAtoms = frag->m_atomHighLight;
    p_selBonds = frag->m_bondHighLight;
  }

  bool selChanged = false;

  if (frag) {

    int nAtoms = frag->numAtoms();
    int nBonds = frag->numBonds();

    int i, j, k;

    if (nAtoms > 0) {
      vector<TAtm*> *atoms = frag->atoms();
      // Search distance
      int min_bond, max_bond;
      min_bond = getParameter("min_bond")->getInteger();
      max_bond = getParameter("max_bond")->getInteger();

      // Bit mask indicating which atoms have been included
      // as part of the radius selection.  Initially set to currently selected
      // atoms.
      vector<int> newAtoms;
      vector<int> shortAtoms;
      vector<bool> newSelectedAtoms(nAtoms, false);

      int nsize = frag->m_atomHighLight.size();
      for (i=0; i<nsize; i++) {
        newSelectedAtoms[(frag->m_atomHighLight)[i]] = true;
      }

      // Bit mask indicating which bonds have been included
      // as part of the radius selection.  Initially set to currently selected
      // bonds.
      vector<bool> selectedBonds(nBonds, false);
      nsize = frag->m_bondHighLight.size();
      for (i=0; i<nsize; i++) {
        selectedBonds[(frag->m_bondHighLight)[i]] = true;
      }

      // -------------------------------
      // Perform bond distance selection
      // -------------------------------
      int ibond;
      int nmin, nmax;
      int offset=0;
      nsize = frag->m_atomHighLight.size();
      for (ibond = 1; ibond <= max_bond; ibond++) {
        if (ibond == 1) {
          nmin = 0;
          nmax = nsize;
        } else {
          nmin = offset;
          nmax = newAtoms.size();
          offset = nmax;
        }
        for (i = nmin; i < nmax; i++) {
          TAtm *atm1;
          if (ibond == 1) {
            atm1 = (*atoms)[(frag->m_atomHighLight)[i]];
          } else {
            atm1 = (*atoms)[newAtoms[i]];
          }
          vector<TAtm*> *connected = atm1->connectedAtoms();
          int atsize = connected->size();
          for (j=0; j<atsize; j++) {
            k = (*connected)[j]->index();
            if (!newSelectedAtoms[k]) {
              newSelectedAtoms[k] = true;
              newAtoms.push_back(k);
              if (ibond<min_bond) {
                shortAtoms.push_back(k);
              }
            }
          }
          delete connected;
        }
      }
      delete atoms;
      nsize = shortAtoms.size();
      for (i=0; i<nsize; i++) {
        newSelectedAtoms[shortAtoms[i]] = false;
      }
      nsize = frag->m_atomHighLight.size();
      for (i=0; i<nsize; i++) {
        newSelectedAtoms[frag->m_atomHighLight[i]] = false;
      }
      for (i=0; i<nAtoms; i++) {
        if (newSelectedAtoms[i]) {
          const vector<TBond*> blist = frag->atomRef(i)->bondList();
          int nbond = blist.size();
          for (j=0; j<nbond; j++) {
            selectedBonds[blist[j]->index()] = true;
          }
        }
      }

      if (getParameter("clear")->getBoolean()) {
        // Clear last selection first if requested
        frag->m_atomHighLight.clear();
        frag->m_bondHighLight.clear();
      }
      for (i=0; i<nAtoms; i++) {
        if (newSelectedAtoms[i]) {
          frag->m_atomHighLight.push_back(i);
        }
      }
      for (i=0; i<nBonds; i++) {
        if (selectedBonds[i]) {
          frag->m_bondHighLight.push_back(i);
        }
      }
    }
  }
  Event event("SelectionChanged","",p_sender);
  EventDispatcher::getDispatcher().publish(event);

  return selChanged;
}


void SelectBondDisCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   frag->m_atomHighLight = p_selAtoms;
   frag->m_bondHighLight = p_selBonds;

}
