#ifndef NMODESTEPCMD_H
#define NMODESTEPCMD_H

#include "viz/SGCommand.hpp"
#include "viz/PropCmd.hpp"


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
