// Edit > Distort Bonds (#619): stretch or shrink the bonds to one atom.
#ifndef DISTORTBONDSCMD_H
#define DISTORTBONDSCMD_H

#include "viz/FragCloneCmd.H"

/// Parameters: centre (atom number from 1; 0 = the selected atom), then one of
/// dr (the same change in Angstrom for every bond), list ("3:0.1,5:-0.05":
/// atom number from 1 and its change), jt (Jahn-Teller preset delta, with
/// axis x|y|z|longest).  Undo restores the cloned fragment.
class DistortBondsCmd : public FragCloneCmd
{
   public:
      DistortBondsCmd(const string& name, Receiver * receiver);
      virtual ~DistortBondsCmd();
      virtual bool execute();
      virtual string getUndoLabel();
   protected:
      virtual void init();
};

#endif
