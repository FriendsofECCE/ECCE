/**
 * @file
 *
 *
 */

#include "util/NotImplementedException.H"
#include "util/StringTokenizer.H"
#include "util/STLUtil.H"

#include "tdat/FilesModel.H"

//Default values
const bool FilesModel::PRINTMD = false;
const int FilesModel::PRINTMDFREQ = 10;
const bool FilesModel::SOLVENTENERGY = false;
const bool FilesModel::SOLUTEENERGY = false;
const bool FilesModel::PRINTSTAT = false;
const int FilesModel::STATFREQ = 10;
const bool FilesModel::SOLUTEBONDS = false;
const int FilesModel::BONDFREQ = 10;
const bool FilesModel::SOLUTEFRCS = false;
const int FilesModel::FORCEFREQ = 10;
const bool FilesModel::PMFDATA = false;
const int FilesModel::PMFFREQ = 10;
const bool FilesModel::PRINTTOP = false;
const bool FilesModel::NBONDPAR = false;
const bool FilesModel::SLVBONDPAR = false;
const bool FilesModel::SLUBONDPAR = false;
const int FilesModel::RSTFREQ = 100;
const bool FilesModel::KEEPRST = false;
const bool FilesModel::WRITETRJ = false;
const int FilesModel::TRJSTORAGE = 0;
const bool FilesModel::USEBATCH = false;
const int FilesModel::BATCHSZ = 100;
const bool FilesModel::COORDSLUPRT = true;
const int FilesModel::COORDSLUFRQ = 100;
const bool FilesModel::COORDSLVPRT = true;
const int FilesModel::COORDSLVFRQ = 100;
const bool FilesModel::VELSLUPRT = false;
const int FilesModel::VELSLUFRQ = 100;
const bool FilesModel::VELSLVPRT = false;
const int FilesModel::VELSLVFRQ = 100;
const bool FilesModel::FRCSLUPRT = false;
const int FilesModel::FRCSLUFRQ = 100;
const bool FilesModel::FRCSLVPRT = false;
const int FilesModel::FRCSLVFRQ = 100;
const bool FilesModel::WRTPOL = false;
const int FilesModel::POLOPT = 0;
const int FilesModel::POLSLUFRQ = 100;
const int FilesModel::POLSLVFRQ = 100;
const bool FilesModel::AVGPROP = false;
const int FilesModel::PROPFREQ = 100;

/**
 * Constructor
 */
FilesModel::FilesModel()
{
  p_printMD = PRINTMD;
  p_printMDFreq = PRINTMDFREQ;
  p_solventEnergy = SOLVENTENERGY;
  p_soluteEnergy = SOLUTEENERGY;
  p_printStat = PRINTSTAT;
  p_statFreq = STATFREQ;
  p_soluteBonds = SOLUTEBONDS;
  p_bondFreq = BONDFREQ;
  p_soluteForces = SOLUTEFRCS;
  p_forceFreq = FORCEFREQ;
  p_printTop = PRINTTOP;
  p_nbondPar = NBONDPAR;
  p_solventBondPar = SLVBONDPAR;
  p_soluteBondPar = SLUBONDPAR;
  p_restartFreq = RSTFREQ;
  p_keepRestart = KEEPRST;
  p_trajectoryStorage = TRJSTORAGE;
  p_useBatches = USEBATCH;
  p_batchSize = BATCHSZ;
  p_soluteCoordsPrint = COORDSLUPRT;
  p_soluteCoordsFreq = COORDSLUFRQ;
  p_solventCoordsPrint = COORDSLVPRT;
  p_solventCoordsFreq = COORDSLVFRQ;
  p_soluteVelocityPrint = VELSLUPRT;
  p_soluteVelocityFreq = VELSLUFRQ;
  p_solventVelocityPrint = VELSLVPRT;
  p_solventVelocityFreq = VELSLVFRQ;
  p_soluteForcesPrint = FRCSLUPRT;
  p_soluteForcesFreq = FRCSLUFRQ;
  p_solventForcesPrint = FRCSLVPRT;
  p_solventForcesFreq = FRCSLVFRQ;
  p_writePolarization = WRTPOL;
  p_polarizationOpts = POLOPT;
  p_solutePolarizationFreq = POLSLUFRQ;
  p_solventPolarizationFreq = POLSLVFRQ;
  p_useAvgProperties = AVGPROP;
  p_propertyFreq = PROPFREQ;
  p_pmfData = PMFDATA;
  p_pmfFreq = PMFFREQ;
}

/**
 * Destructor
 */
FilesModel::~FilesModel()
{
}

/**
 * Accessors
 */
bool FilesModel::getPrintMD() const
{
  return p_printMD;
}

int FilesModel::getPrintMDFreq() const
{
  return p_printMDFreq;
}

bool FilesModel::getSolventEnergy() const
{
  return p_solventEnergy;
}

bool FilesModel::getSoluteEnergy() const
{
  return p_soluteEnergy;
}

bool FilesModel::getPrintStat() const
{
  return p_printStat;
}

