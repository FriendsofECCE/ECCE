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



bool CSLoadColorsCmd::execute()
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

   //  ECCE_DEBUG_ATOM_COLOR=1 prints every colour ECCE hands the scene
   //  graph.  For the uniform-green-atoms report (#83) this separates the
   //  two possibilities that look identical on screen: if the numbers here
   //  are correct per element (oxygen 1,0,0 and hydrogen 1,1,1) then the
   //  data and the lookup are fine and the fault is below us, in the GL
   //  or driver path; if they all come out green, it is ours.
   if (getenv("ECCE_DEBUG_ATOM_COLOR")) {
      printf("[ATOMCOLOR] index=%d  r=%.3f g=%.3f b=%.3f%s\n",
             index, red, green, blue, colors ? "" : "   (NO COLOR NODE)");
      fflush(stdout);
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
