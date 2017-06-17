#include "wxviz/SGViewer.H"
#include "wxviz/BackgroundCmd.H"


BackgroundCmd::BackgroundCmd(const string& name, Receiver *receiver)
  : Command(name, receiver)
{
  init();
}


BackgroundCmd::~BackgroundCmd()
{
}


void BackgroundCmd::init() 
{
  addParameter(new CommandParameter("red",0.));
  addParameter(new CommandParameter("green",0.));
  addParameter(new CommandParameter("blue",0.));
}


bool BackgroundCmd::execute() throw(EcceException)
{
  SGViewer *viewer = dynamic_cast<SGViewer*>(getReceiver());

  double red = getParameter("red")->getDouble();
  double green = getParameter("green")->getDouble();
  double blue = getParameter("blue")->getDouble();

  SbColor color((float)red,(float)green,(float)blue);

  p_undoColor = viewer->getBackgroundColor();

  viewer->setBackgroundColor(color);
  return true;
}


void BackgroundCmd::undo()
{
  SGViewer *viewer = dynamic_cast<SGViewer*>(getReceiver());
  viewer->setBackgroundColor(p_undoColor);
}


bool BackgroundCmd::isUndoable() const
{
  return true;
}
