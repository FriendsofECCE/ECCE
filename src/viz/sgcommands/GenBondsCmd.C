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

#include "viz/GenBondsCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

GenBondsCmd::GenBondsCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
GenBondsCmd::~GenBondsCmd()
{

}



void GenBondsCmd::init()
{

   setHelpMessage("GenBondsCmd.");
}



bool GenBondsCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag->numAtoms() > 0 ) {
      ret = true;

      cloneFragment(frag);  // for undo

      frag->addCovalentBonds();

      // Now delete nubs - used to be separate command
      for (int idx=frag->numAtoms()-1; idx>=0; idx--) {
         if (frag->atomRef(idx)->atomicSymbol() == "Nub") {
            frag->deleteAtom(idx);
         }
      }
      // We have to regenerate the shapes since we removed bonding sites.
      frag->generateShapes();


      frag->touchNumbers();
      sg->touchChemDisplay();

   }

   return ret;
}
