#ifndef NMODESTEPCMD_H
#define NMODESTEPCMD_H

#include "viz/SGCommand.H"
#include "viz/PropCmd.H"


/**
 */
class NModeStepCmd : public PropCmd
{
   public:
      NModeStepCmd(const string& name, Receiver *, IPropCalculation *);
      ~NModeStepCmd();
      void init();
      bool execute() throw(EcceException);

   protected:

};

#endif
