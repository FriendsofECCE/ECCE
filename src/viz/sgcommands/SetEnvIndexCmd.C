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

#include "viz/SetEnvIndexCmd.H"



SetEnvIndexCmd::SetEnvIndexCmd(const string& name, Receiver * receiver, const string& sender)
    : Command(name, receiver)
{
  p_sender = sender;
  init();
}


/**
 * Destructor.
 * Deletes undo related data.
 */
SetEnvIndexCmd::~SetEnvIndexCmd()
{
}


void SetEnvIndexCmd::init()
{
  addParameter(new CommandParameter("atomnum", 0));
  addParameter(new CommandParameter("envIndex", 0));
  setHelpMessage("Set Env index");
}



bool SetEnvIndexCmd::execute() throw(EcceException)
{
  bool ret = false;


  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  if (frag != 0) {

    // Get parameter values
    int atomnum = getParameter("atomnum")->getInteger();
    int envIndex = getParameter("envIndex")->getInteger();

    if ((atomnum < frag->numAtoms()) && (atomnum >= 0)) {

      TAtm* atom = frag->atomRef(atomnum);

      // Save current values and Make change
      p_atomnum = atomnum;
      p_envIndex = atom->dihedralIndex();
      atom->dihedralIndex(envIndex);
      
      ret = true;
      
      // Publish change message
      Event event("GeomChange","",p_sender);
      EventDispatcher::getDispatcher().publish(event);
    }
  }
  return ret;
}


void SetEnvIndexCmd::undo()
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  if ((p_atomnum < frag->numAtoms()) && (p_atomnum >= 0)) {

    TAtm* atom = frag->atomRef(p_atomnum);

    atom->dihedralIndex(p_envIndex);

    Event event("GeomChange");
    EventDispatcher::getDispatcher().publish(event);
  }
}


