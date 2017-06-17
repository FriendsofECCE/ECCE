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

#include "viz/PBCCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

PBCCmd::PBCCmd(const string& name, Receiver * receiver, SGFragment *baseFrag)
    : FragCloneCmd(name, receiver)
{
   // Not sure.... allow 0 to be passed in?
   if (baseFrag) cloneFragment(baseFrag);
}



/**
 * Destructor.
 */
PBCCmd::~PBCCmd()
{

}



void PBCCmd::cloneFragment(SGFragment *frag)
{
   restoreRedoState(frag);

   // Don't do undo stuff if we've already executed
   if (p_undoFrag == 0) {
      p_undoFrag = new SGFragment(*frag);
   }
}



/**
 * Undo the command.
 * Not sure if we'll need to override superclass or not....
 */
void PBCCmd::undo()
{
   //FragCloneCmd::undo();
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   //hack - should this be copied over in fragment?
   DisplayStyle ds = sg->getFragment()->getMainDisplayStyle();

   if (p_undoFrag != 0) {

      restoreFrag(new SGFragment(*p_undoFrag), ds);
      SGFragment *sgfrag = sg->getFragment();


      string replicate;
      if (shouldAutoReplicateOnUndo()) replicate = "replicate";

      // Choose only one event to avoid duplicate processing.
      // Hopefully tools that respond to Residue change also handle
      // the geomchange updates
      // Residues matter since we could have possibly removed the 
      // first add frag, resulting in no residues (blank slate)
      string ename = "GeomChange";
      if (sgfrag->numResidues() > 0) ename = "ResidueChange";

      EventDispatcher::getDispatcher().publish(Event(ename,replicate));


      if (replicate == "") {
        // only send this if its not a replicate because the knowledge
        // of a lattice chance was already processes if this was set.
        // The double processing actually incorrectly causes a restash
        // of base replicated fragments so bad bad bad
        EventDispatcher::getDispatcher().publish(Event("LatticeChange"));
      }
   }
}

bool PBCCmd::shouldAutoReplicate() const
{
   return true;
}

bool PBCCmd::shouldAutoReplicateOnUndo() const
{
   return true;
}
