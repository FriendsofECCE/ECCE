#ifndef PBCREPLICATECMD_H
#define PBCREPLICATECMD_H


#include "viz/PBCCmd.H"

#include <vector>
  using std::vector;

class PBCReplicateCmd : public PBCCmd
{
   public:
      PBCReplicateCmd(const string& name, Receiver * receiver, SGFragment *base);
      ~PBCReplicateCmd();

      virtual bool execute() throw(EcceException);
      virtual bool isUndoable() const;

   protected:
      void init();

};
#endif
