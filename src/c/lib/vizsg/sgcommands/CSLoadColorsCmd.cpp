/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "inv/ChemKit/ChemColor.H"

#include "viz/CSLoadColorsCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

CSLoadColorsCmd::CSLoadColorsCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
CSLoadColorsCmd::~CSLoadColorsCmd()
{
}



void CSLoadColorsCmd::init()
{
   addParameter(new CommandParameter("atomIdx", 1));
   addParameter(new CommandParameter("red", 1.0));
   addParameter(new CommandParameter("green", 1.0));
   addParameter(new CommandParameter("blue", 1.0));

   setHelpMessage("Load system and user element colors.");
}



bool CSLoadColorsCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   int index = getParameter("atomIdx")->getInteger();
   double red = getParameter("red")->getDouble();
   double green = getParameter("green")->getDouble();
   double blue = getParameter("blue")->getDouble();

    ChemColor *colors = sg->getColorNode();
      if (colors) {
         colors->atomColor.set1Value(index, red, green, blue);
      }

   return true;
}


bool CSLoadColorsCmd::isUndoable() const
{
  return false;
}

void CSLoadColorsCmd::undo()
{
}
