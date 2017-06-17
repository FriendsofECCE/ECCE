///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: CompressibilityConverter.H
//
//
// CLASS SYNOPSIS: Class for converting between different compressibility units.
//
// Design
//   Implements different compressibility conversion factors
///////////////////////////////////////////////////////////////////////////////
#ifndef COMPRESSIBILITYCONVERTER_HH
#define COMPRESSIBILITYCONVERTER_HH

#include "util/BaseUnitConverter.H"

class CompressibilityConverter: public BaseUnitConverter
{
  public:
    CompressibilityConverter();
    virtual ~CompressibilityConverter();

    string convertTo(const string &value, const string &toUnits)
               throw (InvalidException);

    double convertTo(double value, const string &toUnits)
               throw (InvalidException);
  private:

};
#endif
