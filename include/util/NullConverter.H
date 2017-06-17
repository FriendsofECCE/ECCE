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
             throw (InvalidException);
             
    virtual double convertTo(double value, const string& toUnits)
             throw (InvalidException);

    virtual string convertFrom(const string& value, const string& fromUnits)
             throw (InvalidException);

    virtual double convertFrom(double value, const string& fromUnits)
             throw (InvalidException);

  protected:
    // valid that the argument is a valid unit
    virtual bool checkString(const string& unit);
};
#endif 
