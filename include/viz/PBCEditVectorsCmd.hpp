#ifndef PBCEDITVECTORSCMD_H
#define PBCEDITVECTORSCMD_H


#include "viz/PBCCmd.H"

#include <vector>
  using std::vector;

/**
 * A command to set lattice vectors a1,a2,a3.
 */
class PBCEditVectorsCmd : public PBCCmd
{
   public:
      PBCEditVectorsCmd(const string& name, Receiver * receiver, SGFragment *base);
      ~PBCEditVectorsCmd();

      virtual bool execute() throw(EcceException);

   protected:
      void init();

};
#endif
