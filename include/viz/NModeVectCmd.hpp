#ifndef NMODEVECTCMD_H
#define NMODEVECTCMD_H

#include "viz/SGCommand.hpp"
#include "viz/PropCmd.hpp"


/**
 */
class NModeVectCmd : public PropCmd
{
   public:
      NModeVectCmd(const string& name, Receiver *, IPropCalculation *);
      ~NModeVectCmd();
      void init();
      bool execute() throw(EcceException);

   protected:

};

#endif
