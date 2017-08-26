/**
 * @file
 *
 *
 */

#ifndef SETBEHAVIORCMD_HH
#define SETBEHAVIORCMD_HH


#include "viz/FragCloneCmd.hpp"


class SetBehaviorCmd : public FragCloneCmd
{
   public:
      SetBehaviorCmd(const string& name, Receiver * receiver, const string& sender="");
      ~SetBehaviorCmd();

      virtual bool execute() throw(EcceException);

   protected:

      void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_sender;

};
#endif
