/**
 * @file
 *
 *
 */

#ifndef SETPOLARIZATIONGROUPCMD_HH
#define SETPOLARIZATIONGROUPCMD_HH


#include "util/Command.H"


class SetPolarizationGroupCmd : public Command
{
   public:
      SetPolarizationGroupCmd(const string& name, Receiver * receiver, const string& sender="");
      ~SetPolarizationGroupCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:

      void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_sender;

      int p_atomnum;
      int p_polarizationGroup;

};
#endif
