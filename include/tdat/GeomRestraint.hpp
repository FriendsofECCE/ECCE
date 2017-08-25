/**
 * @file
 *
 *
 */
#ifndef GEOMRESTRAINT_HH
#define GEOMRESTRAINT_HH

#include "tdat/GeomConstraint.H"


/**
 *
 * Restraints are a type of restraint that make use of a restraining
 * potential.  The potential has a single value associated with it.
 * This value is something the user must typically specify.
 *
 * @todo - come up with default restraint values?
 */
class GeomRestraint : public GeomConstraint
{
  public:

    GeomRestraint(TAtm* atm1, TAtm* atm2);
    GeomRestraint(TAtm* atm1, TAtm* atm2, TAtm* atm3);
    GeomRestraint(TAtm* atm1, TAtm* atm2, TAtm* atm3, TAtm* atm4);
    ~GeomRestraint();

    virtual bool operator==(const GeomConstraint& rhs);
    virtual string typeidHack() const;

    double getRestraintValue() const;
    void setRestraintValue(double value);

  protected:
    GeomRestraint(TAtm* atm1);

    /** value of restraint.  */
    double p_restraintValue;
};
#endif 
