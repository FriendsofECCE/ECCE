#ifndef ISOSURFACECMD__H
#define ISOSURFACECMD__H

#include "viz/PropCmd.H"


/**
 *  Using Molecular Inventor, this method displays an isosurface through
 *  a function over a 3D gridded domain.  The grid can be uniform or
 *  rectilinear.  There are 3 display types for isosurfaces: solid, mesh,
 *  and contour.
 */
class ChemIso;

class IsoSurfaceCmd : public PropCmd
{
  public:
    IsoSurfaceCmd(const string& name, Receiver *receiver,
                  IPropCalculation *calc);
    ~IsoSurfaceCmd();
    void init();
    bool execute();

    /**
     * The ramp's colour at t in [-1, 1] (negative to positive extreme),
     * and the range actually used for a grid's [min, max].  Public so a
     * legend draws the surface's own colours, not a copy of them.
     */
    static void potentialRampColor(double t, double& r, double& g, double& b);
    static void potentialRampRange(float& minValue, float& maxValue);

  protected:


   private:

      /** Red-white-blue ramp for a potential-mapped surface. */
      static void setPotentialRamp(ChemIso *isosurf, float minValue,
                                   float maxValue, double transparency);
};

#endif
