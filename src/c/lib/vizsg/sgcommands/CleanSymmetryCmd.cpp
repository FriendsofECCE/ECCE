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

#include "viz/CleanSymmetryCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

CleanSymmetryCmd::CleanSymmetryCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
CleanSymmetryCmd::~CleanSymmetryCmd()
{

}



void CleanSymmetryCmd::init()
{
   addParameter(new CommandParameter("threshold", 0.));
//   addParameter(new CommandParameter("group", "C1"));

}



bool CleanSymmetryCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();


   if (frag->numAtoms() == 0 ) { return true ; }

   double threshold = getParameter("threshold")->getDouble();

   ret = true;

   cloneFragment(frag);

   SymmetryOps::clean(*frag, threshold,frag->pointGroup());

   frag->touchNumbers();
   sg->touchChemDisplay();

   EventDispatcher::getDispatcher().publish(Event("GeomChange"));

   return ret;
}
