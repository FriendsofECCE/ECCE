/**
 * @file
 *
 *
 */
#ifdef __GNUC__
  #include <ext/hash_set>
  using __gnu_cxx::hash_set;
  using __gnu_cxx::hash;
#endif

#include "util/NullPointerException.H"
#include "tdat/ESPConstraintModel.H"
#include "tdat/ESPConstraint.H"
#include "tdat/Fragment.H"

ESPConstraintModel::ESPConstraintModel(const Fragment& frag)
{
   setFragment(frag);
}

ESPConstraintModel::~ESPConstraintModel()
{
}

int ESPConstraintModel::setFragment(const Fragment& frag)
{
   p_frag = &frag;
   return cleanUnmatchedAtoms();
}

const Fragment& ESPConstraintModel::getFragment() const
{
   return *p_frag;
}

int ESPConstraintModel::size() const
{
  return p_constraints.size();
}

/** 
 * Remove all constraints.
 */
void ESPConstraintModel::clear()
{
  p_constraints.clear();
}


ESPConstraint *
ESPConstraintModel::get(int index) const throw (IndexOutOfRangeException)
{
  ESPConstraint *ret = 0;
  if (index >= 0 && index < p_constraints.size()) {
    ret = p_constraints[index];
  } else {
     throw IndexOutOfRangeException(0, p_constraints.size(), index, WHERE);
  }
  return ret;
}

void ESPConstraintModel::add(ESPConstraint::Type type) 
{
  ESPConstraint *constraint = new ESPConstraint();
  constraint->type(type);
  add(constraint);
}

void ESPConstraintModel::add(ESPConstraint *constraint)
{
  p_constraints.push_back(constraint);
}

bool ESPConstraintModel::remove(int index) throw (IndexOutOfRangeException) 
{
  bool ret = false;
  if (index >= 0 && index < p_constraints.size()) {
    vector<ESPConstraint*>::iterator it = p_constraints.begin() + index;
    p_constraints.erase(it);
    ret = true;
  } else {
     throw IndexOutOfRangeException(0, p_constraints.size(), index, WHERE);
  }
  return ret;
}

/**
 * Determines if a methyl constraint for the specified atom already exists.
 * @param atom - C atom we're looking for.
 */
bool ESPConstraintModel::methylConstraintExists(const TAtm& atom) const
{
  bool found = false;
  for (int idx=p_constraints.size()-1; idx>=0; idx--) {
    ESPConstraint *constraint = p_constraints[idx];
    if (constraint->type() == ESPConstraint::METHYL) {
      vector<TAtm*> g1 = constraint->group1();
      if (g1.size() > 0 && g1[0] == &atom) {
        found = true;
        break;
      }
    }
  }
  return found;
}


/**
 * Determine which if any atoms have no constraints on them.
 * This method uses the fragment passed into the constructor or setFragment.
 */
vector<bool> ESPConstraintModel::unconstrained()
{
  vector<bool> ret;
  const Fragment& frag = reinterpret_cast<const Fragment&>(*p_frag);

  ret.resize(frag.numAtoms(),false);

  int adx;
  for (int idx=0; idx<p_constraints.size(); idx++) {
    ESPConstraint *constraint = p_constraints[idx];
    vector<TAtm*> group = constraint->group1();
    for (adx=0; adx<group.size(); adx++) {
      ret[group[adx]->index()] = true;
    }
    group = constraint->group2();
    for (adx=0; adx<group.size(); adx++) {
      ret[group[adx]->index()] = true;
    }
  }

  return ret;
}


/**
 * Determine which if any atoms have multiple constraints on them.
 * This method uses the fragment passed into the constructor or setFragment.
 */
vector<bool> ESPConstraintModel::multipleeConstrained()
{
  vector<bool> ret;
  vector<int> cnts;
  const Fragment& frag = reinterpret_cast<const Fragment&>(*p_frag);

  ret.resize(frag.numAtoms(),false);
  cnts.resize(frag.numAtoms(),0);

  int idx;
  int adx;
  for (idx=0; idx<p_constraints.size(); idx++) {
    ESPConstraint *constraint = p_constraints[idx];
    vector<TAtm*> group = constraint->group1();
    for (adx=0; adx<group.size(); adx++) {
      cnts[group[adx]->index()] += 1;
    }
    group = constraint->group2();
    for (adx=0; adx<group.size(); adx++) {
      cnts[group[adx]->index()] += 1;
    }
  }

  for (idx=cnts.size()-1; idx>=0; idx-- ) {
    if (cnts[idx] > 1) ret[idx] = true;
  }

  return ret;
}

/**
 * Generate a FIXED constraint for each atom at the atom indices.
 * The charge must be taken from the atoms partial charge.
 */
int ESPConstraintModel::generateFixedConstraints(const vector<int>& atoms)
{
   int ret = 0;
   TAtm *atm;
   int numAtoms = p_frag->numAtoms();
   for (int idx=0; idx<atoms.size(); idx++) {
      // Not much of a check but don't exceed array boundaries
      if (idx<numAtoms) {
         atm = p_frag->atomRef(atoms[idx]);
         if (!atm->isNub()) {
            ESPConstraint *constraint = new ESPConstraint();
            constraint->type(ESPConstraint::FIXED);
            constraint->charge(atm->partialCharge());
            vector<TAtm*> groupofone;
            groupofone.push_back(atm);
            constraint->group1(groupofone);
            add(constraint);
            ret++;
         }
      }
   }
   return ret;
}


