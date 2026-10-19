/**
 * @file
 *
 *
 */
#ifndef INTERACTIONMODEL_HH
#define INTERACTIONMODEL_HH

#include <string>
#include <vector>
  using std::string;
  using std::vector;

#include "util/IOException.H"

#include "tdat/ITaskModel.H"
#include "tdat/FragmentSummary.H"

/**
 * GROMACS-only run settings that every task of a study carries.  They are
 * saved in the Interaction section and only when they differ from the
 * defaults, so an NWChem task's file is untouched.  The defaults are all
 * "off": a default struct leaves the .mdp as it was.
 */
struct GromacsRun
{
   GromacsRun();
   bool operator==(const GromacsRun& o) const;
   bool operator!=(const GromacsRun& o) const { return !(*this == o); }

   string defines;       ///< text of the .mdp define line
   bool   posres;        ///< add -D<posresName> unless defines has it
   string posresName;
   string groups;        ///< lines "name = residue residue"
   int    comGroups;     ///< 0 System, 1 the defined groups
   int    tcGroups;      ///< 0 System, 1 the defined groups
   int    barostat;      ///< 0 C-rescale, 1 Berendsen, 2 Parrinello-Rahman
   int    pcoupltype;    ///< 0 isotropic, 1 semiisotropic, 2 anisotropic
   int    nstpcouple;    ///< 0: leave to grompp
   string extraMdp;      ///< lines written at the end of the .mdp

   /** Empty when grompp takes the barostat/type pair, else why not. */
   static string pairProblem(int barostat, int pcoupltype);
};

/**
 */
class InteractionModel
{
   public:
     /* Default values */
     static const int PARAMSET;
     static const bool PERTURBATIONSET1;
     static const bool PERTURBATIONSET2;
     static const int INTERACTOPT;
     static const int GRID;
     static const double EWALDCONVRG;
     static const int BSPLINEORDER;
     static const bool SHOWFFT;
     static const bool ADVANCEDFFT;
     static const int NFFT;
     static const int FFTOPTION;
     static const bool USESOLVENTONLY;
     static const int MAXPOLITERATION;
     static const double POLTOL;
     static const bool FORDERONLY;
     static const int CUTOFFOPTION;
     static const double SHORTCUT;
     static const double LONGCUT;
     static const int SHORTFREQ;
     static const int LONGFREQ;
     // -1: not chosen yet, 0: off, 1: on (GROMACS periodic-molecules)
     static const int PERIODICMOL;

     /* Constructors */
     InteractionModel();

     virtual ~InteractionModel();

     /* Accessors */
     int getParameterSet() const;
     bool getFirstPerturbationSet() const;
     bool getSecondPerturbationSet() const;
     int getInteractionOption() const;
     int getGridDimensions() const;
     double getEwaldConvergence() const;
     int getBSplineOrder() const;
     bool getAdvancedFFT() const;
     int getFFTNodes() const;
     int getFFTOption() const;
     bool getChargeGridOption() const;
     int getMaximumPolarIter() const;
     double getPolarizationTolerance() const;
     bool getFirstOrderOption() const;
     int getCutoffOption() const;
     double getSRCutoff() const;
     double getLRCutoff() const;
     int getShortListUpdateFreq() const;
     int getLongListUpdateFreq() const;
     int getPeriodicMolecules() const;
     bool getTouched() const;
     const GromacsRun& getGromacsRun() const;

     /* Modifiers */
     void setParameterSet(int opt);
     void setFirstPerturbationSet(bool opt);
     void setSecondPerturbationSet(bool opt);
     void setInteractionOption(int opt);
     void setGridDimensions(int grid);
     void setEwaldConvergence(double tol);
     void setBSplineOrder(int order);
     void setAdvancedFFT(bool opt);
     void setFFTNodes(int n);
     void setFFTOption(int opt);
     void setChargeGridOption(bool opt);
     void setMaximumPolarIter(int iter);
     void setPolarizationTolerance(double tol);
     void setFirstOrderOption(bool opt);
     void setCutoffOption(int opt);
     void setSRCutoff(double cutoff);
     void setLRCutoff(double cutoff);
     void setShortListUpdateFreq(int freq);
     void setLongListUpdateFreq(int freq);
     void setPeriodicMolecules(int opt);
     void setTouched(bool flag);
     void setGromacsRun(const GromacsRun& run);

     /**
      *  Reset model values to defaults
      */
     void reset();
     void setDefaultGrid(FragmentSummary& fragSum);
   protected:

   private:
     int p_paramSet;
     bool p_pert1, p_pert2;
     int p_intrctOpt;

     int p_grid;
     double p_ewaldConv;
     int p_eSpline;
     bool p_showFFT;
     bool p_advancedFFT;
     int p_nfft, p_fftOpt;
     bool p_useSolventOnly;

     int p_maxPolIter;
     double p_polTol;
     bool p_fOrder;

     int p_cutOpt;
     double p_sCut, p_lCut;
     int p_spFreq, p_lpFreq;
     int p_periodicMol;

     int p_touched;
     GromacsRun p_gmxRun;
};
#endif
