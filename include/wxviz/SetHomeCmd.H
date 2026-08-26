#ifndef SETHOMECMD_HH
#define SETHOMECMD_HH


#include "util/Command.H"

class SetHomeCmd : public Command
{
   public:
      SetHomeCmd(const string& name, Receiver * receiver);
      ~SetHomeCmd();

      virtual bool execute();
      virtual void undo();

   protected:
      void init();

};
#endif
