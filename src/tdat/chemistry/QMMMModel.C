/**
 * @file
 *
 *
 */
#include <set>
#include <algorithm>
using std::set;
using std::find;

#include <iostream>
using std::cout;
using std::endl;

#ifdef __GNUC__
  #include <ext/hash_set>
  using __gnu_cxx::hash_set;
  using __gnu_cxx::hash;
#endif

#include "tdat/Fragment.H"
#include "tdat/QMMMParams.H"
#include "tdat/QMMMModel.H"


/**
 * Constructor.
 * The fragment is held as a shared pointer.
 */
QMMMModel::QMMMModel(const Fragment& frag)
{
   p_frag = const_cast<Fragment*>(&frag);
   clear();
}


/**
 * Copy Constructor.
 * Fragment pointer is shared. Other data is deep-copied.
 */
QMMMModel::QMMMModel(const QMMMModel& rhs)
{
   p_frag = 0; 
   clear();

   *this = rhs;
}


/**
 * Destructor.
 */
QMMMModel::~QMMMModel()
{
   p_frag = 0;
   clear();
}


/**
 * Assignment operator.
 * The Fragment and QMMMParams pointers are shared.
 */
QMMMModel& QMMMModel::operator=(const QMMMModel& rhs)
{
   if (this != &rhs) {
      // Shared pointer
      p_frag = rhs.p_frag;

      // Shared pointers
      p_terms = rhs.p_terms;

   }
   return *this;

}


/**
 * Get the total number of terms in the model.
 * @return total number of terms
 */
int QMMMModel::size() const
{
   return p_terms.size();
}


/**
 * Remove all terms in the model
 */
void QMMMModel::clear()
{
   for (int idx=0; idx<p_terms.size(); idx++) {
      delete p_terms[idx];
   }
   p_terms.clear();
}


/**
 * Change the fragment used by this model.
 * @param frag: new fragment used by model
 * @return the number of existing QMMM terms that are removed because they are
 * undefined in the new fragment (no matching atom types)
 */
int QMMMModel::setFragment(const Fragment& frag)
{
   p_frag = const_cast<Fragment*>(&frag);
   return cleanUnmatchedTerms();
}


/**
 * Get the fragment used by this model.
 */
const Fragment& QMMMModel::getFragment() const
{
   return *p_frag;
}


/**
 * Return a list of all QMMM parameter sites that are of the same type (e.g.
 * bonds)
 * @param type: string representing desired parameter sets
 * @return a list of parameters sets of the desired type. The list must be
 * deleted by the calling program.
 */
vector<QMMMParams*> *QMMMModel::getQMMMTerms(string type) const
{
  vector<QMMMParams*> *ret = new vector<QMMMParams*>;
  int size = p_terms.size();
  int idx;
  for (idx=0; idx<size; idx++) {
    if ((p_terms[idx])->getQMMMType() == type) {
      ret->push_back(p_terms[idx]);
    }
  }
  return ret;
}


/**
 * Adds a new set of QMMM parameters to the model.
 * @param term: new QMMM parameter set to add
 * @throw InvalidException if any rule is violated. 
 */
void QMMMModel::addQMMMTerm(QMMMParams *term) 
           throw(InvalidException)
{
  int idx, jdx;
  bool found = false;
  for (idx=0; idx<p_terms.size() && !found; idx++) {
    // check to see if new parameter set matches an existing parameter set
    int new_size, old_size;
    new_size = term->getNumAtoms();
    old_size = p_terms[idx]->getNumAtoms();
    string new_type = term->getQMMMType();
    string old_type = p_terms[idx]->getQMMMType();
    if (new_type != old_type) continue;
    if (new_size != old_size) continue;
    bool same = true;
    if (new_type != "bond_set" && new_type != "angle_set") {
      vector<string>  *new_types = term->getQMMMPotentials();
      vector<string>  *old_types = p_terms[idx]->getQMMMPotentials();
      for (jdx = 0; jdx < new_size && same; jdx++) {
        if ((*new_types)[jdx] != (*old_types)[jdx]) same = false;
      }
      delete new_types;
      delete old_types;
      if (same == true) found = true;
    } else {
      vector<int>  *new_ids = term->getAtomIndices();
      vector<int>  *old_ids = p_terms[idx]->getAtomIndices();
      for (jdx = 0; jdx < new_size && same; jdx++) {
        if ((*new_ids)[jdx] != (*old_ids)[jdx]) same = false;
      }
      delete new_ids;
      delete old_ids;
      if (same == true) found = true;
    }
  }
  if (!found) p_terms.push_back(term);
  else
    throw InvalidException("Atom types match existing parameter set",
                             WHERE);
}

vector<QMMMParams*>::iterator 
QMMMModel::find(const QMMMParams *searchFor) 
{
  vector<QMMMParams*>::iterator ret = p_terms.end();
  for (vector<QMMMParams*>::iterator it = p_terms.begin();
      it != p_terms.end();
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
 * @param term: object to remove
 * @return true if a QMMM parameter set was removed.
 */
bool QMMMModel::remove(const QMMMParams *term)
{
   bool ret = false;
   vector<QMMMParams*>::iterator it = 
      std::find(p_terms.begin(), p_terms.end(), term);
   if (it != p_terms.end()) {
      delete (*it);
      p_terms.erase(it);
      ret = true;
   }
   return ret;
}


/**
 * Remove any terms corresponding to parameter set of type "type".
 */
void QMMMModel::purgeParams(string params)
{
  vector<QMMMParams*> tmp;
  tmp = p_terms;
  p_terms.clear();
  int size = tmp.size();
  int i;
  for (i=0; i<size; i++) {
    string type = tmp[i]->getQMMMType();
    if (type != params) {
      p_terms.push_back(tmp[i]);
    } else {
      delete tmp[i];
    }
  }
}


/**
 * Removes potentials that contain atoms not in the fragment.
 * @return number of potentials removed.
 */
int QMMMModel::cleanUnmatchedTerms()
{
  int ret = 0;
#if 0
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
      QMMMParams *mpot = p_mpotential[idx];
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
        //QMMMModel::remove(p_mpotential[idx]);
        remove(p_mpotential[idx]);
        ret++;
      }
    }
  }

#endif
  return ret;
}

/**
 *  @return the total number of terms in model
 */
int QMMMModel::numQMMMTerms() const
{
  return  p_terms.size();
}

/**
 *  @param idx: the index of desired QMMM term
 *  @return the total number of terms in model
 */
QMMMParams* QMMMModel::getQMMMTerm(int idx) const
{
  return  p_terms[idx];
}
