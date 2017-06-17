#ifndef SHOWFRAGCMD_H
#define SHOWFRAGCMD_H

#include "viz/SGCommand.H"
#include "viz/PropCmd.H"


/**
 */
class ShowFragCmd : public PropCmd
{
   public:
      ShowFragCmd(const string& name, Receiver *, IPropCalculation *);
      ~ShowFragCmd();
      void init();
      bool execute() throw(EcceException);

   protected:

};

#endif