int FilesModel::getStatFreq() const
{
  return p_statFreq;
}

bool FilesModel::getSoluteBonds() const
{
  return p_soluteBonds;
}

int FilesModel::getBondFreq() const
{
  return p_bondFreq;
}

bool FilesModel::getSoluteForces() const
{
  return p_soluteForces;
}

int FilesModel::getForceFreq() const
{
  return p_forceFreq;
}

bool FilesModel::getPmfData() const
{
  return p_pmfData;
}

int FilesModel::getPmfFreq() const
{
  return p_pmfFreq;
}

bool FilesModel::getPrintTop() const
{
  return p_printTop;
}

bool FilesModel::getNbondPar() const
{
  return p_nbondPar;
}

bool FilesModel::getSolventBondPar() const
{
  return p_solventBondPar;
}

bool FilesModel::getSoluteBondPar() const
{
  return p_soluteBondPar;
}

int FilesModel::getRestartFreq() const
{
  return p_restartFreq;
}

bool FilesModel::getKeepRestart() const
{
  return p_keepRestart;
}

bool FilesModel::getWriteTrajectory() const
{
  return p_solventCoordsPrint || p_soluteCoordsPrint ||
         p_solventVelocityPrint || p_soluteVelocityPrint ||
         p_solventForcesPrint || p_soluteForcesPrint;
}

int FilesModel::getTrajectoryStorage() const
{
  return p_trajectoryStorage;
}

bool FilesModel::getUseBatches() const
{
  return p_useBatches;
}

int FilesModel::getBatchSize() const
{
  return p_batchSize;
}

bool FilesModel::getCoordsSolventPrint() const
{
  return p_solventCoordsPrint;
}

int FilesModel::getCoordsSolventFreq() const
{
  return p_solventCoordsFreq;
}

bool FilesModel::getCoordsSolutePrint() const
{
  return p_soluteCoordsPrint;
}

int FilesModel::getCoordsSoluteFreq() const
{
  return p_soluteCoordsFreq;
}

bool FilesModel::getVelocitySolventPrint() const
{
  return p_solventVelocityPrint;
}

int FilesModel::getVelocitySolventFreq() const
{
  return p_solventVelocityFreq;
}

bool FilesModel::getVelocitySolutePrint() const
{
  return p_soluteVelocityPrint;
}

int FilesModel::getVelocitySoluteFreq() const
{
  return p_soluteVelocityFreq;
}

bool FilesModel::getForcesSolventPrint() const
{
  return p_solventForcesPrint;
}

int FilesModel::getForcesSolventFreq() const
{
  return p_solventForcesFreq;
}

bool FilesModel::getForcesSolutePrint() const
{
  return p_soluteForcesPrint;
}

int FilesModel::getForcesSoluteFreq() const
{
  return p_soluteForcesFreq;
}

bool FilesModel::getWritePolarization() const
{
  return p_writePolarization;
}

int FilesModel::getPolarizationOpt() const
{
  return p_polarizationOpts;
}

int FilesModel::getPolarizationSolventFreq() const
{
  return p_solventPolarizationFreq;
}

int FilesModel::getPolarizationSoluteFreq() const
{
  return p_solutePolarizationFreq;
}

bool FilesModel::getAvgProperties() const
{
  return p_useAvgProperties;
}

int FilesModel::getPropertyFreq() const
{
  return p_propertyFreq;
}

/**
 * Modifiers
 */
void FilesModel::setPrintMD(bool flag)
{
  p_printMD = flag;
}

void FilesModel::setPrintMDFreq(int frq)
{
  p_printMDFreq = frq;
}

void FilesModel::setSolventEnergy(bool flag)
{
  p_solventEnergy = flag;
}

void FilesModel::setSoluteEnergy(bool flag)
{
  p_soluteEnergy = flag;
}

void FilesModel::setPrintStat(bool flag)
{
  p_printStat = flag;
}

void FilesModel::setStatFreq(int frq)
{
  p_statFreq = frq;
}

void FilesModel::setSoluteBonds(bool flag)
{
  p_soluteBonds = flag;
}

void FilesModel::setBondFreq(int frq)
{
  p_bondFreq = frq;
}

void FilesModel::setSoluteForces(bool flag)
{
  p_soluteForces = flag;
}

void FilesModel::setForceFreq(int frq)
{
  p_forceFreq = frq;
}

void FilesModel::setPmfData(bool flag)
{
  p_pmfData = flag;
}

void FilesModel::setPmfFreq(int frq)
{
  p_pmfFreq = frq;
}

void FilesModel::setPrintTop(bool flag)
{
  p_printTop = flag;
}

void FilesModel::setNbondPar(bool flag)
{
  p_nbondPar = flag;
}

void FilesModel::setSolventBondPar(bool flag)
{
  p_solventBondPar = flag;
}

void FilesModel::setSoluteBondPar(bool flag)
{
  p_soluteBondPar = flag;
}

void FilesModel::setRestartFreq(int freq)
{
  p_restartFreq = freq;
}

