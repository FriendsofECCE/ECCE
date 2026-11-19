#ifndef SOLVATECMD_H
#define SOLVATECMD_H

#include "viz/PBCCmd.H"

/**
 * Fill the periodic cell, or a box of `margin` Angstrom around the
 * structure when it has none, with a shipped solvent box (#1118).
 * Parameters: solvent (file stem in data/client/solvents), margin, scale
 * (solute-solvent cut-off, times the sum of vdW radii).  The geometry is in
 * tdat/Solvate.H.
 */
class SolvateCmd : public PBCCmd
{
   public:
      SolvateCmd(const string& name, Receiver * receiver, SGFragment *base);
      ~SolvateCmd();

      virtual bool execute();
      virtual bool shouldAutoReplicate() const;
      virtual bool shouldAutoReplicateOnUndo() const;

      // After execute: solvent molecules and atoms added, solvent density.
      int molecules() const { return p_molecules; }
      int atomsAdded() const { return p_atoms; }
      double density() const { return p_density; }

   protected:
      void init();

   private:
      int p_molecules;
      int p_atoms;
      double p_density;
};
#endif
