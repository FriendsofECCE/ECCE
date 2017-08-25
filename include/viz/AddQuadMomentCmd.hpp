#ifndef ADDQUADMOMENTCMD__H
#define ADDQUADMOMENTCMD__H

#include "viz/AddVectorCmd.H"

/**
 *  Create a quadrapole moment vector.  
 *  The vectors' origin depends on whether
 *  it is computed by center of mass or center of charge.
 *
 * Works for any quad property.
 */
class AddQuadMomentCmd : public AddVectorCmd
{
   public:
      AddQuadMomentCmd(const string& name, Receiver *, IPropCalculation *);
      ~AddQuadMomentCmd();

      // AddVectorCmd virtuals
      virtual bool getDisplacements(vector<double*>& displacements,
                                  vector<SIGN>& signs);

};

#endif
