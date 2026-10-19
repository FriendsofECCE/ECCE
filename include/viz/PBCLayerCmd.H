#ifndef PBCLAYERCMD_H
#define PBCLAYERCMD_H

#include "viz/PBCCmd.H"

/**
 * Layer builder: the structure with its cell is one layer; stack N copies
 * along the a-b normal with a lateral shift, optionally filling the gaps
 * with ions and water (geometry: tdat/LayerStack.H).  Undo restores the
 * whole structure and cell.
 */
class PBCLayerCmd : public PBCCmd
{
   public:
      PBCLayerCmd(const string& name, Receiver * receiver, SGFragment *base);
      virtual ~PBCLayerCmd();

      virtual bool execute();

      // The command works on the structure as it is (replicated atoms
      // are real atoms) and leaves a 1x1x1 cell.
      virtual bool shouldAutoReplicate() const;
      virtual bool shouldAutoReplicateOnUndo() const;

   protected:
      void init();
};

#endif
