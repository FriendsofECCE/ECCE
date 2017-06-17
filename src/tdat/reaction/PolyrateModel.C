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

#include "tdat/PolyrateModel.H"

// Default values
const bool PolyrateModel::FROPT = true;
const int PolyrateModel::RVRXEXP = 0;
const int PolyrateModel::FRWSYM = 1;
const int PolyrateModel::REVSYM = 1;
const double PolyrateModel::TMPLST[] = {298.0, 300.0, 400.0, 600.0, 1000.0,
                                        1500.0, 2400.0};
const bool PolyrateModel::ANALYSIS = false;
const bool PolyrateModel::SADDLE = true;
const double PolyrateModel::FREQSCAL = 1.0;
const int PolyrateModel::DEG1 = 1;
const int PolyrateModel::DEG2 = 0;
const int PolyrateModel::DEG3 = 0;
const double PolyrateModel::ES1 = 0.0;
const double PolyrateModel::ES2 = 0.0;
const double PolyrateModel::ES3 = 0.0;
const char PolyrateModel::DEFAX = 'Z';
/**
 * Constructor
 */
PolyrateModel::PolyrateModel() : ITaskModel()
{
   setTitle("POLYRATE VTST Reaction Rate");
   setUrl("");
   reset();
}


/**
 * Constructor
 */
PolyrateModel::PolyrateModel(const string& url, const string& name) : ITaskModel()
{
   setTitle("POLYRATE VTST Reaction Rate");
   setUrl(url);
   reset();
}

void PolyrateModel::reset()
{
   p_frwrdRvrsOpt = FROPT;
   p_rvrsRxExp = RVRXEXP;
   p_frwrdSymFac = FRWSYM;
   p_rvrsSymFac = REVSYM;
   p_analysis = ANALYSIS;
   p_saddle = SADDLE;
   p_freqscal = FREQSCAL;
   int i;
   for (i=0; i<5; i++) {
     p_compStat[i] = false;
     p_deg1[i] = DEG1;
     p_deg2[i] = DEG2;
     p_deg3[i] = DEG3;
     p_es1[i] = ES1;
     p_es2[i] = ES2;
     p_es3[i] = ES3;
     p_linStat[i] = false;
     p_axis[i] = DEFAX;
     p_numModes[i] = 0;
     (p_modes[i]).clear();
     (p_anharmonicity[i]).clear();
     (p_torsions[i]).clear();
     (p_torsionOpts[i]).clear();
   }

   p_tmpList.clear();
   int numtmp = sizeof(TMPLST)/sizeof(double);
   for (int it=0; it<numtmp; it++)
     p_tmpList.push_back(TMPLST[it]);
}

/**
 * Destructor
 */
PolyrateModel::~PolyrateModel()
{
}

void PolyrateModel::generateInputFile() throw(InvalidException)
{
   throw NotImplementedException("generateInputFile not implemented",WHERE);
}


void PolyrateModel::run() throw(InvalidException)
{
   throw NotImplementedException("run not implemented",WHERE);
}

/**
 * Accessors
 */

bool PolyrateModel::getFrwrdRevOpt() const
{
  return p_frwrdRvrsOpt;
}

int PolyrateModel::getReverseRxScaleExp() const
{
  return p_rvrsRxExp;
}

int PolyrateModel::getForwardSymmetryFactor() const
{
  return p_frwrdSymFac;
}

int PolyrateModel::getReverseSymmetryFactor() const
{
  return p_rvrsSymFac;
}

double PolyrateModel::getTemperature(int idx) const
{
  return p_tmpList[idx];
}

int PolyrateModel::getTemperatureListSize() const
{
  return p_tmpList.size();
}

