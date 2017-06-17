/**
 * @file
 *
 *
 */

#include <iostream>
  using std::cout;
  using std::endl;
  using std::ends;

#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;

#include "tdat/TAtm.H"

#include "wxgui/WindowEvent.H"
  using namespace ecce;

#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

#include "viz/ClearBehaviorCmd.H"



ClearBehaviorCmd::ClearBehaviorCmd(const string& name, Receiver * receiver, const string& sender)
    : FragCloneCmd(name, receiver)
{
  p_sender = sender;
  init();
}


/**
 * Destructor.
 * Deletes undo related data.
 */
ClearBehaviorCmd::~ClearBehaviorCmd()
{
}


void ClearBehaviorCmd::init()
{
}



bool ClearBehaviorCmd::execute() throw(EcceException)
{
  bool ret = false;

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  if (frag != 0) {
     frag->clearAtomBehavior();

     // Publish change message
     Event event("GeomChange","",p_sender);
     EventDispatcher::getDispatcher().publish(event);
  }

  return ret;
}
 
