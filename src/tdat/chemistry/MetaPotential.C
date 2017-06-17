/**
 * @file
 *
 *
 */
#include <set>
using std::set;
using std::less;

#include "tdat/MetaPotential.H"
#include "tdat/Measures.H"



/**
 *  Defines a meta-dynamics potential.
 *
 *  @param natoms: number of atoms involved in meta-dynamics potential
 *                 definition
 *  @param ids: list of atom IDs used in meta-dynamics potential
 *                 definition
 *  @param npar: number of parameters needed for meta-dynamics potential
 *  @param vals: values of parameters needed for meta-dynamics potential
 */
MetaPotential::MetaPotential(int natoms, vector<int> *ids, int npar,
    vector<double> *vals)
{
   p_natoms = natoms;
   p_nparams = npar;
   int i;
   for (i=0; i<p_natoms; i++) {
     p_IDs.push_back((*ids)[i]);
   }
   for (i=0; i<p_nparams; i++) {
     p_params.push_back((*vals)[i]);
   }
}


/**
 *  Copy Constructor
 */
MetaPotential::MetaPotential(const MetaPotential& rhs)
{
   *this = rhs;
   p_natoms = rhs.p_natoms;
   p_nparams = rhs.p_nparams;
   int i;
   for(i=0; i<p_natoms; i++) {
     p_IDs.push_back((rhs.p_IDs)[i]);
   }
   for(i=0; i<p_nparams; i++) {
     p_params.push_back((rhs.p_params)[i]);
   }
}


/**
 *  Destructor
 */
MetaPotential::~MetaPotential()
{
  p_IDs.clear();
  p_params.clear();
}


/**
 * Equality operator - true if the same atoms are used.
 * Order of atoms is not important.
 */
bool MetaPotential::operator==(const MetaPotential& rhs)
{
  bool ret = false;
  if (p_natoms == rhs.p_natoms) {

    // build sorted sets for easy comparison that handles arbitrary order
    // Maybe OBE if we decide to order these
    set<int, less<int> > c1;
    set<int, less<int> > c2;
    for (int idx=0; idx<p_natoms; idx++) {
      c1.insert(p_IDs[idx]);
      c2.insert(rhs.p_IDs[idx]);
    }
    ret = c1 == c2;
  }
  return ret;
}


/**
 * Assignment operator.
 */
MetaPotential& MetaPotential::operator=(const MetaPotential& rhs)
{
  if (this != &rhs) {
    p_natoms = rhs.p_natoms;
    p_nparams = rhs.p_nparams;
    int i;
    for(i=0; i<p_natoms; i++) {
      p_IDs[i] = (rhs.p_IDs)[i];
    }
    for(i=0; i<p_nparams; i++) {
      p_params[i] = (rhs.p_params)[i];
    }
  }
  return *this;
}


/**
 *  Return a list of atom IDs for atoms involved in meta-dynamics potential
 *  
 *  @return non-null vector; caller must delete vector
 */
vector<int> *MetaPotential::getAtoms() const
{
  vector<int> *ret = new vector<int>;
  int i;
  for(i=0; i<p_natoms; i++) {
    ret->push_back(p_IDs[i]);
  }
  return ret;
}


/**
 *  Return meta-dynamics potential parameter values
 *  
 *  @return non-null vector; caller must delete vector
 */
vector<double> *MetaPotential::getParams()
{
  vector<double> *ret = new vector<double>;
  int i;
  for(i=0; i<p_nparams; i++) {
    ret->push_back(p_params[i]);
  }
  return ret;
}



/**
 *  Set meta-dynamics potential parameter value.
 */
void MetaPotential::setPotentialValue(const double& value, int idx)
{
  if (idx < 0 || idx >= p_nparams) {
    // TODO: Some kind of error action
  }
  p_params[idx] = value;
}


/**
 *  Return number of atoms in meta-potential
 *  
 *  @return: number of atoms used in defining meta-dynamics potential
 */
int MetaPotential::getNumAtoms() const
{
   return p_natoms;
}


/**
 *  Return number of parameters in meta-potential
 *  
 *  @return: number of parameters used in defining meta-dynamics potential
 */
int MetaPotential::getNumParams() const
{
   return p_nparams;
}