bool PolyrateModel::getAnalysisFlag() const
{
  return p_analysis;
}
bool PolyrateModel::getSaddleFlag() const
{
  return p_saddle;
}
double PolyrateModel::getFrequencyScale() const
{
  return p_freqscal;
}
bool PolyrateModel::getComponentStatus(string component) const
{
  bool ret = false;
  ret = p_compStat[getComponentIndex(component)];
  return ret;
}
int PolyrateModel::getDegeneracy1(string component) const
{
  int ret = 0;
  ret = p_deg1[getComponentIndex(component)];
  return ret;
}
int PolyrateModel::getDegeneracy2(string component) const
{
  int ret = 0;
  ret = p_deg2[getComponentIndex(component)];
  return ret;
}
int PolyrateModel::getDegeneracy3(string component) const
{
  int ret = 0;
  ret = p_deg3[getComponentIndex(component)];
  return ret;
}
double PolyrateModel::getElectronicEnergy1(string component) const
{
  double ret = 0;
  ret = p_es1[getComponentIndex(component)];
  return ret;
}
double PolyrateModel::getElectronicEnergy2(string component) const
{
  double ret = 0;
  ret = p_es2[getComponentIndex(component)];
  return ret;
}
double PolyrateModel::getElectronicEnergy3(string component) const
{
  double ret = 0;
  ret = p_es3[getComponentIndex(component)];
  return ret;
}
bool PolyrateModel::getLinearStatus(string component) const
{
  bool ret = false;
  ret = p_linStat[getComponentIndex(component)];
  return ret;
}
char PolyrateModel::getComponentAxis(string component) const
{
  char ret = '\0';
  ret = p_axis[getComponentIndex(component)];
  return ret;
}
int PolyrateModel::getNumModes(string component) const
{
  return p_numModes[getComponentIndex(component)];
}
int PolyrateModel::getNumTorsionModes(string component) const
{
  int ret = 0;
  int i;
  int icomp = getComponentIndex(component);
  int nmodes = p_numModes[icomp];
  for (i=0; i<nmodes; i++) {
    if ((p_anharmonicity[icomp])[i] == "tor") ret++;
  }
  return ret;
}
int PolyrateModel::getNumStretchModes(string component) const
{
  int ret = 0;
  int i;
  int icomp = getComponentIndex(component);
  int nmodes = p_numModes[icomp];
  for (i=0; i<nmodes; i++) {
    if ((p_anharmonicity[icomp])[i] == "morse") ret++;
  }
  return ret;
}
int PolyrateModel::getMode(string component, int idx) const
{
  return (p_modes[getComponentIndex(component)])[idx];
}
int PolyrateModel::getModeIndex(string component, int mode) const
{
  int icmp = getComponentIndex(component);
  int numModes = p_numModes[icmp];
  int i, idx;
  for (i=0; i<numModes; i++) {
    if ((p_modes[icmp])[i] == mode) {
      idx = i;
      break;
    }
  }
  return idx;
}
int PolyrateModel::getTorsionModeIndex(string component, int mode) const
{
  int icomp = getComponentIndex(component);
  int numModes = p_numModes[icomp];
  int i, idx, icnt;
  icnt = 0;
  idx = -1;
  for (i=0; i<numModes; i++) {
    if (p_anharmonicity[icomp][i] == "tor") {
      if ((p_modes[icomp])[i] == mode) {
        idx = i;
        break;
      }
      icnt++;
    }
  }
  return idx;
}
string PolyrateModel::getAnharmonicity(string component, int mode) const
{
  int icmp = getComponentIndex(component);
  int numModes = p_numModes[icmp];
  int i, idx;
  string ret;
  for (i=0; i<numModes; i++) {
    if ((p_modes[icmp])[i] == mode) {
      idx = i;
      break;
    }
  }
  return (p_anharmonicity[icmp])[idx];
}
string PolyrateModel::getTorsion(string component, int mode) const
{
  int icmp = getComponentIndex(component);
  int numModes = p_numModes[icmp];
  int i;
  string ret;
  for (i=0; i<numModes; i++) {
    if ((p_modes[icmp])[i] == mode) {
      ret = (p_torsions[icmp])[i];
      break;
    }
  }
  return ret;
}

vector<string> PolyrateModel::getTorsionOpts(string component, int mode) const
{
  vector<string> ret;
  int icomp = getComponentIndex(component);
  int idx = getModeIndex(component, mode);
  ret = (p_torsionOpts[icomp])[idx];
  return ret;
}

int PolyrateModel::getNumAtoms(string component) const
{
  int icomp = getComponentIndex(component);
  return p_numAtoms[icomp];
}

/**
 * Modifiers
 */

void PolyrateModel::setFrwrdRevOpt(bool opt)
{
  p_frwrdRvrsOpt = opt;
}

void PolyrateModel::setReverseRxScaleExp(int exp)
{
  p_rvrsRxExp = exp;
}

void PolyrateModel::setForwardSymmetryFactor(int factor)
{
  p_frwrdSymFac = factor;
}

void PolyrateModel::setReverseSymmetryFactor(int factor)
{
  p_rvrsSymFac = factor;
}

void PolyrateModel::addTemperature(double temp)
{
  p_tmpList.push_back(temp);
}

void PolyrateModel::deleteTemperature(int idx)
{
  vector<double>::iterator it;
  it = p_tmpList.begin();
  for (int i=0; i<idx; i++) {
    it++;
  }
  p_tmpList.erase(it);
}

void PolyrateModel::clearTemperature()
{
  p_tmpList.clear();
}

void PolyrateModel::setAnalysisFlag(bool flag)
{
  p_analysis = flag;
}

void PolyrateModel::setSaddleFlag(bool flag)
{
  p_saddle = flag;
}

void PolyrateModel::setFrequencyScale(double scale)
{
  p_freqscal = scale;
}

