///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PressureConverter.H
//
//
// CLASS SYNOPSIS: Class for converting between different angle units.
//
// Design
//   Implements different angle conversion factors
///////////////////////////////////////////////////////////////////////////////
#ifndef PRESSURECONVERTER_HH
#define PRESSURECONVERTER_HH

#include "util/BaseUnitConverter.H"

class PressureConverter: public BaseUnitConverter
{
  public:

    PressureConverter();
    virtual ~PressureConverter();

    string convertTo(const string& value, const string& toUnits)
           throw (InvalidException);

    double convertTo(double value, const string& toUnits)
           throw (InvalidException);
};
#endif
