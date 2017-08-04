/**
 * @file
 *
 *
 */
#include <iostream>
  using std::cout;
  using std::endl;
#include <fstream>
  using std::ifstream;

#include "util/NotImplementedException.H"

#include "tdat/NWDirdyModel.H"

// Default values
const bool NWDirdyModel::AUTOSYM = true;
const double NWDirdyModel::SYMTOL = 0.001; // the NWChem default is 0.01, but
                                           // Rob didn't like that value
const double NWDirdyModel::FRAGSEP = 100.0;
const bool NWDirdyModel::USESP = false;
const string NWDirdyModel::CATEGORY = "DFT";
const string NWDirdyModel::THEORY = "RDFT";
const string NWDirdyModel::SPCATEGORY = "None";
const string NWDirdyModel::SPTHEORY = "None";
const bool NWDirdyModel::EXPCOEFF = false;
const bool NWDirdyModel::SPEXPCOEFF = false;
const double NWDirdyModel::MEPINC = 0.01;
const double NWDirdyModel::SAVEINC = 0.1;
const double NWDirdyModel::HESSINC = 0.1;
const double NWDirdyModel::RMASS = 1.0;
const bool NWDirdyModel::TSTONLY = false;
const double NWDirdyModel::SLOW = -1.0;
const double NWDirdyModel::SHIGH = 1.0;
const bool NWDirdyModel::SWITCH = false;
const int NWDirdyModel::INTOPT = 0;
const bool NWDirdyModel::PRINT = false;
const bool NWDirdyModel::AUTOZ = false;
const bool NWDirdyModel::MEMORYFLAG = false;
const int NWDirdyModel::MEMORY = 2;
const bool NWDirdyModel::DISKFLAG = false;
const int NWDirdyModel::DISK = 64;
const int NWDirdyModel::NUMATOMS = 0;

/**
 * Constructor
 */
NWDirdyModel::NWDirdyModel() : ITaskModel()
{
   setTitle("NW Direct Dynamics");
   setUrl("");
   p_numAtoms = 0;
   p_transition = (Fragment*)0;
   p_react1 = (Fragment*)0;
   p_react2 = (Fragment*)0;
   p_prod1 = (Fragment*)0;
   p_prod2 = (Fragment*)0;
   p_products = true;
   reset();
}


/**
 * Constructor
 */
NWDirdyModel::NWDirdyModel(const string& url, const string& name) : ITaskModel()
{
   setTitle("NW Direct Dynamics");
   setUrl(url);
   p_numAtoms = 0;
   p_transition = (Fragment*)0;
   p_react1 = (Fragment*)0;
   p_react2 = (Fragment*)0;
   p_prod1 = (Fragment*)0;
   p_prod2 = (Fragment*)0;
   p_products = true;
   reset();
}

void NWDirdyModel::reset()
{
   p_autosym = AUTOSYM;
   p_symTol = SYMTOL;
   p_fragSep = FRAGSEP;
   p_useSP = USESP;
   p_category = CATEGORY;
   p_theory = THEORY;
   p_SPCategory = SPCATEGORY;
   p_SPTheory = SPTHEORY;
   p_expCoeff = EXPCOEFF;
   p_SPExpCoeff = SPEXPCOEFF;
   p_MEPInc = MEPINC;
   p_saveInc = SAVEINC;
   p_hessInc = HESSINC;
   p_rmass = RMASS;
   p_tstOnly = TSTONLY;
   p_sLow = SLOW;
   p_sHigh = SHIGH;
   p_switch = SWITCH;
   p_integrationOpt = INTOPT;
   p_print = PRINT;
   p_products = true;
   p_autoz = AUTOZ;
   p_memoryFlag = MEMORYFLAG;
   p_memory = MEMORY;
   p_diskFlag = DISKFLAG;
   p_disk = DISK;
   p_linearTS = false;
   p_linearProd1 = false;
   p_linearProd2 = false;
   p_linearReact1 = false;
   p_linearReact2 = false;
   p_numAtomsTS = NUMATOMS;
   p_numAtomsProd1 = NUMATOMS;
   p_numAtomsProd2 = NUMATOMS;
   p_numAtomsReact1 = NUMATOMS;
   p_numAtomsReact2 = NUMATOMS;
}

/**
 * Destructor
 */
