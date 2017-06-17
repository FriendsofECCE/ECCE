#include <iostream>
  using std::cout;
  using std::endl;

//#include "inv/nodes/SoSwitch.H"

#include "util/InternalException.H"

#include "dsm/IPropCalculation.H"

#include "tdat/PropVector.H"

#include "viz/cvSGContainer.H"
#include "viz/AddDipoleMomentCmd.H"

AddDipoleMomentCmd::AddDipoleMomentCmd(const string& name, Receiver *receiver,
                         IPropCalculation *calc)
  : AddVectorCmd(name, receiver, calc)
{
  // Parent class does everything
}

AddDipoleMomentCmd::~AddDipoleMomentCmd()
{
}

bool AddDipoleMomentCmd::getDisplacements(vector<double*>& displacements,
                                          vector<SIGN>& signs)
{
   bool ret = true;

   // Get the calculation
   IPropCalculation *propcalc = getCalculation();
   INTERNALEXCEPTION(propcalc, "No calculation object!");

   // Get the property 
   PropVector *vec = (PropVector*) propcalc->getProperty(getPropertyName());

   if (vec != (PropVector*)0) {
      ret = true;

      displacements.push_back(new double[3]);
      displacements[0][0] = vec->value(0);
      displacements[0][1]= vec->value(1);
      displacements[0][2] = vec->value(2);

      signs.push_back(NEUTRAL);
   }
   return ret;
}
