/**
 * @file
 *
 *
 */
#ifndef POLYRATEMODEL_HH
#define POLYRATEMODEL_HH

#include <iostream>
  using std::istream;
  using std::ostream;

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "util/IOException.H"
#include "util/EcceSortedVector.H"

#include "tdat/ITaskModel.H"
#include "tdat/Fragment.H"


/**
 * Model for implementing NWChem DirDyVTST Editor.
 */
class PolyrateModel : public ITaskModel
{

   public:
      /* Default parameters */
      static const int MULTTS;
      static const int MULTR1;
      static const int MULTR2;
      static const int MULTP1;
      static const int MULTP2;
      static const bool FROPT;
      static const int RVRXEXP;
      static const int FRWSYM;
      static const int REVSYM;
      static const double TMPLST[];
      static const bool ANALYSIS;
      static const bool SADDLE;
      static const double FREQSCAL;
      static const int DEG1;
      static const int DEG2;
      static const int DEG3;
      static const double ES1;
      static const double ES2;
      static const double ES3;
      static const char DEFAX;

      /* Constructors */
      PolyrateModel();
      PolyrateModel(const string& url, const string& name);

      virtual ~PolyrateModel();

      /* ITaskModel virtuals */
      virtual void generateInputFile() throw(InvalidException);
      virtual void run() throw(InvalidException);
      void reset();

      /* Accessors */
      bool getFrwrdRevOpt() const;
      int getReverseRxScaleExp() const;
      int getForwardSymmetryFactor() const;
      int getReverseSymmetryFactor() const;
      double getTemperature(int idx) const;
      int getTemperatureListSize() const;
      bool getAnalysisFlag() const;
      bool getSaddleFlag() const;
      double getFrequencyScale() const;
      bool getComponentStatus(string component) const;
      int getDegeneracy1(string component) const;
      int getDegeneracy2(string component) const;
      int getDegeneracy3(string component) const;
      double getElectronicEnergy1(string component) const;
      double getElectronicEnergy2(string component) const;
      double getElectronicEnergy3(string component) const;
      bool getLinearStatus(string component) const;
      char getComponentAxis(string component) const;
      int getNumModes(string component) const;
      int getNumTorsionModes(string component) const;
      int getNumStretchModes(string component) const;
      int getMode(string component, int idx) const;
      int getModeIndex(string component, int mode) const;
      int getTorsionModeIndex(string component, int mode) const;
      string getAnharmonicity(string component, int mode) const;
      string getTorsion(string component, int mode) const;
      vector<string> getTorsionOpts(string component, int mode) const;
      int getNumAtoms(string component) const;

      /* Modifiers */
      void setFrwrdRevOpt(bool opt);
      void setReverseRxScaleExp(int exp);
      void setForwardSymmetryFactor(int factor);
      void setReverseSymmetryFactor(int factor);
      void addTemperature(double tmp);
      void deleteTemperature(int idx);
      void clearTemperature();
      void setAnalysisFlag(bool flag);
      void setSaddleFlag(bool flag);
      void setFrequencyScale(double scale);
      void setComponentStatus(string component, bool status);
      void setDegeneracy1(string component, int deg);
      void setDegeneracy2(string component, int deg);
      void setDegeneracy3(string component, int deg);
      void setElectronicEnergy1(string component, double nrg);
      void setElectronicEnergy2(string component, double nrg);
      void setElectronicEnergy3(string component, double nrg);
      void setLinearStatus(string component, bool isLinear);
      void setComponentAxis(string component, char axis);
      void setNumModes(string component, int num);
      void setMode(string component, int idx, int mode);
      void addMode(string component, int mode);
      void setAnharmonicity(string component, int mode, string anhrm);
      void setTorsion(string component, int mode, string torsion);
      void deleteMode(string component, int mode);
      void addTorsionOpt(string component, int mode, string opt);
      void deleteTorsionOpt(string component, int mode, int idx);
      void setNumAtoms(string component, int num);

      /* Utility functions */
      void translateTorsion(string cmd, int &mode, int &dmin, int &sym,
	                    string &scheme, string &level, int &ngrp);
      void translateTorsionOpt(string cmd, int &mode, string &type, string &values);
      bool checkTorsionOpts(string component, int mode);
      void translateMorse(string cmd, int &mode);

   private:
      int getComponentIndex(string component) const;

      bool p_frwrdRvrsOpt;
      int p_rvrsRxExp;
      int p_frwrdSymFac;
      int p_rvrsSymFac;
      bool p_analysis;
      bool p_saddle;
      double p_freqscal;
      double p_es1[5], p_es2[5], p_es3[5];
      int p_deg1[5], p_deg2[5], p_deg3[5];
      bool p_compStat[5];
      bool p_linStat[5];
      char p_axis[5];
      EcceSortedVector<double, less<double> > p_tmpList;
      int p_numAtoms[5];

      // Anharmonicity parameters
      int p_numModes[5];
      vector<int> p_modes[5];
      vector<string> p_anharmonicity[5];
      vector<string> p_torsions[5];
      vector<vector<string> > p_torsionOpts[5];
};
#endif
