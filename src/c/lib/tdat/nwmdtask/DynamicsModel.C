/**
 * @file
 *
 *
 */

#include "util/NotImplementedException.H"
#include "util/StringTokenizer.H"
#include "util/STLUtil.H"

#include "tdat/DynamicsModel.H"

/* Default values */
const int DynamicsModel::INTEGOPT = 0;
const bool DynamicsModel::RESUME = false;
const int DynamicsModel::EQSTEPS = 0;
const int DynamicsModel::DATASTEPS = 0;
const double DynamicsModel::TIMESTEP = 0.001;
const double DynamicsModel::INITTIME = 0.0;
const bool DynamicsModel::RECENTER = false;
const int DynamicsModel::RECENTERFREQ = 1;
const int DynamicsModel::RECENTEROPT = 1;
const bool DynamicsModel::USEFRAC = false;
const int DynamicsModel::FRAC1 = 0;
const int DynamicsModel::FRAC2 = 0;
const int DynamicsModel::FRAC3 = 0;
const int DynamicsModel::FRAC4 = 0;
const int DynamicsModel::FRAC5 = 0;
const bool DynamicsModel::REMOVECM = false;
const int DynamicsModel::REMOVECMFREQ = 10;
const bool DynamicsModel::USENVT = false;
const double DynamicsModel::NVTTEMP = 298.15;
const bool DynamicsModel::USEDBLRELAX = false;
const double DynamicsModel::SLVRXTIME = 0.1;
const double DynamicsModel::SLURXTIME = 0.1;
const bool DynamicsModel::USEANNEALING = false;
const double DynamicsModel::FINALTEMP = 298.15;
const double DynamicsModel::INITANNEALTIME = 0.0;
const double DynamicsModel::FINALANNEALTIME = 100.0;
const bool DynamicsModel::USENPT = false;
const double DynamicsModel::NPTPRSSR = 1.01325e5;
const double DynamicsModel::NPTRXTIME = 0.5;
const double DynamicsModel::COMPRESSIBILITY = 4.53e-10;
const int DynamicsModel::VOLADJUSTALG = 0;
const bool DynamicsModel::USEVELRETHERM = false;
const int DynamicsModel::RETHERMFREQ = 1;
const double DynamicsModel::RETHERMTEMP = 298.15;
const double DynamicsModel::RETHERMFRAC = 1.0;
const int DynamicsModel::RETHERMOPT = 0;

/**
 * Constructor
 */
DynamicsModel::DynamicsModel()
{
  p_intOpt = DynamicsModel::INTEGOPT;
  p_resumeOpt = DynamicsModel::RESUME;
  p_eqSteps = DynamicsModel::EQSTEPS;
  p_dataSteps = DynamicsModel::DATASTEPS;
  p_timeStep = DynamicsModel::TIMESTEP;
  p_initTime = DynamicsModel::INITTIME;
  p_recenter = DynamicsModel::RECENTER;
  p_rcntrFreq = DynamicsModel::RECENTERFREQ;
  p_rcntrOpt = DynamicsModel::RECENTEROPT;
  p_useFrac = DynamicsModel::USEFRAC;
  p_frac1 = DynamicsModel::FRAC1;
  p_frac2 = DynamicsModel::FRAC2;
  p_frac3 = DynamicsModel::FRAC3;
  p_frac4 = DynamicsModel::FRAC4;
  p_frac5 = DynamicsModel::FRAC5;
  p_rmCM = DynamicsModel::REMOVECM;
  p_rmCMFreq = DynamicsModel::REMOVECMFREQ;
  p_useNVT = DynamicsModel::USENVT;
  p_NVTTemp = DynamicsModel::NVTTEMP;
  p_useDoubleRelaxation = DynamicsModel::USEDBLRELAX;
  p_solventRxTime = DynamicsModel::SLVRXTIME;
  p_soluteRxTime = DynamicsModel::SLURXTIME;
  p_useAnnealing = DynamicsModel::USEANNEALING;
  p_finalTemp = DynamicsModel::FINALTEMP;
  p_initAnnealTime = DynamicsModel::INITANNEALTIME;
  p_finalAnnealTime = DynamicsModel::FINALANNEALTIME;
  p_useNPT = DynamicsModel::USENPT;
  p_NPTPrssr = DynamicsModel::NPTPRSSR;
  p_NPTRxTime = DynamicsModel::NPTRXTIME;
  p_compressibility = DynamicsModel::COMPRESSIBILITY;
  p_volAdjustAlg = DynamicsModel::VOLADJUSTALG;
  p_useVelRthrm = DynamicsModel::USEVELRETHERM;
  p_rthrmFreq = DynamicsModel::RETHERMFREQ;
  p_rthrmTemp = DynamicsModel::RETHERMTEMP;
  p_rthrmFrac = DynamicsModel::RETHERMFRAC;
  p_rthrmOpt = DynamicsModel::RETHERMOPT;
}


