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


#include "viz/TwoDMoveCmd.H"
#include "viz/FragCloneCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

TwoDMoveCmd::TwoDMoveCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 */
TwoDMoveCmd::~TwoDMoveCmd()
{
}



void TwoDMoveCmd::init()
{
   addParameter(new CommandParameter("deltax", 0.0));
   addParameter(new CommandParameter("deltay", 0.0));
   addParameter(new CommandParameter("deltaz", 0.0));
   addParameter(new CommandParameter("movez", false));
   addParameter(new CommandParameter("doundo", false));
}



bool TwoDMoveCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   bool ret = false;

   if (frag && frag->m_atomHighLight.size() > 0 ) {

      bool selChanged = false;
      double deltaX = getParameter("deltax")->getDouble();
      double deltaY = getParameter("deltay")->getDouble();
      double deltaZ = getParameter("deltaz")->getDouble();
      bool moveZ = getParameter("movez")->getBoolean();
      bool doundo = getParameter("doundo")->getBoolean();

      // For now at least we don't neccesarily want to use undo for
      // every motion;  this currently maps to user stopping the motion
      if (doundo) {
         cloneFragment(frag);  // undo
         ret = true;
      }

      int idx;
      TAtm *atm;

      // If an atom is selected, we need to force selection of nubs too.
      // Make a bit mask to do this.
      vector<bool> atomBits(frag->numAtoms(), false);
      for (idx=0; idx<frag->m_atomHighLight.size(); idx++) {
         atomBits[frag->m_atomHighLight[idx]] = true;
      }


      vector<TAtm*> *atoms = frag->atoms();

      // Also if its a nub, make sure its owner goes too
      for (idx=0; idx<atoms->size(); idx++) {
         if (atomBits[idx]) {
            atm = (*atoms)[idx];
            if (atm->atomicSymbol() == "Nub") {
               vector<TAtm*> *connectedAtoms = atm->connectedAtoms();
               for (int jdx=0; jdx<connectedAtoms->size(); jdx++) {
                  // Should only be one
                  TAtm *catom = (*connectedAtoms)[jdx];
                  atomBits[catom->index()] = true;
                  selChanged = true;
               }
               delete connectedAtoms;
            }
         }
      }

      // Add connected nubs to the selection bit mask
      for (idx=0; idx<atoms->size(); idx++) {
         if (atomBits[idx]) {
            atm = (*atoms)[idx];
            vector<TAtm*> *connectedAtoms = atm->connectedAtoms();
            for (int jdx=0; jdx<connectedAtoms->size(); jdx++) {
               TAtm *catom = (*connectedAtoms)[jdx];
               if (catom->atomicSymbol() == "Nub") {
                  atomBits[catom->index()] = true;
                  selChanged = true;
               }
            }
            delete connectedAtoms;

         }
      }
      delete atoms;


      // Reset highlight array if necessary
      if (selChanged) {
         frag->m_atomHighLight.clear();
         for (idx=0; idx<atomBits.size(); idx++) {
            if (atomBits[idx]) {
               frag->m_atomHighLight.push_back(idx);
            }
         }
      }


      for (idx=0; idx<frag->m_atomHighLight.size(); idx++) {
         atm = frag->atomRef(frag->m_atomHighLight[idx]);
         const double *coords = atm->coordinates();
         if (!moveZ) {
            // In this case, motion is in x, y.  z is probably 0 but
            // we choose not to even bother
            atm->coordinates(coords[0] + deltaX, coords[1] + deltaY, 
                  coords[2] + deltaZ);
         } else {
            // Interpret y motion as inward/outward (z) motion.  Ignore x.
            //cout << "xyz " << deltaX << " " << deltaY << " " << deltaZ << endl;
            //atm->coordinates(coords[0], coords[1], coords[2] + deltaY);
         }
      }
      // Calling a touch method clears selection so we have to say we
      // changed the selection in order for it to restore.
      // kinda costly for large selections but I didn't track down a better
      // work around
      //sg->touchChemDisplay();
      frag->touch();


      if (selChanged) {
         EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));
      }
      EventDispatcher::getDispatcher().publish(Event("GeomChange"));
   }

   return ret;
}
