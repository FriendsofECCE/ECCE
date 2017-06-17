/**
 * @file
 *
 *
 */

#ifndef SETCHARGEGROUPCMD_HH
#define SETCHARGEGROUPCMD_HH


#include "util/Command.H"


class SetChargeGroupCmd : public Command
{
   public:
      SetChargeGroupCmd(const string& name, Receiver * receiver, const string& sender="");
      ~SetChargeGroupCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:

      void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_sender;

      int p_atomnum;
      int p_chargeGroup;

};
#endif
