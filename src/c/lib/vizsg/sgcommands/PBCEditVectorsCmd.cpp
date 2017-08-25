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

#include "viz/PBCEditVectorsCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/SGLattice.H"

PBCEditVectorsCmd::PBCEditVectorsCmd(const string& name, Receiver * receiver, SGFragment *base)
    : PBCCmd(name, receiver, base)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
PBCEditVectorsCmd::~PBCEditVectorsCmd()
{

}



void PBCEditVectorsCmd::init()
{
   addParameter(new CommandParameter("a1x", 1.0));
   addParameter(new CommandParameter("a1y", 1.0));
   addParameter(new CommandParameter("a1z", 1.0));
   addParameter(new CommandParameter("a2x", 1.0));
   addParameter(new CommandParameter("a2y", 1.0));
   addParameter(new CommandParameter("a2z", 1.0));
   addParameter(new CommandParameter("a3x", 1.0));
   addParameter(new CommandParameter("a3y", 1.0));
   addParameter(new CommandParameter("a3z", 1.0));
   addParameter(new CommandParameter("lock", false));
}



bool PBCEditVectorsCmd::execute() throw(EcceException)
{
   bool ret = true;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   if (frag) {

      LatticeDef *lattice = frag->getLattice();
      if (lattice) {
         cloneFragment(frag);
         double a1x = getParameter("a1x")->getDouble();
         double a1y = getParameter("a1y")->getDouble();
         double a1z = getParameter("a1z")->getDouble();

         double a2x = getParameter("a2x")->getDouble();
         double a2y = getParameter("a2y")->getDouble();
         double a2z = getParameter("a2z")->getDouble();

         double a3x = getParameter("a3x")->getDouble();
         double a3y = getParameter("a3y")->getDouble();
         double a3z = getParameter("a3z")->getDouble();

         bool lock = getParameter("lock")->getBoolean();

         if (!lock) {
           frag->convertToFractionalCoords(false);
           lattice->assign(MPoint(a1x,a1y,a1z), MPoint(a2x,a2y,a2z), MPoint(a3x,a3y,a3z));
           frag->convertFromFractionalCoords();
           sg->touchChemDisplay();
           EventDispatcher::getDispatcher().publish(Event("GeomChange"));
         } else {
           lattice->assign(MPoint(a1x,a1y,a1z), MPoint(a2x,a2y,a2z), MPoint(a3x,a3y,a3z));
         }

         sg->touchLattice();
      }
   }


   return ret;
}
