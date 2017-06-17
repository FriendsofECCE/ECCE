/**
 * @file
 *
 *
 */

#ifndef SETSUFFIXCMD_HH
#define SETSUFFIXCMD_HH


#include "util/Command.H"


class SetSuffixCmd : public Command
{
   public:
      SetSuffixCmd(const string& name, Receiver * receiver, const string& sender="");
      ~SetSuffixCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:

      void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_sender;

      int p_atomnum;
      string p_suffix;

};
#endif