void PolyrateModel::setComponentStatus(string component, bool status)
{
  p_compStat[getComponentIndex(component)] = status;
}

void PolyrateModel::setDegeneracy1(string component, int deg)
{
  p_deg1[getComponentIndex(component)] = deg;
}

void PolyrateModel::setDegeneracy2(string component, int deg)
{
  p_deg2[getComponentIndex(component)] = deg;
}

void PolyrateModel::setDegeneracy3(string component, int deg)
{
  p_deg3[getComponentIndex(component)] = deg;
}

void PolyrateModel::setElectronicEnergy1(string component, double nrg)
{
  p_es1[getComponentIndex(component)] = nrg;
}

void PolyrateModel::setElectronicEnergy2(string component, double nrg)
{
  p_es2[getComponentIndex(component)] = nrg;
}

void PolyrateModel::setElectronicEnergy3(string component, double nrg)
{
  p_es3[getComponentIndex(component)] = nrg;
}

void PolyrateModel::setLinearStatus(string component, bool status)
{
  p_linStat[getComponentIndex(component)] = status;
}

void PolyrateModel::setComponentAxis(string component, char axis)
{
  p_axis[getComponentIndex(component)] = axis;
}

void PolyrateModel::setNumModes(string component, int num)
{
  p_numModes[getComponentIndex(component)] = num;
}

void PolyrateModel::setMode(string component, int idx, int mode)
{
  (p_modes[getComponentIndex(component)])[idx] = mode;
}

void PolyrateModel::addMode(string component, int mode)
{
  (p_modes[getComponentIndex(component)]).push_back(mode);
  p_numModes[getComponentIndex(component)]++;
  (p_anharmonicity[getComponentIndex(component)]).push_back("");
  p_torsions[getComponentIndex(component)].push_back("");
  vector<string> opt;
  (p_torsionOpts[getComponentIndex(component)]).push_back(opt);
}

void PolyrateModel::setAnharmonicity(string component, int mode, string anhrm)
{
  int icmp = getComponentIndex(component);
  int numModes = p_numModes[icmp];
  int i;
  for (i=0; i<numModes; i++) {
    if ((p_modes[icmp])[i] == mode) {
      (p_anharmonicity[icmp])[i] = anhrm;
    }
  }
}

void PolyrateModel::setTorsion(string component, int mode, string torsion)
{
  int icmp = getComponentIndex(component);
  int numModes = p_numModes[icmp];
  int i;
  for (i=0; i<numModes; i++) {
    if ((p_modes[icmp])[i] == mode) {
      p_torsions[icmp][i] = torsion;
      break;
    }
  }
}

void PolyrateModel::deleteMode(string component, int mode)
{
  int i, idx, icomp;
  idx = -1;
  icomp = getComponentIndex(component);
  for (i=0; i<p_numModes[icomp]; i++) {
    if ((p_modes[icomp])[i] == mode) {
      idx = i;
      break;
    }
  }
  if (idx >= 0) {
    vector<int>::iterator itr_md;
    vector<string>::iterator itr_trsn, itr_ah;
    vector<vector<string> >::iterator itr_opts;
    itr_md = p_modes[icomp].begin()+idx;
    itr_ah = p_anharmonicity[icomp].begin()+idx;
    itr_trsn = p_torsions[icomp].begin()+idx;
    itr_opts = p_torsionOpts[icomp].begin()+idx;
    p_modes[icomp].erase(itr_md);
    p_anharmonicity[icomp].erase(itr_ah);
    p_torsions[icomp].erase(itr_trsn);
    p_torsionOpts[icomp].erase(itr_opts);
    p_numModes[icomp]--;
  }
}

void PolyrateModel::addTorsionOpt(string component, int mode, string opt)
{
  int icomp = getComponentIndex(component);
  int idx = getModeIndex(component, mode);
  (p_torsionOpts[icomp])[idx].push_back(opt);
}

void PolyrateModel::deleteTorsionOpt(string component, int mode, int idx)
{
  int icomp = getComponentIndex(component);
  int imode = getModeIndex(component, mode);
  vector<string>::iterator itr = (p_torsionOpts[icomp])[imode].begin();
  itr+=idx;
  (p_torsionOpts[icomp])[imode].erase(itr);
}

void PolyrateModel::setNumAtoms(string component, int num)
{
  int icomp = getComponentIndex(component);
  p_numAtoms[icomp] = num;
}


/**
 * Utility functions
 */
int PolyrateModel::getComponentIndex(string component) const
{
  int ret = 0;
  if (component == "react1") {
    ret = 0;
  } else if (component == "react2") {
    ret = 1;
  } else if (component == "transition") {
    ret = 2;
  } else if (component == "prod1") {
    ret = 3;
  } else if (component == "prod2") {
    ret = 4;
  }
  return ret;
}

