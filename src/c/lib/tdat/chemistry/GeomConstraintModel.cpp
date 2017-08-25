/**
 * @file
 *
 *
 */
#include <set>
#include <algorithm>
using std::set;
using std::find;

#ifdef __GNUC__
  #include <ext/hash_set>
  using __gnu_cxx::hash_set;
  using __gnu_cxx::hash;
#endif

#include "tdat/Fragment.H"
#include "tdat/GeomConstraint.H"
#include "tdat/GeomRestraint.H"
#include "tdat/GeomConstraintModel.H"
#include "tdat/GeomConstraintRules.H"


/**
 * Constructor.
 * The rules object is copied.
 * The fragment is held as a shared pointer.
 */
GeomConstraintModel::GeomConstraintModel(const Fragment& frag,
                                         const GeomConstraintRules& rules)
{
   p_frag = const_cast<Fragment*>(&frag);
   p_rules = new GeomConstraintRules(rules);
}



/**
 * Copy Constructor.
 * The Fragment and TAtm pointers are shared.  Other data
 * is deep-copied.
 */
GeomConstraintModel::GeomConstraintModel(const GeomConstraintModel& rhs)
{
   p_frag = 0; 
   p_rules = 0;

   *this = rhs;
}


/**
 * Destructor.
 */
GeomConstraintModel::~GeomConstraintModel()
{
   delete p_rules;

   p_frag = 0;

   clear();
}



/**
 * Assignment operator.
 * The Fragment and TAtm pointers are shared.  Other data
 * is deep-copied.
 */
GeomConstraintModel& GeomConstraintModel::operator=(const GeomConstraintModel& rhs)
{
   if (this != &rhs) {
      // Shared pointer
      p_frag = rhs.p_frag;

      // Shared pointers
      p_constraints = rhs.p_constraints;

      p_rules = new GeomConstraintRules(rhs.getRules());

   }
   return *this;

}


/**
 * Get the total number of constraints on the model.
 * Note that each fixed atom is considered a constraint.
 * @return total number of constraints, restraints, and fixed atoms
 */
int GeomConstraintModel::size() const
{
   return p_constraints.size();
}



/**
 * Remove all constraints, restraints, and fixed atoms.
 */
void GeomConstraintModel::clear()
{
   for (int idx=0; idx<p_constraints.size(); idx++) {
      delete p_constraints[idx];
   }
   p_constraints.clear();
}



/**
 * Recomputes all constraint values.
 * The recomputeConstraintValue is called for all constraint objects.
 * This may be useful in the case where coordinates of one or more
 * atoms have changed.
 *
 * This method should probably be depricated since setting a constraint
 * value no longer implies changing atom positions.
 */
void GeomConstraintModel::recomputeAll()
{
   for (int idx=0; idx<p_constraints.size(); idx++) {
      p_constraints[idx]->recomputeConstraintValue();
   }
}



/**
 * Change the fragment used by this model.
 */
int GeomConstraintModel::setFragment(const Fragment& frag)
{
   p_frag = const_cast<Fragment*>(&frag);
   return cleanUnmatchedAtoms();
}



/**
 * Get the fragment used by this model.
 */
const Fragment& GeomConstraintModel::getFragment() const
{
   return *p_frag;
}


/** 
 * Change geometry constraint rules used by this model.
 */
void GeomConstraintModel::setRules(GeomConstraintRules *rules)
{
   if (p_rules != 0) {
      delete p_rules;
   }
   p_rules = rules;
}


/**
 * Get the rules used by this model.
 */
const GeomConstraintRules& GeomConstraintModel::getRules() const
{
   return *p_rules;
}



/**
 * Get all constraints that apply to 'numAtoms'.
 * For example, if numAtoms==2, this will return all bond constraints.
 * where constraints implies both constraints and restraints.
 */
vector<GeomConstraint*> *GeomConstraintModel::getOfType(int numAtoms) const
{
   vector<GeomConstraint*> *ret = new vector<GeomConstraint*>;

   vector<GeomConstraint*>::const_iterator it;
   for (it=p_constraints.begin(); it!=p_constraints.end(); it++) {
      if ((*it)->getNumAtoms() == numAtoms) {
         ret->push_back((*it));
      }
   }

   return ret;
}



