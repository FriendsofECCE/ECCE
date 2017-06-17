/**
 * @file
 *
 *
 */

#ifndef SETPARTIALCHARGECMD_HH
#define SETPARTIALCHARGECMD_HH


#include "util/Command.H"

class wxWindow;

class SetPartialChargeCmd : public Command
{
   public:
      SetPartialChargeCmd(const string& name, Receiver * receiver, const string& sender="");
      ~SetPartialChargeCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:

      void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_sender;

      int p_atomnum;
      double p_partialCharge;

};
#endif
