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
#include "util/StringTokenizer.H"
#include "util/STLUtil.H"
#include "util/StringConverter.H"

#include "tdat/PrepareModel.H"
#include "tdat/Fragment.H"

/* Default values */
const bool PrepareModel::FIRSTTASK = true;
const bool PrepareModel::NEWTOPOLOGY = true;
const int PrepareModel::NUMMODELS = 1;
const PrepareModel::DefaultHistidine
      PrepareModel::DEFAULTHIS = PrepareModel::HIE;
const bool PrepareModel::SOLVATE = false;
const bool PrepareModel::SETSOLVENT = false;
const int PrepareModel::GEOMOPT = 0;
const double PrepareModel::TOUCH = 0.23;
const double PrepareModel::EXPANDVOL = 0.1;
const double PrepareModel::SOLVENTTOL = 0.0;
const int PrepareModel::SOLVENTOPT = 0;
const string PrepareModel::SOLVENTNAME = "HOH";
const string PrepareModel::SOLVENTMODEL = "spce";
const int PrepareModel::INITIALATOM = 1;
const int PrepareModel::FINALATOM = 1;
const int PrepareModel::TRNSOPTION = 1;
const double PrepareModel::TRNSX = 0.1;
const double PrepareModel::TRNSY = 0.1;
const double PrepareModel::TRNSZ = 0.1;
const int PrepareModel::ORNTOPTION = 1;
const double PrepareModel::ANGLE = 0.0;
const double PrepareModel::ORIENTX = 0.1;
const double PrepareModel::ORIENTY = 0.1;
const double PrepareModel::ORIENTZ = 0.1;
const int PrepareModel::RMVOPT = 0;
const bool PrepareModel::XRANGE = false;
const bool PrepareModel::YRANGE = false;
const bool PrepareModel::ZRANGE = false;
const int PrepareModel::REPOPTS = 0;
const int PrepareModel::INDMODCELL = 1;
const double PrepareModel::MCLLMINSEP = 0.23;
const bool PrepareModel::RANDXYZ = false;
const bool PrepareModel::INVCOPY = false;
const bool PrepareModel::USEZBOX = false;
const bool PrepareModel::SETIONS = false;
const string PrepareModel::IONOPTION = "Cl";
const double PrepareModel::CISCALE = 1.0;
const bool PrepareModel::ADDLNK = false;
const int PrepareModel::CINUM = 1;
const double PrepareModel::MINSEPARATION = 1.2;
const int PrepareModel::CIGRID = 24;
const double PrepareModel::CIMINDIS = 0.2;
const int PrepareModel::BCOPT = 1;
const int PrepareModel::HEAPMEM = 5;
const int PrepareModel::STACKMEM = 60;
const int PrepareModel::GLOBALMEM = 60;
const int PrepareModel::PRINTLEVEL = 1;
const bool PrepareModel::USEMEM = false;
const bool PrepareModel::USEFRAC = false;
const int PrepareModel::FRAC1 = 0;
const int PrepareModel::FRAC2 = 0;
const int PrepareModel::FRAC3 = 0;
const int PrepareModel::FRAC4 = 0;
const int PrepareModel::FRAC5 = 0;
const int PrepareModel::FRAC6 = 0;
const int PrepareModel::FRAC7 = 0;
const int PrepareModel::FRAC8 = 0;
const int PrepareModel::FRAC9 = 0;
const int PrepareModel::FRAC10 = 0;

/**
 * Constructor
 */
PrepareModel::PrepareModel() : ITaskModel()
{
   setTitle("Prepare");
   setUrl("");
   reset();
}


/**
 * Constructor
 */
PrepareModel::PrepareModel(const string& url, const string& name) : ITaskModel()
{
   setTitle(name);
   setUrl(url);
   reset();
}

/**
 * Destructor
 */
PrepareModel::~PrepareModel()
{
}

