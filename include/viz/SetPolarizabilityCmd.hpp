/**
 * @file
 *
 *
 */

#ifndef SETPOLARIZABILITYCMD_HH
#define SETPOLARIZABILITYCMD_HH


#include "util/Command.H"


class SetPolarizabilityCmd : public Command
{
   public:
      SetPolarizabilityCmd(const string& name, Receiver * receiver, const string& sender="");
      ~SetPolarizabilityCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:

      void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_sender;

      int p_atomnum;
      float p_polarization;

};
#endif
