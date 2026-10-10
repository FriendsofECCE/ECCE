#ifndef ADDDIPOLEMOMENTCMD__H
#define ADDDIPOLEMOMENTCMD__H

#include "viz/AddVectorCmd.H"

/**
 *   The dipole moment as one arrow from the centre of nuclear charge, along
 *   mu as the codes report it (from the negative to the positive end).  The
 *   "Amplitude" is the length in Angstrom per Debye, so arrows of different
 *   molecules compare directly.
 */
class AddDipoleMomentCmd : public AddVectorCmd
{
   public:
      AddDipoleMomentCmd(const string& name, Receiver *, IPropCalculation *);
      ~AddDipoleMomentCmd();

      // AddVectorCmd virtuals
      virtual bool getDisplacements(vector<double*>& displacements,
                                  vector<SIGN>& signs);
      virtual double getScaleFactor();
      virtual void shapeVector(VRVector *);

      // Default length of the arrow, Angstrom per Debye.
      static const double ANGSTROM_PER_DEBYE;

};

#endif