vector<GeomConstraint*> *GeomConstraintModel::getAtomConstraints() const
{
   return getOfType(1);
}



vector<GeomConstraint*> *GeomConstraintModel::getBondConstraints() const
{
   return getOfType(2);
}


vector<GeomConstraint*> *GeomConstraintModel::getAngleConstraints() const
{
   return getOfType(3);
}



vector<GeomConstraint*> *GeomConstraintModel::getTorsionConstraints() const
{
   return getOfType(4);
}


/**
 * Adds the constraint to the model if no rules are violated.
 * Rules are defined by the GeomConstraintRules class.
 * @param constraint object to add
 * @throw InvalidException if any rule is violated. 
 */
void GeomConstraintModel::addConstraint(GeomConstraint *constraint) 
           throw(InvalidException)
{
   GeomRestraint *restraint  = dynamic_cast<GeomRestraint*>(constraint);

   int nAtoms = constraint->getNumAtoms();

   if (restraint == 0) {
      // Constraint rules
      if ( !p_rules->supportsAtomConstraints() && nAtoms == 1) {
         throw InvalidException(
               "Fixed atoms are not supported by this code.", WHERE);
      }

      if ( !p_rules->supportsConstrainBonds() && nAtoms == 2) {
         throw InvalidException(
               "Bond constraints are not supported by this code.", WHERE);
      }

      if ( !p_rules->supportsConstrainAngles() && nAtoms == 3) {
         throw InvalidException(
               "Angle constraints are not supported by this code.", WHERE);
      }

      if ( !p_rules->supportsConstrainTorsions() && nAtoms == 4) {
         throw InvalidException(
               "Torsion constraints are not supported by this code.", WHERE);
      }
   } else {
      // Restraint rules

      if ( !p_rules->supportsRestrainBonds() && nAtoms == 2) {
         throw InvalidException(
               "Bond restraints are not supported by this code.", WHERE);
      }

      if ( !p_rules->supportsRestrainAngles() && nAtoms == 3) {
         throw InvalidException(
               "Angle restraints are not supported by this code.", WHERE);
      }

      if ( !p_rules->supportsRestrainTorsions() && nAtoms == 4) {
         throw InvalidException(
               "Torsion restraints are not supported by this code.", WHERE);
      }
   }

   // Categorize our existing types of constraints in order to
   // enforce the next set of rules.  If this gets to be a performance
   // issue, this info can be kept as state.
   int nFixedAtoms = 0;
   int nConstraints = 0;
   int nRestraints = 0;
   categorize(nFixedAtoms, nConstraints, nRestraints);

   // If its a fixed atom, then there may be a rule against mixing with
   // other types of constraints.  If the prospective constraint is
   // a fixed coord, make sure other types don't exist.  Also have to check 
   // if existing constraints are fixed coords and the prospective 
   // constraint is of another type.
   if (restraint == 0 && !p_rules->supportsFixedAtomsAndConstraints() ) {
      if (nAtoms == 1 ) {
         if (nConstraints > 0 ) {
            throw InvalidException(
                  "This code does not support fixed atoms in conjunction "
                  "with other constraints.  You must remove existing "
                  "constraints to fix atoms.", WHERE);

         }
      } else if (nAtoms > 1 ) {
         if (nFixedAtoms > 0) {
            throw InvalidException(
                  "This code does not support fixed atoms in conjunction "
                  "with other constraints.", WHERE);
         }
      }
   }


   if (restraint && !p_rules->supportsFixedAtomsAndRestraints() ) {
      if (nAtoms == 1 ) {
         if (nRestraints > 0 ) {
            throw InvalidException(
                  "This code does not support fixed atoms in conjunction "
                  "with restraints.  You must remove existing "
                  "restraints to fix atoms.", WHERE);

         }
      } else if (nAtoms > 1 ) {
         if (nFixedAtoms > 0) {
            throw InvalidException(
                  "This code does not support fixed atoms in conjunction "
                  "with restraints.", WHERE);
         }
      }
   }

   // This rule means that you can't mix constraints and restraints
   // If nAtoms==1, then its a fixed atom and this rule doesn't apply
   if ( nAtoms > 1 && !p_rules->supportsMixedConstraintsAndRestraints() ) {
      if ((restraint == 0 && nRestraints > 0) ||
          (restraint != 0 && nConstraints > 0)) {
         throw InvalidException(
               "This code does not support the simultaneous use of "
               "both constraints and restraints.", WHERE);
      }
   }

   p_constraints.push_back(constraint);
}



