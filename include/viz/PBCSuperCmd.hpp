#ifndef PBCSUPERCMD_H
#define PBCSUPERCMD_H


#include "viz/PBCCmd.H"

#include <vector>
  using std::vector;

class SGFragment; 

/**
 * A command to make a superlattice from a currently replicated lattice.
 */
class PBCSuperCmd : public PBCCmd
{
   public:
      PBCSuperCmd(const string& name, Receiver * receiver, SGFragment *base);
      ~PBCSuperCmd();

      virtual bool execute() throw(EcceException);

      virtual bool shouldAutoReplicate() const;
      virtual bool shouldAutoReplicateOnUndo() const;

   protected:
      void init();

};
#endif
