/**
 * @file
 *
 *
 */

#include "util/NotImplementedException.H"
#include "util/StringTokenizer.H"
#include "util/STLUtil.H"

#include "tdat/OptimizeModel.H"

//Default values
const bool OptimizeModel::USESD = true;
const int OptimizeModel::SDMAXITER = 100;
const double OptimizeModel::SDISTEPSIZE = 0.001;
const double OptimizeModel::SDTOLERANCE = 0.0001;
const double OptimizeModel::SDMAXSTEPSIZE = 0.05;
const bool OptimizeModel::USECG = true;
const int OptimizeModel::CGMAXITER = 100;
const double OptimizeModel::CGSEARCHINT = 0.001;
const double OptimizeModel::CGTHRESHOLD = 0.0001;
const int OptimizeModel::CGREFRESHINT = 10;

/**
 * Constructor
 */
OptimizeModel::OptimizeModel()
{
  p_useSD =USESD;
  p_sdMaxIt = SDMAXITER;
  p_sdISize = SDISTEPSIZE;
  p_sdTol = SDTOLERANCE;
  p_sdMaxSize = SDMAXSTEPSIZE;
  p_useCG =USECG;
  p_cgMaxIt = CGMAXITER;
  p_cgISearch = CGSEARCHINT;
  p_cgThresh = CGTHRESHOLD;
  p_cgRefresh = CGREFRESHINT;
}



/**
 * Destructor
 */
OptimizeModel::~OptimizeModel()
{
}

/**
 * Accessors
 */
bool OptimizeModel::getUseSD() const
{
  return p_useSD;
}

int OptimizeModel::getSDMaxIterations() const
{
  return p_sdMaxIt;
}

double OptimizeModel::getSDInitialStepSize() const
{
  return p_sdISize;
}

double OptimizeModel::getSDTolerance() const
{
  return p_sdTol;
}

double OptimizeModel::getSDMaxStepSize() const
{
  return p_sdMaxSize;
}

bool OptimizeModel::getUseCG() const
{
  return p_useCG;
}

int OptimizeModel::getCGMaxIterations() const
{
  return p_cgMaxIt;
}

double OptimizeModel::getCGInitialSearchInterval() const
{
  return p_cgISearch;
}

double OptimizeModel::getCGThresholdSize() const
{
  return p_cgThresh;
}

int OptimizeModel::getCGRefreshCycle() const
{
  return p_cgRefresh;
}

/**
 * Modifiers
 */
void OptimizeModel::setUseSD(bool flag)
{
  p_useSD = flag;
}

void OptimizeModel::setSDMaxIterations(int it)
{
  p_sdMaxIt = it;
}

void OptimizeModel::setSDInitialStepSize(double size)
{
  p_sdISize = size;
}

void OptimizeModel::setSDTolerance(double tol)
{
  p_sdTol = tol;
}

void OptimizeModel::setSDMaxStepSize(double size)
{
  p_sdMaxSize = size;
}

void OptimizeModel::setUseCG(bool flag)
{
  p_useCG = flag;
}

void OptimizeModel::setCGMaxIterations(int it)
{
  p_cgMaxIt = it;
}

void OptimizeModel::setCGInitialSearchInterval(double size)
{
  p_cgISearch = size;
}

void OptimizeModel::setCGThresholdSize(double size)
{
  p_cgThresh = size;
}

void OptimizeModel::setCGRefreshCycle(int it)
{
  p_cgRefresh = it;
}

void OptimizeModel::reset()
{
  setUseSD(OptimizeModel::USESD);
  setSDMaxIterations(OptimizeModel::SDMAXITER);
  setSDInitialStepSize(OptimizeModel::SDISTEPSIZE);
  setSDTolerance(OptimizeModel::SDTOLERANCE);
  setSDMaxStepSize(OptimizeModel::SDMAXSTEPSIZE);
  setUseSD(OptimizeModel::USECG);
  setCGMaxIterations(OptimizeModel::CGMAXITER);
  setCGInitialSearchInterval(OptimizeModel::CGSEARCHINT);
  setCGThresholdSize(OptimizeModel::CGTHRESHOLD);
  setCGRefreshCycle(OptimizeModel::CGREFRESHINT);
}
