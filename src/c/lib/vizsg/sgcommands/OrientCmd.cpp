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


#include "viz/OrientCmd.hpp"
#include "viz/FragCloneCmd.hpp"
#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"

OrientCmd::OrientCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
OrientCmd::~OrientCmd()
{
}



void OrientCmd::init()
{
   setHelpMessage("Orient molecule to align with coordinate axes");
}



// @todo chain list
// @todo radius
bool OrientCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   cloneFragment(frag);  // undo

   frag->orientFrag();

   EventDispatcher::getDispatcher().publish(Event("GeomChange"));

   return true;
}
