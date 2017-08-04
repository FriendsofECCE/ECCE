#include <iostream>
  using namespace std;

#include "wxviz/SGViewer.H"
#include "wxviz/GoHomeCmd.H"

GoHomeCmd::GoHomeCmd(const string& name, Receiver *receiver)
       : Command(name, receiver)
{
  init();
}

GoHomeCmd::~GoHomeCmd()
{
}

void GoHomeCmd::init() 
{
}

bool GoHomeCmd::execute() throw(EcceException)
{
  SGViewer *viewer = dynamic_cast<SGViewer*>(getReceiver());
  viewer->resetToHomePosition();

  return true;
}

void GoHomeCmd::undo()
{
}

bool GoHomeCmd::isUndoable() const
{
   return false;
}
