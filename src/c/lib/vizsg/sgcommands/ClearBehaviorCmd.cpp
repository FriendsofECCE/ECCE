/**
 * @file
 *
 *
 */

#include <iostream>
  using std::cout;
  using std::endl;
  using std::ends;

#include "util/EventDispatcher.hpp"
#include "util/Event.hpp"
  using namespace ecce;

#include "tdat/TAtm.hpp"

#include "wxgui/WindowEvent.hpp"
  using namespace ecce;

#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"

#include "viz/ClearBehaviorCmd.hpp"



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
 
