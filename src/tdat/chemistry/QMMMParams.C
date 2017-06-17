/**
 * @file
 *
 *
 */
#include <set>
using std::set;
using std::less;

#include "util/STLUtil.H"
#include "tdat/QMMMParams.H"
#include "tdat/Measures.H"



/**
 *  Defines parameters for a QMMM potential term.
 *
 *  @param type: QM/MM term type, e.g. bond,angle,torsion,vdw (Van der Waals)
 *  @param natoms: number of atoms involved in QM/MM term
 *  @param atypes: list of atom types used in QM/MM term
 *  @param npar: number of parameters needed for QM/MM term
 *  @param vals: values of parameters needed for QM/MM term
 */
QMMMParams::QMMMParams(string type, int natoms, vector<string> *atypes, int npar,
    vector<double> *vals)
{
   STLUtil::stripLeadingAndTrailingWhiteSpace(type);
   p_type = type;
   p_natoms = natoms;
   p_nparams = npar;
   int i;
   for (i=0; i<p_natoms; i++) {
     p_atypes.push_back((*atypes)[i]);
   }
   for (i=0; i<p_nparams; i++) {
     p_params.push_back((*vals)[i]);
   }
   p_qmmm = (QMMMParams*)0;
}

/**
 *  Associates a set of QMMM parameters with a particular bond or angle.
 *
 *  @param type: QM/MM term type, e.g. bond,angle,torsion,vdw (Van der Waals)
 *  @param natoms: number of atoms involved in QM/MM term
 *  @param atypes: list of atom indices used in QM/MM term
 *  @param qmmm: pointer to another QM/MM terms containing parameters for this
 *               term
 */
QMMMParams::QMMMParams(string type, int natoms, vector<int> *indices,
    QMMMParams *qmmm)
{
   STLUtil::stripLeadingAndTrailingWhiteSpace(type);
   p_type = type;
   p_natoms = natoms;
   p_nparams = 0;
   int i;
   for (i=0; i<p_natoms; i++) {
     p_indices.push_back((*indices)[i]);
   }
   p_qmmm = qmmm;
}

/**
 *  Defines QMMM parameters for a particular bond or angle.
 *
 *  @param type: QM/MM term type, e.g. bond,angle,torsion,vdw (Van der Waals)
 *  @param natoms: number of atoms involved in QM/MM term
 *  @param indices: indices used in QM/MM term
 *  @param npar: number of parameters needed for QM/MM term
 *  @param vals: values of parameters needed for QM/MM term
 */
QMMMParams::QMMMParams(string type, int natoms, vector<int> *indices, int npar,
    vector<double> *vals)
{
   STLUtil::stripLeadingAndTrailingWhiteSpace(type);
   p_type = type;
   p_natoms = natoms;
   p_nparams = npar;
   int i;
   for (i=0; i<p_natoms; i++) {
     p_indices.push_back((*indices)[i]);
   }
   for (i=0; i<p_nparams; i++) {
     p_params.push_back((*vals)[i]);
   }
   p_qmmm = (QMMMParams*)0;
}


/**
 *  Copy Constructor
 */
QMMMParams::QMMMParams(const QMMMParams& rhs)
{
   *this = rhs;
   p_type = rhs.p_type;
   p_natoms = rhs.p_natoms;
   p_nparams = rhs.p_nparams;
   int i;
   if (p_atypes.size() > 0) {
     for(i=0; i<p_natoms; i++) {
       p_atypes.push_back((rhs.p_atypes)[i]);
     }
   } else if (p_indices.size() > 0) {
     for(i=0; i<p_natoms; i++) {
       p_indices.push_back((rhs.p_indices)[i]);
     }
   }
   for(i=0; i<p_nparams; i++) {
     p_params.push_back((rhs.p_params)[i]);
   }
   p_qmmm = rhs.p_qmmm;
}


/**
 *  Destructor
 */
QMMMParams::~QMMMParams()
{
  p_atypes.clear();
  p_params.clear();
  p_indices.clear();
}


/**
 * Equality operator - true if the same atoms are used.
 * Order of atoms is not important.
 */