void PrepareModel::reset()
{
   p_isFirst = FIRSTTASK;
   p_numModels = NUMMODELS;
   p_defHistidine = DEFAULTHIS;
   p_setSolvent = SETSOLVENT;
   p_solvate = SOLVATE;
   p_solventOpt = SOLVENTOPT;
   p_solventName = SOLVENTNAME;
   p_solventModel = SOLVENTMODEL;
   p_BCOpt = BCOPT;
   p_geomOpt = GEOMOPT;
   p_minSluSluDis = MINSEPARATION;
   p_touch = TOUCH;
   p_expandVol = EXPANDVOL;
   p_envelope = SOLVENTTOL;
   p_initAtom = INITIALATOM;
   p_finalAtom = FINALATOM;
   p_translateOption = TRNSOPTION;
   p_trnsX = TRNSX;
   p_trnsY = TRNSY;
   p_trnsZ = TRNSZ;
   p_orientOption = ORNTOPTION;
   p_angle = ANGLE;
   p_orntX = ORIENTX;
   p_orntY = ORIENTY;
   p_orntZ = ORIENTZ;
   p_removeOpt = RMVOPT;
   p_xRange = XRANGE;
   p_yRange = YRANGE;
   p_zRange = ZRANGE;
   p_CIScale = CISCALE;
   p_addLink = ADDLNK;
   p_setIons = SETIONS;
   p_ionOption = IONOPTION;
   p_CIGrid = CIGRID;
   p_CIMinDis = CIMINDIS;
   p_CINumber = CINUM;
   p_useMemory = USEMEM;
   p_heap = HEAPMEM;
   p_stack = STACKMEM;
   p_global = GLOBALMEM;
   p_printLevel = PRINTLEVEL;
   p_useFrac = USEFRAC;
   p_frac1 = FRAC1;
   p_frac2 = FRAC2;
   p_frac3 = FRAC3;
   p_frac4 = FRAC4;
   p_frac5 = FRAC5;
   p_frac6 = FRAC6;
   p_frac7 = FRAC7;
   p_frac8 = FRAC8;
   p_frac9 = FRAC9;
   p_frac10 = FRAC10;
   p_repeatOption = REPOPTS;
   p_xRepeat = INDMODCELL;
   p_yRepeat = INDMODCELL;
   p_zRepeat = INDMODCELL;
   p_minRSep = MCLLMINSEP;
   p_ranX = RANDXYZ;
   p_ranY = RANDXYZ;
   p_ranZ = RANDXYZ;
   p_invertZCopy = INVCOPY;
   p_useZBoxDim = USEZBOX;
   p_skipX = INDMODCELL;
   p_skipY = INDMODCELL;
   p_skipZ = INDMODCELL;

   // clear lists
   p_cIons.clear();
   p_links.clear();
   p_coordCmd.clear();
   p_pmfCmd.clear();

   p_changedTopology = NEWTOPOLOGY;
}

/**
 * Modifiers
 */
void PrepareModel::setIsFirstTask(bool flag)
{
  p_isFirst = flag;
}

void PrepareModel::setPDBSource(string source)
{
  p_PDBsource = source;
}

void PrepareModel::setNumModels(int numModels)
{
  p_numModels = numModels;
}

void PrepareModel::setAltLocVec(vector<string> altLocVec)
{
  p_altLocVec = altLocVec;
}

void PrepareModel::setChainVec(vector<string> chainVec)
{
  p_chainVec = chainVec;
}

void PrepareModel::setDefaultHistidine(PrepareModel::DefaultHistidine def)
{
  p_defHistidine = def;
}

void PrepareModel::setAddLinks(bool flag)
{
  p_addLink = flag;
}

void PrepareModel::setCounterIons(vector<string> ions)
{
  p_cIons = ions;
}

void PrepareModel::setCounterIonScale(double scale)
{
  p_CIScale = scale;
}

void PrepareModel::setSolvateFlag(bool flag)
{
  p_solvate = flag;
}

void PrepareModel::setSetSolvent(bool flag)
{
  p_setSolvent = flag;
}

void PrepareModel::setSolventOption(int opt)
{
  p_solventOpt = opt;
}

void PrepareModel::setSolventName(string str)
{
  p_solventName = str;
}

void PrepareModel::setSolventModel(string str)
{
  p_solventModel = str;
}

void PrepareModel::setBoundaryCondition(int opt)
{
  p_BCOpt = opt;
}

