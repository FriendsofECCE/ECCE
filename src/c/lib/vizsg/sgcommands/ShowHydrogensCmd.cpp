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


#include "viz/ShowHydrogensCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

ShowHydrogensCmd::ShowHydrogensCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 */
ShowHydrogensCmd::~ShowHydrogensCmd()
{

}



void ShowHydrogensCmd::init()
{

   setHelpMessage("ShowHydrogensCmd");

   p_switch = true;
   addParameter(new CommandParameter("switch", p_switch));
}



bool ShowHydrogensCmd::execute() throw(EcceException)
{
   bool ret = true;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   // Can't really get old value so just negate current setting
   p_switch = !getParameter("switch")->getBoolean();
   sg->setShowHydrogens(getParameter("switch")->getBoolean());


   return ret;
}


/**
 * Currently returns false to disable undo of this command.
 * Undo is implemented but we would need to add messaging so the
 * UI could stay in sink as use does undo/redo.
 */
bool ShowHydrogensCmd::isUndoable() const
{
   return false;
}


void ShowHydrogensCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   sg->setShowHydrogens(p_switch);
}
