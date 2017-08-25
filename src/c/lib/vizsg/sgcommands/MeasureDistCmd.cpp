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

#include "viz/AtomMeasureDist.H"
#include "viz/MeasureDistCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

MeasureDistCmd::MeasureDistCmd(const string& name, Receiver * receiver)
    : MeasureBaseCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 */
MeasureDistCmd::~MeasureDistCmd()
{

}



void MeasureDistCmd::init()
{
   setHelpMessage("Bond Distance: Select two atoms or a bond to display "
         "the a distance measurement.  Once the measurement is displayed "
         "click on the line to edit the distance.  The last atom selected "
         "will move.");
}



bool MeasureDistCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag) {
      if (frag->m_atomHighLight.size() == 2) {
         ret = true;

         p_addIndex = sg->getTopLabel()->getNumChildren();

         SoSeparator *sep = new SoSeparator;
         AtomMeasureDist *dist=new
            AtomMeasureDist(frag,
                  frag->atomRef(frag->m_atomHighLight[0]),
                  frag->atomRef(frag->m_atomHighLight[1]),
                  sep);
         sep->addChild(dist);

         sg->getTopLabel()->addChild(sep);

         EventDispatcher::getDispatcher().publish(Event("MeasureAdded"));

      } else {
         throw EcceException("Select two atoms to create a distance measure.",WHERE);
      }
   }

   return ret;
}