void PrepareModel::setGeometryOption(int opt)
{
  p_geomOpt = opt;
}

void PrepareModel::setMinSluSluDist(double value)
{
  p_minSluSluDis = value;
}

void PrepareModel::setMinSluSlvDist(double dist)
{
  p_touch = dist;
}

void PrepareModel::setExpandVolPar(double param)
{
  p_expandVol = param;
}

void PrepareModel::setEnvelopePar(double param)
{
  p_envelope = param;
}

void PrepareModel::appendCmdList(string cmd)
{
  p_coordCmd.push_back(cmd);
}

void PrepareModel::clearCmdList()
{
  p_coordCmd.clear();
}

void PrepareModel::deleteCmdList(int index)
{
  vector<string>::iterator it;
  it=p_coordCmd.begin(); 
  for (int i=0; i<index; i++) {
    it++;
  }
  p_coordCmd.erase(it);
}

void PrepareModel::insertCmdList(int index, string cmd)
{
  vector<string>::iterator it;
  it=p_coordCmd.begin(); 
  for (int i=0; i<index; i++) {
    it++;
  }
  p_coordCmd.insert(it, cmd);
}

void PrepareModel::appendLinkList(string link)
{
  p_links.push_back(link);
}

void PrepareModel::clearLinkList()
{
  p_links.clear();
}

void PrepareModel::deleteLinkList(int index)
{
  vector<string>::iterator it;
  it=p_links.begin(); 
  for (int i=0; i<index; i++) {
    it++;
  }
  p_links.erase(it);
}

void PrepareModel::appendCounterIonList(string ion)
{
  p_cIons.push_back(ion);
}

void PrepareModel::clearCounterIonList()
{
  p_cIons.clear();
}

void PrepareModel::deleteCounterIonList(int index)
{
  vector<string>::iterator it;
  it=p_cIons.begin(); 
  for (int i=0; i<index; i++) {
    it++;
  }
  p_cIons.erase(it);
}

void PrepareModel::appendPmfCmdList(string cmd)
{
  p_pmfCmd.push_back(cmd);
}

void PrepareModel::deletePmfCmdList(int index)
{
  vector<string>::iterator it;
  it = p_pmfCmd.begin();
  for (int i = 0; i < index; i++) {
    it++;
  }
  p_pmfCmd.erase(it);
}

void PrepareModel::clearPmfCmdList()
{
  p_pmfCmd.clear();
}

void PrepareModel::insertPmfCmdList(int index, string cmd)
{
  vector<string>::iterator it;
  it = p_pmfCmd.begin(); 
  for (int i = 0; i < index; i++) {
    it++;
  }
  p_pmfCmd.insert(it, cmd);
}

int PrepareModel::addPmfSelection(string selection, int index)
{
  p_pmfSelCount[selection]++;

  // is selection already used?  if so, return it (even if index is specified)
  map<string,int>::iterator stringIter;
  stringIter = p_pmfSelStringToInt.find(selection);
  if (stringIter != p_pmfSelStringToInt.end()) {
    return stringIter->second;
  }

  // since this is a new selection, is there an index already in mind?
  // if so, skip the free-index finder
  if (index <= 0) {
    // find first available index
    index = 1;
    map<int,string>::iterator intIter = p_pmfSelIntToString.begin();
    for (; intIter != p_pmfSelIntToString.end(); intIter++) {
      if (index != intIter->first) {
        break;
      }
      index++;
    }
  }

  p_pmfSelStringToInt[selection] = index;
  p_pmfSelIntToString[index] = selection;

  return index;
}

void PrepareModel::removePmfSelection(string selection)
{
  p_pmfSelCount[selection]--;

  // did we remove the last 'reference'?  if so, remove the selection
  if (p_pmfSelCount[selection] <= 0) {
    int index = p_pmfSelStringToInt[selection];
    p_pmfSelStringToInt.erase(selection);
    p_pmfSelIntToString.erase(index);
  }
}

void PrepareModel::removePmfSelection(int selection)
{
  removePmfSelection(p_pmfSelIntToString[selection]);
}