void FilesModel::setKeepRestart(bool flag)
{
  p_keepRestart = flag;
}

void FilesModel::setTrajectoryStorage(int store)
{
  p_trajectoryStorage = store;
}

void FilesModel::setUseBatches(bool flag)
{
  p_useBatches = flag;
}

void FilesModel::setBatchSize(int size)
{
  p_batchSize = size;
}

void FilesModel::setCoordsSolventPrint(bool flag)
{
  p_solventCoordsPrint = flag;
}

void FilesModel::setCoordsSolventFreq(int freq)
{
  p_solventCoordsFreq = freq;
}

void FilesModel::setCoordsSolutePrint(bool flag)
{
  p_soluteCoordsPrint = flag;
}

void FilesModel::setCoordsSoluteFreq(int freq)
{
  p_soluteCoordsFreq = freq;
}

void FilesModel::setVelocitySolventPrint(bool flag)
{
  p_solventVelocityPrint = flag;
}

void FilesModel::setVelocitySolventFreq(int freq)
{
  p_solventVelocityFreq = freq;
}

void FilesModel::setVelocitySolutePrint(bool flag)
{
  p_soluteVelocityPrint = flag;
}

void FilesModel::setVelocitySoluteFreq(int freq)
{
  p_soluteVelocityFreq = freq;
}

void FilesModel::setForcesSolventPrint(bool flag)
{
  p_solventForcesPrint = flag;
}

void FilesModel::setForcesSolventFreq(int freq)
{
  p_solventForcesFreq = freq;
}

void FilesModel::setForcesSolutePrint(bool flag)
{
  p_soluteForcesPrint = flag;
}

void FilesModel::setForcesSoluteFreq(int freq)
{
  p_soluteForcesFreq = freq;
}

void FilesModel::setWritePolarization(bool flag)
{
  p_writePolarization = flag;
}

void FilesModel::setPolarizationOpt(int opt)
{
  p_polarizationOpts = opt;
}

void FilesModel::setPolarizationSolventFreq(int freq)
{
  p_solventPolarizationFreq = freq;
}

void FilesModel::setPolarizationSoluteFreq(int freq)
{
  p_solutePolarizationFreq = freq;
}

void FilesModel::setAvgProperties(bool flag)
{
  p_useAvgProperties = flag;
}

void FilesModel::setPropertyFreq(int freq)
{
  p_propertyFreq = freq;
}

/**
 * Reset model to default
 */
void FilesModel::reset()
{
  setPrintMD(FilesModel::PRINTMD);
  setPrintMDFreq(FilesModel::PRINTMDFREQ);
  setSolventEnergy(FilesModel::SOLVENTENERGY);
  setSoluteEnergy(FilesModel::SOLUTEENERGY);
  setPrintStat(FilesModel::PRINTSTAT);
  setStatFreq(FilesModel::STATFREQ);
  setSoluteBonds(FilesModel::SOLUTEBONDS);
  setBondFreq(FilesModel::BONDFREQ);
  setSoluteForces(FilesModel::SOLUTEFRCS);
  setForceFreq(FilesModel::FORCEFREQ);
  setPrintTop(FilesModel::PRINTTOP);
  setNbondPar(FilesModel::NBONDPAR);
  setSolventBondPar(FilesModel::SLVBONDPAR);
  setSoluteBondPar(FilesModel::SLUBONDPAR);
  setRestartFreq(FilesModel::RSTFREQ);
  setTrajectoryStorage(FilesModel::TRJSTORAGE);
  setUseBatches(FilesModel::USEBATCH);
  setBatchSize(FilesModel::BATCHSZ);
  setCoordsSolutePrint(FilesModel::COORDSLUPRT);
  setCoordsSoluteFreq(FilesModel::COORDSLUFRQ);
  setCoordsSolventPrint(FilesModel::COORDSLVPRT);
  setCoordsSolventFreq(FilesModel::COORDSLVFRQ);
  setVelocitySolutePrint(FilesModel::VELSLUPRT);
  setVelocitySoluteFreq(FilesModel::VELSLUFRQ);
  setVelocitySolventPrint(FilesModel::VELSLVPRT);
  setVelocitySolventFreq(FilesModel::VELSLVFRQ);
  setForcesSolutePrint(FilesModel::FRCSLUPRT);
  setForcesSoluteFreq(FilesModel::FRCSLUFRQ);
  setForcesSolventPrint(FilesModel::FRCSLVPRT);
  setForcesSolventFreq(FilesModel::FRCSLVFRQ);
  setWritePolarization(FilesModel::WRTPOL);
  setPolarizationOpt(FilesModel::POLOPT);
  setPolarizationSoluteFreq(FilesModel::POLSLUFRQ);
  setPolarizationSolventFreq(FilesModel::POLSLVFRQ);
  setAvgProperties(FilesModel::AVGPROP);
  setPropertyFreq(FilesModel::PROPFREQ);
  setPmfData(FilesModel::PMFDATA);
  setPmfFreq(FilesModel::PMFFREQ);
}
