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

#include "viz/NewFragmentCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

NewFragmentCmd::NewFragmentCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
NewFragmentCmd::~NewFragmentCmd()
{

}



void NewFragmentCmd::init()
{

   setHelpMessage("Clear the existing structure.");
}



bool NewFragmentCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag->numAtoms() > 0 ) {
      ret = true;
      LatticeDef *lattice = frag->getLattice();

      cloneFragment(frag);

      // Not the best solution but have to delete measures
      sg->getTopLabel()->removeAllChildren();

      string ename = "GeomChange";
      if (frag->numResidues() > 0) {
         ename = "ResidueChange";
      }

      frag->clear();

      frag->touchNumbers();
      sg->touchChemDisplay();

      Event event(ename);
      EventDispatcher::getDispatcher().publish(event);
      if (lattice) {
         EventDispatcher::getDispatcher().publish(Event("LatticeChange"));
      }
   }

   return ret;
}