void PrepareModel::clearPmfSelection()
{
  p_pmfSelCount.clear();
  p_pmfSelIntToString.clear();
  p_pmfSelStringToInt.clear();
}

void PrepareModel::setInitialAtom(int iatom)
{
  p_initAtom = iatom;
}

void PrepareModel::setFinalAtom(int iatom)
{
  p_finalAtom = iatom;
}

void PrepareModel::setTranslateOption(int opt)
{
  p_translateOption = opt;
}

void PrepareModel::setTranslateX(double value)
{
  p_trnsX = value;
}

void PrepareModel::setTranslateY(double value)
{
  p_trnsY = value;
}

void PrepareModel::setTranslateZ(double value)
{
  p_trnsZ = value;
}

void PrepareModel::setOrientOption(int opt)
{
  p_orientOption = opt;
}

void PrepareModel::setAngle(double value)
{
  p_angle = value;
}

void PrepareModel::setOrientX(double value)
{
  p_orntX = value;
}

void PrepareModel::setRemoveOption(int opt)
{
  p_removeOpt = opt;
}

void PrepareModel::setRemoveXRange(bool flag)
{
  p_xRange = flag;
}

void PrepareModel::setRemoveYRange(bool flag)
{
  p_yRange = flag;
}

void PrepareModel::setRemoveZRange(bool flag)
{
  p_zRange = flag;
}

void PrepareModel::setRepeatOption(int opt)
{
  p_repeatOption = opt;
}

void PrepareModel::setXRepeat(int freq)
{
  p_xRepeat = freq;
}

void PrepareModel::setYRepeat(int freq)
{
  p_yRepeat = freq;
}

void PrepareModel::setZRepeat(int freq)
{
  p_zRepeat = freq;
}

void PrepareModel::setMinRepeatSeparation(double value)
{
  p_minRSep = value;
}

void PrepareModel::setRandomX(bool flag)
{
  p_ranX = flag;
}

void PrepareModel::setRandomY(bool flag)
{
  p_ranY = flag;
}

void PrepareModel::setRandomZ(bool flag)
{
  p_ranZ = flag;
}

void PrepareModel::setInvertZCopy(bool flag)
{
  p_invertZCopy = flag;
}

void PrepareModel::setUseZBoxDim(bool flag)
{
  p_useZBoxDim = flag;
}

void PrepareModel::setSkipX(int freq)
{
  p_skipX = freq;
}

void PrepareModel::setSkipY(int freq)
{
  p_skipY = freq;
}

void PrepareModel::setSkipZ(int freq)
{
  p_skipZ = freq;
}

void PrepareModel::setOrientY(double value)
{
  p_orntY = value;
}

void PrepareModel::setOrientZ(double value)
{
  p_orntZ = value;
}

void PrepareModel::setSetIons(bool flag)
{
  p_setIons = flag;
}

void PrepareModel::setCounterIonOpt(string opt)
{
  p_ionOption = opt;
}

void PrepareModel::setCINumber(int num)
{
  p_CINumber = num;
}

void PrepareModel::setCIGridSize(int size)
{
  p_CIGrid = size;
}

void PrepareModel::setMinAtomCIDis(double dist)
{
  p_CIMinDis = dist;
}

void PrepareModel::setUseMemory(bool flag)
{
  p_useMemory = flag;
}

void PrepareModel::setHeapMemory(int mem)
{
  p_heap = mem;
}

void PrepareModel::setStackMemory(int mem)
{
  p_stack = mem;
}

void PrepareModel::setGlobalMemory(int mem)
{
  p_global = mem;
}

void PrepareModel::setPrintLevel(int level)
{
  p_printLevel = level;
}

void PrepareModel::setChangedTopology(bool flag)
{
  p_changedTopology = flag;
}

void PrepareModel::setUseFractions(bool flag)
{
  p_useFrac = flag;
}

void PrepareModel::setFraction1(int frac)
{
  p_frac1 = frac;
}

void PrepareModel::setFraction2(int frac)
{
  p_frac2 = frac;
}

void PrepareModel::setFraction3(int frac)
{
  p_frac3 = frac;
}

void PrepareModel::setFraction4(int frac)
{
  p_frac4 = frac;
}

