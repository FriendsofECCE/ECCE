///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: EnergyConverter.H
//
//
// CLASS SYNOPSIS: Class for converting between different energy units.
//
// Design
//   Implements different energy conversion factors
///////////////////////////////////////////////////////////////////////////////
#ifndef ENERGYCONVERTER_HH
#define ENERGYCONVERTER_HH

#include "util/BaseUnitConverter.H"

class EnergyConverter: public BaseUnitConverter
{
  public:
    EnergyConverter();
    virtual ~EnergyConverter();

  private:

};
#endif
