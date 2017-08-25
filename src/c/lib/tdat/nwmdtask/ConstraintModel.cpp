/**
 * @file
 *
 *
 */
#include "util/NotImplementedException.H"
#include "util/StringTokenizer.H"
#include "util/STLUtil.H"

#include "tdat/ConstraintModel.H"
#include "tdat/Fragment.H"

//Default values
const bool ConstraintModel::USESHAKE = true;
const int ConstraintModel::SHAKEOPT = 0;
const int ConstraintModel::MAXSLVITER = 100;
const double ConstraintModel::SLVTOL = 0.001;
const int ConstraintModel::MAXSLUITER = 100;
const int ConstraintModel::FIXOPTION = 0;
const int ConstraintModel::SELECTOPTION = 0;
const double ConstraintModel::SLUTOL = 0.001;
const double ConstraintModel::RADIUS = 1.0;
const bool ConstraintModel::PERMANENT = false;
const bool ConstraintModel::HEAVYONLY = false;

/**
 * Constructor
 */
ConstraintModel::ConstraintModel()
{
  p_url = "";
  p_useSHAKE = USESHAKE;
  p_SHAKEOpt = SHAKEOPT;
  p_maxSlvIter = MAXSLVITER;
  p_slvTol = SLVTOL;
  p_maxSluIter = MAXSLUITER;
  p_fixOpt = FIXOPTION;
  p_selectOpt = SELECTOPTION;
  p_sluTol = SLUTOL;
  p_radius = RADIUS;
  p_permanent = PERMANENT;
  p_heavyOnly = HEAVYONLY;
}

/**
 * Destructor
 */
ConstraintModel::~ConstraintModel()
{
}

/**
 * Modifiers
 */
void ConstraintModel::setURL(string url)
{
  p_url = url;
}

void ConstraintModel::setUseSHAKE(bool flag)
{
  p_useSHAKE = flag;
}

void ConstraintModel::setSHAKEOpt(int opt)
{
  p_SHAKEOpt = opt;
}

void ConstraintModel::setMaxSolventIter(int iter)
{
  p_maxSlvIter = iter;
}

void ConstraintModel::setSolventTol(double tol)
{
  p_slvTol = tol;
}

void ConstraintModel::setMaxSoluteIter(int iter)
{
  p_maxSluIter = iter;
}

void ConstraintModel::setSoluteTol(double tol)
{
  p_sluTol = tol;
}

void ConstraintModel::appendFixAtomCmd(string cmd)
{
  p_fixAtoms.push_back(cmd);
}

void ConstraintModel::clearFixAtomCmd()
{
  p_fixAtoms.clear();
}

void ConstraintModel::deleteFixAtomCmd(int id)
{
  vector<string>::iterator it;
  it = p_fixAtoms.begin();
  for (int i=0; i<id; i++) {
    it++;
  }
  if (it != p_fixAtoms.end()) {
    p_fixAtoms.erase(it);
  }
}

void ConstraintModel::setFixAtomRadius(double rad)
{
  p_radius = rad;
}

void ConstraintModel::setFixOption(int opt)
{
  p_fixOpt = opt;
}

void ConstraintModel::setSelectOption(int opt)
{
  p_selectOpt = opt;
}

void ConstraintModel::setPermanentOption(bool flag)
{
  p_permanent = flag;
}

void ConstraintModel::setHeavyAtomsOnly(bool flag)
{
  p_heavyOnly = flag;
}

/**
 * Accessors
 */
string ConstraintModel::getURL() const
{
  return p_url;
}

bool ConstraintModel::getUseSHAKE() const
{
  return p_useSHAKE;
}

int ConstraintModel::getSHAKEOpt() const
{
  return p_SHAKEOpt;
}

int ConstraintModel::getMaxSolventIter() const
{
  return p_maxSlvIter;
}

double ConstraintModel::getSolventTol() const
{
  return p_slvTol;
}

int ConstraintModel::getMaxSoluteIter() const
{
  return p_maxSluIter;
}

double ConstraintModel::getSoluteTol() const
{
  return p_sluTol;
}

vector<string> ConstraintModel::getFixAtomCmds() const
{
  return p_fixAtoms;
}

double ConstraintModel::getFixAtomRadius() const
{
  return p_radius;
}

int ConstraintModel::getFixOption() const
{
  return p_fixOpt;
}

int ConstraintModel::getSelectOption() const
{
  return p_selectOpt;
}

bool ConstraintModel::getPermanentOption() const
{
  return p_permanent;
}

bool ConstraintModel::getHeavyAtomsOnly() const
{
  return p_heavyOnly;
}

/**
 * Reset model to default values  
 */
void ConstraintModel::reset()
{
  p_fixAtoms.clear();
  setUseSHAKE(ConstraintModel::USESHAKE);
  setSHAKEOpt(ConstraintModel::SHAKEOPT);
  setMaxSolventIter(ConstraintModel::MAXSLVITER);
  setSolventTol(ConstraintModel::SLVTOL);
  setMaxSoluteIter(ConstraintModel::MAXSLUITER);
  setSoluteTol(ConstraintModel::SLUTOL);
  setFixOption(ConstraintModel::FIXOPTION);
  setSelectOption(ConstraintModel::SELECTOPTION);
  setFixAtomRadius(ConstraintModel::RADIUS);
  setPermanentOption(ConstraintModel::PERMANENT);
  setHeavyAtomsOnly(ConstraintModel::HEAVYONLY);
}
