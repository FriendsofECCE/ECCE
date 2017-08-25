#ifndef PBCCREATECMD_H
#define PBCCREATECMD_H


#include "viz/FragCloneCmd.H"

#include <vector>
  using std::vector;

/**
 * A command to create a new lattice.
 */
class PBCCreateCmd : public FragCloneCmd
{
   public:
      PBCCreateCmd(const string& name, Receiver * receiver);
      ~PBCCreateCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:
      void init();

};
#endif
