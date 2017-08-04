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


#include "viz/OrientCmd.H"
#include "viz/FragCloneCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

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
