#ifndef GTSTEPCMD_H
#define GTSTEPCMD_H

#include "viz/PropCmd.H"

class GTStepCmd : public PropCmd
{
  public:
    GTStepCmd(const string& name, Receiver *, IPropCalculation *);
    virtual ~GTStepCmd();

    virtual bool execute();

  protected:
    virtual void init();
};

#endif // GTSTEPCMD_H