bool QMMMParams::operator==(const QMMMParams& rhs)
{
  bool ret = true;
  if (p_type != rhs.p_type) ret = false;
  if (ret && p_natoms == rhs.p_natoms) {

    // build sorted sets for easy comparison that handles arbitrary order
    // Maybe OBE if we decide to order these
    if (p_atypes.size() > 0) {
      set<string, less<string> > c1;
      set<string, less<string> > c2;
      for (int idx=0; idx<p_natoms; idx++) {
        c1.insert(p_atypes[idx]);
        c2.insert(rhs.p_atypes[idx]);
      }
      ret = c1 == c2;
    } else if (p_indices.size() > 0) {
      set<int, less<int> > c1;
      set<int, less<int> > c2;
      for (int idx=0; idx<p_natoms; idx++) {
        c1.insert(p_indices[idx]);
        c2.insert(rhs.p_indices[idx]);
      }
      ret = c1 == c2 && p_qmmm == rhs.p_qmmm;
    }
  } else {
    ret = false;
  }
  return ret;
}


/**
 * Assignment operator.
 */
QMMMParams& QMMMParams::operator=(const QMMMParams& rhs)
{
  if (this != &rhs) {
    p_type = rhs.p_type;
    p_natoms = rhs.p_natoms;
    p_nparams = rhs.p_nparams;
    int i;
    for(i=0; i<p_natoms; i++) {
      p_atypes[i] = (rhs.p_atypes)[i];
    }
    for(i=0; i<p_nparams; i++) {
      p_params[i] = (rhs.p_params)[i];
    }
    p_qmmm = rhs.p_qmmm;
  }
  return *this;
}

/**
 * Check to see if any parameters are non-zero
 */
bool QMMMParams::isZeroParams()
{
  int i;
  bool ret = true;
  if (p_qmmm == (QMMMParams*)0) {
    for (i=0; i<p_nparams; i++) {
      if (p_params[i] != 0.0) ret = false;
    }
  } else {
    ret = false;
  }
  return ret;
}

/**
 *  Return QM/MM term type
 *  
 *  @return: QM/MM term type
 */
string QMMMParams::getQMMMType() const
{
   return p_type;
}


/**
 *  Return a list of potentials for atoms involved in QM/MM term
 *  
 *  @return non-null vector; caller must delete vector
 */
vector<string> *QMMMParams::getQMMMPotentials() const
{
  vector<string> *ret = new vector<string>;
  int i;
  for(i=0; i<p_natoms; i++) {
    ret->push_back(p_atypes[i]);
  }
  return ret;
}


/**
 *  Return a list of indices for atoms involved in QM/MM term
 *  
 *  @return non-null vector; caller must delete vector
 */
vector<int> *QMMMParams::getAtomIndices() const
{
  vector<int> *ret = new vector<int>;
  int i;
  for(i=0; i<p_natoms; i++) {
    ret->push_back(p_indices[i]);
  }
  return ret;
}


/**
 *  Return QM/MM term parameter values
 *  
 *  @return non-null vector; caller must delete vector
 */
vector<double> *QMMMParams::getParams()
{
  vector<double> *ret = new vector<double>;
  int i;
  for(i=0; i<p_nparams; i++) {
    ret->push_back(p_params[i]);
  }
  return ret;
}

/**
 *  Return QM/MM pointer to parameter values
 *  
 *  @return QMMMParams pointers
 */
QMMMParams *QMMMParams::getQMMMParams()
{
  return p_qmmm;
}


/**
 *  Set a QM/MM term parameter value
 *  @param val: new value for parameter
 *  @param idx: index for parameter
 *  @return non-null vector; caller must delete vector
 */
void QMMMParams::setParamValue(double val, int idx)
{
  p_params[idx] = val;
}


/**
 *  Return number of atoms in QM/MM term
 *  
 *  @return: number of atoms used in defining QM/MM term
 */
int QMMMParams::getNumAtoms() const
{
   return p_natoms;
}


/**
 *  Return number of parameters in QM/MM term
 *  
 *  @return: number of parameters used in defining QM/MM
 */
int QMMMParams::getNumParams() const
{
   return p_nparams;
}
