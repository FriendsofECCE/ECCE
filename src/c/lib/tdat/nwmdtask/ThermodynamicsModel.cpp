/**
 * @file
 *
 *
 */

#include "tdat/ThermodynamicsModel.H"

/* Default values */
const bool   ThermodynamicsModel::USEPMF = false;
const int    ThermodynamicsModel::PMFOPT = 0;
const int    ThermodynamicsModel::PMFEQUILHARM = 0;
const double ThermodynamicsModel::PMFSCALE = 0.0;
const bool   ThermodynamicsModel::USETHERMOINTEGRATION = false;
const int    ThermodynamicsModel::INITIALCONDITIONSOPTION = 0;
const int    ThermodynamicsModel::DIRECTIONOPTION = 0;
const int    ThermodynamicsModel::ENSEMBLESMAX = 21;
const int    ThermodynamicsModel::ENSEMBLESNUM = 21;
const double ThermodynamicsModel::ERRORMAX = 5.0;
const double ThermodynamicsModel::DRIFTMAX = 5.0;

/* Constructors */
ThermodynamicsModel::ThermodynamicsModel()
{
  reset();
}

ThermodynamicsModel::~ThermodynamicsModel()
{
}

/* Modifiers */
void ThermodynamicsModel::setUsePmf(bool val)
{
  p_usePmf = val;
}

void ThermodynamicsModel::setPmfOpt(int val)
{
  p_pmfOpt = val;
}

void ThermodynamicsModel::setPmfEquilharm(int val)
{
  p_pmfEquilharm = val;
}

void ThermodynamicsModel::setPmfScale(double val)
{
  p_pmfScale = val;
}

void ThermodynamicsModel::setUseThermoIntegration(bool val)
{
  p_useThermoIntegration = val;
}

void ThermodynamicsModel::setInitialConditionsOpt(int val)
{
  p_initialConditionsOpt = val;
}

void ThermodynamicsModel::setDirectionOpt(int val)
{
  p_directionOpt = val;
}

void ThermodynamicsModel::setEnsembleNumber(int val)
{
  p_ensembleNumber = val;
}

void ThermodynamicsModel::setEnsembleMax(int val)
{
  p_ensembleMax = val;
}

void ThermodynamicsModel::setErrorMax(double val)
{
  p_errorMax = val;
}

void ThermodynamicsModel::setDriftMax(double val)
{
  p_driftMax = val;
}


/* Accessors */
bool ThermodynamicsModel::getUsePmf() const
{
  return p_usePmf;
}

int ThermodynamicsModel::getPmfOpt() const
{
  return p_pmfOpt;
}

int ThermodynamicsModel::getPmfEquilharm() const
{
  return p_pmfEquilharm;
}

double ThermodynamicsModel::getPmfScale() const
{
  return p_pmfScale;
}

bool ThermodynamicsModel::getUseThermoIntegration() const
{
  return p_useThermoIntegration;
}

int ThermodynamicsModel::getInitialConditionsOpt() const
{
  return p_initialConditionsOpt;
}

int ThermodynamicsModel::getDirectionOpt() const
{
  return p_directionOpt;
}

int ThermodynamicsModel::getEnsembleNumber() const
{
  return p_ensembleNumber;
}

int ThermodynamicsModel::getEnsembleMax() const
{
  return p_ensembleMax;
}

double ThermodynamicsModel::getErrorMax() const
{
  return p_errorMax;
}

double ThermodynamicsModel::getDriftMax() const
{
  return p_driftMax;
}


/**
*  Reset all values in model back to default values.
*/
void ThermodynamicsModel::reset()
{
  p_usePmf = ThermodynamicsModel::USEPMF;
  p_pmfOpt = ThermodynamicsModel::PMFOPT;
  p_pmfEquilharm = ThermodynamicsModel::PMFEQUILHARM;
  p_pmfScale = ThermodynamicsModel::PMFSCALE;
  p_useThermoIntegration = ThermodynamicsModel::USETHERMOINTEGRATION;
  p_initialConditionsOpt = ThermodynamicsModel::INITIALCONDITIONSOPTION;
  p_directionOpt = ThermodynamicsModel::DIRECTIONOPTION;
  p_ensembleNumber = ThermodynamicsModel::ENSEMBLESNUM;
  p_ensembleMax = ThermodynamicsModel::ENSEMBLESMAX;
  p_errorMax = ThermodynamicsModel::ERRORMAX;
  p_driftMax = ThermodynamicsModel::DRIFTMAX;
} 

