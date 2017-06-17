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

#include "tdat/SymmetryOps.H"

#include "viz/RemoveGhostsCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

RemoveGhostsCmd::RemoveGhostsCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
RemoveGhostsCmd::~RemoveGhostsCmd()
{

}



void RemoveGhostsCmd::init()
{
}



bool RemoveGhostsCmd::execute() throw(EcceException)
{
   bool ret = true;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   // Use selection to pre-delete objects attached to atoms that may be
   // deleted.
   sg->cleanAtomDeletes();

   cloneFragment(frag);

   SymmetryOps::removeGhosts(*frag);

   frag->touchNumbers();
   sg->touchChemDisplay();

   frag->m_atomHighLight.clear();
   frag->m_bondHighLight.clear();

   EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));
   EventDispatcher::getDispatcher().publish(Event("GeomChange"));

   return ret;
}
