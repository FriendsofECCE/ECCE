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

#include "viz/SetAtomNameCmd.H"



SetAtomNameCmd::SetAtomNameCmd(const string& name, Receiver * receiver, const string& sender)
    : Command(name, receiver)
{
  p_sender = sender;
  init();
}


/**
 * Destructor.
 * Deletes undo related data.
 */
SetAtomNameCmd::~SetAtomNameCmd()
{
}


void SetAtomNameCmd::init()
{
  addParameter(new CommandParameter("atomnum", 0));
  addParameter(new CommandParameter("atomName", ""));
  setHelpMessage("Set Atom Name");
}



bool SetAtomNameCmd::execute() throw(EcceException)
{
  bool ret = false;


  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  if (frag != 0) {

    // Get parameter values
    int atomnum = getParameter("atomnum")->getInteger();
    string atomName = getParameter("atomName")->getString();

    if ((atomnum < frag->numAtoms()) && (atomnum >= 0)) {

      TAtm* atom = frag->atomRef(atomnum);

      // Save current values and Make change
      p_atomnum = atomnum;
      p_atomName = atom->atomName();
      atom->atomName(atomName.c_str());
      
      ret = true;
      
      // Publish change message
      Event event("GeomChange","",p_sender);
      EventDispatcher::getDispatcher().publish(event);
    }
  }
  return ret;
}


void SetAtomNameCmd::undo()
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  if ((p_atomnum < frag->numAtoms()) && (p_atomnum >= 0)) {

    TAtm* atom = frag->atomRef(p_atomnum);

    atom->atomName(p_atomName.c_str());

    Event event("GeomChange");
    EventDispatcher::getDispatcher().publish(event);
  }
}




