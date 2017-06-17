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

#include "viz/ClearResidueInfoCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

ClearResidueInfoCmd::ClearResidueInfoCmd(const string& name, Receiver * receiver)
      : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
ClearResidueInfoCmd::~ClearResidueInfoCmd()
{

}



void ClearResidueInfoCmd::init()
{
}



bool ClearResidueInfoCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   // Set up undo data
   cloneFragment(frag);

   frag->clearResidueInfo();

   EventDispatcher::getDispatcher().publish(Event("ResidueChange"));

   return true;
}
