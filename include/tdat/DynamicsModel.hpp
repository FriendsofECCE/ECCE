/**
 * @file
 *
 *
 */
#ifndef DYNAMICSMODEL_HH
#define DYNAMICSMODEL_HH


#include <string>
#include <vector>
  using std::string;
  using std::vector;

#include "util/IOException.H"


/**
 */
class DynamicsModel
{

   public:
     /* Default values */
     static const int INTEGOPT;
     static const bool RESUME;
     static const int EQSTEPS;
     static const int DATASTEPS;
     static const double TIMESTEP;
     static const double INITTIME;
     static const bool RECENTER;
     static const int RECENTERFREQ;
     static const int RECENTEROPT;
     static const bool USEFRAC;
     static const int FRAC1;
     static const int FRAC2;
     static const int FRAC3;
     static const int FRAC4;
     static const int FRAC5;
     static const bool REMOVECM;
     static const int REMOVECMFREQ;
     static const bool USENVT;
     static const double NVTTEMP;
     static const bool USEDBLRELAX;
     static const double SLVRXTIME;
     static const double SLURXTIME;
     static const bool USEANNEALING;
     static const double FINALTEMP;
     static const double INITANNEALTIME;
     static const double FINALANNEALTIME;
     static const bool USENPT;
     static const double NPTPRSSR;
     static const double NPTRXTIME;
     static const double COMPRESSIBILITY;
     static const int VOLADJUSTALG;
     static const bool USEVELRETHERM;
     static const int RETHERMFREQ;
     static const double RETHERMTEMP;
     static const double RETHERMFRAC;
     static const int RETHERMOPT;

     /* Constructors */
     DynamicsModel();

     virtual ~DynamicsModel();

     /* Modifiers */
     void setIntegrationOpts(int opt);
     void setResumeOpt(bool flag);
     void setEquilibrationSteps(int steps);
     void setDataSteps(int steps);
     void setTimeStep(double step);
     void setInitialTime(double time);
     void setRecenterSystem(bool flag);
     void setRecenterFreq(int freq);
     void setRecenterOpts(int opt);
     void setUseFractions(bool flag);
     void setFraction1(int frac);
     void setFraction2(int frac);
     void setFraction3(int frac);
     void setFraction4(int frac);
     void setFraction5(int frac);
     void setRemoveCM(bool flag);
     void setRemoveCMFreq(int freq);
     void setUseNVT(bool flag);
     void setNVTTemperature(double temp);
     void setUseDoubleRelaxation(bool flag);
     void setSolventRxTime(double time);
     void setSoluteRxTime(double time);
     void setUseAnnealing(bool flag);
     void setFinalAnnealTemp(double temp);
     void setInitAnnealTime(double time);
     void setFinalAnnealTime(double time);
     void setUseNPT(bool flag);
     void setNPTPressure(double pres);
     void setNPTRxTime(double time);
     void setCompressibility(double cmpr);
     void setVolumeAdjustAlg(int alg);
     void setUseVelRetherm(bool flag);
     void setRethermFreq(int freq);
     void setRethermTemp(double temp);
     void setRethermFrac(double frac);
     void setRethermOpts(int opt);
     void setURL(string url);

     /* Accessors */
     int getIntegrationOpts() const;
     bool getResumeOpt() const;
     int getEquilibrationSteps() const;
     int getDataSteps() const;
     double getTimeStep() const;
     double getInitialTime() const;
     bool getRecenterSystem() const;
     int getRecenterFreq() const;
     int getRecenterOpts() const;
     bool getUseFractions() const;
     int getFraction1() const;
     int getFraction2() const;
     int getFraction3() const;
     int getFraction4() const;
     int getFraction5() const;
     bool getRemoveCM() const;
     int getRemoveCMFreq() const;
     bool getUseNVT() const;
     double getNVTTemperature() const;
     bool getUseDoubleRelaxation() const;
     double getSolventRxTime() const;
     double getSoluteRxTime() const;
     bool getUseAnnealing() const;
     double getFinalAnnealTemp() const;
     double getInitAnnealTime() const;
     double getFinalAnnealTime() const;
     bool getUseNPT() const;
     double getNPTPressure() const;
     double getNPTRxTime() const;
     double getCompressibility() const;
     int getVolumeAdjustAlg() const;
     bool getUseVelRetherm() const;
     int getRethermFreq() const;
     double getRethermTemp() const;
     double getRethermFrac() const;
     int getRethermOpts() const;
     string getURL() const;

     /**
      * Reset values in model to default values
      */
     void reset();
   protected:

   private:
     bool p_resumeOpt;
     int p_intOpt;
     int p_eqSteps;
     int p_dataSteps;
     double p_timeStep;
     double p_initTime;
     bool p_recenter;
     int p_rcntrFreq;
     int p_rcntrOpt;
     bool p_useFrac;
     int p_frac1, p_frac2, p_frac3, p_frac4, p_frac5;
     bool p_rmCM;
     int p_rmCMFreq;
     bool p_useNVT;
     double p_NVTTemp;
     bool p_useDoubleRelaxation;
     double p_solventRxTime;
     double p_soluteRxTime;
     bool p_useAnnealing;
     double p_finalTemp;
     double p_initAnnealTime;
     double p_finalAnnealTime;
     bool p_useNPT;
     double p_NPTPrssr;
     double p_NPTRxTime;
     double p_compressibility;
     int p_volAdjustAlg;
     bool p_useVelRthrm;
     int p_rthrmFreq;
     double p_rthrmTemp;
     double p_rthrmFrac;
     int p_rthrmOpt;
     string p_url;
};
#endif
