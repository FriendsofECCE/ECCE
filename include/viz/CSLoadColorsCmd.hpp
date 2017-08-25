/**
 * @file
 *
 *
 */
#ifndef  CSLOADCOLORSCMD_H
#define  CSLOADCOLORSCMD_H

#include "util/Command.H"

class Fragment;

class CSLoadColorsCmd : public Command
{
   public:
      CSLoadColorsCmd(const string& name, Receiver * receiver);
      virtual ~CSLoadColorsCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();
      virtual bool isUndoable() const;

   protected:
      virtual void init();


};

#endif




