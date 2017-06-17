/**
 * @file
 *
 *
 */

#ifndef SETCONNECTIONCMD_HH
#define SETCONNECTIONCMD_HH


#include "util/Command.H"


class SetConnectionCmd : public Command
{
   public:
      SetConnectionCmd(const string& name, Receiver * receiver, const string& sender="");
      ~SetConnectionCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:

      void init();
      /* sender of event - default to "". Used to filter own messages */
      string p_sender;

      int p_atomnum;
      int p_connectionIndex;

};
#endif
