///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Inversion.H
//
//
// CLASS SYNOPSIS:
//      Represents an inversion angle in a forcefield, provides access to its
//      constituent atoms and bond orders
//
// EXPORTED TYPES:
//    Class Inversion
///////////////////////////////////////////////////////////////////////////////
#ifndef INVERSION_HH
#define INVERSION_HH

#include <float.h> // DBL_EPSILON
#include <math.h> 

#include <vector>
  using std::vector;

#include "tdat/TBond.H"

#define ONEMINUS5EPS (1.0 - 5.0*DBL_EPSILON)

class Inversion
{

public:

  // Constructors
        Inversion( int iatom1, int iatom2, int iatom3, int iatom4,
		 double obond12, double obond13, double obond14);
        Inversion(const Inversion&);

  // Virtual Destructor
        virtual ~Inversion(void);

  // Modifiers
        void setParams(double* params);

  // Accessors
       int atom1Index(void) const;
       int atom2Index(void) const;
       int atom3Index(void) const;
       int atom4Index(void) const;
       double bond21Order(void) const;
       double bond31Order(void) const;
       double bond41Order(void) const;

       double energy(const double *c1, const double *c2,
                     const double *c3, const double *c4);
       void forceVector(const double *c1, const double *c2,
                        const double *c3, const double *c4,
                        double* force);

  // State
       double      *p_params;

  // cached atom and bond indices (expensive to compute)
       int          p_atom1Index;                 
       int          p_atom2Index;                 
       int          p_atom3Index;                 
       int          p_atom4Index;                 
       double       p_bOrder21;
       double       p_bOrder31;
       double       p_bOrder41;

}; // Inversion

#endif /* INVERSION_HH */
