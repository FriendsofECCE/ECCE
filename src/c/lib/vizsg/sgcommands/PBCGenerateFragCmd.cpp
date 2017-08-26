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

#include "viz/PBCGenerateFragCmd.hpp"
#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"

PBCGenerateFragCmd::PBCGenerateFragCmd(const string& name, 
                                       Receiver * receiver,
                                       SGFragment *base)
    : PBCCmd(name, receiver, base)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
PBCGenerateFragCmd::~PBCGenerateFragCmd()
{

}



void PBCGenerateFragCmd::init()
{
   addParameter(new CommandParameter("threshold", 0.));

}



bool PBCGenerateFragCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag->numAtoms() == 0 ) { return true ; }

   double threshold = getParameter("threshold")->getDouble();

   cloneFragment(frag);

   SymmetryOps::generateLatticeFragment(*frag, threshold);

   frag->touchNumbers();
   sg->touchChemDisplay();

   EventDispatcher::getDispatcher().publish(Event("GeomChange"));

   return true;
}
