#ifndef NMODEVECTCMD_H
#define NMODEVECTCMD_H

#include "viz/SGCommand.H"
#include "viz/PropCmd.H"


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