NWDirdyModel::~NWDirdyModel()
{
  if (p_transition)
    delete p_transition;
  if (p_react1)
    delete p_react1;
  if (p_react2)
    delete p_react2;
  if (p_prod1)
    delete p_prod1;
  if (p_prod2)
    delete p_prod2;
}

void NWDirdyModel::generateInputFile() throw(InvalidException)
{
   throw NotImplementedException("generateInputFile not implemented",WHERE);
}


void NWDirdyModel::run() throw(InvalidException)
{
   throw NotImplementedException("run not implemented",WHERE);
}

/**
 *  Accessors
 */
bool NWDirdyModel::getProductsFlag() const
{
  return p_products;
}

int NWDirdyModel::getNumSelectedAtoms() const
{
  return p_numAtoms;
}

vector<int> NWDirdyModel::getSelectionList() const
{
  return p_selectList;
}

Fragment* NWDirdyModel::getTransitionState() const
{
  return p_transition;
}

Fragment* NWDirdyModel::getReactant1() const
{
  return p_react1;
}

Fragment* NWDirdyModel::getReactant2() const
{
  return p_react2;
}

Fragment* NWDirdyModel::getProduct1() const
{
  return p_prod1;
}

Fragment* NWDirdyModel::getProduct2() const
{
  return p_prod2;
}

bool NWDirdyModel::getAutoSym() const
{
  return p_autosym;
}

double NWDirdyModel::getSymmetryTol() const
{
  return p_symTol;
}

double NWDirdyModel::getFragmentSep() const
{
  return p_fragSep;
}

bool NWDirdyModel::getUseSP() const
{
  return p_useSP;
}

string NWDirdyModel::getCategory() const
{
  return p_category;
}

string NWDirdyModel::getTheory() const
{
  return p_theory;
}

string NWDirdyModel::getSPCategory() const
{
  return p_SPCategory;
}

string NWDirdyModel::getSPTheory() const
{
  return p_SPTheory;
}

bool NWDirdyModel::getExpCoeff() const
{
  return p_expCoeff;
}

bool NWDirdyModel::getSPExpCoeff() const
{
  return p_SPExpCoeff;
}

double NWDirdyModel::getMEPIncrement() const
{
  return p_MEPInc;
}

double NWDirdyModel::getSaveIncrement() const
{
  return p_saveInc;
}

double NWDirdyModel::getHessianIncrement() const
{
  return p_hessInc;
}

double NWDirdyModel::getReducedMass() const
{
  return p_rmass;
}

bool NWDirdyModel::getTSTOnly() const
{
  return p_tstOnly;
}

double NWDirdyModel::getLowerSLimit() const
{
  return p_sLow;
}

double NWDirdyModel::getHigherSLimit() const
{
  return p_sHigh;
}

bool NWDirdyModel::getSwitchSign() const
{
  return p_switch;
}

int NWDirdyModel::getIntegrationOpt() const
{
  return p_integrationOpt;
}

bool NWDirdyModel::getPrintFlag() const
{
  return p_print;
}

bool NWDirdyModel::getAutoZ() const
{
  return p_autoz;
}

bool NWDirdyModel::getMemoryFlag() const
{
  return p_memoryFlag;
}

int NWDirdyModel::getMemory() const
{
  return p_memory;
}

bool NWDirdyModel::getDiskFlag() const
{
  return p_diskFlag;
}

int NWDirdyModel::getDisk() const
{
  return p_disk;
}

bool NWDirdyModel::getLinearTS() const
{
  return p_linearTS;
}

bool NWDirdyModel::getLinearProd1() const
{
  return p_linearProd1;
}

bool NWDirdyModel::getLinearProd2() const
{
  return p_linearProd2;
}

bool NWDirdyModel::getLinearReact1() const
{
  return p_linearReact1;
}

bool NWDirdyModel::getLinearReact2() const
{
  return p_linearReact2;
}

int NWDirdyModel::getNumAtomsTS() const
{
  return p_numAtomsTS;
}

int NWDirdyModel::getNumAtomsProd1() const
{
  return p_numAtomsProd1;
}

int NWDirdyModel::getNumAtomsProd2() const
{
  return p_numAtomsProd2;
}

int NWDirdyModel::getNumAtomsReact1() const
{
  return p_numAtomsReact1;
}

int NWDirdyModel::getNumAtomsReact2() const
{
  return p_numAtomsReact2;
}

/**
 *  Modifiers
 */
