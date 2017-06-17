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

#include "viz/PBCSuperCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/SGLattice.H"

PBCSuperCmd::PBCSuperCmd(const string& name, Receiver * receiver, SGFragment *base)
    : PBCCmd(name, receiver, base)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
PBCSuperCmd::~PBCSuperCmd()
{

}



void PBCSuperCmd::init()
{
}



bool PBCSuperCmd::execute() throw(EcceException)
{
   bool ret = true;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   if (frag) {

      LatticeDef *lattice = frag->getLattice();
      if (lattice) {

         MPoint a, b, c;
         // Get basis vectors for unit cell.
         a.xyz((*(lattice->toVectors()))[0]);
         b.xyz((*(lattice->toVectors()))[1]);
         c.xyz((*(lattice->toVectors()))[2]);

         int n1, n2, n3;
         lattice->getReplicationFactors(n1, n2, n3);

         a.scale((double)n1);
         b.scale((double)n2);
         c.scale((double)n3);

         lattice->assign(a, b, c);
         
         n1 = 1;
         n2 = 1;
         n3 = 1;
         lattice->setReplicationFactors(n1, n2, n3);

         sg->touchLattice();
         // Technically, this is a geom change because the atoms are no
         // longer fake, replicated visuals
         EventDispatcher::getDispatcher().publish(Event("GeomChange"));
         // Don't send two messages because we end up duplicating 
         // the fragment both times. Optimization...
         //EventDispatcher::getDispatcher().publish(Event("LatticeChange"));
      }
   }


   return ret;
}


/**
 * Indicate that we don't want to apply auto replicate.
 * Probably a moot point because the dims will be set to 1,1,1.
 */
bool PBCSuperCmd::shouldAutoReplicate() const
{
   return false;
}

bool PBCSuperCmd::shouldAutoReplicateOnUndo() const
{
   return true;
}