/**
 * Performs basic pre-request rules checking.
 * This method can be called in advance of a request to see if there is
 * any chance that the major types of constraints can be added.  Without
 * full detail of a specific request, the test cannot guarantee the outcome
 * but its useful to guide user actions.
 * 
 * @param fixedAtoms set to true if it may be possible to add fix atoms
 * @param constraints set to true if it may be possible to add constraints 
 * @param restraints set to true if it may be possible to add restraints 
 */
void GeomConstraintModel::pretestRules(bool& fixedAtoms,
                                       bool& constraints,
                                       bool& restraints) const
{
   fixedAtoms = true;
   constraints = true;
   restraints = true;

   int nFixedAtoms = 0;
   int nConstraints = 0;
   int nRestraints = 0;
   categorize(nFixedAtoms, nConstraints, nRestraints);

   // Check on fixed atoms.  If the code supports them then we still need
   // to check if they conflict with existing constraints
   fixedAtoms = p_rules->supportsAtomConstraints();
   if ( fixedAtoms && !p_rules->supportsFixedAtomsAndConstraints() ) {
      fixedAtoms = (nConstraints == 0 );
   }
   if ( fixedAtoms && !p_rules->supportsFixedAtomsAndRestraints() ) {
      fixedAtoms = (nRestraints == 0);
   }

   // CHeck on constraints.  If any type of bond,angle,torsion constraint
   // is allowed we stil have to see if it would conflict with existing
   // constraints
   constraints = p_rules->supportsConstrainBonds() ||
                 p_rules->supportsConstrainAngles() ||
                 p_rules->supportsConstrainTorsions();
   if (constraints && !p_rules->supportsFixedAtomsAndConstraints() ) {
      constraints = nFixedAtoms == 0;
   }
   if (constraints && !p_rules->supportsMixedConstraintsAndRestraints() ) {
      constraints = nRestraints == 0;
   }
   
   
   // Check on restraints.  If any type of bond,angle,torsion constraint
   // is allowed we stil have to see if it would conflict with existing
   // constraints
   restraints = p_rules->supportsRestrainBonds() ||
                 p_rules->supportsRestrainAngles() ||
                 p_rules->supportsRestrainTorsions();
   if (restraints && !p_rules->supportsFixedAtomsAndRestraints() ) {
      restraints = nFixedAtoms == 0;
   }
   if (restraints && !p_rules->supportsMixedConstraintsAndRestraints() ) {
      restraints = nConstraints == 0;
   }
}


/**
 * Categorize the set of constraints currently in the model.
 * Every constraint fits into exactly one of the categories.
 * All parameters initilized to 0.
 */
void GeomConstraintModel::categorize(int& nFixedAtoms,
                                     int& nConstraints,
                                     int& nRestraints) const
{
   nFixedAtoms = 0;
   nConstraints = 0;
   nRestraints = 0;

   GeomRestraint *restraint;
   for (vector<GeomConstraint*>::const_iterator it =p_constraints.begin(); 
         it!=p_constraints.end(); 
         it++) {
      restraint  = dynamic_cast<GeomRestraint*>( (*it) );
      if (restraint != 0) {
         nRestraints++;
      } else if ( (*it)->getNumAtoms() == 1) {
         nFixedAtoms++;
      } else {
         nConstraints++;
      }
   }
}


/**
 * Adds fixed atom constraints for each atom in the list.
 * Duplicates will not be added.
 * @return true if duplicates were discarded
 */
