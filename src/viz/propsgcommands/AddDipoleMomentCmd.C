#include <iostream>
  using std::cout;
  using std::endl;

//#include "inv/nodes/SoSwitch.H"

#include "util/InternalException.H"

#include "dsm/IPropCalculation.H"

#include "tdat/PropVector.H"

#include "viz/cvSGContainer.H"
#include "viz/VRVector.H"
#include "viz/AddDipoleMomentCmd.H"

// 2 D (water) is then about one O-H bond long.
const double AddDipoleMomentCmd::ANGSTROM_PER_DEBYE = 0.5;

AddDipoleMomentCmd::AddDipoleMomentCmd(const string& name, Receiver *receiver,
                         IPropCalculation *calc)
  : AddVectorCmd(name, receiver, calc)
{
  p_preferredScaleFactor = ANGSTROM_PER_DEBYE;
}


/**
 * VRVector puts its tip at 1.075 |direction|; divided out so that the tip
 * is exactly Amplitude x |mu| Angstrom from the origin.
 */
double AddDipoleMomentCmd::getScaleFactor()
{
  return AddVectorCmd::getScaleFactor() / 1.075;
}


/**
 * A shaft and head of constant size, set against the default bond radius
 * (0.14 A): with the length-proportional shape a large dipole drew a club
 * and a small one a needle.
 */
void AddDipoleMomentCmd::shapeVector(VRVector *vec)
{
  vec->fixedThickness(0.06f, 0.15f, 0.30f);
  vec->showThroughMolecule();
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
