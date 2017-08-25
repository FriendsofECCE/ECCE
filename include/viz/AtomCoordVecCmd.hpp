#ifndef ATOMCOORDVECCMD__H
#define ATOMCOORDVECCMD__H

#include "viz/AddVectorCmd.H"

/**
 * Command to create vectors for Electric field.
 */
class AtomCoordVecCmd : public AddVectorCmd
{
   public:
      AtomCoordVecCmd(const string& namee, Receiver *receiver,
                        IPropCalculation *calc);
      ~AtomCoordVecCmd();

      virtual bool getDisplacements(vector<double*>& displacements,
                                  vector<SIGN>& signs);

      virtual double getScaleFactor(); // takes into account log scale
};

#endif
