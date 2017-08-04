#include <iostream>
  using std::cout;
  using std::endl;

#include "inv/nodes/SoSwitch.H"

#include "viz/SGContainer.H"
#include "viz/SurfDisplayTypeCmd.H"

SurfDisplayTypeCmd::SurfDisplayTypeCmd(const string& name, Receiver *receiver,
                                       IPropCalculation *calc)
  : PropCmd(name, receiver, calc)
{
  init();
}

SurfDisplayTypeCmd::~SurfDisplayTypeCmd() 
{
}

void SurfDisplayTypeCmd::init()
{
   addParameter(new CommandParameter("IsosurfStyle", "Solid"));
}

bool SurfDisplayTypeCmd::execute() throw(EcceException)
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  string displayType = getParameter("IsosurfStyle")->getString();;

  SoSwitch *isoSwitch = (SoSwitch*)((sg->getMORoot())->getChild(0));
  if (displayType=="Solid" || displayType=="Transparent") {
    isoSwitch->whichChild = 0;
  }
  else if (displayType == "Mesh")
    isoSwitch->whichChild = 1;
  else if (displayType == "Contour")
    isoSwitch->whichChild = 2;
  else if (displayType == "None")  // hide
    isoSwitch->whichChild = SO_SWITCH_NONE;

  return true;

}


