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

#include "viz/RenumberResiduesCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

RenumberResiduesCmd::RenumberResiduesCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 */
RenumberResiduesCmd::~RenumberResiduesCmd()
{

}



void RenumberResiduesCmd::init()
{
}



bool RenumberResiduesCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag) {
      ret = true;

      // NOTE: could do a more efficient undo than cloning here
      cloneFragment(frag);

      frag->renumberResidues();

      EventDispatcher::getDispatcher().publish(Event("ResidueChange"));
   }


   return ret;
}
