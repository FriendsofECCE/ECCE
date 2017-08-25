/**
 * @file
 *
 *
 */
#ifndef  FRAGASSIGNCMD_H
#define  FRAGASSIGNCMD_H

#include "util/Command.H"

class SGFragment;

/**
 * A class that supports undo by cloning copying fragment 
 * (atom and bond) attributes.  Undo simply copies the stashed values
 * back into the main fragment.
 *
 * Many commands can simply inherit from this class and call
 * the cloneFragment method at the appropriate point in their
 * execute method to support undo.  
 *
 * NOTE: This undo cmd is cheaper than the FragCloneCmd but can't be
 * used if the number of atoms, bonds, residues changes.
 */
class FragAssignCmd : public Command
{
   public:
      FragAssignCmd(const string& name, Receiver * receiver);
      virtual ~FragAssignCmd();

      virtual void cloneFragment(SGFragment *frag);
      virtual void clearUndoFragment();
      virtual void restoreRedoState(SGFragment *frag);

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:
      virtual void init();

      SGFragment *p_undoFrag;


};

#endif
