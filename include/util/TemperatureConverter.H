///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TemperatureConverter.H
//
//
// CLASS SYNOPSIS: Class for converting between different temperature units.
//
// Design
//   Implements different temperature conversion formulas
///////////////////////////////////////////////////////////////////////////////
#ifndef TEMPERATURECONVERTER_HH
#define TEMPERATURECONVERTER_HH

#include "util/BaseUnitConverter.H"

class TemperatureConverter: public BaseUnitConverter
{
  public:

    TemperatureConverter();
    virtual ~TemperatureConverter();

    /**
     *  All conversion formulas must be reimplemented since
     *  temperature conversions are not simple scaling relations
     */
    string convertTo(const string &value, const string &toUnits)
;

    double convertTo(double value, const string &toUnits)
;

    string convertFrom(const string &value, const string &toUnits)
;

    double convertFrom(double value, const string &toUnits)
;

    double convertToK(double value, const string &fromUnits);

    double convertFromK(double value, const string &toUnits);

};
#endif