void PolyrateModel::translateTorsion(string cmd, int &mode, int &dmin, int &sym,
                                     string &scheme, string &level, int &ngrp)
{
  StringTokenizer tokenizer(cmd);
  mode = atoi(tokenizer.next().c_str());
  dmin = atoi(tokenizer.next().c_str());
  sym = atoi(tokenizer.next().c_str());
  scheme = tokenizer.next();
  level = tokenizer.next();
  ngrp = atoi(tokenizer.next().c_str());
}

void PolyrateModel::translateMorse(string cmd, int &mode)
{
  StringTokenizer tokenizer(cmd);
  mode = atoi(tokenizer.next().c_str());
}

void PolyrateModel::translateTorsionOpt(string cmd, int &mode, string &type,
                                        string &values)
{
  StringTokenizer tokenizer(cmd);
  vector<string> tokens = tokenizer.tokenize(" \t");
  if (tokens.size() > 0) {
    mode = atoi(tokens[0].c_str());
  } else {
    mode = 0;
  }
  if (tokens.size() > 1) {
    type = tokens[1];
  } else {
    type = "";
  }
  if (tokens.size() > 2) {
    values = tokens[2];
    for (int i = 3; i<(int)tokens.size(); i++) {
      values.append(" ");
      values.append(tokens[i]);
    }
  } else {
    values = "";
  }
}

/**
 * This is a utility function to check and see if all the torsion options
 * associated with a given anharmonic torsion mode have been specified
 */
bool PolyrateModel::checkTorsionOpts(string component, int mode)
{
  bool ret = true;
  int icomp = getComponentIndex(component);
  int imode = getModeIndex(component, mode);
  vector<string> opts = (p_torsionOpts[icomp])[imode];
  bool omega = false;
  bool uu = false;
  bool ii = false;
  bool sigma = false;
  bool ww = false;
  bool isb = false;
  bool nbond = false;
  int i, tmode;
  string type, values;
  string opt;
  for (i=0; i<(int)opts.size(); i++) {
    translateTorsionOpt(opts[i], tmode, type, values);
    if (type == "OMEGA") {
      omega = true;
    } else if (type == "U") {
      uu = true;
    } else if (type == "I") {
      ii = true;
    } else if (type == "SIGMA") {
      sigma = true;
    } else if (type == "W") {
      ww = true;
    } else if (type == "ISB") {
      isb = true;
    } else if (type == "NBOND") {
      nbond = true;
    }
  }
  string cmd = getTorsion(component, mode);
  int dmin, sym, ngrp;
  string scheme, level;
  translateTorsion(cmd, tmode, dmin, sym, scheme, level, ngrp);
  if (dmin == 1) {
    if (scheme == "OW" && !ww) {
      ret = false;
    } else if (scheme == "RO" && !isb) {
      ret = false;
    } else if (scheme == "CO" && (!isb || !nbond)) {
      ret = false;
    } else if (scheme == "RW" && (!isb || !ww)) {
      ret = false;
    } else if (scheme == "CW" && (!isb || !nbond || !ww)) {
      ret = false;
    }
  } else if (dmin > 1) {
    if (level == "FULL") {
      if (sigma && uu) {
        if (scheme == "OW") {
          if (!omega || !ww) {
            ret = false;
          }
        } else {
          if (ii && isb) {
            if (scheme == "RO" && !omega) {
              ret = false;
            } else if (scheme == "CO" && (!nbond || !omega)) {
              ret = false;
            } else if (scheme == "RW" && !ww) {
              ret = false;
            } else if (scheme == "CW" && (!nbond || !ww)) {
              ret = false;
            }
          } else {
            ret = false;
          }
        }
      } else {
        ret = false;
      }
    } else if (level == "SF") {
      if (sigma && uu) {
        if (scheme == "OW") {
          if (!ww) {
            ret = false;
          }
        } else {
          if (ii && isb) {
            if (scheme == "CO" && !nbond) {
              ret = false;
            } else if (scheme == "RW" && !ww) {
              ret = false;
            } else if (scheme == "CW" && (!nbond || !ww)) {
              ret = false;
            }
          } else {
            ret = false;
          }
        }
      } else {
        ret = false;
      }
    } else if (level == "SC") {
      if (sigma) {
        if (scheme == "OW") {
          if (!ww) {
            ret = false;
          }
        } else {
          if (isb) {
            if (scheme == "CO" && !nbond) {
              ret = false;
            } else if (scheme == "RW" && !ww) {
              ret = false;
            } else if (scheme == "CW" && (!nbond || !ww)) {
              ret = false;
            }
          } else {
            ret = false;
          }
        }
      } else {
        ret = false;
      }
    }
  } else {
    ret = false;
  }
  return ret;
}
