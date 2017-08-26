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

#include "viz/ClearResidueInfoCmd.hpp"
#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"

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