bool GeomConstraintModel::fixAtoms(const vector<int>& atoms)
           throw(InvalidException)
{
   bool ret = false;
   vector<GeomConstraint*> *constraints = getAtomConstraints();
   const Fragment& frag = getFragment();

   set<int,less<int> > haveAtoms;

   // May need to use a bit mask to improve performance here
   // Or maybe just non vector accessors to atoms would help too
   // But we build a set of all atoms currently declared as fixed atoms.
   for (int idx=0; idx<constraints->size(); idx++) {
      vector<TAtm*> *fixed = (*constraints)[idx]->getAtoms();
      haveAtoms.insert((*fixed)[0]->index());
      delete fixed;
   }
   delete constraints;

   for (int adx=0; adx<atoms.size(); adx++) {
      if (haveAtoms.find(atoms[adx]) == haveAtoms.end()) {
         GeomConstraint *constraint = 
                new GeomConstraint(frag.atomRef(atoms[adx]));
         // Check rules gets done here
         addConstraint(constraint);
         haveAtoms.insert(atoms[adx]);
      } else {
         ret = true;
      }
   }
   return ret;
}



/**
 * Adds a single constraint for the selected atoms.  
 * The atoms must be related as a bond, angle or torsion.
 *
 * Duplicates will not be added.
 * @param atoms vector of 2, 3, or 4 atoms representing a bond, angle, 
 *             or torsion
 * @return true if duplicates were discarded
 */
bool GeomConstraintModel::addConstraint(const vector<int>& atoms)
           throw(InvalidException)
{
   bool ret = false;
   if (atoms.size() < 2 || atoms.size() > 4) {
      throw InvalidException("Two, three, or four atoms must be supplied.",
                             WHERE);
   }

   const Fragment& frag = getFragment();
   GeomConstraint *constraint = 0;

   TAtm *a1=0, *a2=0, *a3=0, *a4=0;
   a1 = frag.atomRef(atoms[0]);
   a2 = frag.atomRef(atoms[1]);
   if (atoms.size() == 2) {
      constraint = new GeomConstraint(a1, a2);
   } else if (atoms.size() == 3) {
      a3 = frag.atomRef(atoms[2]);
      constraint = new GeomConstraint(a1, a2, a3);
   } else if (atoms.size() == 4) {
      a3 = frag.atomRef(atoms[2]);
      a4 = frag.atomRef(atoms[3]);
      constraint = new GeomConstraint(a1, a2, a3, a4);
   }

   if (find(constraint) == p_constraints.end()) {
      // This checks rules
      addConstraint(constraint);
   } else {
      ret = true;
      delete constraint;
   }
   return ret;

}



/**
 * Adds a single constraint for the selected atoms.  
 * The atoms must be related as a bond, angle or torsion.
 *
 * Duplicates will not be added.
 * @param atoms vector of 2, 3, or 4 atoms representing a bond, angle, 
 *             or torsion
 * @return true if duplicates were discarded
 */
bool GeomConstraintModel::addRestraint(const vector<int>& atoms)
           throw(InvalidException)
{
   bool ret = false;
   if (atoms.size() < 2 || atoms.size() > 4) {
      throw InvalidException("Two, three, or four atoms must be supplied.",
                             WHERE);
   }

   const Fragment& frag = getFragment();
   GeomConstraint *restraint = 0;

   TAtm *a1=0, *a2=0, *a3=0, *a4=0;
   a1 = frag.atomRef(atoms[0]);
   a2 = frag.atomRef(atoms[1]);
   if (atoms.size() == 2) {
      restraint =new GeomRestraint(a1, a2);
   } else if (atoms.size() == 3) {
      a3 = frag.atomRef(atoms[2]);
      restraint =new GeomRestraint(a1, a2, a3);
   } else if (atoms.size() == 4) {
      a3 = frag.atomRef(atoms[2]);
      a4 = frag.atomRef(atoms[3]);
      restraint =new GeomRestraint(a1, a2, a3, a4);
   }

   if (find(restraint) == p_constraints.end()) {
      // This checks rules
      addConstraint(restraint);
   } else {
      delete restraint;
      ret = true;
   }
   return ret;

}


