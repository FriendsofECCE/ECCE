/**
 * @file
 *
 *
 */
#include "util/NotImplementedException.H"
#include "util/StringTokenizer.H"
#include "util/STLUtil.H"

#include "tdat/InteractionModel.H"
#include "tdat/Fragment.H"

// Default values
const int InteractionModel::PARAMSET = 1;
const bool InteractionModel::PERTURBATIONSET1 = false;
const bool InteractionModel::PERTURBATIONSET2 = false;
const int InteractionModel::INTERACTOPT = 1;
const int InteractionModel::GRID = 8;
const double InteractionModel::EWALDCONVRG = 0.0001;
const int InteractionModel::BSPLINEORDER = 4;
const bool InteractionModel::SHOWFFT = false;
const bool InteractionModel::ADVANCEDFFT = false;
const int InteractionModel::NFFT = 2;
const int InteractionModel::FFTOPTION = 0;
const bool InteractionModel::USESOLVENTONLY = false;
const int InteractionModel::MAXPOLITERATION = 14;
const double InteractionModel::POLTOL = 0.001;
const bool InteractionModel::FORDERONLY = false;
const int InteractionModel::CUTOFFOPTION = 1;
const double InteractionModel::SHORTCUT = 0.9;
const double InteractionModel::LONGCUT = 0.9;
const int InteractionModel::SHORTFREQ = 1;
const int InteractionModel::LONGFREQ = 1;

/**
 * Constructor
 */
InteractionModel::InteractionModel()
{
  p_paramSet = PARAMSET;
  p_pert1 = PERTURBATIONSET1;
  p_pert2 = PERTURBATIONSET2;
  p_intrctOpt = INTERACTOPT;
  p_grid = GRID;
  p_ewaldConv = EWALDCONVRG;
  p_eSpline = BSPLINEORDER;
  p_advancedFFT = ADVANCEDFFT;
  p_nfft = NFFT;
  p_fftOpt = FFTOPTION;
  p_useSolventOnly = USESOLVENTONLY;
  p_maxPolIter = MAXPOLITERATION;
  p_polTol = POLTOL;
  p_fOrder = FORDERONLY;
  p_cutOpt = CUTOFFOPTION;
  p_sCut = SHORTCUT;
  p_lCut = LONGCUT;
  p_spFreq = SHORTFREQ;
  p_lpFreq = LONGFREQ;
  p_touched = false;
}

/**
 * Destructor
 */
InteractionModel::~InteractionModel()
{
}

/**
 * Accessors
 */

int InteractionModel::getParameterSet() const
{
  return p_paramSet;
}

bool InteractionModel::getFirstPerturbationSet() const
{
  return p_pert1;
}

bool InteractionModel::getSecondPerturbationSet() const
{
  return p_pert2;
}

int InteractionModel::getInteractionOption() const
{
  return p_intrctOpt;
}

int InteractionModel::getGridDimensions() const
{
  return p_grid;
}

double InteractionModel::getEwaldConvergence() const
{
  return p_ewaldConv;
}

int InteractionModel::getBSplineOrder() const
{
  return p_eSpline;
}

bool InteractionModel::getAdvancedFFT() const
{
  return p_advancedFFT;
}

int InteractionModel::getFFTNodes() const
{
  return p_nfft;
}

int InteractionModel::getFFTOption() const
{
  return p_fftOpt;
}

bool InteractionModel::getChargeGridOption() const
{
  return p_useSolventOnly;
}

int InteractionModel::getMaximumPolarIter() const
{
  return p_maxPolIter;
}

double InteractionModel::getPolarizationTolerance() const
{
  return p_polTol;
}

bool InteractionModel::getFirstOrderOption() const
{
  return p_fOrder;
}

int InteractionModel::getCutoffOption() const
{
  return p_cutOpt;
}

double InteractionModel::getSRCutoff() const
{
  return p_sCut;
}

double InteractionModel::getLRCutoff() const
{
  return p_lCut;
}

int InteractionModel::getShortListUpdateFreq() const
{
  return p_spFreq;
}

