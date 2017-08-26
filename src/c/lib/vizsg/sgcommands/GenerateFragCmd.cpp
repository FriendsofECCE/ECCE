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

#include "viz/GenerateFragCmd.hpp"
#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"

GenerateFragCmd::GenerateFragCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
GenerateFragCmd::~GenerateFragCmd()
{

}



void GenerateFragCmd::init()
{
   addParameter(new CommandParameter("threshold", 0.));

}



bool GenerateFragCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag->numAtoms() == 0 ) { return true ; }

   double threshold = getParameter("threshold")->getDouble();

   cloneFragment(frag);

   SymmetryOps::generateFragment(*frag, threshold);

   frag->touchNumbers();
   sg->touchChemDisplay();

   EventDispatcher::getDispatcher().publish(Event("GeomChange"));

   return true;
}
