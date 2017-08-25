#ifndef TRANSFORMCELLCMD_H
#define TRANSFORMCELLCMD_H

#include "viz/FragCloneCmd.H"

#include <vector>
  using std::vector;

/**
 * A command to fold outlying atoms into the unit cell.
 */
class TransformCellCmd : public FragCloneCmd
{
   public:
      TransformCellCmd(const string& name,Receiver *receiver);
      ~TransformCellCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:
      void init();

};
#endif
