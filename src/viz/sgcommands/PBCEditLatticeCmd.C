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

#include "viz/PBCEditLatticeCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/SGLattice.H"

PBCEditLatticeCmd::PBCEditLatticeCmd(const string& name, Receiver * receiver, SGFragment *base)
    : PBCCmd(name, receiver, base)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
PBCEditLatticeCmd::~PBCEditLatticeCmd()
{

}



void PBCEditLatticeCmd::init()
{
   addParameter(new CommandParameter("a", 1));
   addParameter(new CommandParameter("b", 1.0));
   addParameter(new CommandParameter("c", 0.0));
   addParameter(new CommandParameter("alpha", 90.0));
   addParameter(new CommandParameter("beta", 90.0));
   addParameter(new CommandParameter("gamma", 90.0));
   addParameter(new CommandParameter("lock", false));
}



bool PBCEditLatticeCmd::execute() throw(EcceException)
{
   bool ret = true;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   if (frag) {

      LatticeDef *lattice = frag->getLattice();
      if (lattice) {
         double a = getParameter("a")->getDouble();
         double b = getParameter("b")->getDouble();
         double c = getParameter("c")->getDouble();
         double alpha = getParameter("alpha")->getDouble();
         double beta = getParameter("beta")->getDouble();
         double gamma = getParameter("gamma")->getDouble();
         bool lock = getParameter("lock")->getBoolean();

         if (!lock) {
           frag->convertToFractionalCoords(false);
           lattice->assign(a, b, c, alpha, beta, gamma);
           frag->convertFromFractionalCoords();
           sg->touchChemDisplay();
           EventDispatcher::getDispatcher().publish(Event("GeomChange"));
         } else {
           lattice->assign(a, b, c, alpha, beta, gamma);
           EventDispatcher::getDispatcher().publish(Event("LatticeChange"));
         }
         sg->touchLattice();
      }
   }


   return ret;
}
