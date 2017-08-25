/**
 * @file
 *
 *
 */

#ifndef  COMPUTEMOCMD_H
#define  COMPUTEMOCMD_H

#include "viz/PropCmd.H"

#include "viz/SGContainer.H"

class TGBSAngFunc;
class TGBSConfig;
class TGaussianBasisSet;
class JCode;
class ProgressMonitor;


class  ComputeMoCmd :  public  PropCmd
{
   public:
      ComputeMoCmd(const string& name, Receiver * receiver,
                   IPropCalculation *calc, ProgressMonitor *monitor);
      virtual ~ComputeMoCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();


      ComputeMoCmd();
      static int Glorder[35];
      static int Gmorder[35];
      static int Gnorder[35];
      static int NWlorder[35];
      static int NWmorder[35];
      static int NWnorder[35];
      static int HONlorder[35];
      static int HONmorder[35];
      static int HONnorder[35];
      static double pi;
      static double pi32;
      static double sNormFactor;
      static double pNormFactor;
      static double dNormFactor;
      static double fNormFactor;
      static double gNormFactor;
      static double hNormFactor;
      static double iNormFactor;
      float *makeField(int length);
      void zeroField(float *field,int length);
      bool squareCheckInterrupt(const char *type, int mo, int max);
      bool moCheckInterrupt(const char *type, const char *atom,
            const char *shell, int cur, int max, unsigned long MO);
      static void computeMinMax(float const *field, int size, float& min, 
            float& max, float& absMax);
      static double computeFieldValue(float const *field,int isize,
            float deltaX, float deltaY, float deltaZ);

      void setGrid (SingleGrid *grid, unsigned long& gridRes, float& xDelta,
            float& yDelta, float& zDelta);
      vector <TGaussianBasisSet*> normalize(string atomID, 
            vector <const TGaussianBasisSet*> gbslist,
            TGBSConfig& gbsConfig, const JCode* code);

      int getRequestedGridSize();
      static double getNormFactor(int shell_type);
      static double getalphapow(int shell_type);
      static double getoddNormalize(int shell_type, int index, TGBSAngFunc *angfunc);


      // State
      ProgressMonitor   *p_monitor;
      float             p_percentFraction;
};

#endif
