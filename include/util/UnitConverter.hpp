///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: UnitConverter.H
//
//
// CLASS SYNOPSIS: The interface definition for unit conversion.
//
// Design
//   This defines the generic conversion routines
///////////////////////////////////////////////////////////////////////////////
#ifndef UNITCONVERTER_HH
#define UNITCONVERTER_HH

#include <string>

using std::string;

#include "util/InvalidException.H"

/**
 * This is the base class for creating unit conversion routines. Each unit
 * class, e.g. Energy, requires its own UnitConverter class. The conversion
 * factors are all stored in the individual UnitConverters.
 */

class UnitConverter
{
  public:

    /**
     * This sets the units that all values will be converted to or from.
     */
    virtual void setBaseUnits(const string& base)
             throw (InvalidException) = 0;

    /**
     * Convert the value from the base units (defined with setBaseUnits) to
     * the units defined in toUnits.
     */
    virtual string convertTo(const string& value, const string& toUnits)
             throw (InvalidException) = 0;
             
    virtual double convertTo(double value, const string& toUnits)
             throw (InvalidException) = 0;

    /**
     * Convert the value to the base units (defined with setBaseUnits) from
     * the units defined in fromUnits.
     */
    virtual string convertFrom(const string& value, const string& fromUnits)
             throw (InvalidException) = 0;

    virtual double convertFrom(double value, const string& fromUnits)
             throw (InvalidException) = 0;

  protected:
};

#endif
