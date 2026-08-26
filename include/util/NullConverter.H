///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: NullConverter.H
//
//
// CLASS SYNOPSIS: Null class that doesn't do any conversion
//
// Design
//   Always returns the same value
///////////////////////////////////////////////////////////////////////////////
#ifndef NULLCONVERTER_HH
#define NULLCONVERTER_HH

#include "util/BaseUnitConverter.H"

class NullConverter: public BaseUnitConverter
{
  public:
    NullConverter();
    virtual ~NullConverter();

    virtual string convertTo(const string& value, const string& toUnits)
;
             
    virtual double convertTo(double value, const string& toUnits)
;

    virtual string convertFrom(const string& value, const string& fromUnits)
;

    virtual double convertFrom(double value, const string& fromUnits)
;

  protected:
    // valid that the argument is a valid unit
    virtual bool checkString(const string& unit);
};
#endif 