void PrepareModel::setFraction5(int frac)
{
  p_frac5 = frac;
}

void PrepareModel::setFraction6(int frac)
{
  p_frac6 = frac;
}

void PrepareModel::setFraction7(int frac)
{
  p_frac7 = frac;
}

void PrepareModel::setFraction8(int frac)
{
  p_frac8 = frac;
}

void PrepareModel::setFraction9(int frac)
{
  p_frac9 = frac;
}

void PrepareModel::setFraction10(int frac)
{
  p_frac10 = frac;
}

void PrepareModel::setDefaultDims(double *dims)
{
  for (int i=0; i<6; i++) {
    p_defaultDims[i] = dims[i];
  }
}

/**
 * Accessors
 */
bool PrepareModel::getIsFirstTask() const
{
  return p_isFirst;
}

string PrepareModel::getPDBSource() const
{
  return p_PDBsource;
}

int PrepareModel::getNumModels() const
{
  return p_numModels;
}

vector<string> PrepareModel::getAltLocVec() const
{
  return p_altLocVec;
}

vector<string> PrepareModel::getChainVec() const
{
  return p_chainVec;
}

PrepareModel::DefaultHistidine PrepareModel::getDefaultHistidine() const
{
  return p_defHistidine;
}

bool PrepareModel::getAddLinks() const
{
  return p_addLink;
}

bool PrepareModel::getSolvateFlag() const
{
  return p_solvate;
}

bool PrepareModel::getSetSolvent() const
{
  return p_setSolvent;
}

int PrepareModel::getSolventOption() const
{
  return p_solventOpt;
}

string PrepareModel::getSolventName() const
{
  return p_solventName;
}

string PrepareModel::getSolventModel() const
{
  return p_solventModel;
}

int PrepareModel::getBoundaryCondition() const
{
  return p_BCOpt;
}

int PrepareModel::getGeometryOption() const
{
  return p_geomOpt;
}

double PrepareModel::getMinSluSluDist() const
{
  return p_minSluSluDis;
}

double PrepareModel::getMinSluSlvDist() const
{
  return p_touch;
}

double PrepareModel::getExpandVolPar() const
{
  return p_expandVol;
}

double PrepareModel::getEnvelopePar() const
{
  return p_envelope;
}

vector<string> PrepareModel::getCmdList() const
{
  return p_coordCmd;
}

vector<string> PrepareModel::getLinkList() const
{
  return p_links;
}

vector<string> PrepareModel::getCounterIonList() const
{
  return p_cIons;
}

vector<string> PrepareModel::getPmfList() const
{
  return p_pmfCmd;
}

map<string,int>  PrepareModel::getPmfSelection() const
{
  return p_pmfSelStringToInt;
}

string PrepareModel::getPmfSelection(int selection)
{
  map<int, string>::iterator iter = p_pmfSelIntToString.find(selection);
  if (iter != p_pmfSelIntToString.end()) {
    return iter->second;
  } else {
    return "";
  }
}

int PrepareModel::getPmfSelection(string selection)
{
  map<string, int>::iterator iter = p_pmfSelStringToInt.find(selection);
  if (iter != p_pmfSelStringToInt.end()) {
    return iter->second;
  } else {
    return -1;
  }
}

double PrepareModel::getCounterIonScale() const
{
  return p_CIScale;
}

bool PrepareModel::getSetIons() const
{
  return p_setIons;
}

string PrepareModel::getCounterIonOpt() const
{
  return p_ionOption;
}

int PrepareModel::getCINumber() const
{
  return p_CINumber;
}

int PrepareModel::getCIGridSize() const
{
  return p_CIGrid;
}

double PrepareModel::getMinAtomCIDis() const
{
  return p_CIMinDis;
}

int PrepareModel::getInitialAtom() const
{
  return p_initAtom;
}

int PrepareModel::getFinalAtom() const
{
  return p_finalAtom;
}

int PrepareModel::getTranslateOption() const
{
  return p_translateOption;
}

double PrepareModel::getTranslateX() const
{
  return p_trnsX;
}

double PrepareModel::getTranslateY() const
{
  return p_trnsY;
}

