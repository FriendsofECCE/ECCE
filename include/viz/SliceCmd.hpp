#ifndef SLICECMD_H
#define SLICECMD_H

#include "viz/FragCloneCmd.H"

#include <vector>
  using std::vector;

/**
 * A command to fold outlying atoms into the unit cell.
 */
class SliceCmd : public FragCloneCmd
{
   public:
      SliceCmd(const string& name, Receiver * receiver);
      ~SliceCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:
      void init();

};
#endif