vector<GeomConstraint*>::iterator 
GeomConstraintModel::find(const GeomConstraint *searchFor) 
{
   vector<GeomConstraint*>::iterator ret = p_constraints.end();

//   bool ret = false;
   for (vector<GeomConstraint*>::iterator it = p_constraints.begin();
        it != p_constraints.end();
        it++) {
      if (*(*it) == *searchFor) {
         //ret = true;
         ret = it;
         break;
      }
   }
   return ret;
}



/**
 * Remove constraints with the same atoms as the passed in constraint.
 * @param constraint object to remove (operator==)
 * @return true if a constraint was removed.
 */
bool GeomConstraintModel::removeLike(const GeomConstraint& constraint)
{
   bool ret = false;
   vector<GeomConstraint*>::iterator it = 
       find(dynamic_cast<const GeomConstraint*>(&constraint));

   if (it != p_constraints.end()) {
      ret = true;
      p_constraints.erase(it);
      delete *it;
   }
   return ret;
}





/**
 * Remove the constraint that is passed in.
 * Equality is determined by address (pointer value).
 * @param constraint object to remove
 * @return true if a constraint was removed.
 */
bool GeomConstraintModel::remove(const GeomConstraint *constraint)
{
   bool ret = false;
   vector<GeomConstraint*>::iterator it = 
      std::find(p_constraints.begin(), p_constraints.end(), constraint);
   if (it != p_constraints.end()) {
      delete (*it);
      p_constraints.erase(it);
      ret = true;
   }
   return ret;
}



void GeomConstraintModel::remove(int index)
{
  if (index >= 0 && index < p_constraints.size()) {
    vector<GeomConstraint*>::iterator it = p_constraints.begin() + index;
    p_constraints.erase(it);
  }
}




/**
 * Removes constraints that contain atoms not in the fragment.
 * @return number of constraints removed.
 */
int GeomConstraintModel::cleanUnmatchedAtoms()
{
  int ret = 0;
  vector<TAtm*> *validAtoms = p_frag->atoms();

  int numAtoms = validAtoms->size();
  if (numAtoms == 0) {
    ret = p_constraints.size();
    clear();
  } else {
    // The number of atoms in the system could be pretty large so we probably
    // don't want nested for loops.  We use TAtm pointers for comparison.  Sets
    // (binary trees) don't scale well with addresses so use a hash.
    hash_set<unsigned long> mymap(numAtoms);
    int idx;
    for (idx=0; idx<numAtoms; idx++) {
      mymap.insert((unsigned long)(*validAtoms)[idx]);
    }

    //Now that we have our fast hash map, loop through all the constraints
    //Do in reverse order for safe removal
    for (idx=p_constraints.size()-1; idx>=0; idx--) {
      int adx;
      GeomConstraint *constraint = p_constraints[idx];
      vector<TAtm*> *catoms = constraint->getAtoms();
      bool xit = false;
      for (adx=0; !xit && adx<catoms->size(); adx++) {
        if (mymap.find((unsigned long)((*catoms)[adx])) == mymap.end()) {
          // Uh oh, this atom is not there
          xit = true;
        }
      }
      delete catoms;

      if (xit) {
        remove(idx);
        ret++;
      }
    }
  }
  delete validAtoms;

  return ret;
}



/**
 * Convert indices to atom objects.
 * Useful for deserialization.
 *
 * @param indices vector of atom indices
 * @return non-null vector; caller should delete vector but not atoms
 */
vector<TAtm*> *
GeomConstraintModel::indicesToAtoms(const vector<int>& indices) const
                                    throw (InvalidException)
{
   vector<TAtm*> *ret = new vector<TAtm*>;
   if (p_frag) {
     int maxAtoms = p_frag->numAtoms();
     for (int idx=0; idx<indices.size(); idx++) {
        if (indices[idx] < maxAtoms) {
           ret->push_back(p_frag->atomRef(indices[idx]));
        }
     }
   } else {
      throw InvalidException("GeomConstraintModel has no Fragment",WHERE);
   }
   return ret;
}
