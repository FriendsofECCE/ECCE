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

#include "viz/AtomMeasureTorsion.H"
#include "viz/MeasureTorsionCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

MeasureTorsionCmd::MeasureTorsionCmd(const string& name, Receiver * receiver)
    : MeasureBaseCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 */
MeasureTorsionCmd::~MeasureTorsionCmd()
{

}



void MeasureTorsionCmd::init()
{
   setHelpMessage("Torsion: Select four atoms to display a dihedral measurement.  "
         "Once the measurement is displayed, click on the plane to edit the "
         "angle.  The last atom selected will move.");
}



bool MeasureTorsionCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag) {
      if (frag->m_atomHighLight.size() == 4) {
         ret = true;

         p_addIndex = sg->getTopLabel()->getNumChildren();

         SoSeparator *sep = new SoSeparator;
         AtomMeasureTorsion *torsion = new
            AtomMeasureTorsion(frag,
                  frag->atomRef(frag->m_atomHighLight[0]),
                  frag->atomRef(frag->m_atomHighLight[1]),
                  frag->atomRef(frag->m_atomHighLight[2]),
                  frag->atomRef(frag->m_atomHighLight[3]),
                  sep);
         sep->addChild(torsion);

         sg->getTopLabel()->addChild(sep);

         EventDispatcher::getDispatcher().publish(Event("MeasureAdded"));

      } else {
         throw EcceException( "Select four atoms to create a torsion measure.", WHERE);
      }
   }

   return ret;
}
