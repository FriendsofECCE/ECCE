/**
 * @file
 *
 *
 */

#ifndef SETENVINDEXCMD_HH
#define SETENVINDEXCMD_HH


#include "util/Command.H"


class SetEnvIndexCmd : public Command
{
   public:
      SetEnvIndexCmd(const string& name, Receiver * receiver, const string& sender="");
      ~SetEnvIndexCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:

      void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_sender;

      int p_atomnum;
      int p_envIndex;

};
#endif
