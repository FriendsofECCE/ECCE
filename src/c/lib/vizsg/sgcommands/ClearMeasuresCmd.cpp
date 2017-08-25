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

#include "viz/ClearMeasuresCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

ClearMeasuresCmd::ClearMeasuresCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 */
ClearMeasuresCmd::~ClearMeasuresCmd()
{

}



void ClearMeasuresCmd::init()
{
   setHelpMessage("Bond Distance: Select two atoms or a bond to display "
         "the a distance measurement.  Once the measurement is displayed "
         "click on the line to edit the distance.  The last atom selected "
         "will move.");
}



bool ClearMeasuresCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   if ( sg->getTopLabel()->getNumChildren()) {
      ret = true;
      sg->getTopLabel()->removeAllChildren();

      EventDispatcher::getDispatcher().publish(Event("MeasureAdded"));

   }

   return ret;
}


/**
 * For now make this command undoable.  It wouldn't be that hard to
 * save off the current measure info to restore it...
 */
bool ClearMeasuresCmd::isUndoable() const
{
   return false;
}

void ClearMeasuresCmd::undo()
{
}
