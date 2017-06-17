///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: TRefIsotope.C
//
//
// DESIGN:
//    TRefIsotope object are intended to be created only during the
//    loading of the Chart of Nuclides.
//
///////////////////////////////////////////////////////////////////////////////

#include <ctype.h>

#include <iostream>
  using std::ostream;
  using std::endl;

#include "tdat/TRefIsotope.H"



// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Virtual TRefIsotope Object Destructor
//
///////////////////////////////////////////////////////////////////////////////
TRefIsotope::~TRefIsotope(void)
{
}

// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Identity Based Comparison Operators
//
///////////////////////////////////////////////////////////////////////////////
bool TRefIsotope::operator==(const TRefIsotope& refIsotope) const
{ return ( this == &refIsotope); }
bool TRefIsotope::operator!=(const TRefIsotope& refIsotope) const
{ return ( this != &refIsotope); }


// ------------ Modifiers ------------

// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Accessors for Information Contained in Each RefIsotope
//    Assume that we are within a transaction for each accessor
//
///////////////////////////////////////////////////////////////////////////////
string TRefIsotope::atomicSymbol(void) const
{ return p_atomicSymbol; }
int TRefIsotope::atomicMassNumber(void) const
{ return p_atomicMassNumber; }
float TRefIsotope::isotopeWeight(void) const
{ return p_isotopeWeight; }
float TRefIsotope::abundance(void) const
{ return p_abundance; }
float TRefIsotope::halflife(void) const
{ return p_halflife; }

// ---------- General Methods  ----------


// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Hidden Constructor/Destructor ---
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Hidden Default Constructor and Virtual Destructor
//
//  Implementation
//    We hide the default constructor to ensure the compiler
//    doesn't create one that everyone can use.
//
///////////////////////////////////////////////////////////////////////////////
TRefIsotope::TRefIsotope(void)
{}

// ---------- Derived Class Modifiers ----------
////////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Protected modifiers for class
//  
//  Implementation
//    Assume that we are within a transaction for each accessor
//
////////////////////////////////////////////////////////////////////////////////
void TRefIsotope::atomicSymbol(const string& newAtomicSymbol)
{ p_atomicSymbol = newAtomicSymbol; }
void TRefIsotope::atomicMassNumber(int newAtomicMassNumber)
{
  //EE_ASSERT(newAtomicMassNumber > 0, EE_FATAL,
  //          "Isotopic Mass Number Less than zero");
  p_atomicMassNumber = newAtomicMassNumber;
}
void TRefIsotope::isotopeWeight(float newIsotopeWeight)
{
  //EE_ASSERT(newIsotopeWeight > 0.0, EE_FATAL,
  //          "Isotope Weight Less than zero");
  p_isotopeWeight = newIsotopeWeight;
}
void TRefIsotope::abundance(float newAbundance)
{
  //EE_ASSERT( (newAbundance >= 0.0) && (newAbundance <= 1.0), EE_FATAL, 
  //            "Abundance Not in Range 0 - 1");
  p_abundance = newAbundance;
}
void TRefIsotope::halflife(float newHalflife)
{
  //EE_ASSERT(newHalflife > 0.0, EE_FATAL,
  //          "Half Life Less than zero");
  p_halflife = newHalflife;
}

// ------------------------
// Private Member Functions
// ------------------------

// ---------- Friends ----------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Print a Formatted RefIsotope Object to the Specified Stream
//
//  Implementation
//    We use public accesors but kept our friendship, just in case.
//
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const TRefIsotope& refIsotope)
{
  os << "[TRefIsotope<";
  os << "symbol: " << refIsotope.atomicSymbol() << ",";
  os << "number: " << refIsotope.atomicMassNumber() << ",";
  os << "weight: " << refIsotope.isotopeWeight() << ",";
  os << "abundance: " << refIsotope.abundance() << ",";
  os << "halflife: " << refIsotope.halflife() << " seconds,";
  os << ">]" << endl;
  return os;
}


// ---------- Encapsulated Behavior ----------
