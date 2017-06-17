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

#include "viz/LengthEditCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

LengthEditCmd::LengthEditCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 */
LengthEditCmd::~LengthEditCmd()
{

}



void LengthEditCmd::init()
{
   addParameter(new CommandParameter("length", 1.44));
   addParameter(new CommandParameter("atom1", -1));
   addParameter(new CommandParameter("atom2", -1));
   addParameter(new CommandParameter("all", true));
}



bool LengthEditCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag) {
      double length = getParameter("length")->getDouble();

      int atom1 = getParameter("atom1")->getInteger();
      int atom2 = getParameter("atom2")->getInteger();
      bool moveAll = getParameter("all")->getBoolean();

      // Use the supplied atoms
      if (atom1 != -1 && atom2 != -1) {
         cloneFragment(frag);

         ret = frag->bondLength(*frag->atomRef(atom1), *frag->atomRef(atom2),
               length, moveAll);

         if (!ret) { 
            throw EcceException(
              "Cyclical dependency found.  The distance cannot be changed with"
              " this method. ", WHERE);
         } else {
            sg->touchChemDisplay();
            EventDispatcher::getDispatcher().publish(Event("GeomChange"));
         }
      }
   }


   return ret;
}
