/**
 * @file 
 *
 *
 */
#ifndef DIAG3_HH
#define DIAG3_HH

/**
 * Diag3 Calculates the eigenvalues and eigenvectors of symmetric 3x3 matrices.
 */
class Diag3
{

   public:
      Diag3(double xx, double xy, double xz, 
            double yy, double yz, double zz);

      virtual ~Diag3();

      void calculate(double ew[3], double e0[3], double e1[3], double e2[3]);

   protected:

      double p_xx;
      double p_xy;
      double p_xz;
      double p_yy;
      double p_yz;
      double p_zz;

}; 

#endif
