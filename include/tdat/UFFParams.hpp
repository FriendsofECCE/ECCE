///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: UFFParams.H
//
//
// CLASS SYNOPSIS:
//
// EXPORTED TYPES:
//    Class UFFParams
//
// LOCAL TYPES:
//    //<place locally used classes,typedefs here>
//
// SEE ALSO:
//    ForceField.H
//      
// DESCRIPTION:
//              <High level description of class, limitations...>
//      Changed to a class, rather than a bag of static functions.
//
//      Provides parameters for each
//      internal coordinate in the "Universal Force Field" (UFF) of Rappe' and 
//      Goddard (JACS 114(25) 10024 (1992).  The scalar forces here must
//      be multiplied by unit Cartesian vectors along each internal 
//      coordinate (provided by geometry routines in the FFCoordinates class)
//      to get contributions to a Cartesian force vector.  
//
//      The sets of Torsions around a bond and Inversions around an atom
//      vary among forcefields (some include one; most, like the UFF,
//      include all), so these sets are actually generated here. 
//
// EXAMPLES:
//     In the test directory .../src/db/ff/test, see
//     the test program testForceField.C.
//
// TO DO:
//   * Use TAtm::Shapes enumeration to identify geometries throughout.
//   * Fix casts of Center* to TAtm* when TAtm go away
//   * Cache unique parameters sets in ForceField, rather than generating
//     a new set for each internal coordinate.
//   * Move all functions but the essential API to be protected:
//   * cache atom types in state
//
///////////////////////////////////////////////////////////////////////////////
#ifndef UFFPARAMS_HH
#define UFFPARAMS_HH

#include <iostream>
  using std::ostream;

#include <vector>
  using std::vector;


class TAtm;

class UFFParams
{

   public:

      UFFParams(const vector<TAtm*>* atmPtrs);

      // Virtual Destructor  (moved back from protected)
      virtual ~UFFParams(void);

      // Accessors
      double partialCharge( int iatom) const;
      double displayRadii( int iatom) const;
      bool   isGroup5( int atomicNumber ) const;
      bool   isGroup6( int atomicNumber ) const;
      bool   isMetal( int atomicNumber ) const;
      int    period(  int atomicNumber ) const;
      bool   isResonant( int iatom ) const;
      bool   hasDouble( int iatom ) const;
      bool   hasSp2Neighbor( int iatom ) const;
      double vsp3( int atomicNumber ) const;

      char*  atomType( int uffTypeIndex ) const;
      double covRadius( int uffTypeIndex ) const;
      double bondAngle( int uffTypeIndex ) const;
      double vdWRadius( int uffTypeIndex ) const;
      double vdWEnergy( int uffTypeIndex ) const;
      double vdWScale( int uffTypeIndex ) const;
      double effCharge( int uffTypeIndex ) const;
      double electroneg( int uffTypeIndex ) const;

      ////////////////////////////////////////////////////////////////////////
      // debug functions to watch for overwrite of static data
      ////////////////////////////////////////////////////////////////////////
      bool staticsOK(void) const;
      ostream& dumpStatics(ostream& os) const;

      int uffIndex( int iatom ) const;

      int initUFFIndex( int iatom ) const;

      double r12( int uffTypeIndex1,
            int uffTypeIndex2,
            double bondOrder = 1.0) const;

   protected:

   private:

      // State

      // from ForceField
      const vector<TAtm*>* p_atmPtrs;

      int*                 p_atomTypeList;
      double*              p_partialCharge;
      double*              p_displayRadii;

      // Encapsulated Behavior
      void                 initAtomTypeList( void );
      void                 initPartialCharges( void );
      void                 initDisplayRadii( void );

      // Friends
      friend ostream& operator<< (ostream& os, const UFFParams&);

}; // UFFParams

#endif /* UFFPARAMS_HH */
