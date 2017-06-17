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

#include "viz/DepthCueCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

DepthCueCmd::DepthCueCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
DepthCueCmd::~DepthCueCmd()
{

}



void DepthCueCmd::init()
{
   addParameter(new CommandParameter("hazeOn", false));
   addParameter(new CommandParameter("red", 0.0));
   addParameter(new CommandParameter("green", 0.0));
   addParameter(new CommandParameter("blue", 0.0));

   setHelpMessage("Enable/Disable depth cueing.");
}



bool DepthCueCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   bool flag = getParameter("hazeOn")->getBoolean();
   double r = getParameter("red")->getDouble();
   double g = getParameter("green")->getDouble();
   double b = getParameter("blue")->getDouble();

   sg->setDepthCue( flag, (float)r, (float)g, (float)b);


   return true;
}

/**
 * Currently returns false to disable undo of this command.
 * Would need to implement undo and add messaging so the
 * UI could stay in sink as user does undo/redo.
 */
bool DepthCueCmd::isUndoable() const
{
   return false;
}



void DepthCueCmd::undo()
{
   // Not implemented
   // SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

}
