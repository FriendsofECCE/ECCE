#ifndef PBCFOLDCMD_H
#define PBCFOLDCMD_H


#include "viz/PBCCmd.H"

#include <vector>
  using std::vector;

/**
 * A command to fold outlying atoms into the unit cell.
 */
class PBCFoldCmd : public PBCCmd
{
   public:
      PBCFoldCmd(const string& name, Receiver * receiver, SGFragment *base);
      ~PBCFoldCmd();

      virtual bool execute() throw(EcceException);

   protected:
      void init();

};
#endif
