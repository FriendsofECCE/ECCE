///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: AngleConverter.H
//
//
// CLASS SYNOPSIS: Class for converting between different angle units.
//
// Design
//   Implements different angle conversion factors
///////////////////////////////////////////////////////////////////////////////
#ifndef ANGLECONVERTER_HH
#define ANGLECONVERTER_HH

#include "util/BaseUnitConverter.H"

class AngleConverter: public BaseUnitConverter
{
  public:

    AngleConverter();
    virtual ~AngleConverter();

    string convertTo(const string &value, const string &toUnits)
               throw (InvalidException);

    double convertTo(double value, const string &toUnits)
               throw (InvalidException);

};
#endif
