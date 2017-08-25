/**
 * @file
 *
 *
 */
#ifndef MEASURES_HH
#define MEASURES_HH

#include "tdat/TAtm.H"

/**
  * Class that computes different measures between atoms (bonds, angles,
  * torsions). This class has no accessors or modifiers and basically
  * provides static methods that can be used to evaluate properties of
  * collections of atoms.
  */
class Measures
{
  public:

    // Measures

    static double computeLength(TAtm *a1, TAtm *a2);

    static double computeAngle(TAtm *a1, TAtm *a2, TAtm *a3);

    static double computeDihedral(TAtm *a1, TAtm *a2, TAtm *a3, TAtm *a4);

  private:

};
#endif // MEASURES_HH
