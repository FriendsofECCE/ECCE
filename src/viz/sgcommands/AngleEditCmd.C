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

#include "viz/AngleEditCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

AngleEditCmd::AngleEditCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 */
AngleEditCmd::~AngleEditCmd()
{

}



void AngleEditCmd::init()
{
   addParameter(new CommandParameter("angle", -1.0));
   addParameter(new CommandParameter("atom1", -1));
   addParameter(new CommandParameter("atom2", -1));
   addParameter(new CommandParameter("atom3", -1));
   addParameter(new CommandParameter("all", true));
}



bool AngleEditCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag) {
      double angle = getParameter("angle")->getDouble();

      int atom1 = getParameter("atom1")->getInteger();
      int atom2 = getParameter("atom2")->getInteger();
      int atom3 = getParameter("atom3")->getInteger();
      bool moveConnected = getParameter("all")->getBoolean();

      // Use the supplied atoms
      if (atom1 != -1 && atom2 != -1 && atom3 != -1) {
         cloneFragment(frag);

         TAtm  *ref1 = frag->atomRef(atom1);
         TAtm  *ref2 = frag->atomRef(atom2);
         TAtm  *ref3 = frag->atomRef(atom3);

         ret = frag->angle(*ref1, *ref2, *ref3, angle, moveConnected);

         if (!ret) { 
            throw EcceException(
                  "A cyclical system found so the angle was not "
                  "changed.  The third atom selected is the one that we try "
                  "to move.", WHERE);
         } else {
            sg->touchChemDisplay();
            EventDispatcher::getDispatcher().publish(Event("GeomChange"));
         }
      }
   }


   return ret;
}
