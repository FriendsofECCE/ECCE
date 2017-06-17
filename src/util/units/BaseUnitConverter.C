/**
 * @file
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "util/BaseUnitConverter.H"


// Helper types for map
typedef map<string, double, less<string> >::const_iterator BUCIterator;
typedef map<string, double, less<string> >::value_type BUCMapPair;


/**
 * Add a unit name and its conversion factor.
 * If the unit name already exists, it will be replaced.
 *
 * @param unit Unit name
 * @param value Unit conversion factor
 */
void BaseUnitConverter::addConversion(const string& unit, double value)
{
  p_conversions.insert( BUCMapPair(unit, value) );
}


/**
 * Sets the base units that are converted to or from in subsequent convert
 * calls.
 *
 * @param unit Unit name
 *
 * @throw InvalidException if the unit is not supported.
 */
void BaseUnitConverter::setBaseUnits(const string& unit)
  throw (InvalidException)
{
  if (!checkString(unit)) {
    throw InvalidException(unit + " is an unknown unit.", WHERE);
  }

  p_base = unit;
  p_baseValue = getValue(unit);
}



/**
 * Checks to see if the specified unit is supported.
 *
 * @param unit Unit Name
 *
 * @return true if unit is supported
 */
bool BaseUnitConverter::checkString(const string& unit)
{
  return (p_conversions.find(unit) != p_conversions.end());
}

/**
 * Gets the conversion factor for the specified unit.
 *
 * NOTE: Should only be called after checkString() validates the unit.
 * 1.0 will be returned if the unit is not valid.
 *
 * @param unit Unit name
 *
 * @return Conversion factor
 */
double BaseUnitConverter::getValue(const string& unit)
{
  double ret = 1.0;
  BUCIterator it = p_conversions.find(unit);
  if (it != p_conversions.end())  {
     ret = (*it).second;
  }
  return ret;
}

/**
 * Constructs and returns error message reporting the type of conversion
 * that was attempted but failed.
 *
 * @param from Name of from unit
 * @param to name of to unit
 *
 * @return Error message
 */
string BaseUnitConverter::getErrorMessage(const string& from, const string& to)
{
   string msg = "Conversion from ";
   msg += from;
   msg += " to ";
   msg += to;
   msg += " is not defined.";
   return msg;
}

/**
 * Converts from base units to 'toUnits'.
 *
 * @param value - string representation of value. Unaltered if to/from units
 *                are identical.
 * @param toUnits - name of units to convert to.
 *
 * @return string value
 *
 * @throw InvalidException - toUnits are unknown.
 */
string BaseUnitConverter::convertTo(const string& value, const string& toUnits)
  throw (InvalidException)
{
  string ret = value;
  if (!checkString(toUnits)) {
    throw InvalidException(getErrorMessage(p_base, toUnits), WHERE);
  }
  if (p_base != toUnits) {
     double rvalue = atof(value.c_str());
     double newValue = convertTo(rvalue, toUnits);
     char buf[240];
     sprintf(buf,"%f",newValue);
     ret = buf;
  }
  return ret;
}

/**
 * Converts from base units to 'toUnits'.
 *
 * @param value - string representation of value. Unaltered if to/from units
 *                are identical.
 * @param toUnits - name of units to convert to.
 *
 * @return double value
 *
 * @throw InvalidException - toUnits are unknown.
 */
double BaseUnitConverter::convertTo(double value, const string& toUnits)
  throw (InvalidException)
{
  if (!checkString(toUnits)) {
    throw InvalidException(getErrorMessage(p_base, toUnits), WHERE);
  }

  double rUnit = getValue(toUnits);
  double ratio = rUnit/p_baseValue;
  return value*ratio;
}

/**
 * Converts from fromUnits to base units.
 *
 * @param value - string representation of value. Unaltered if to/from units
 *                are identical.
 * @param fromUnits - name of units to convert from.
 *
 * @return string value
 *
 * @throw InvalidException - fromUnits are unknown.
 */
string BaseUnitConverter::convertFrom(const string& value, const string& fromUnits)
  throw (InvalidException)
{
  string ret = value;

  if (!checkString(fromUnits)) {
    throw InvalidException(getErrorMessage(fromUnits, p_base), WHERE);
  }

  if (p_base == fromUnits) {
     double rvalue = atof(value.c_str());
     double newValue = convertFrom(rvalue, fromUnits);
     char buf[240];
     sprintf(buf,"%f",newValue);
     ret = buf;
  }
  return ret;
}

/**
 * Converts from fromUnits to base units.
 * @param value - string representation of value. Unaltered if to/from units
 *                are identical.
 * @param fromUnits - name of units to convert from.
 *
 * @return double value
 *
 * @throw InvalidException - fromUnits are unknown.
 */
double BaseUnitConverter::convertFrom(double value, const string& fromUnits)
  throw (InvalidException)
{
  if (!checkString(fromUnits)) {
    throw InvalidException(getErrorMessage(fromUnits, p_base), WHERE);
  }

  double rUnit = getValue(fromUnits);
  double ratio = p_baseValue/rUnit;
  return value*ratio;
}
