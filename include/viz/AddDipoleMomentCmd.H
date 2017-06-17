#ifndef ADDDIPOLEMOMENTCMD__H
#define ADDDIPOLEMOMENTCMD__H

#include "viz/AddVectorCmd.H"

/**
 *   Create a dipole moment vector.  The vector origin depends on whether
 *   it is computed by center of mass or center of charge.  The vector
 *   length is determined by the vector data.  We allow a scale factor
 *   to help users make printouts but this is a bit hacky and they
 *   may get confused by it - we'll see.
 */
class AddDipoleMomentCmd : public AddVectorCmd
{
   public:
      AddDipoleMomentCmd(const string& name, Receiver *, IPropCalculation *);
      ~AddDipoleMomentCmd();

      // AddVectorCmd virtuals
      virtual bool getDisplacements(vector<double*>& displacements,
                                  vector<SIGN>& signs);

};

#endif
