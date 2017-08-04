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

#include "tdat/ForceField.H"
#include "tdat/FFCoords.H"
#include "tdat/UFFDesc.H"

#include "tdat/TBond.H"

#include "viz/CleanCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

CleanCmd::CleanCmd(const string& name, Receiver * receiver,
                   ProgressMonitor *mon)
    : FragCloneCmd(name, receiver)
{
   init();
   p_monitor = mon;
   p_redo = 0;
}



/**
 * Destructor.
 * Deletes undo related data.
 */
CleanCmd::~CleanCmd()
{
   p_monitor = 0;
   if (p_redo != 0) delete p_redo;
}



void CleanCmd::init()
{
   addParameter(new CommandParameter("iterations", 100));
   addParameter(new CommandParameter("nonBondCutoff", 5.0));
   addParameter(new CommandParameter("convergence", .0001));
}





bool CleanCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   // Since clean is so slow, use cached redo frag if available
   if (p_redo) {
        
     // Copied from superclass undo
     //hack - should this be copied over in fragment?
     DisplayStyle ds = sg->getFragment()->getMainDisplayStyle();
      sg->getCSTop()->removeAllChildren();
      frag = new SGFragment(*p_redo);
      frag->setMainDisplayStyle(ds);
      sg->getCSTop()->addChild(frag);
      sg->getCSTop()->addChild(sg->getDisplayGroup());
   } else {

      if (frag->numAtoms() > 1 ) {
         ret = true;

         int iterations = getParameter("iterations")->getInteger();

         // not currently using - don't remember why
         double convergence = getParameter("convergence")->getDouble();

         double nonBondCutoff = getParameter("nonBondCutoff")->getDouble();

         vector<int> *highlight = &frag->m_atomHighLight;
         UFFDesc desc;
         ForceField forceField(frag, highlight, &desc, p_monitor);

         cloneFragment(frag);

         forceField.optimize( iterations,
               convergence,
               nonBondCutoff
               );
         sg->fixAllNubs();


      }
      frag->touchNumbers();
      sg->touchChemDisplay();

      Event event("GeomChange");
      EventDispatcher::getDispatcher().publish(event);
   }

   return ret;
}



void CleanCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   if (p_undoFrag) {
      p_redo = new SGFragment(*frag);
   }
   FragCloneCmd::undo();
}
