/**
 * @file
 *
 *
 */
#ifndef  FRAGCLONECMD_H
#define  FRAGCLONECMD_H

#include "util/Command.H"
#include "tdat/DisplayStyle.H"

class SGFragment;

/**
 * A class that supports undo by cloning a fragment.
 * Many commands can simply inherit from this class and call
 * the cloneFragment method at the appropriate point in their
 * execute method to support undo.  Cloning an entire fragment
 * to support undo can be expensive so where feasible, a smarter
 * method should be developed.
 */
class FragCloneCmd : public Command
{
   public:
      FragCloneCmd(const string& name, Receiver * receiver);
      virtual ~FragCloneCmd();

      virtual void cloneFragment(SGFragment *frag);
      virtual void clearUndoFragment();
      virtual void restoreRedoState(SGFragment *frag);

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:
      virtual void init();
      virtual void restoreFrag(SGFragment *frag, DisplayStyle ds);


      SGFragment *p_undoFrag;


};

#endif
