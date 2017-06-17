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

#include "viz/FragCloneCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

FragCloneCmd::FragCloneCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   p_undoFrag = 0;
}



/**
 * Destructor.
 * Deletes undo related data.
 */
FragCloneCmd::~FragCloneCmd()
{
   clearUndoFragment();

}


bool FragCloneCmd::execute() throw(EcceException)
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();
  cloneFragment(frag);
  return true;
}


void FragCloneCmd::cloneFragment(SGFragment *frag)
{
   restoreRedoState(frag);

   // Don't do undo stuff if we've already executed
   if (p_undoFrag == 0) {
      p_undoFrag = new SGFragment(*frag);
   }
}


void FragCloneCmd::clearUndoFragment()
{
   if (p_undoFrag) {
      delete p_undoFrag;
      p_undoFrag = 0;
   }
}

/**
 * Redo is implemented by calling execute again.
 * However, when undoing a chain of things and then redoing, the
 * selection (interactively done by user) can't propagate.
 * So this method is used to restore selection to the fragment from
 * the undo fragment (what selection was prior to original execution)
 * before re-executing.
 * 
 * Commands that don't subclass this class must address this problem 
 * themselves.
 */
void FragCloneCmd::restoreRedoState(SGFragment *frag)
{
   if (p_undoFrag) {
      frag->m_atomHighLight = p_undoFrag->m_atomHighLight;
      frag->m_bondHighLight = p_undoFrag->m_bondHighLight;
   }
}


void FragCloneCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   //hack - should this be copied over in fragment?
   DisplayStyle ds = sg->getFragment()->getMainDisplayStyle();

   if (p_undoFrag != 0) {
      restoreFrag(new SGFragment(*p_undoFrag), ds);

      SGFragment *sgfrag = sg->getFragment();
      EventDispatcher::getDispatcher().publish(Event("GeomChange"));
      // We could have possibly removed the first add frag, resulting in no
      // residues (blank slate)
      if (sgfrag->numResidues() > 0) {
        EventDispatcher::getDispatcher().publish(Event("ResidueChange"));
      }
   }
}


void FragCloneCmd::init()
{
}

void FragCloneCmd::restoreFrag(SGFragment *frag, DisplayStyle ds)
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   sg->cleanAtomDeletes();

   sg->getCSTop()->removeAllChildren();
   SGFragment *sgfrag = new SGFragment(*frag);
   sgfrag->setMainDisplayStyle(ds);
   sg->getCSTop()->addChild(sgfrag);
   sg->touchChemDisplay();
   sg->touchLattice();
   sg->getCSTop()->addChild(sg->getDisplayGroup());
}

