/**
 * @file
 *
 *
 */
#include "util/NotImplementedException.H"
#include "util/StringTokenizer.H"
#include "util/STLUtil.H"

#include "tdat/ControlModel.H"

/* Default values */
const bool ControlModel::USEBALANCE = false;
const bool ControlModel::BALANCECELLSZ = true;
const bool ControlModel::BALANCEZONLY = false;
const bool ControlModel::REDISTRIBUTEPAIRS = true;
const double ControlModel::COUPLING = 0.75;
const int ControlModel::BALANCEFREQ = 10;
const int ControlModel::SYNCHOPT = 0;
const bool ControlModel::ADVCONFIGOPT = false;
const bool ControlModel::PROCCONFIG = false;
const int ControlModel::XPROC = 1;
const int ControlModel::YPROC = 1;
const int ControlModel::ZPROC = 1;
const bool ControlModel::CELLCONFIG = false;
const int ControlModel::XCELL = 1;
const int ControlModel::YCELL = 1;
const int ControlModel::ZCELL = 1;
const bool ControlModel::USEMAXSLV = false;
const int ControlModel::MAXSLV = 1;
const bool ControlModel::USEMAXSLU = false;
const int ControlModel::MAXSLU = 1;
const bool ControlModel::PACK = true;

/**
 * Constructor
 */
ControlModel::ControlModel()
{
  p_loadBalance = USEBALANCE;
  p_balanceCellSize =  BALANCECELLSZ;
  p_balanceZOnly =  BALANCEZONLY;
  p_redistributePairs = REDISTRIBUTEPAIRS;
  p_balanceStrength = COUPLING;
  p_adjustFreq = BALANCEFREQ;
  p_synchOption = SYNCHOPT;
  p_advancedConfig = ADVCONFIGOPT;
  p_setProcCnfg = PROCCONFIG;
  p_xProc = XPROC;
  p_yProc = YPROC;
  p_zProc = ZPROC;
  p_setCellCnfg = CELLCONFIG;
  p_xCell = XPROC;
  p_yCell = YPROC;
  p_zCell = ZPROC;
  p_setMaxSlv = USEMAXSLV;
  p_MaxSlv = MAXSLV;
  p_setMaxSlu = USEMAXSLU;
  p_MaxSlu = MAXSLU;
  p_pack = PACK;
}

/**
 * Destructor
 */
ControlModel::~ControlModel()
{
}

/**
 * Modifiers
 */
void ControlModel::setLoadBalance(bool flag)
{
  p_loadBalance = flag;
}

void ControlModel::setBalanceCellSize(bool flag)
{
  p_balanceCellSize = flag;
}

void ControlModel::setBalanceZOnly(bool flag)
{
  p_balanceZOnly = flag;
}

void ControlModel::setRedistributePairs(bool flag)
{
  p_redistributePairs = flag;
}

void ControlModel::setCouplingStrength(double param)
{
  p_balanceStrength = param;
}

void ControlModel::setAdjustFrequency(int freq)
{
  p_adjustFreq = freq;
}

void ControlModel::setSynchOption(int opt)
{
  p_synchOption = opt;
}

void ControlModel::setAdvancedConfigOpt(bool flag)
{
  p_advancedConfig = flag;
}

void ControlModel::setProcessorConfig(bool flag)
{
  p_setProcCnfg = flag;
}

void ControlModel::setXProc(int dim)
{
  p_xProc = dim;
}

void ControlModel::setYProc(int dim)
{
  p_yProc = dim;
}

void ControlModel::setZProc(int dim)
{
  p_zProc = dim;
}

void ControlModel::setCellConfig(bool flag)
{
  p_setCellCnfg = flag;
}

void ControlModel::setXCell(int dim)
{
  p_xCell = dim;
}

void ControlModel::setYCell(int dim)
{
  p_yCell = dim;
}

void ControlModel::setZCell(int dim)
{
  p_zCell = dim;
}

void ControlModel::setUseMaxSolvent(bool flag)
{
  p_setMaxSlv = flag;
}

void ControlModel::setMaxSolvent(int max)
{
  p_MaxSlv = max;
}

void ControlModel::setUseMaxSolute(bool flag)
{
  p_setMaxSlu = flag;
}

void ControlModel::setMaxSolute(int max)
{
  p_MaxSlu = max;
}

void ControlModel::setPackOption(bool flag)
{
  p_pack = flag;
}

/**
 * Accessors
 */
bool ControlModel::getLoadBalance() const
{
  return p_loadBalance;
}

bool ControlModel::getBalanceCellSize() const
{
  return p_balanceCellSize;
}

bool ControlModel::getBalanceZOnly() const
{
  return p_balanceZOnly;
}

bool ControlModel::getRedistributePairs() const
{
  return p_redistributePairs;
}

double ControlModel::getCouplingStrength() const
{
  return p_balanceStrength;
}

int ControlModel::getAdjustFrequency() const
{
  return p_adjustFreq;
}

int ControlModel::getSynchOption() const
{
  return p_synchOption;
}

bool ControlModel::getAdvancedConfigOpt() const
{
  return p_advancedConfig;
}

bool ControlModel::getProcessorConfig() const
{
  return p_setProcCnfg;
}

int ControlModel::getXProc() const
{
  return p_xProc;
}

int ControlModel::getYProc() const
{
  return p_yProc;
}

int ControlModel::getZProc() const
{
  return p_zProc;
}

bool ControlModel::getCellConfig() const
{
  return p_setCellCnfg;
}

int ControlModel::getXCell() const
{
  return p_xCell;
}

int ControlModel::getYCell() const
{
  return p_yCell;
}

int ControlModel::getZCell() const
{
  return p_zCell;
}

bool ControlModel::getUseMaxSolvent() const
{
  return p_setMaxSlv;
}

int ControlModel::getMaxSolvent() const
{
  return p_MaxSlv;
}

bool ControlModel::getUseMaxSolute() const
{
  return p_setMaxSlu;
}

int ControlModel::getMaxSolute() const
{
  return p_MaxSlu;
}

bool ControlModel::getPackOption() const
{
  return p_pack;
}

/**
 * Reset model to default values
 */
void ControlModel::reset()
{
  setLoadBalance(ControlModel::USEBALANCE);
  setBalanceCellSize(ControlModel::BALANCECELLSZ);
  setBalanceZOnly(ControlModel::BALANCEZONLY);
  setRedistributePairs(ControlModel::REDISTRIBUTEPAIRS);
  setCouplingStrength(ControlModel::COUPLING);
  setAdjustFrequency(ControlModel::BALANCEFREQ);
  setSynchOption(ControlModel::SYNCHOPT);
  setAdvancedConfigOpt(ControlModel::ADVCONFIGOPT);
  setProcessorConfig(ControlModel::PROCCONFIG);
  setXProc(ControlModel::XPROC);
  setYProc(ControlModel::YPROC);
  setZProc(ControlModel::ZPROC);
  setCellConfig(ControlModel::CELLCONFIG);
  setXCell(ControlModel::XCELL);
  setYCell(ControlModel::YCELL);
  setZCell(ControlModel::ZCELL);
  setUseMaxSolvent(ControlModel::USEMAXSLV);
  setMaxSolvent(ControlModel::MAXSLV);
  setUseMaxSolute(ControlModel::USEMAXSLU);
  setMaxSolute(ControlModel::MAXSLU);
  setPackOption(ControlModel::PACK);
}
