#ifndef ADDPOLARIZABILITYCMD__H
#define ADDPOLARIZABILITYCMD__H

#include "viz/AddVectorCmd.H"

class PropVector;

/**
 *   The polarizability tensor as one translucent ellipsoid at the centre of
 *   nuclear charge.  Its semi-axes lie along the eigenvectors of the
 *   symmetrised tensor and are the eigenvalues times "Amplitude", in
 *   Angstrom per a.u. of polarizability.
 */
class AddPolarizabilityCmd : public AddVectorCmd
{
   public:
      AddPolarizabilityCmd(const string& name, Receiver *, IPropCalculation *);
      ~AddPolarizabilityCmd();

      virtual bool execute();

      // Unused: this command draws no arrows.
      virtual bool getDisplacements(vector<double*>& displacements,
                                  vector<SIGN>& signs);

      /**
       * Eigenvalues (largest first) and unit eigenvectors (right-handed) of
       * the symmetrised tensor; accepts 6 (xx xy xz yy yz zz) or 9
       * (row-major) values.  False if the property has neither.
       */
      static bool principalAxes(const PropVector *vec, double ew[3],
                                double ev[3][3]);

      // The default scale puts the longest semi-axis this far from the centre.
      static const double LONGEST_SEMI_AXIS;
};

#endif
