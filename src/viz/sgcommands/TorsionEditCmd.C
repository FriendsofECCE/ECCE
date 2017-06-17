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

#include "viz/TorsionEditCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

TorsionEditCmd::TorsionEditCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 */
TorsionEditCmd::~TorsionEditCmd()
{

}



void TorsionEditCmd::init()
{
   addParameter(new CommandParameter("torsion", -1.0));
   addParameter(new CommandParameter("atom1", -1));
   addParameter(new CommandParameter("atom2", -1));
   addParameter(new CommandParameter("atom3", -1));
   addParameter(new CommandParameter("atom4", -1));
   addParameter(new CommandParameter("all", true));
}



bool TorsionEditCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag) {
      double torsion = getParameter("torsion")->getDouble();

      int atom1 = getParameter("atom1")->getInteger();
      int atom2 = getParameter("atom2")->getInteger();
      int atom3 = getParameter("atom3")->getInteger();
      int atom4 = getParameter("atom4")->getInteger();
      bool rotateAll = getParameter("all")->getBoolean();

      // Use the supplied atoms
      if (atom1 != -1 && atom2 != -1 && atom3 != -1 && atom4 != -1) {
         cloneFragment(frag);

         TAtm  *ref1 = frag->atomRef(atom1);
         TAtm  *ref2 = frag->atomRef(atom2);
         TAtm  *ref3 = frag->atomRef(atom3);
         TAtm  *ref4 = frag->atomRef(atom4);

         if (rotateAll) {
           ret = !frag->dihedralAll(ref1, ref2, ref3, ref4, torsion);
         } else {
           ret = !frag->dihedral(ref1, ref2, ref3, ref4, torsion);
         }

         if (!ret) { 
            throw EcceException(
                  "A cyclical system found so the torsion was not "
                  "changed.  The fourth atom selected is the one that we try "
                  "to move.", WHERE);
         } else {
            sg->touchChemDisplay();
            EventDispatcher::getDispatcher().publish(Event("GeomChange"));
         }
      }
   }


   return ret;
}
