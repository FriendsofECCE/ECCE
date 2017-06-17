#ifndef ISOSURFACECMD__H
#define ISOSURFACECMD__H

#include "viz/PropCmd.H"


/**
 *  Using Molecular Inventor, this method displays an isosurface through
 *  a function over a 3D gridded domain.  The grid can be uniform or
 *  rectilinear.  There are 3 display types for isosurfaces: solid, mesh,
 *  and contour.
 */
class IsoSurfaceCmd : public PropCmd
{
  public:
    IsoSurfaceCmd(const string& name, Receiver *receiver,
                  IPropCalculation *calc);
    ~IsoSurfaceCmd();
    void init();
    bool execute() throw(EcceException);

  protected:

};

#endif
