#ifndef ISOVALUECMD_H
#define ISOVALUECMD_H

#include "viz/PropCmd.H"


/**
 * Change the isovalue of an isosurface.  Also used to set the color
 * and color transparency.  
 *
 * Transparency only works for solids.
 */
class IsoValueCmd : public PropCmd
{
   public:
      IsoValueCmd(const string& name, Receiver *, IPropCalculation *);
      ~IsoValueCmd();
      void init();
      bool execute() throw(EcceException);

   protected:

};

#endif
