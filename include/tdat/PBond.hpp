///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PBond.H
//
//
// CLASS SYNOPSIS:
//
//      Represents a bond in a forcefield; provides access to its constituent
//      atoms.
//
// EXPORTED TYPES:
//    Class PBond
//
// DESCRIPTION:
//      State includes:
//      indices of constituent atoms
//      parameters for Harmonic oscillator represented by this bond
///////////////////////////////////////////////////////////////////////////////
#ifndef PBOND_HH
#define PBOND_HH

#include <vector>
  using std::vector;

#include "tdat/TBond.H"
#include "tdat/TAtm.H"
#include "tdat/Fragment.H"
#include "tdat/FFCoords.H"

class PBond
{

   public:

      enum Parameter {
         forceconst,    // 0
         equillength    // 1
      };
      // Constructors
      PBond( int atom1Index, int atom2Index);
      PBond(const PBond&);

      // Virtual Destructor
      virtual ~PBond(void);

      void setParams(double r12, double forceConst );

      // Accessors 
      int atom1Index(void) const;
      int atom2Index(void) const;

      // General Methods
      double energy(const double* c1, const double* c2) const;
      void forceVector(const double *c1, const double *c2, double *force);

   private:

      double*                  p_params;

      int                      p_atom1Index;
      int                      p_atom2Index;

}; // PBond

#endif /* PBOND_HH */
