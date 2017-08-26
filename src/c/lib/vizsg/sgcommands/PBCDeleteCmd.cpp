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

#include "viz/PBCDeleteCmd.hpp"
#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"
#include "viz/SGLattice.hpp"

PBCDeleteCmd::PBCDeleteCmd(const string& name, Receiver * receiver, SGFragment *base)
    : PBCCmd(name, receiver,base)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
PBCDeleteCmd::~PBCDeleteCmd()
{

}



void PBCDeleteCmd::init()
{
}



/**
 * Totally brain dead command to create the lattice defined as 
 * the sqare unit lattice.
 * Someday maybe we'll plug in an nwchem module to figure out a good one.
 */
bool PBCDeleteCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   if (frag) {
      LatticeDef *lattice = frag->getLattice();
      if (lattice) {
         frag->setLattice(0);
         sg->touchLattice();
         EventDispatcher::getDispatcher().publish(Event("LatticeChange"));
         ret = true;
      }

   }
   return ret;
}

bool PBCDeleteCmd::shouldAutoReplicate() const
{
   return true;
}
