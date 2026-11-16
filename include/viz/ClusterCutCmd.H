#ifndef CLUSTERCUTCMD_H
#define CLUSTERCUTCMD_H

#include "viz/PBCCmd.H"

/**
 * Replace a periodic structure by a non-periodic cluster cut around the
 * selected atoms (#1033).  Parameters: radius (Angstrom), whole (0 none,
 * 1 molecules, 2 formula units), cap (0 none, 1 H, 2 OH).  The maths is in
 * tdat/ClusterCut.H.
 */
class ClusterCutCmd : public PBCCmd
{
   public:
      ClusterCutCmd(const string& name, Receiver * receiver, SGFragment *base);
      ~ClusterCutCmd();

      virtual bool execute();
      virtual bool shouldAutoReplicate() const;
      // Undo brings the lattice back, so the replicated view must follow.
      virtual bool shouldAutoReplicateOnUndo() const;

      // Atoms in the cluster and how many of them are caps, after execute.
      int clusterAtoms() const { return p_atoms; }
      int clusterCaps() const { return p_caps; }

   protected:
      void init();

   private:
      int p_atoms;
      int p_caps;
};
#endif
