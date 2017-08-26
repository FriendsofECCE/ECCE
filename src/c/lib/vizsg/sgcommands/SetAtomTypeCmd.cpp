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

#include "viz/SetAtomTypeCmd.hpp"



SetAtomTypeCmd::SetAtomTypeCmd(const string& name, Receiver * receiver, const string& sender)
    : Command(name, receiver)
{
  p_sender = sender;
  init();
}


/**
 * Destructor.
 * Deletes undo related data.
 */
SetAtomTypeCmd::~SetAtomTypeCmd()
{
}


void SetAtomTypeCmd::init()
{
  addParameter(new CommandParameter("atomnum", 0));
  addParameter(new CommandParameter("atomType", ""));
  setHelpMessage("Set Atom Type");
}



bool SetAtomTypeCmd::execute() throw(EcceException)
{
  bool ret = false;


  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  if (frag != 0) {

    // Get parameter values
    int atomnum = getParameter("atomnum")->getInteger();
    string atomType = getParameter("atomType")->getString();

    if ((atomnum < frag->numAtoms()) && (atomnum >= 0)) {

      TAtm* atom = frag->atomRef(atomnum);

      // Save current values and Make change
      p_atomnum = atomnum;
      p_atomType = atom->atomType();
      atom->atomType(atomType.c_str());

      ret = true;

      // Publish change message
      Event event("GeomChange","",p_sender);
      EventDispatcher::getDispatcher().publish(event);
    }
  }
  return ret;
}


void SetAtomTypeCmd::undo()
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  if ((p_atomnum < frag->numAtoms()) && (p_atomnum >= 0)) {

    TAtm* atom = frag->atomRef(p_atomnum);

    atom->atomType(p_atomType.c_str());

    Event event("GeomChange");
    EventDispatcher::getDispatcher().publish(event);
  }
}




