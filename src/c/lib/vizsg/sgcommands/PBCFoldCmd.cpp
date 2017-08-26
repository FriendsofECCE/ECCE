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

#include "viz/PBCFoldCmd.hpp"
#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"
#include "viz/SGLattice.hpp"

PBCFoldCmd::PBCFoldCmd(const string& name, Receiver * receiver, SGFragment *base)
    : PBCCmd(name, receiver, base)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
PBCFoldCmd::~PBCFoldCmd()
{

}



void PBCFoldCmd::init()
{
}



bool PBCFoldCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   if (frag) {

     // Check for valid LatticeDef object
     LatticeDef *lattice = frag->getLattice();
     if (lattice) {
       ret = true;

       frag->convertToFractionalCoords(false);

       int natoms = frag->numAtoms();
       int i;
       TAtm* atm;
       double f1, f2, f3;
       const double *crd;

       for (i=0; i<natoms; i++) {
         atm = frag->atomRef(i);
         crd = atm->coordinates();
         f1 = crd[0];
         f2 = crd[1];
         f3 = crd[2];

         f1 = f1-(double)(int)f1;
         if (f1<0.0) f1 += 1.0;
         f2 = f2-(double)(int)f2;
         if (f2<0.0) f2 += 1.0;
         f3 = f3-(double)(int)f3;
         if (f3<0.0) f3 += 1.0;

         atm->coordinates(f1,f2,f3);
       }

       frag->convertFromFractionalCoords();

       frag->deleteAllBonds();
       frag->addCovalentBonds();
       frag->touchNumbers();

       sg->touchChemDisplay();
       sg->touchLattice();
       EventDispatcher::getDispatcher().publish(Event("GeomChange"));
     }
   }
   return ret;
}

