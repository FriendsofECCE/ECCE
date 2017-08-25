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
#include "tdat/MetaPotential.H"
#include "tdat/MetaPotentialModel.H"


/**
 * Constructor.
 * The fragment is held as a shared pointer.
 */
MetaPotentialModel::MetaPotentialModel(const Fragment& frag)
{
   p_frag = const_cast<Fragment*>(&frag);
}


/**
 * Copy Constructor.
 * Fragment pointer is shared. Other data is deep-copied.
 */
MetaPotentialModel::MetaPotentialModel(const MetaPotentialModel& rhs)
{
   p_frag = 0; 

   *this = rhs;
}


/**
 * Destructor.
 */
MetaPotentialModel::~MetaPotentialModel()
{
   p_frag = 0;
   clear();
}


/**
 * Assignment operator.
 * The Fragment and TAtm pointers are shared.  Other data
 * is deep-copied.
 */
MetaPotentialModel& MetaPotentialModel::operator=(const MetaPotentialModel& rhs)
{
   if (this != &rhs) {
      // Shared pointer
      p_frag = rhs.p_frag;

      // Shared pointers
      p_mpotential = rhs.p_mpotential;

   }
   return *this;

}


/**
 * Get the total number of potentials in the model.
 * @return total number of potentials
 */
int MetaPotentialModel::size() const
{
   return p_mpotential.size();
}


/**
 * Remove potentials
 */
void MetaPotentialModel::clear()
{
   for (int idx=0; idx<p_mpotential.size(); idx++) {
      delete p_mpotential[idx];
   }
   p_mpotential.clear();
}


/**
 * Change the fragment used by this model.
 * @return the number of existing potentials that are removed because they are
 * undefined in the new fragment (no matching atom indices)
 */
int MetaPotentialModel::setFragment(const Fragment& frag)
{
   p_frag = const_cast<Fragment*>(&frag);
   return cleanUnmatchedAtoms();
}


/**
 * Get the fragment used by this model.
 */
const Fragment& MetaPotentialModel::getFragment() const
{
   return *p_frag;
}


/**
 * Return a list of all potentials that are defined by 2 atoms
 * @return a list of potentials in the model that are defined by two atoms. This
 * list must be deleted by the calling program.
 */
vector<MetaPotential*> *MetaPotentialModel::get2SitePotentials() const
{
  vector<MetaPotential*> *ret = new vector<MetaPotential*>;
  int size = p_mpotential.size();
  int idx;
  for (idx=0; idx<size; idx++) {
    if ((p_mpotential[idx])->getNumAtoms() == 2) {
      ret->push_back(p_mpotential[idx]);
    }
  }
  return ret;
}


/**
 * Return a list of all potentials that are defined by 3 atoms
 * @return a list of potentials in the model that are defined by 3 atoms. This
 * list must be deleted by the calling program.
 */
vector<MetaPotential*> *MetaPotentialModel::get3SitePotentials() const
{
  vector<MetaPotential*> *ret = new vector<MetaPotential*>;
  int size = p_mpotential.size();
  int idx;
  for (idx=0; idx<size; idx++) {
    if ((p_mpotential[idx])->getNumAtoms() == 3) {
      ret->push_back(p_mpotential[idx]);
    }
  }
  return ret;
}


/**
 * Adds the potential to the model.
 * @param potential object to add
 * @throw InvalidException if any rule is violated. 
 */
void MetaPotentialModel::addPotential(MetaPotential *mpot) 
           throw(InvalidException)
{
   p_mpotential.push_back(mpot);
}

#if 0
/**
 * Categorize the set of constraints currently in the model.
 * Every constraint fits into exactly one of the categories.
 * All parameters initilized to 0.
 */
