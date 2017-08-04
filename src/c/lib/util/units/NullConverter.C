/**
 * @file
 *
 *
 */

#include <math.h>

#include "util/NullConverter.H"

/**
 * @file
 *
 *
 */
NullConverter::NullConverter()
{
}

/**
 * Destructor.
 */
NullConverter::~NullConverter()
{
}


/**
 * Performs null conversion.
 * @return the value passed in.
 */
string NullConverter::convertTo(const string& value, const string& toUnits)
  throw (InvalidException)
{
  return value;
}

/**
 * Performs null conversion.
 * @return the value passed in.
 */
double NullConverter::convertTo(double value, const string& toUnits)
  throw (InvalidException)
{
  return value;
}

/**
 * Performs null conversion.
 * @return the value passed in.
 */
string NullConverter::convertFrom(const string& value, const string& fromUnits)
  throw (InvalidException)
{
  return value;
}

/**
 * Performs null conversion.
 * @return the value passed in.
 */
double NullConverter::convertFrom(double value, const string& fromUnits)
  throw (InvalidException)
{
  return value;
}


/**
 * All conversions are possible with this converter so it simply returns true.
 * @return true
 */
bool NullConverter::checkString(const string& units)
{
  return true;
}
