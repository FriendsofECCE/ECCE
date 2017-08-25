/**
 * @file
 *
 *
 */
#ifndef GEOMCONSTRAINT_HH
#define GEOMCONSTRAINT_HH

#include <vector>

using std::vector;

#include "tdat/TAtm.H"

/**
  * Class for defining constraints associated with geometry optimizations.
  *
  * Logically, a constraint is applied to one or more TAtm object and
  * a value associated with the relationship of those atoms is fixed.
  * For example, in the case of two atoms, the value is the bond distance,
  * for three atoms, the value is the angle, and for four atoms, the
  * value is the torsion.  
  *
  * There is also a special type of constraint on a single atom that fixes
  * its coordinates.  There is no value in this case.
  */
class GeomConstraint
{
  public:

    GeomConstraint(TAtm* atm1);
    GeomConstraint(TAtm* atm1, TAtm* atm2);
    GeomConstraint(TAtm* atm1, TAtm* atm2, TAtm* atm3);
    GeomConstraint(TAtm* atm1, TAtm* atm2, TAtm* atm3, TAtm* atm4);
    GeomConstraint(const GeomConstraint& rhs);
    virtual ~GeomConstraint();

    virtual bool operator==(const GeomConstraint& rhs);
    GeomConstraint& operator=(const GeomConstraint& rhs);

    virtual string typeidHack() const;

    int getNumAtoms() const;
    vector<TAtm *> *getAtoms() const;
    TAtm *getAtom(int n) const;
    vector<TAtm *> *getNameOrderedAtoms() const;

    TAtm *getAtom1() const;
    TAtm *getAtom2() const;
    TAtm *getAtom3() const;
    TAtm *getAtom4() const;

    void   setConstraintValue(double value);
    double getConstraintValue() const;

    void recomputeConstraintValue();

  protected:

    /**
     * list of atom pointers to atoms involved in constraint
     * The number of atoms determines the type of constraint (bond, angle...)
     */
    vector<TAtm*> p_atoms;

    /** value of constraint parameter */
    double p_constraintValue;


};
#endif 