void MetaPotentialModel::categorize(int& nFixedAtoms,
                                     int& nConstraints,
                                     int& nRestraints) const
{
   nFixedAtoms = 0;
   nConstraints = 0;
   nRestraints = 0;

   GeomRestraint *restraint;
   for (vector<MetaPotential*>::const_iterator it =p_mpotential.begin(); 
         it!=p_mpotential.end(); 
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
#endif

/**
 * Adds a single constraint for the selected atoms.  
 * The atoms must be related as a bond, angle or torsion.
 *
 * Duplicates will not be added.
 * @param atoms vector of 2, 3, or 4 atoms representing a bond, angle, 
 *             or torsion
 * @return true if duplicates were discarded
 */
bool MetaPotentialModel::addPotential(const vector<int>& atoms)
           throw(InvalidException)
{
   bool ret = false;
   if (atoms.size() < 2 || atoms.size() > 3) {
      throw InvalidException("Two or three atoms must be supplied.",
                             WHERE);
   }

   MetaPotential *potential = 0;
   vector<double> params;
   vector<int> atom_copy;
   for (int i=0; i<atoms.size(); i++) {
     atom_copy.push_back(atoms[i]);
   }

   if (atoms.size() == 2) {
      params.push_back(0.00005);
      params.push_back(0.1);
      potential = new MetaPotential(2,&atom_copy,2,&params);
      ret = true;
   }

   if (atoms.size() == 3) {
      params.push_back(0.00005);
      params.push_back(0.1);
      potential = new MetaPotential(3,&atom_copy,2,&params);
      ret = true;
   }

   if (find(potential) == p_mpotential.end()) {
      addPotential(potential);
   } else {
      ret = true;
      delete potential;
   }
   return ret;

}


vector<MetaPotential*>::iterator 
MetaPotentialModel::find(const MetaPotential *searchFor) 
{
  vector<MetaPotential*>::iterator ret = p_mpotential.end();
  for (vector<MetaPotential*>::iterator it = p_mpotential.begin();
      it != p_mpotential.end();
      it++) {
    if (*(*it) == *searchFor) {
      ret = it;
      break;
    }
  }
  return ret;
}


/**
 * Remove the potential that is passed in.
 * Equality is determined by address (pointer value).
 * @param potential object to remove
 * @return true if a constraint was removed.
 */
bool MetaPotentialModel::remove(const MetaPotential *mpot)
{
   bool ret = false;
   vector<MetaPotential*>::iterator it = 
      std::find(p_mpotential.begin(), p_mpotential.end(), mpot);
   if (it != p_mpotential.end()) {
      delete (*it);
      p_mpotential.erase(it);
      ret = true;
   }
   return ret;
}


/**
 * Removes potentials that contain atoms not in the fragment.
 * @return number of potentials removed.
 */
int MetaPotentialModel::cleanUnmatchedAtoms()
{
  int ret = 0;
  vector<TAtm*> *validAtoms = p_frag->atoms();
  vector<int> validIDs;

  int numAtoms = validAtoms->size();
  int idx;
  for (idx=0; idx<numAtoms; idx++) {
    validIDs.push_back(((*validAtoms)[idx])->index());
  }
  if (numAtoms == 0) {
    ret = p_mpotential.size();
    clear();
  } else {
    // The number of atoms in the system could be pretty large so we probably
    // don't want nested for loops. Sets
    // (binary trees) don't scale well with addresses so use a hash.
    hash_set<int> mymap(numAtoms);
    for (idx=0; idx<numAtoms; idx++) {
      mymap.insert(validIDs[idx]);
    }

    //Now that we have our fast hash map, loop through all the constraints
    //Do in reverse order for safe removal
    for (idx=p_mpotential.size()-1; idx>=0; idx--) {
      int adx;
      MetaPotential *mpot = p_mpotential[idx];
      vector<int> *catoms = mpot->getAtoms();
      bool xit = false;
      for (adx=0; !xit && adx<catoms->size(); adx++) {
        if (mymap.find((*catoms)[adx]) == mymap.end()) {
          // Uh oh, this atom is not there
          xit = true;
        }
      }
      delete catoms;

      if (xit) {
        //MetaPotentialModel::remove(p_mpotential[idx]);
        remove(p_mpotential[idx]);
        ret++;
      }
    }
  }

  return ret;
}
