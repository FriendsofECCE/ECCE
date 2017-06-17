///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Torsion.H
//
//
// CLASS SYNOPSIS:
//      Represents a torsion angle in a forcefield, provides access to its
//      constituent atoms and bond orders
//
// EXPORTED TYPES:
//    Class Torsion
//
// SEE ALSO:
//    ForceField.H
//      
// DESCRIPTION:
//      State consists of constituant atoms, bond orders of the bonds forming
//      the torsion, and the parameters associated with corresponding
//      potential function.
///////////////////////////////////////////////////////////////////////////////
#ifndef TORSION_HH
#define TORSION_HH

#include <float.h> // DBL_EPSILON
#include <math.h> 

#include <vector>
  using std::vector;

#include "tdat/TBond.H"

#define ONEMINUS5EPS (1.0 - 5.0*DBL_EPSILON)

class Torsion
{

public:

  // Constructors
        Torsion( int iatom1, int iatom2, int iatom3, int iatom4,
		 double obond12, double obond32, double obond43);
        Torsion(const Torsion&);

  // Virtual Destructor
        virtual ~Torsion(void);

  // Modifiers
        void setParams(double* params);

  // Accessors
       int atom1Index(void) const;
       int atom2Index(void) const;
       int atom3Index(void) const;
       int atom4Index(void) const;
       double bond12Order(void) const;
       double bond32Order(void) const;
       double bond43Order(void) const;

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
       double       p_bOrder12;
       double       p_bOrder32;
       double       p_bOrder43;

}; // Torsion

#endif /* TORSION_HH */
