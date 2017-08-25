#ifndef DUMPSGCMD_H
#define DUMPSGCMD_H


#include "util/Command.H"

class DumpSGCmd : public Command
{
   public:
      DumpSGCmd(const string& name, Receiver * receiver);
      ~DumpSGCmd();

      virtual bool execute() throw(EcceException);
      virtual bool isUndoable() const;
      virtual void undo();

   protected:
      void init();

};
#endif
