#ifndef GOHOMECMD_HH
#define GOHOMECMD_HH


#include "util/Command.H"

class GoHomeCmd : public Command
{
   public:
      GoHomeCmd(const string& name, Receiver * receiver);
      ~GoHomeCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();
      virtual bool isUndoable() const;

   protected:
      void init();

};
#endif
