/**
 * @file 
 *
 *
 */
#ifndef BASEUNITCONVERTER_HH
#define BASEUNITCONVERTER_HH

#include <string>
#include <map>

using std::string;
using std::map;
using std::less;

#include "util/UnitConverter.H"

/**
 * Abstract base class that provides structures and
 * convenience functions suitable for most anticipated subclasses.
 *
 * Supports Unit Converter subclasses that are
 * essentially a set of names and conversion factors.
 *
 * Many conversions are simply a matter of mulitplying a converstion factor.
 * This base class keeps a set of conversions factors for units.
 */
class BaseUnitConverter : public UnitConverter
{
  public:
    virtual ~BaseUnitConverter() {};

    virtual void setBaseUnits(const string& base)
             throw (InvalidException);

    virtual string convertTo(const string& value, const string& toUnits)
             throw (InvalidException);
             
    virtual double convertTo(double value, const string& toUnits)
             throw (InvalidException);

    virtual string convertFrom(const string& value, const string& fromUnits)
             throw (InvalidException);

    virtual double convertFrom(double value, const string& fromUnits)
             throw (InvalidException);

  protected:

    virtual string getErrorMessage(const string& from, const string& to);

    virtual void addConversion(const string& unit, double value);

    virtual bool checkString(const string& unit);

    virtual double getValue(const string& unit);


    /**
     * The base units that server as the 'from' units in conversion requests
     */
    string p_base;

    /**
     * The conversion factor for the base value.
     *
     * @todo eliminate this?
     */
    double p_baseValue;

    /**
     * A map of units and their conversion factor.
     */
    map<string, double, less<string> > p_conversions;

};

#endif
