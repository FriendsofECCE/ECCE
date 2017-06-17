///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TRefIsotope.H
//
//
// CLASS SYNOPSIS:
//   TRefIsotopes Objects Contain Chart of the Nuclides Reference Information.
//
// EXPORTED TYPES:
//    Class TRefIsotope
//
// LOCAL TYPES:
//
// SEE ALSO:
//      
// DESCRIPTION:
//   Class TRefIsotope represents library-type reference information about
//   common isotopes.
//   The collection of all TRefIsotopes form the Chart of the Nuclides.
//
// EXAMPLES:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TREFISOTOPE_HH
#define TREFISOTOPE_HH

#include <iostream>
  using std::ostream;

#include <string>
  using std::string;


class TRefIsotope 
{

   public:

      // Constructors - not supported for reference information

      // Virtual Destructor
      virtual ~TRefIsotope(void);

      // Operators
      bool operator==(const TRefIsotope& refelement) const;
      bool operator!=(const TRefIsotope& refelement) const;

      // Modifiers - not supported for reference information.

      // Accessors
      string           atomicSymbol(void) const;
      int                 atomicMassNumber(void) const;
      float               isotopeWeight(void) const;
      float               abundance(void) const;
      float               halflife(void) const;

   protected:

      // Hidden/friend Constructor
      TRefIsotope(void);

      // Derived/Friend Class Modifiers
      void             atomicSymbol(const string& newSymbol);
      void             atomicMassNumber(int newAtomicMassNumber);
      void             isotopeWeight(float newIsotopicWeight);
      void             abundance(float newAbundance);
      void             halflife(float newHalflife);

   private:

      // State
      string p_atomicSymbol;
      float     p_isotopeWeight;
      float     p_abundance;           // .98, not 98%
      float     p_halflife;            // in seconds
      int       p_atomicMassNumber;    //  = #neutrons + #protons


      // Friends
      friend ostream& operator<< (ostream& os, const TRefIsotope& isotope);
      friend class TChartOfNuclides;

}; // TRefIsotope

#endif // REFISOTOPE_HH
