#ifndef CENTERVIEWCMD_HH
#define CENTERVIEWCMD_HH


#include "util/Command.H"

class CenterViewCmd : public Command
{
   public:
      CenterViewCmd(const string& name, Receiver * receiver);
      ~CenterViewCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();
      virtual bool isUndoable() const;

   protected:
      void init();

};
#endif
