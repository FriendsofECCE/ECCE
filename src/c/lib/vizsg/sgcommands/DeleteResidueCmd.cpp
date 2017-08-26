/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "util/EventDispatcher.hpp"
#include "util/Event.hpp"
   using namespace ecce;

#include "viz/DeleteResidueCmd.hpp"
#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"

DeleteResidueCmd::DeleteResidueCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 */
DeleteResidueCmd::~DeleteResidueCmd()
{

}



void DeleteResidueCmd::init()
{
   // vector of row indices to delete
   vector<int> residues;
   addParameter(new CommandParameter("residues", residues));
}



bool DeleteResidueCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag && frag->numResidues() > 0) {
      ret = true;

      // TODO probably need to make sure m_atomHighLight matchs the 
      // residues parameter list.  Might be lucky and it works because
      // selection is synced?
      sg->cleanAtomDeletes();

      // NOTE: could do a more efficient undo than cloning here
      cloneFragment(frag);

      vector<int> *residues = getParameter("residues")->getIntegerList();

      frag->deleteResidues(*residues);
      delete residues;

      frag->touchNumbers();
      sg->fixAllNubs();
      sg->touchChemDisplay();

      EventDispatcher::getDispatcher().publish(Event("ResidueChange"));
   }


   return ret;
}
