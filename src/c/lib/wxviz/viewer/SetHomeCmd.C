#include <iostream>
  using namespace std;

#include "wxviz/SGViewer.H"
#include "wxviz/SetHomeCmd.H"

SetHomeCmd::SetHomeCmd(const string& name, Receiver *receiver)
       : Command(name, receiver)
{
  init();
}

SetHomeCmd::~SetHomeCmd()
{
}

void SetHomeCmd::init() 
{
}

bool SetHomeCmd::execute() throw(EcceException)
{
  SGViewer *viewer = dynamic_cast<SGViewer*>(getReceiver());
  viewer->saveHomePosition();

  return true;
}

void SetHomeCmd::undo()
{
   cout << "TODO SetHomeCmd " << endl;
}
