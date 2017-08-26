#ifndef SETHOMECMD_HH
#define SETHOMECMD_HH


#include "util/Command.hpp"

class SetHomeCmd : public Command
{
   public:
      SetHomeCmd(const string& name, Receiver * receiver);
      ~SetHomeCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:
      void init();

};
#endif
