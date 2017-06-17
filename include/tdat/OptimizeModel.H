/**
 * @file
 *
 *
 */
#ifndef OPTIMIZEMODEL_HH
#define OPTIMIZEMODEL_HH

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "util/IOException.H"


/**
 */
class OptimizeModel 
{

   public:
      /* Default values */
      static const bool USESD;
      static const int SDMAXITER;
      static const double SDISTEPSIZE;
      static const double SDTOLERANCE;
      static const double SDMAXSTEPSIZE;
      static const bool USECG;
      static const int CGMAXITER;
      static const double CGSEARCHINT;
      static const double CGTHRESHOLD;
      static const int CGREFRESHINT;

      /* Constructors */
      OptimizeModel();

      virtual ~OptimizeModel();

      /* Accessors */
      bool getUseSD() const;
      int getSDMaxIterations() const;
      double getSDInitialStepSize() const;
      double getSDTolerance() const;
      double getSDMaxStepSize() const;
      bool getUseCG() const;
      int getCGMaxIterations() const;
      double getCGInitialSearchInterval() const;
      double getCGThresholdSize() const;
      int getCGRefreshCycle() const;

      /* Modifiers */
      void setUseSD(bool flag);
      void setSDMaxIterations(int it);
      void setSDInitialStepSize(double size);
      void setSDTolerance(double tol);
      void setSDMaxStepSize(double size);
      void setUseCG(bool flag);
      void setCGMaxIterations(int it);
      void setCGInitialSearchInterval(double size);
      void setCGThresholdSize(double size);
      void setCGRefreshCycle(int cycle);

      /**
       * Reset model values to defaults
       */
      void reset();
   protected:

   private:
      bool p_useSD;
      int p_sdMaxIt;
      double p_sdISize;
      double p_sdTol;
      double p_sdMaxSize;

      bool p_useCG;
      int p_cgMaxIt;
      double p_cgISearch;
      double p_cgThresh;
      int p_cgRefresh;

};
#endif