/**
 * Destructor
 */
DynamicsModel::~DynamicsModel()
{
}

/**
 * Modifiers
 */
void DynamicsModel::setIntegrationOpts(int opt)
{
  p_intOpt = opt;
}

void DynamicsModel::setResumeOpt(bool flag)
{
  p_resumeOpt = flag;
}

void DynamicsModel::setEquilibrationSteps(int steps)
{
  p_eqSteps = steps;
}

void DynamicsModel::setDataSteps(int steps)
{
  p_dataSteps = steps;
}

void DynamicsModel::setTimeStep(double step)
{
  p_timeStep = step;
}

void DynamicsModel::setInitialTime(double time)
{
  p_initTime = time;
}

void DynamicsModel::setRecenterSystem(bool flag)
{
  p_recenter = flag;
}

void DynamicsModel::setRecenterFreq(int freq)
{
  p_rcntrFreq = freq;
}

void DynamicsModel::setRecenterOpts(int opt)
{
  p_rcntrOpt = opt;
}

void DynamicsModel::setUseFractions(bool flag)
{
  p_useFrac = flag;
}

void DynamicsModel::setFraction1(int frac)
{
  p_frac1 = frac;
}

void DynamicsModel::setFraction2(int frac)
{
  p_frac2 = frac;
}

void DynamicsModel::setFraction3(int frac)
{
  p_frac3 = frac;
}

void DynamicsModel::setFraction4(int frac)
{
  p_frac4 = frac;
}

void DynamicsModel::setFraction5(int frac)
{
  p_frac5 = frac;
}

void DynamicsModel::setRemoveCM(bool flag)
{
  p_rmCM = flag;
}

void DynamicsModel::setRemoveCMFreq(int freq)
{
  p_rmCMFreq = freq;
}

void DynamicsModel::setUseNVT(bool flag)
{
  p_useNVT = flag;
}

void DynamicsModel::setNVTTemperature(double temp)
{
  p_NVTTemp = temp;
}

void DynamicsModel::setUseDoubleRelaxation(bool flag)
{
  p_useDoubleRelaxation = flag;
}

void DynamicsModel::setSolventRxTime(double time)
{
  p_solventRxTime = time;
}

void DynamicsModel::setSoluteRxTime(double time)
{
  p_soluteRxTime = time;
}

void DynamicsModel::setUseAnnealing(bool flag)
{
  p_useAnnealing = flag;
}

void DynamicsModel::setFinalAnnealTemp(double temp)
{
  p_finalTemp = temp;
}

void DynamicsModel::setInitAnnealTime(double time)
{
  p_initAnnealTime = time;
}

void DynamicsModel::setFinalAnnealTime(double time)
{
  p_finalAnnealTime = time;
}

void DynamicsModel::setUseNPT(bool flag)
{
  p_useNPT = flag;
}

void DynamicsModel::setNPTPressure(double prssr)
{
  p_NPTPrssr = prssr;
}

void DynamicsModel::setNPTRxTime(double time)
{
  p_NPTRxTime = time;
}

void DynamicsModel::setCompressibility(double cmpr)
{
  p_compressibility = cmpr;
}

void DynamicsModel::setVolumeAdjustAlg(int alg)
{
  p_volAdjustAlg = alg;
}

void DynamicsModel::setUseVelRetherm(bool flag)
{
  p_useVelRthrm = flag;
}

void DynamicsModel::setRethermFreq(int freq)
{
  p_rthrmFreq = freq;
}

void DynamicsModel::setRethermTemp(double temp)
{
  p_rthrmTemp = temp;
}

void DynamicsModel::setRethermFrac(double frac)
{
  p_rthrmFrac = frac;
}

void DynamicsModel::setRethermOpts(int opt)
{
  p_rthrmOpt = opt;
}

void DynamicsModel::setURL(string url)
{
  p_url = url;
}
/**
 * Accessors
 */
int DynamicsModel::getIntegrationOpts() const
{
  return p_intOpt;
}

bool DynamicsModel::getResumeOpt() const
{
  return p_resumeOpt;
}

int DynamicsModel::getEquilibrationSteps() const
{
  return p_eqSteps;
}

int DynamicsModel::getDataSteps() const
{
  return p_dataSteps;
}

double DynamicsModel::getTimeStep() const
{
  return p_timeStep;
}

double DynamicsModel::getInitialTime() const
{
  return p_initTime;
}

bool DynamicsModel::getRecenterSystem() const
{
  return p_recenter;
}

int DynamicsModel::getRecenterFreq() const
{
  return p_rcntrFreq;
}

int DynamicsModel::getRecenterOpts() const
{
  return p_rcntrOpt;
}

bool DynamicsModel::getUseFractions() const
{
  return p_useFrac;
}

int DynamicsModel::getFraction1() const
{
  return p_frac1;
}

