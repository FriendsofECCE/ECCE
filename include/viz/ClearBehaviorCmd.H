/**
 * @file
 *
 *
 */

#ifndef CLEARBEHAVIORCMD_HH
#define CLEARBEHAVIORCMD_HH


#include "viz/FragCloneCmd.H"


class ClearBehaviorCmd : public FragCloneCmd
{
   public:
      ClearBehaviorCmd(const string& name, Receiver * receiver, const string& sender="");
      ~ClearBehaviorCmd();

      virtual bool execute() throw(EcceException);

   protected:

      void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_sender;

};
#endif
