/**
 * @file
 *
 *
 */

#ifndef  COMPUTEMOCMD_H
#define  COMPUTEMOCMD_H

#include "viz/PropCmd.H"

#include "viz/SGContainer.H"

/**
 * Field types naming an electron density surface coloured by the
 * electrostatic potential.
 *
 * There are two, and the distinction is real rather than a quality
 * setting.  The canonical potential is the one a unit positive test
 * charge feels,
 *
 *     V(r) = sum_A Z_A/|r-R_A| - integral rho(r')/|r-r'| dr'
 *
 * whose second term is an integral over the electron density.  The
 * other replaces that integral with a sum over fitted atomic point
 * charges: far cheaper, wrong close to the surface where the density is
 * not point-like, and what most viewers show by default.
 *
 * So they are named for their METHOD, not "accurate" and "fast".  A
 * user who does not know the difference should still be able to read
 * which one produced the picture in their paper.
 *
 * Shared because MoPanel offers these and ComputeMoCmd acts on them,
 * and a field type that does not match between the two produces a plain
 * surface with no error anywhere -- the same two-hand-maintained-string
 * hazard as a dialog choice that does not match its generator.
 */
#define ESP_FIELD_TYPE         "Density (ESP)"
#define ESP_CHARGES_FIELD_TYPE "Density (ESP from atomic charges)"

class TAtm;

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

      virtual bool execute();

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

      /**
       * The electrostatic potential on the grid, for colouring the
       * density surface just computed.  Returns false if the user
       * interrupted it.
       */
      bool computeEsp(SingleGrid *grid, vector<TAtm*> *atoms,
                      unsigned long gridRes,
                      float xDelta, float yDelta, float zDelta);
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
