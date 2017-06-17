/**
 * @file
 *
 *
 */
#ifndef  CSSTYLECMD_H
#define  CSSTYLECMD_H

#include "util/Command.H"

class Fragment;

class CSStyleCmd : public Command
{
   public:
      CSStyleCmd(const string& name, Receiver * receiver);
      virtual ~CSStyleCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

      virtual bool isUndoable() const;

   protected:
      virtual void init();

};

#endif




