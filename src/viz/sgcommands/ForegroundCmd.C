
#include "inv/ChemKit/ChemColor.H"

#include "viz/SGContainer.H"
#include "viz/ForegroundCmd.H"
#include "viz/EcceDispParam.H"


ForegroundCmd::ForegroundCmd(const string& name, Receiver *receiver)
  : Command(name, receiver)
{
  init();
}


ForegroundCmd::~ForegroundCmd()
{
}


void ForegroundCmd::init()
{
  SGContainer * sg = dynamic_cast<SGContainer*>(getReceiver());
  p_red = 1.0;
  p_green = 1.0;
  p_blue = 1.0;
  ChemColor *colors = sg->getColorNode();
  if (colors) {
    p_red = colors->atomLabelColor[0][0];
    p_green = colors->atomLabelColor[0][1];
    p_blue = colors->atomLabelColor[0][2];
  }
  addParameter(new CommandParameter("red", p_red));
  addParameter(new CommandParameter("green", p_green));
  addParameter(new CommandParameter("blue", p_blue));
}


bool ForegroundCmd::execute() throw(EcceException)
{
  SGContainer * sg = dynamic_cast<SGContainer*>(getReceiver());

  float red = getParameter("red")->getDouble();
  float green = getParameter("green")->getDouble();
  float blue = getParameter("blue")->getDouble();

  ChemColor *colors = sg->getColorNode();
  if (colors) {
    colors->atomLabelColor.set1Value(0, red, green, blue);
    colors->bondLabelColor.set1Value(0, red, green, blue);
  }

  // Measures use this
  EcceDispParam options;
  options.setForeground(red,green,blue);

  return colors;
}


void ForegroundCmd::undo()
{
  SGContainer * sg = dynamic_cast<SGContainer*>(getReceiver());
  ChemColor *colors = sg->getColorNode();
  if (colors) {
    colors->atomLabelColor.set1Value(0, p_red, p_green, p_blue);
    colors->bondLabelColor.set1Value(0, p_red, p_green, p_blue);
  }
}


bool ForegroundCmd::isUndoable() const
{
  return true;
}
