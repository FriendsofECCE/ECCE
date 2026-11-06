// Add a molecule from SMILES, or replace a terminal group by a SMILES
// fragment (#833).  The chemistry is in tdat/FragGraft.H.
#ifndef SMILESCMD_H
#define SMILESCMD_H

#include "viz/FragCloneCmd.H"

/// Parameters: mode ("add" or "replace"), smiles, extended (add: start from
/// all non-ring single-bond torsions at 180), first and attach (replace:
/// 0-based atom indices; the group is the side of the bond attach-first
/// that holds `first`, and attach < 0 lets the command choose it).
/// The SMILES of a replacement writes the attachment point as [*].
/// Formal charges become the atoms' charges; the error message says why a
/// call failed, and then the structure is unchanged.  Undo restores the
/// cloned fragment.
class SmilesCmd : public FragCloneCmd
{
   public:
      SmilesCmd(const string& name, Receiver * receiver);
      virtual ~SmilesCmd();
      virtual bool execute();
      virtual string getUndoLabel();
   protected:
      virtual void init();
   private:
      bool addMolecule();
      bool replaceGroup();
};

#endif
