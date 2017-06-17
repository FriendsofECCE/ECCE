#include "inv/nodes/SoSeparator.H"


#include "wxviz/SGViewer.H"
#include "wxviz/AxisCmd.H"

#include "viz/EcceAxis.H"

AxisCmd::AxisCmd(const string& name, Receiver *receiver)
       : Command(name, receiver)
{
  init();
}

AxisCmd::~AxisCmd()
{
}

void AxisCmd::init() 
{
   addParameter(new CommandParameter("switch","false"));
}

bool AxisCmd::execute() throw(EcceException)
{
  SGViewer *viewer = dynamic_cast<SGViewer*>(getReceiver());

  bool value = getParameter("switch")->getBoolean();

  SoSeparator *axistop = viewer->getAxisRoot();
  if (value) {
     axistop->addChild(new EcceAxis);
  } else {
     axistop->removeAllChildren();
  }

  return true;
}

/**
 * Currently returns false to disable undo of this command.
 * Would need to implement undo and add messaging so the
 * UI could stay in sink as user does undo/redo.
 */
bool AxisCmd::isUndoable() const
{
   return false;
}

void AxisCmd::undo()
{
}
