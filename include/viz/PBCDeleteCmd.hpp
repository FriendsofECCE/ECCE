#ifndef PBCDELETECMD_H
#define PBCDELETECMD_H


#include "viz/PBCCmd.H"

#include <vector>
  using std::vector;

/**
 * A command to delete an existing lattice.
 */
class PBCDeleteCmd : public PBCCmd
{
   public:
      PBCDeleteCmd(const string& name, Receiver * receiver, SGFragment *base);
      ~PBCDeleteCmd();

      virtual bool execute() throw(EcceException);
      virtual bool shouldAutoReplicate() const;

   protected:
      void init();

};
#endif
