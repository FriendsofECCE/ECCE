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

#include "viz/GenerateGhostsCmd.hpp"
#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"

GenerateGhostsCmd::GenerateGhostsCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
GenerateGhostsCmd::~GenerateGhostsCmd()
{

}



void GenerateGhostsCmd::init()
{
}



bool GenerateGhostsCmd::execute() throw(EcceException)
{
   bool ret = true;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   cloneFragment(frag);

   SymmetryOps::addGhosts(*frag);

   frag->touchNumbers();
   sg->touchChemDisplay();

   EventDispatcher::getDispatcher().publish(Event("GeomChange"));

   return ret;
}
