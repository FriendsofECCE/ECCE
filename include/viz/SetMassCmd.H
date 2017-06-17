/**
 * @file
 *
 *
 */

#ifndef SETMASSCMD_HH
#define SETMASSCMD_HH


#include "viz/FragCloneCmd.H"


class SetMassCmd : public FragCloneCmd
{
   public:
      SetMassCmd(const string& name, Receiver * receiver, const string& sender="");
      ~SetMassCmd();

      virtual bool execute() throw(EcceException);

   protected:

      void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_sender;


};
#endif
