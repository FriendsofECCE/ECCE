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

#include "viz/DeleteCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

DeleteCmd::DeleteCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
DeleteCmd::~DeleteCmd()
{

}



void DeleteCmd::init()
{

   setHelpMessage("Delete current selection.");
}



/**
 *   Go through all selected atoms and delete them while creating a nub
 *   to take the place of any now deleted bonds.
 *
 * 1. make bit mask of selected atoms (which to delete).  This is
 *    actually done in the begining of execute.
 * 1a. add & subtract nubs from the selection based on selection of parent
 *     atom.  Done from excecute by calling excludeStrayNubs
 * 2. delete any selected bonds  connected to unselected atoms.
 *    Done by logic of execute
 * 3. make bitmask of first atom pointers
 * 3a. Based on first atom pointers and atoms scheduled for delete, make an
 *     external vector of what the new first atom pointers will be when we are
 *     done.
 * 4. for all atoms scheduled for delete, get connected atoms.  For any
 *    connected atom that is connected to an unselected atom,
 *    create a nub to replace the broken bond.  Save nub list (don't add
 *    yet or it messes up the bit mask)
 * 5. Now add all nubs to their parent
 * 6. renumber since we added
 * 7. delete atoms from back to front to avoid messing up indices as we go.
 * 8. reset the first atom pointers to those that we cached
 * 9. delete empty residues
 * 10. renumber again since we are done deleting
 */
bool DeleteCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   int numResidues = frag->numResidues();

   int numAtomsH = frag->m_atomHighLight.size();
   int numBondsH = frag->m_bondHighLight.size();


   // just return if there isn't anything to do
   if (numAtomsH == 0 && numBondsH == 0) return false;

   // We need to use a sorted vector to do this right.
   int numAtoms = frag->numAtoms();
   vector<bool> *atomsToDelete = new vector<bool>(numAtoms, false);
   for (int idx=0; idx<numAtomsH; idx++) {
      (*atomsToDelete)[frag->m_atomHighLight[idx]] = true;
   }

   frag->syncNubSelection(*atomsToDelete);

   // Use selection to pre-delete objects attached to atoms that may be
   // deleted.  Probably should be improved to update m_atomHighLight
   // after the above syncNubSelection
   sg->cleanAtomDeletes();

   // This is for optimization purposes mainly.  Imagine a 1000 atom
   // system where only one atom or one bond is selected.  We want to do
   // the minimal work.  Mixed deletes have overhead due to their
   // complexity.
   ret = true;
   cloneFragment(frag);
   if (numAtomsH == 0 && numBondsH != 0) {
      frag->deleteSelectedBonds();
   } else if (numAtomsH != 0 && numBondsH == 0) {
      frag->deleteSelectedAtoms(*atomsToDelete);
   } else if (frag->numAtoms() == numAtomsH) {
      frag->clear();
   } else {
      frag->deleteSelectedAtomsAndBonds(*atomsToDelete);
   }

   delete atomsToDelete;

   sg->fixAllNubs();
   frag->touchNumbers();
   sg->touchChemDisplay();

   frag->m_atomHighLight.clear();
   frag->m_bondHighLight.clear();

   EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));
   EventDispatcher::getDispatcher().publish(Event("GeomChange"));
   if (numResidues != frag->numResidues()) {
     EventDispatcher::getDispatcher().publish(Event("ResidueChange"));
   }


   return ret;
}
