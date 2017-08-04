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

#include "viz/FindSymmetryCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

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
