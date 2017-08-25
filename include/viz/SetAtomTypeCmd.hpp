/**
 * @file
 *
 *
 */

#ifndef SETATOMTYPECMD_HH
#define SETATOMTYPECMD_HH


#include "util/Command.H"


class SetAtomTypeCmd : public Command
{
   public:
      SetAtomTypeCmd(const string& name, Receiver * receiver, const string& sender="");
      ~SetAtomTypeCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:

      void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_sender;

      int p_atomnum;
      string p_atomType;

};
#endif
