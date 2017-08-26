#ifndef NMODETRACECMD_H
#define NMODETRACECMD_H

#include "viz/SGCommand.hpp"
#include "viz/PropCmd.hpp"


/**
 */
class NModeTraceCmd : public PropCmd
{
   public:
      NModeTraceCmd(const string& name, Receiver *, IPropCalculation *);
      ~NModeTraceCmd();
      void init();
      bool execute() throw(EcceException);

   protected:

};

#endif
