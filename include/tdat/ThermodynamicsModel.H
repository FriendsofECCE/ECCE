/**
 * @file
 *
 *
 */
#ifndef THERMODYNAMICSMODEL_HH
#define THERMODYNAMICSMODEL_HH

/**
 */
class ThermodynamicsModel 
{

   public:
      /* Default values */
      static const bool USEPMF;
      static const int PMFOPT;
      static const int PMFEQUILHARM;
      static const double PMFSCALE;
      static const bool USETHERMOINTEGRATION;
      static const int INITIALCONDITIONSOPTION;
      static const int DIRECTIONOPTION;
      static const int ENSEMBLESNUM;
      static const int ENSEMBLESMAX;
      static const double ERRORMAX;
      static const double DRIFTMAX;


      /* Constructors */
      ThermodynamicsModel();

      virtual ~ThermodynamicsModel();

      /* Modifiers */
      void setUsePmf(bool val);
      void setPmfOpt(int val);
      void setPmfEquilharm(int val);
      void setPmfScale(double val);
      void setUseThermoIntegration(bool val);
      void setInitialConditionsOpt(int val);
      void setDirectionOpt(int val);
      void setEnsembleNumber(int val);
      void setEnsembleMax(int val);
      void setErrorMax(double val);
      void setDriftMax(double val);

      /* Accessors */
      bool   getUsePmf() const;
      int    getPmfOpt() const;
      int    getPmfEquilharm() const;
      double getPmfScale() const;
      bool   getUseThermoIntegration() const;
      int    getInitialConditionsOpt() const;
      int    getDirectionOpt() const;
      int    getEnsembleNumber() const;
      int    getEnsembleMax() const;
      double getErrorMax() const;
      double getDriftMax() const;

      /**
       *  Reset all values in model back to default values.
       */
      void reset();

   protected:

   private:
      bool   p_usePmf;
      int    p_pmfOpt;
      int    p_pmfEquilharm;
      double p_pmfScale;
      bool   p_useThermoIntegration;
      int    p_initialConditionsOpt;
      int    p_directionOpt;
      int    p_ensembleNumber;
      int    p_ensembleMax;
      double p_errorMax;
      double p_driftMax;

};
#endif // THERMODYNAMICSMODEL_HH

