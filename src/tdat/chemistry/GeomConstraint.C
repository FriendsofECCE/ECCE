/**
 * @file
 *
 *
 */
#include <set>
using std::set;
using std::less;

#include "tdat/GeomConstraint.H"
#include "tdat/Measures.H"



/**
 *  Fixes and atom.
 *
 *  @param atm1 atm2: pointers to atoms involved in bond
 */
GeomConstraint::GeomConstraint(TAtm *atm1)
{
   p_atoms.push_back(atm1);
   p_constraintValue = -1.;
}


/**
 *  Create a geometry constraint between two atoms.
 *  The constraint value is computed as the distance between
 *  the atoms.
 *
 *  @param atm1 atm2: pointers to atoms involved in bond
 */
GeomConstraint::GeomConstraint(TAtm *atm1, TAtm *atm2)
{
   p_atoms.push_back(atm1);
   p_atoms.push_back(atm2);
   p_constraintValue = Measures::computeLength(atm1, atm2);
}



/**
 *  Create a geometry constraint on an angle.
 *  The angle is defined as running from atm1 to atm2, to atm3.
 *  The initial constraint value is computed as the angle.
 *
 *  @param atm1 atm2 atm3: pointers to atoms involved in angle;
 *    all atom pointers must point to valid objects.
 */
GeomConstraint::GeomConstraint(TAtm *atm1, TAtm *atm2, TAtm *atm3)
{
   p_atoms.push_back(atm1);
   p_atoms.push_back(atm2);
   p_atoms.push_back(atm3);
   p_constraintValue = Measures::computeAngle(atm1, atm2, atm3);
}



/**
 *  Create a geometry constraint on a torsion angle. The torsion is
 *  defined as running from atm1 to atm2, to atm3, to atm4. 
 *  The constraint value is inialized to the torson.
 *
 *  @param atm1 atm2 atm3 atm4: pointers to atoms involved in torsion
 */
GeomConstraint::GeomConstraint(TAtm *atm1, TAtm *atm2, TAtm *atm3, TAtm *atm4)
{
   p_atoms.push_back(atm1);
   p_atoms.push_back(atm2);
   p_atoms.push_back(atm3);
   p_atoms.push_back(atm4);
   p_constraintValue = Measures::computeDihedral(atm1, atm2, atm3, atm4);
}



/**
 *  Copy Constructor
 */
GeomConstraint::GeomConstraint(const GeomConstraint& rhs)
{
   *this = rhs;
}


/**
 *  Destructor
 */
GeomConstraint::~GeomConstraint()
{
}


/**
 * Equality operator - true if the same atoms are used.
 * Order of atoms is not important.
 */
bool GeomConstraint::operator==(const GeomConstraint& rhs)
{
   bool ret = false;
   if (rhs.typeidHack() == typeidHack()) {
      if (p_atoms.size() == rhs.getNumAtoms()) {

         // build sorted sets for easy comparison that handles arbitrary order
         // Maybe OBE if we decide to order these
         set<int, less<int> > c1;
         set<int, less<int> > c2;
         for (int idx=0; idx<p_atoms.size(); idx++) {
            c1.insert(p_atoms[idx]->index());
            c2.insert(rhs.p_atoms[idx]->index());
         }
         ret = c1 == c2;
      }
   }
   return ret;
}


/**
 * Returns class name.
 * Use instead of C++ typeid() since we don't have support on all compilers.
 */
string GeomConstraint::typeidHack() const
{
   return "GeomConstraint";
}


/**
 * Assignment operator.
 */
GeomConstraint& GeomConstraint::operator=(const GeomConstraint& rhs)
{
   if (this != &rhs) {
      p_atoms = rhs.p_atoms;
      setConstraintValue( rhs.getConstraintValue() );
   }
   return *this;
}



/**
 *  Return a list of atom pointers for atoms involved in constraint
 *  
 *  @return non-null vector; caller must delete vector but not atoms
 */
