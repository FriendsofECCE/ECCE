/**
 * @file
 *
 *
 */
#ifndef  MEASUREBASECMD_H
#define  MEASUREBASECMD_H

#include "util/Command.H"

class Fragment;

/**
 * Abstract command for adding measures.
 * Supports undo by keeping track of the index of the added measure
 * so it can be remove.
 */
class MeasureBaseCmd : public Command
{
   public:
      MeasureBaseCmd(const string& name, Receiver * receiver);
      virtual ~MeasureBaseCmd();

      virtual void undo();

   protected:

      /** undo data.  We will support undo by removing the new measure. */
      int p_addIndex;

};

#endif




