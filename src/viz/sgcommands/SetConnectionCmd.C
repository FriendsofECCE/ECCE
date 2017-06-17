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

#include "viz/SetConnectionCmd.H"



SetConnectionCmd::SetConnectionCmd(const string& name, Receiver * receiver, const string& sender)
    : Command(name, receiver)
{
  p_sender = sender;
  init();
}


/**
 * Destructor.
 * Deletes undo related data.
 */
SetConnectionCmd::~SetConnectionCmd()
{
}


void SetConnectionCmd::init()
{
  addParameter(new CommandParameter("atomnum", 0));
  addParameter(new CommandParameter("connectionIndex", 0));
  setHelpMessage("Set connection index");
}



bool SetConnectionCmd::execute() throw(EcceException)
{
  bool ret = false;

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  if (frag != 0) {

    // Get parameter values
    int atomnum = getParameter("atomnum")->getInteger();
    int connectionIndex = getParameter("connectionIndex")->getInteger();

    if ((atomnum < frag->numAtoms()) && (atomnum >= 0)) {

      TAtm* atom = frag->atomRef(atomnum);

      // Save current values and Make change
      p_atomnum = atomnum;
      p_connectionIndex = atom->connectionIndex();
      atom->connectionIndex(connectionIndex);
      
      ret = true;
      
      // Publish change message
      Event event("GeomChange","",p_sender);
      EventDispatcher::getDispatcher().publish(event);
    }
  }
  return ret;
}


void SetConnectionCmd::undo()
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  if ((p_atomnum < frag->numAtoms()) && (p_atomnum >= 0)) {

    TAtm* atom = frag->atomRef(p_atomnum);

    atom->connectionIndex(p_connectionIndex);

    Event event("GeomChange");
    EventDispatcher::getDispatcher().publish(event);
  }
}


