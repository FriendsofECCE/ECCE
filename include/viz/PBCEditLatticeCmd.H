#ifndef PBCEDITLATTICECMD_H
#define PBCEDITLATTICECMD_H


#include "viz/PBCCmd.H"

#include <vector>
  using std::vector;

/**
 * A command to set lattice values a,b,c,alpha,beta,gamma.
 */
class PBCEditLatticeCmd : public PBCCmd
{
   public:
      PBCEditLatticeCmd(const string& name, Receiver * receiver, SGFragment *base);
      ~PBCEditLatticeCmd();

      virtual bool execute() throw(EcceException);

   protected:
      void init();

};
#endif
