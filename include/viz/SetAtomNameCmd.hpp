/**
 * @file
 *
 *
 */

#ifndef SETATOMNAMECMD_HH
#define SETATOMNAMECMD_HH


#include "util/Command.H"


class SetAtomNameCmd : public Command
{
   public:
      SetAtomNameCmd(const string& name, Receiver * receiver, const string& sender="");
      ~SetAtomNameCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:

      void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_sender;

      int p_atomnum;
      string p_atomName;

};
#endif
