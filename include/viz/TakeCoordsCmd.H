// Edit > Take Coordinates From... (#620): the atoms in "set" get the
// coordinates of the same atoms of another structure.
#ifndef TAKECOORDSCMD_H
#define TAKECOORDSCMD_H

#include "viz/FragCloneCmd.H"

/// Parameters: set (1-based atom list, "1-3,7"; empty = the selected atoms),
/// elements and coords (the other structure: space-separated symbols and 3
/// numbers per atom, Angstrom), align (fit it onto this one over the atoms
/// outside the set first).  The first mismatch in atom count or element is
/// the error message.  Undo restores the cloned fragment.
class TakeCoordsCmd : public FragCloneCmd
{
   public:
      TakeCoordsCmd(const string& name, Receiver * receiver);
      virtual ~TakeCoordsCmd();
      virtual bool execute();
      virtual string getUndoLabel();
      /// RMSD of the fit of the last execute() with align on, else 0.
      double rmsd() const { return p_rmsd; }
   protected:
      virtual void init();
   private:
      double p_rmsd;
};

#endif
