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

#include "viz/FindIrreduceFragCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

FindIrreduceFragCmd::FindIrreduceFragCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
FindIrreduceFragCmd::~FindIrreduceFragCmd()
{

}



void FindIrreduceFragCmd::init()
{
   addParameter(new CommandParameter("threshold", 0.));

}



bool FindIrreduceFragCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag->numAtoms() == 0 ) { return true ; }

   // Use selection to pre-delete objects attached to atoms that may be
   // deleted.
   sg->cleanAtomDeletes();

   double threshold = getParameter("threshold")->getDouble();

   cloneFragment(frag);

   SymmetryOps::findIrreducible(*frag, threshold, frag->pointGroup());

   frag->touchNumbers();
   sg->touchChemDisplay();

   frag->m_atomHighLight.clear();
   frag->m_bondHighLight.clear();

   EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));
   EventDispatcher::getDispatcher().publish(Event("GeomChange"));

   return true;
}