int InteractionModel::getLongListUpdateFreq() const
{
  return p_lpFreq;
}

bool InteractionModel::getTouched() const
{
  return p_touched;
}

/**
 * Modifiers
 */

void InteractionModel::setParameterSet(int opt)
{
  p_paramSet = opt;
}

void InteractionModel::setFirstPerturbationSet(bool opt)
{
  p_pert1 = opt;
}

void InteractionModel::setSecondPerturbationSet(bool opt)
{
  p_pert2 = opt;
}

void InteractionModel::setInteractionOption(int opt)
{
  p_intrctOpt = opt;
}

void InteractionModel::setGridDimensions(int grid)
{
  p_grid = grid;
}

void InteractionModel::setEwaldConvergence(double tol)
{
  p_ewaldConv = tol;
}

void InteractionModel::setBSplineOrder(int order)
{
  p_eSpline = order;
}

void InteractionModel::setAdvancedFFT(bool opt)
{
  p_advancedFFT = opt;
}

void InteractionModel::setFFTNodes(int n)
{
  p_nfft = n;
}

void InteractionModel::setFFTOption(int opt)
{
  p_fftOpt = opt;
}

void InteractionModel::setChargeGridOption(bool opt)
{
  p_useSolventOnly = opt;
}

void InteractionModel::setMaximumPolarIter(int iter)
{
  p_maxPolIter = iter;
}

void InteractionModel::setPolarizationTolerance(double tol)
{
  p_polTol = tol;
}

void InteractionModel::setFirstOrderOption(bool opt)
{
  p_fOrder = opt;
}

void InteractionModel::setCutoffOption(int opt)
{
  p_cutOpt = opt;
}

void InteractionModel::setSRCutoff(double cutoff)
{
  p_sCut = cutoff;
}

void InteractionModel::setLRCutoff(double cutoff)
{
  p_lCut = cutoff;
}

void InteractionModel::setShortListUpdateFreq(int freq)
{
  p_spFreq = freq;
}

void InteractionModel::setLongListUpdateFreq(int freq)
{
  p_lpFreq = freq;
}

void InteractionModel::setTouched(bool flag)
{
  p_touched = flag;
}

/**
 * Reset model to defaults
 */
void InteractionModel::reset()
{
  setParameterSet(InteractionModel::PARAMSET);
  setFirstPerturbationSet(InteractionModel::PERTURBATIONSET1);
  setSecondPerturbationSet(InteractionModel::PERTURBATIONSET2);
  setInteractionOption(InteractionModel::INTERACTOPT);
  setGridDimensions(InteractionModel::GRID);
  setEwaldConvergence(InteractionModel::EWALDCONVRG);
  setBSplineOrder(InteractionModel::BSPLINEORDER);
  setAdvancedFFT(InteractionModel::ADVANCEDFFT);
  setFFTNodes(InteractionModel::NFFT);
  setFFTOption(InteractionModel::FFTOPTION);
  setChargeGridOption(InteractionModel::USESOLVENTONLY);
  setMaximumPolarIter(InteractionModel::MAXPOLITERATION);
  setPolarizationTolerance(InteractionModel::POLTOL);
  setFirstOrderOption(InteractionModel::FORDERONLY);
  setCutoffOption(InteractionModel::CUTOFFOPTION);
  setSRCutoff(InteractionModel::SHORTCUT);
  setLRCutoff(InteractionModel::LONGCUT);
  setShortListUpdateFreq(InteractionModel::SHORTFREQ);
  setLongListUpdateFreq(InteractionModel::LONGFREQ);
}

void InteractionModel::setDefaultGrid(FragmentSummary& fragSum)
{
  double dims[6];
  fragSum.getDimensions(dims);
  double maxSize = dims[1]-dims[0];
  if (maxSize < dims[3] - dims[2]) maxSize = dims[3]-dims[2];
  if (maxSize < dims[5] - dims[4]) maxSize = dims[5]-dims[4];
  int gridsize = InteractionModel::GRID;
  while (maxSize/((double)gridsize)>1.0) {
    gridsize *= 2;
  }
  setGridDimensions(gridsize);
}