double PrepareModel::getTranslateZ() const
{
  return p_trnsZ;
}

int PrepareModel::getOrientOption() const
{
  return p_orientOption;
}

double PrepareModel::getAngle() const
{
  return p_angle;
}

double PrepareModel::getOrientX() const
{
  return p_orntX;
}

int PrepareModel::getRemoveOption() const
{
  return p_removeOpt;
}

bool PrepareModel::getRemoveXRange() const
{
  return p_xRange;
}

bool PrepareModel::getRemoveYRange() const
{
  return p_yRange;
}

bool PrepareModel::getRemoveZRange() const
{
  return p_zRange;
}

int PrepareModel::getRepeatOption() const
{
  return p_repeatOption;
}

int PrepareModel::getXRepeat() const
{
  return p_xRepeat;
}

int PrepareModel::getYRepeat() const
{
  return p_yRepeat;
}

int PrepareModel::getZRepeat() const
{
  return p_zRepeat;
}

double PrepareModel::getMinRepeatSeparation() const
{
  return p_minRSep;
}

bool PrepareModel::getRandomX() const
{
  return p_ranX;
}

bool PrepareModel::getRandomY() const
{
  return p_ranY;
}

bool PrepareModel::getRandomZ() const
{
  return p_ranZ;
}

bool PrepareModel::getInvertZCopy() const
{
  return p_invertZCopy;
}

bool PrepareModel::getUseZBoxDim() const
{
  return p_useZBoxDim;
}

int PrepareModel::getSkipX() const
{
  return p_skipX;
}

int PrepareModel::getSkipY() const
{
  return p_skipY;
}

int PrepareModel::getSkipZ() const
{
  return p_skipZ;
}

double PrepareModel::getOrientY() const
{
  return p_orntY;
}

double PrepareModel::getOrientZ() const
{
  return p_orntZ;
}

bool PrepareModel::getUseMemory() const
{
  return p_useMemory;
}

int PrepareModel::getHeapMemory() const
{
  return p_heap;
}

int PrepareModel::getStackMemory() const
{
  return p_stack;
}

int PrepareModel::getGlobalMemory() const
{
  return p_global;
}

int PrepareModel::getPrintLevel() const
{
  return p_printLevel;
}

bool PrepareModel::getUseFractions() const
{
  return p_useFrac;
}

int PrepareModel::getFraction1() const
{
  return p_frac1;
}

int PrepareModel::getFraction2() const
{
  return p_frac2;
}

int PrepareModel::getFraction3() const
{
  return p_frac3;
}

int PrepareModel::getFraction4() const
{
  return p_frac4;
}

int PrepareModel::getFraction5() const
{
  return p_frac5;
}

int PrepareModel::getFraction6() const
{
  return p_frac6;
}

int PrepareModel::getFraction7() const
{
  return p_frac7;
}

int PrepareModel::getFraction8() const
{
  return p_frac8;
}

int PrepareModel::getFraction9() const
{
  return p_frac9;
}

int PrepareModel::getFraction10() const
{
  return p_frac10;
}

void PrepareModel::getDefaultDims(double *dims) const
{
  for (int i=0; i<6; i++) {
    dims[i] = p_defaultDims[i];
  }
}

bool PrepareModel::prescanPDB(const string& url)
{
  Fragment frag;
  int numModels,totAtoms;
  vector<string> altLocVec;
  vector<bool> grpFlagVec;
  vector<string> chainVec;
  ifstream is(url.c_str());
  bool success;
  success = frag.prescanPDB(is,numModels,altLocVec,grpFlagVec,
                               chainVec,totAtoms);
  is.close();
  if (success) {
    p_numModels = numModels;
    p_altLocVec = altLocVec;
    p_chainVec = chainVec;
  }
  return success;
}

bool PrepareModel::getChangedTopology() const
{
  return p_changedTopology;
}

void PrepareModel::generateInputFile() throw(InvalidException)
{
   throw NotImplementedException("generateInputFile not implemented",WHERE);
}

void PrepareModel::run() throw(InvalidException)
{
   throw NotImplementedException("run not implemented",WHERE);
}
