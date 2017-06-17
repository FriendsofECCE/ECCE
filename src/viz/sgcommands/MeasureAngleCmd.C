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

#include "viz/AtomMeasureAngle.H"
#include "viz/MeasureAngleCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

MeasureAngleCmd::MeasureAngleCmd(const string& name, Receiver * receiver)
    : MeasureBaseCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 */
MeasureAngleCmd::~MeasureAngleCmd()
{

}



void MeasureAngleCmd::init()
{
   setHelpMessage("Angle: Select three atoms to display an angle measurement.  "
             "Once the measurement is displayed, click on the line to edit the "
             "angle.  The last atom selected will move.");
}



bool MeasureAngleCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag) {
      //   verify if atoms where selected
      if (frag->m_atomHighLight.size() == 3) {
         ret = true;

         p_addIndex = sg->getTopLabel()->getNumChildren();

         SoSeparator *sep = new SoSeparator;
         AtomMeasureAngle *angle=new
            AtomMeasureAngle(frag,
                  frag->atomRef(frag->m_atomHighLight[0]),
                  frag->atomRef(frag->m_atomHighLight[1]),
                  frag->atomRef(frag->m_atomHighLight[2]),
                  sep);
         sep->addChild(angle);


         sg->getTopLabel()->addChild(sep);

         EventDispatcher::getDispatcher().publish(Event("MeasureAdded"));

      } else {
         throw EcceException("Select three atoms to create an angle measure.", WHERE);
      }
   }

   return ret;
}
