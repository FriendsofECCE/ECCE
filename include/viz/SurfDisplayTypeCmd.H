#ifndef SURFDISPLAYTYPECMD_H
#define SURFDISPLAYTYPECMD_H


#include "viz/PropCmd.H"

/**
 * Switch between the available display types for an isosurface.
 */
class SurfDisplayTypeCmd : public PropCmd
{
  public:
    SurfDisplayTypeCmd(const string& name, Receiver *, IPropCalculation *);
    ~SurfDisplayTypeCmd();
    void init();
    bool execute() throw(EcceException);

  protected:

};

#endif
