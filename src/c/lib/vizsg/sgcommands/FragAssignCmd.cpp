/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "util/InvalidException.H"
#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "tdat/TAtm.H"
#include "tdat/TBond.H"

#include "viz/FragAssignCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

FragAssignCmd::FragAssignCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   p_undoFrag = 0;
}



/**
 * Destructor.
 * Deletes undo related data.
 */
FragAssignCmd::~FragAssignCmd()
{
   clearUndoFragment();

}


bool FragAssignCmd::execute() throw(EcceException)
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();
  cloneFragment(frag);
  return true;
}


void FragAssignCmd::cloneFragment(SGFragment *frag)
{
   restoreRedoState(frag);

   // Don't do undo stuff if we've already executed
   if (p_undoFrag == 0) {
      p_undoFrag = new SGFragment(*frag);
   }
}


void FragAssignCmd::clearUndoFragment()
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
void FragAssignCmd::restoreRedoState(SGFragment *frag)
{
   if (p_undoFrag) {
      frag->m_atomHighLight = p_undoFrag->m_atomHighLight;
      frag->m_bondHighLight = p_undoFrag->m_bondHighLight;
   }
}


void FragAssignCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();


   if (p_undoFrag != 0) {
     //hack - should this be copied over in fragment?
     DisplayStyle ds = frag->getMainDisplayStyle();

    vector<TAtm*> *oldAtoms = p_undoFrag->atoms();
    vector<TAtm*> *newAtoms = frag->atoms();
    int cnt = oldAtoms->size();
    INVALIDEXCEPTION(cnt == newAtoms->size(),"Assign undo atom mismatch");
    int idx;
    for (idx=0; idx<cnt; idx++) {
        (*newAtoms)[idx]->shallowCopy((*oldAtoms)[idx]);
    }

    delete oldAtoms;
    delete newAtoms;

    // Now the bonds - bonds haven't change but maybe order did
    vector<TBond*>* const oldBonds = p_undoFrag->bonds();
    vector<TBond*>* const newBonds = frag->bonds();
    cnt = oldBonds->size();
    for (idx=0; idx<cnt; idx++) {
      (*newBonds)[idx]->order((*oldBonds)[idx]->order());
      (*newBonds)[idx]->displayStyle((*oldBonds)[idx]->displayStyle());
    }

    frag->touchNumbers();
    sg->touchChemDisplay();

    delete oldBonds;
    delete newBonds;

      EventDispatcher::getDispatcher().publish(Event("GeomChange"));
   }
}


void FragAssignCmd::init()
{
}

