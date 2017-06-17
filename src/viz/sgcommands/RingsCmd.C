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

#include "viz/RingsCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

RingsCmd::RingsCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
RingsCmd::~RingsCmd()
{

}



void RingsCmd::init()
{
   addParameter(new CommandParameter("switch", false));

   setHelpMessage("Enable/Disable aromatic ring display.");
}



bool RingsCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   bool flag = getParameter("switch")->getBoolean();

   p_lastState = sg->getShowRings();

   if (p_lastState != flag) {
      sg->setShowRings(flag);
      sg->touchChemDisplay();
   }


   return true;
}


/**
 * Currently returns false to disable undo of this command.
 * Undo is implemented but we would need to add messaging so the
 * UI could stay in sink as user does undo/redo.
 */
bool RingsCmd::isUndoable() const
{
   return false;
}

void RingsCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   sg->setShowRings(p_lastState);
   sg->touchChemDisplay();

}
