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

#include "tdat/SymmetryOps.hpp"

#include "viz/FindSymmetryCmd.hpp"
#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"

FindSymmetryCmd::FindSymmetryCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
FindSymmetryCmd::~FindSymmetryCmd()
{

}



void FindSymmetryCmd::init()
{
   addParameter(new CommandParameter("threshold", 0.));

}



bool FindSymmetryCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag->numAtoms() == 0 ) { return true ; }

   double threshold = getParameter("threshold")->getDouble();

   cloneFragment(frag);

   string group = SymmetryOps::find(*frag, threshold);

   // add group value to molecule
   frag->pointGroup(group);

   // set useSymmetry flag for benefit of calced
   frag->useSymmetry(true);


   frag->touchNumbers();
   sg->touchChemDisplay();

   EventDispatcher::getDispatcher().publish(Event("GeomChange"));

   return true;
}