void NWDirdyModel::setProductsFlag(bool flag)
{
  p_products = flag;
}

void NWDirdyModel::setNumSelectedAtoms(int natoms)
{
  p_numAtoms = natoms;
}

void NWDirdyModel::setSelectionList(vector<int> list)
{
  p_selectList = list;
}

void NWDirdyModel::setTransitionState(Fragment *frag)
{
  p_transition = frag;
}

void NWDirdyModel::setReactant1(Fragment *frag)
{
  p_react1 = frag;
}

void NWDirdyModel::setReactant2(Fragment *frag)
{
  p_react2 = frag;
}

void NWDirdyModel::setProduct1(Fragment *frag)
{
  p_prod1 = frag;
}

void NWDirdyModel::setProduct2(Fragment *frag)
{
  p_prod2 = frag;
}

void NWDirdyModel::setAutoSym(bool flag)
{
  p_autosym = flag;
}

void NWDirdyModel::setSymmetryTol(double tol)
{
  p_symTol = tol;
}

void NWDirdyModel::setFragmentSep(double sep)
{
  p_fragSep = sep;
}

void NWDirdyModel::setUseSP(bool flag)
{
  p_useSP = flag;
}

void NWDirdyModel::setCategory(string cat)
{
  p_category = cat;
}

void NWDirdyModel::setTheory(string thry)
{
  p_theory = thry;
}

void NWDirdyModel::setSPCategory(string cat)
{
  p_SPCategory = cat;
}

void NWDirdyModel::setSPTheory(string thry)
{
  p_SPTheory = thry;
}

void NWDirdyModel::setExpCoeff(bool flag)
{
  p_expCoeff = flag;
}

void NWDirdyModel::setSPExpCoeff(bool flag)
{
  p_SPExpCoeff = flag;
}

void NWDirdyModel::setMEPIncrement(double inc)
{
  p_MEPInc = inc;
}

void NWDirdyModel::setSaveIncrement(double inc)
{
  p_saveInc = inc;
}

void NWDirdyModel::setHessianIncrement(double inc)
{
  p_hessInc = inc;
}

void NWDirdyModel::setReducedMass(double mass)
{
  p_rmass = mass;
}

void NWDirdyModel::setTSTOnly(bool flag)
{
  p_tstOnly = flag;
}

void NWDirdyModel::setLowerSLimit(double lim)
{
  p_sLow = lim;
}

void NWDirdyModel::setHigherSLimit(double lim)
{
  p_sHigh = lim;
}

void NWDirdyModel::setSwitchSign(bool flag)
{
  p_switch = flag;
}

void NWDirdyModel::setIntegrationOpt(int opt)
{
  p_integrationOpt = opt;
}

void NWDirdyModel::setPrintFlag(bool flag)
{
  p_print = flag;
}

void NWDirdyModel::setAutoZ(bool flag)
{
  p_autoz = flag;
}

void NWDirdyModel::setMemoryFlag(bool flag)
{
  p_memoryFlag = flag;
}

void NWDirdyModel::setMemory(int mem)
{
  p_memory = mem;
}

void NWDirdyModel::setDiskFlag(bool flag)
{
  p_diskFlag = flag;
}

void NWDirdyModel::setDisk(int disk)
{
  p_disk = disk;
}

void NWDirdyModel::setLinearTS(bool flag)
{
  p_linearTS = flag;
}

void NWDirdyModel::setLinearProd1(bool flag)
{
  p_linearProd1 = flag;
}

void NWDirdyModel::setLinearProd2(bool flag)
{
  p_linearProd2 = flag;
}

void NWDirdyModel::setLinearReact1(bool flag)
{
  p_linearReact1 = flag;
}

void NWDirdyModel::setLinearReact2(bool flag)
{
  p_linearReact2 = flag;
}

void NWDirdyModel::setNumAtomsTS(int num)
{
  p_numAtomsTS = num;
}

void NWDirdyModel::setNumAtomsProd1(int num)
{
  p_numAtomsProd1 = num;
}

void NWDirdyModel::setNumAtomsProd2(int num)
{
  p_numAtomsProd2 = num;
}

void NWDirdyModel::setNumAtomsReact1(int num)
{
  p_numAtomsReact1 = num;
}

void NWDirdyModel::setNumAtomsReact2(int num)
{
  p_numAtomsReact2 = num;
}
