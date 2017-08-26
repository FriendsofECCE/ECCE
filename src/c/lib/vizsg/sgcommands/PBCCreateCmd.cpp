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

#include "viz/PBCCreateCmd.hpp"
#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"
#include "viz/SGLattice.hpp"

PBCCreateCmd::PBCCreateCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
PBCCreateCmd::~PBCCreateCmd()
{

}



void PBCCreateCmd::init()
{

}



/**
 * Totally brain dead command to create the lattice defined as 
 * the sqare unit lattice.
 * Someday maybe we'll plug in an nwchem module to figure out a good one.
 */
bool PBCCreateCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   if (frag) {
     cloneFragment(frag);
     ret = true;

     // Check for valid LatticeDef object
     LatticeDef *lattice = new LatticeDef(5.0, 5.0, 5.0, 90., 90., 90.);
     frag->setLattice(lattice);
     sg->touchLattice();
     EventDispatcher::getDispatcher().publish(Event("LatticeChange"));
   }
   return ret;
}

void PBCCreateCmd::undo()
{
   FragCloneCmd::undo();
   EventDispatcher::getDispatcher().publish(Event("LatticeChange"));
}
