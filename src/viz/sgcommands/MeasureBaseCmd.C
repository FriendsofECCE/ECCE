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

#include "viz/MeasureBaseCmd.H"
#include "viz/SGContainer.H"

MeasureBaseCmd::MeasureBaseCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   p_addIndex = -1;
}



/**
 * Destructor.
 */
MeasureBaseCmd::~MeasureBaseCmd()
{

}


void MeasureBaseCmd::undo()
{
   if (p_addIndex >= 0) {
     SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
     int numChildren = sg->getTopLabel()->getNumChildren();
     if (p_addIndex < numChildren) {
        sg->getTopLabel()->removeChild(p_addIndex);
     }
   }
}
