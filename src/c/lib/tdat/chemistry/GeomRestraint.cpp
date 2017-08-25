/**
 * @file
 *
 *
 */
#include <set>
using std::set;
using std::less;


#include "tdat/GeomRestraint.H"


/**
 *  Restraints on single atoms do nothing.
 *  Included for completeness with superclass.
 *
 */
GeomRestraint::GeomRestraint(TAtm *atm1)
   : GeomConstraint(atm1)
{
   // This is a hidden constructor.  Default value doesn't really matter
   p_restraintValue = 0.0;
}

/**
 *  Create a geometry restraint on a bond
 *
 *  @param atm1 atm2: pointers to atoms involved in bond
 */
GeomRestraint::GeomRestraint(TAtm *atm1, TAtm *atm2)
   : GeomConstraint(atm1, atm2)
{

   // Default value from Marat:
   // I used 5 for several cases and it worked fine. I don't think that
   // there is universal rule here. I would suggest using something between
   // and 1 and 5.
   p_restraintValue = 5.0;
}



/**
 *  Create a geometry restraint on an angle
 *
 *  @param atm1 atm2 atm3: pointers to atoms involved in angle
 *  @todo figure out a good default value once supported by nwchem.
 */
GeomRestraint::GeomRestraint(TAtm *atm1, TAtm *atm2, TAtm *atm3)
   : GeomConstraint(atm1, atm2, atm3)
{
   // Not supported yet so we don't know what a good default is
   p_restraintValue = 0.0;
}



/**
 *  Create a geometry restraint on a torsion angle
 *
 *  @param atm1 atm2 atm3 atm4: pointers to atoms involved in torsion
 *  @todo figure out a good default value once supported by nwchem.
 */
GeomRestraint::GeomRestraint(TAtm *atm1, TAtm *atm2, TAtm *atm3, TAtm *atm4)
   :GeomConstraint(atm1, atm2, atm3, atm4)
{
   // Not supported yet so we don't know what a good default is
   p_restraintValue = 0.0;
}


/**
 *  Destructor
 */
GeomRestraint::~GeomRestraint()
{
}


/**
 * Returns class name.
 * Use instead of C++ typeid() since we don't have support on all compilers.
 */
string GeomRestraint::typeidHack() const
{
   return "GeomRestraint";
}


/**
 * Equality operator - checks atoms and values.
 */
bool GeomRestraint::operator==(const GeomConstraint& rhs)
{
   bool ret = false;
   if (rhs.typeidHack() == typeidHack()) {
      if (p_atoms.size() == rhs.getNumAtoms()) {

         // build sorted sets for easy comparison that handles arbitrary order
         // Maybe OBE if we decide to order these
         set<int, less<int> > c1;
         set<int, less<int> > c2;
         int numAtoms = getNumAtoms();
         for (int idx=0; idx<numAtoms; idx++) {
            c1.insert(getAtom(idx)->index());
            c2.insert(rhs.getAtom(idx)->index());
         }
         ret = c1 == c2;
      }

      if (ret) {
         const GeomRestraint *restraint = 
                 dynamic_cast<const GeomRestraint*>(&rhs);
         if ( restraint->getRestraintValue() == this->getRestraintValue()) {
            ret = true;
         }
      }
   }

   return ret;
}



/**
 *  Return restraint value
 */

double GeomRestraint::getRestraintValue() const
{
   return p_restraintValue;
}


/**
 *  Set value of restraint paramter
 */
void GeomRestraint::setRestraintValue(double value)
{
   p_restraintValue = value;
}