vector<TAtm*> *GeomConstraint::getAtoms() const
{
   vector<TAtm*> *ret = new vector<TAtm*>;
   *ret = p_atoms;
   return ret;
}



/**
 *  Return a list of atom pointers ordered for presentation.
 *  Presentation order is currently defined as alphabetical in the
 *  context of atoms whose order can be swapped without affecting
 *  operations.  So for example, the center atom of an angle cannot be
 *  changed.
 *  
 *  @return non-null vector; caller must delete vector but not atoms
 */
vector<TAtm*> *GeomConstraint::getNameOrderedAtoms() const
{
   vector<TAtm*> *ret = new vector<TAtm*>;
   if (p_atoms.size() == 1) {
      *ret = p_atoms;
   } else if (p_atoms.size() == 2) {
      string atsym1 = p_atoms[0]->atomicSymbol();
      string atsym2 = p_atoms[1]->atomicSymbol();
      if (atsym1 < atsym2) {
         *ret = p_atoms;
      } else {
         ret->push_back(p_atoms[1]);
         ret->push_back(p_atoms[0]);
      }
   } else if (p_atoms.size() == 3) {
      string atsym1 = p_atoms[0]->atomicSymbol();
      string atsym3 = p_atoms[2]->atomicSymbol();
      if (atsym1 < atsym3) {
         *ret = p_atoms;
      } else {
         ret->push_back(p_atoms[2]);
         ret->push_back(p_atoms[1]);
         ret->push_back(p_atoms[0]);
      }
   } else if (p_atoms.size() == 4) {
      string atsym2 = p_atoms[1]->atomicSymbol();
      string atsym3 = p_atoms[2]->atomicSymbol();
      if (atsym2 < atsym3) {
         *ret = p_atoms;
      } else {
         ret->push_back(p_atoms[3]);
         ret->push_back(p_atoms[2]);
         ret->push_back(p_atoms[1]);
         ret->push_back(p_atoms[0]);
      }
   }
   return ret;
}


/**
 * Get the first atom for this constraint.
 * @return possibly 0 atom pointer
 */
TAtm *GeomConstraint::getAtom1() const
{
   return getAtom(0);
}



/**
 * Get the second atom for this constraint.
 * @return possibly 0 atom pointer
 */
TAtm *GeomConstraint::getAtom2() const
{
   return getAtom(1);
}



/**
 * Get the third atom for this constraint.
 * @return possibly 0 atom pointer
 */
TAtm *GeomConstraint::getAtom3() const
{
   return getAtom(2);
}



/**
 * Get the fourth atom for this constraint.
 * @return possibly 0 atom pointer
 */
TAtm *GeomConstraint::getAtom4() const
{
   return getAtom(3);
}



/**
 * Get the nth atom for this constraint.
 * @return possibly 0 atom pointer
 */
TAtm *GeomConstraint::getAtom(int n) const
{
   TAtm *ret = 0;
   if (p_atoms.size() >= n ) {
      ret = p_atoms[n];
   }
   return ret;
}



/**
 *  Return constraint value
 */
double GeomConstraint::getConstraintValue() const
{
   return p_constraintValue;
}



/**
 *  Set constraint value.
 */
void GeomConstraint::setConstraintValue(double value)
{
   p_constraintValue = value;
}





/**
 *  Return number of atoms in constraint
 */
int GeomConstraint::getNumAtoms() const
{
   return p_atoms.size();
}



/**
 *  Set value of constraint based on current geometry of atoms
 */
void GeomConstraint::recomputeConstraintValue()
{
   if (p_atoms.size() == 2) {
      p_constraintValue = Measures::computeLength(p_atoms[0], p_atoms[1]);
   } else if (p_atoms.size() == 3) {
      p_constraintValue = Measures::computeAngle(p_atoms[0], p_atoms[1],
            p_atoms[2]);
   } else if (p_atoms.size() == 4) {
      p_constraintValue = Measures::computeDihedral(p_atoms[0], p_atoms[1],
            p_atoms[2], p_atoms[3]);
   }
}