int DynamicsModel::getFraction2() const
{
  return p_frac2;
}

int DynamicsModel::getFraction3() const
{
  return p_frac3;
}

int DynamicsModel::getFraction4() const
{
  return p_frac4;
}

int DynamicsModel::getFraction5() const
{
  return p_frac5;
}

bool DynamicsModel::getRemoveCM() const
{
  return p_rmCM;
}

int DynamicsModel::getRemoveCMFreq() const
{
  return p_rmCMFreq;
}

bool DynamicsModel::getUseNVT() const
{
  return p_useNVT;
}

double DynamicsModel::getNVTTemperature() const
{
  return p_NVTTemp;
}

bool DynamicsModel::getUseDoubleRelaxation() const
{
  return p_useDoubleRelaxation;
}

double DynamicsModel::getSolventRxTime() const
{
  return p_solventRxTime;
}

double DynamicsModel::getSoluteRxTime() const
{
  return p_soluteRxTime;
}

bool DynamicsModel::getUseAnnealing() const
{
  return p_useAnnealing;
}

double DynamicsModel::getFinalAnnealTemp() const
{
  return p_finalTemp;
}

double DynamicsModel::getInitAnnealTime() const
{
  return p_initAnnealTime;
}

double DynamicsModel::getFinalAnnealTime() const
{
  return p_finalAnnealTime;
}

bool DynamicsModel::getUseNPT() const
{
  return p_useNPT;
}

double DynamicsModel::getNPTPressure() const
{
  return p_NPTPrssr;
}

double DynamicsModel::getNPTRxTime() const
{
  return p_NPTRxTime;
}

double DynamicsModel::getCompressibility() const
{
  return p_compressibility;
}

int DynamicsModel::getVolumeAdjustAlg() const
{
  return p_volAdjustAlg;
}

bool DynamicsModel::getUseVelRetherm() const
{
  return p_useVelRthrm;
}

int DynamicsModel::getRethermFreq() const
{
  return p_rthrmFreq;
}

double DynamicsModel::getRethermTemp() const
{
  return p_rthrmTemp;
}

double DynamicsModel::getRethermFrac() const
{
  return p_rthrmFrac;
}

int DynamicsModel::getRethermOpts() const
{
  return p_rthrmOpt;
}

string DynamicsModel::getURL() const
{
  return p_url;
}
/**
 * Reset model values to defaults
 */
void DynamicsModel::reset()
{
  setIntegrationOpts(DynamicsModel::INTEGOPT);
  if (getResumeOpt() > 0) {
    setResumeOpt(0);
  }
  setEquilibrationSteps(DynamicsModel::EQSTEPS);
  setDataSteps(DynamicsModel::DATASTEPS);
  setTimeStep(DynamicsModel::TIMESTEP);
  setInitialTime(DynamicsModel::INITTIME);
  setRecenterSystem(DynamicsModel::RECENTER);
  setRecenterFreq(DynamicsModel::RECENTERFREQ);
  setRecenterOpts(DynamicsModel::RECENTEROPT);
  setUseFractions(DynamicsModel::USEFRAC);
  setFraction1(DynamicsModel::FRAC1);
  setFraction2(DynamicsModel::FRAC2);
  setFraction3(DynamicsModel::FRAC3);
  setFraction4(DynamicsModel::FRAC4);
  setFraction5(DynamicsModel::FRAC5);
  setRemoveCM(DynamicsModel::REMOVECM);
  setRemoveCMFreq(DynamicsModel::REMOVECMFREQ);
  setUseNVT(DynamicsModel::USENVT);
  setNVTTemperature(DynamicsModel::NVTTEMP);
  setUseDoubleRelaxation(DynamicsModel::USEDBLRELAX);
  setSolventRxTime(DynamicsModel::SLVRXTIME);
  setSoluteRxTime(DynamicsModel::SLURXTIME);
  setUseAnnealing(DynamicsModel::USEANNEALING);
  setFinalAnnealTemp(DynamicsModel::FINALTEMP);
  setInitAnnealTime(DynamicsModel::INITANNEALTIME);
  setFinalAnnealTime(DynamicsModel::FINALANNEALTIME);
  setUseNPT(DynamicsModel::USENPT);
  setNPTPressure(DynamicsModel::NPTPRSSR);
  setNPTRxTime(DynamicsModel::NPTRXTIME);
  setCompressibility(DynamicsModel::COMPRESSIBILITY);
  setVolumeAdjustAlg(DynamicsModel::VOLADJUSTALG);
  setUseVelRetherm(DynamicsModel::USEVELRETHERM);
  setRethermFreq(DynamicsModel::RETHERMFREQ);
  setRethermTemp(DynamicsModel::RETHERMTEMP);
  setRethermFrac(DynamicsModel::RETHERMFRAC);
  setRethermOpts(DynamicsModel::RETHERMOPT);
}
