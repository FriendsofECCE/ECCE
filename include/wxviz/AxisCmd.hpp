#ifndef AXISCMD_H
#define AXISCMD_H


#include "util/Command.H"

class AxisCmd : public Command
{
   public:
      AxisCmd(const string& name, Receiver * receiver);
      ~AxisCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();
      virtual bool isUndoable() const;

   protected:
      void init();

};
#endif