/**
 * Generate Methyl group constraints.
 * Generate a constraint for any C atom that has exactly 3 H atoms.
 * But make sure a methyl constraint doesn't already exist for that C.
 * This method uses the fragment passed into the constructor or setFragment.
 * @return number of constraints created.
 */
int ESPConstraintModel::generateMethylConstraints()
{
  int ret = 0; 
  const Fragment& frag = reinterpret_cast<const Fragment&>(*p_frag);

  vector<TAtm*> *allatoms = frag.atoms();

  TAtm *atom = 0;
  for (int idx=0; idx<allatoms->size(); idx++) {
    atom = (*allatoms)[idx];
    if (atom->atomicSymbol() == "C") {
      vector<TAtm*> g1,g2;
      g1.push_back(atom);
      vector<TAtm*> *toatoms = atom->connectedAtoms();
      int numH = 0;
      for (int adx=0; adx<toatoms->size(); adx++) {
         if ((*toatoms)[adx]->atomicNumber() == 1 /* H */) {
            numH++;
            g2.push_back((*toatoms)[adx]);
         }
      }
      if (numH == 3 && !methylConstraintExists(*atom)) {
        ESPConstraint *constraint = new ESPConstraint;
        constraint->type(ESPConstraint::METHYL);
        for (int jdx=0; jdx<numH; jdx++) {
          g1.push_back(g2[jdx]);
        }
        constraint->group1(g1);
        p_constraints.push_back(constraint);
        ret++;
      }
      delete toatoms;
    }
  }

  delete allatoms;

  return ret;
}


/**
 * Generates equality constraints for all hydrogen sets.
 * Each set (>1) of H atoms attached to a single central atom get added to
 * their own constraint. There is currently no protection against duplicates
 * if its called twice.
 * This method uses the fragment passed into the constructor or setFragment.
 *
 * return number of constraints created.
 */
int ESPConstraintModel::generateHydrogenConstraints()
{
  int ret = 0; 
  // Convert to reference
  const Fragment& frag = reinterpret_cast<const Fragment&>(*p_frag);

  vector<TAtm*> *allatoms = frag.atoms();

  TAtm *atom = 0;
  int isize = p_constraints.size();
  for (int idx=0; idx<allatoms->size(); idx++) {
    atom = (*allatoms)[idx];
    if (atom->atomicSymbol() != "H") {
      vector<TAtm*> g1;
      vector<TAtm*> *toatoms = atom->connectedAtoms();
      for (int adx=0; adx<toatoms->size(); adx++) {
        if ((*toatoms)[adx]->atomicNumber() == 1 /* H */) {
          g1.push_back((*toatoms)[adx]);
        }
      }
      if (g1.size() > 1) {
        ESPConstraint *constraint = new ESPConstraint;
        constraint->type(ESPConstraint::EQUAL);
        constraint->group1(g1);
        // Check preexisting constraints to see if constraint already
        // exists
        bool found = false;
        for (int i=0; i<isize; i++) {
          if (*p_constraints[i] == *constraint) {
            found = true;
            break;
          }
        }
        if (!found) {
          p_constraints.push_back(constraint);
          ret++;
        } else {
          delete constraint;
        }
      }
      delete toatoms;
    }
  }

  delete allatoms;

  return ret;
}




/**
 * Removes constraints that contain atoms not in the validAtoms list.
 * This method uses the fragment passed into the constructor or setFragment.
 * @return number of constraints removed.
 */
int ESPConstraintModel::cleanUnmatchedAtoms()
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
      ESPConstraint *constraint = p_constraints[idx];
      vector<TAtm*> group = constraint->group1();
      bool xit = false;
      for (adx=0; xit==false && adx<group.size(); adx++) {
        if (mymap.find((unsigned long)(group[adx])) == mymap.end()) {
          // Oh oh this atom is not there
          xit = true;
        }
      }
      group = constraint->group2();
      for (adx=0; xit==false && adx<group.size(); adx++) {
        if (mymap.find((unsigned long)group[adx]) == mymap.end()) {
          // Oh oh this atom is not there
          xit = true;
        }
      }

      if (xit == true) {
        remove(idx);
        ret++;
      }
    }
  }
  delete validAtoms;

  return ret;
}



/**
 * Converts vector<int> to corresponsing vector<TAtm*> using Fragment.
 *  useful for serialization.
 *
 * @throws NullPointerException if internal state has no fragment.
 */
vector<TAtm*> 
ESPConstraintModel::indicesToGroup(const vector<int>& indices) const
{
   NULLPOINTEREXCEPTION(p_frag,"ESPConstraintModel has no Fragment");

   vector<TAtm*> ret;
   int maxAtoms = p_frag->numAtoms();
   for (int idx=0; idx<indices.size(); idx++) {
      if (indices[idx] < maxAtoms) {
         ret.push_back(p_frag->atomRef(indices[idx]));
      }
   }
   return ret;
}
