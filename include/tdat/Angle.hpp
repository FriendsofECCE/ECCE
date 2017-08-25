///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Angle.H
//
//
// CLASS SYNOPSIS:
//
// EXPORTED TYPES:
//    Class Angle
//
// SEE ALSO:
//      ForceField.H
//
// DESCRIPTION:
//      Represents a bond angle in a forcefield, provides access to its
//      constituent atoms and the parameters that characterize its contrubion
//      to the potential energy. Also provides methods for evaluating energy
//      and forces due to a single angle potential.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef ANGLE_HH
#define ANGLE_HH

#include <float.h> // DBL_EPSILON
#include <math.h>  // fabs()

#include <vector>
  using std::vector;

#include "tdat/ForceField.H"

// used in expressions like sqrt(1-a*a*ONEMINUS5EPS) to avoid sqrt(-DBL_EPSILON)
#define ONEMINUS5EPS (1.0 - 5.0*DBL_EPSILON)

#define  PI  3.14159265358979323846
#define  RAD * 180.0 / PI

class Angle
{
   public:

      // Constructors
      Angle( int iatom1, int iatom2, int iatom3,
            double obond1, double obond2);
      Angle(Angle&);

      // Virtual Destructor
      virtual ~Angle(void);

      // Modifiers
      void setParams( double* params );

      // Accessors
      int atom1Index(void) const;
      int atom2Index(void) const;
      int atom3Index(void) const;
      double bondOrder1(void) const;
      double bondOrder2(void) const;

      double cosine(const double *c1, const double *c2,
            const double *c3) const;
      double sine(const double *c1, const double *c2,
            const double *c3) const;

      double energy(const double *c1, const double *c2, const double *c3);
      void forceVector(const double *c1, const double *c2,
            const double *c3, double* force);

   private: 
      double           *p_params;
      int               p_atom1Index;                 
      int               p_atom2Index;                 
      int               p_atom3Index;                 
      double            p_bOrder1;
      double            p_bOrder2;

}; // Angle

#endif /* ANGLE_HH */
