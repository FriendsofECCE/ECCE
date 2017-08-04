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

#include "viz/SetBehaviorCmd.H"



SetBehaviorCmd::SetBehaviorCmd(const string& name, Receiver * receiver, const string& sender)
    : FragCloneCmd(name, receiver)
{
  p_sender = sender;
  init();
}


/**
 * Destructor.
 * Deletes undo related data.
 */
SetBehaviorCmd::~SetBehaviorCmd()
{
}


void SetBehaviorCmd::init()
{
  addParameter(new CommandParameter("atomnum", 0));
  addParameter(new CommandParameter("behavior", TAtm::UNSPECIFIED));
  setHelpMessage("Set behavior");
}



bool SetBehaviorCmd::execute() throw(EcceException)
{
  bool ret = false;

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  if (frag != 0) {

     // get paramter values
     int ibehavior = 0;
     ibehavior = getParameter("behavior")->getInteger();
     TAtm::BehaviorType behavior =
        static_cast<TAtm::BehaviorType>(ibehavior);

     if (frag->m_atomHighLight.size() > 0) {

        // Not sure how to retain these messages
        /*
        string undoMsg = "Clear atom behavior tags";
        if (behavior == TAtm::QUANTUM) {
          undoMsg = "Mark as quantum atoms";
        } 
        else if (behavior == TAtm::POINT) {
          undoMsg = "Mark as point charges";
        }
        */

        for (int idx=0; idx<frag->m_atomHighLight.size(); idx++) {

           // Change behavior of the atom.
           // We have a rule that you can't change a Quantum atom
           // to a point charge using this method.
           TAtm* highAtom = frag->atomRef(frag->m_atomHighLight[idx]);
           if (behavior == TAtm::POINT) {
              if ( highAtom->getBehaviorType() != TAtm::QUANTUM) {
                 highAtom->setBehaviorType(behavior);
              }
           } 
           else {
              highAtom->setBehaviorType(behavior);
           }
        }

        // Publish change message
        Event event("GeomChange","",p_sender);
        EventDispatcher::getDispatcher().publish(event);
     }
  }
  return ret;
}
 




