///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: DriftConverter.H
//
//
// CLASS SYNOPSIS: Class for converting between different energy units.
//
// Design
//   Implements different drift conversion factors (This is a term in the
//   NWChem MD input)
///////////////////////////////////////////////////////////////////////////////
#ifndef DRIFTCONVERTER_HH
#define DRIFTCONVERTER_HH

#include "util/BaseUnitConverter.H"

class DriftConverter: public BaseUnitConverter
{
  public:
    DriftConverter();
    virtual ~DriftConverter();

  private:

};
#endif
