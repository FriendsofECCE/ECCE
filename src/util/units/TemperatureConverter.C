/**
 * @file
 *
 *
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "util/TemperatureConverter.H"

/**
 * Constructor.
 * Valid conversion factors are set in this function and nowhere else!!!!!
 */
TemperatureConverter::TemperatureConverter()
{
   addConversion( "Kelvin", 1.0);
   addConversion( "C", 1.0);

   setBaseUnits("Kelvin");
}



/**
 * Destructor.
 */
TemperatureConverter::~TemperatureConverter()
{
}



/**
 * Overload conversion routines because conversion is not a simple scaling
 * relation.
 */
string TemperatureConverter::convertTo(const string& value, const string& toUnits)
   throw (InvalidException)
{
   string ret = value;
   if (!checkString(toUnits)) {
      throw InvalidException(getErrorMessage(p_base, toUnits),WHERE);
   }
   if (p_base != toUnits) {
      double rvalue = atof(value.c_str());
      double newValue = convertFromK(rvalue, toUnits);
      char buf[240];
      sprintf(buf,"%lf",newValue);
      ret = buf;
   }
   return ret;
}

double TemperatureConverter::convertTo(double value, const string& toUnits)
   throw (InvalidException)
{
   double ret = value;
   if (!checkString(toUnits)) {
      throw InvalidException(getErrorMessage(p_base, toUnits),WHERE);
   }
   if (p_base != toUnits) {
      ret = convertFromK(value, toUnits);
   }
   return ret;
}

string TemperatureConverter::convertFrom(const string& value, const string& fromUnits)
   throw (InvalidException)
{
   string ret = value;
   if (!checkString(fromUnits)) {
      throw InvalidException(getErrorMessage(p_base, fromUnits),WHERE);
   }
   if (p_base != fromUnits) {
      double rvalue = atof(value.c_str());
      double newValue = convertToK(rvalue, fromUnits);
      char buf[240];
      sprintf(buf,"%lf",newValue);
      ret = buf;
   }
   return ret;
}

double TemperatureConverter::convertFrom(double value, const string& fromUnits)
   throw (InvalidException)
{
   double ret = value;
   if (!checkString(fromUnits)) {
      throw InvalidException(getErrorMessage(p_base, fromUnits),WHERE);
   }
   if (p_base != fromUnits) {
      ret = convertToK(value, fromUnits);
   }
   return ret;
}

double TemperatureConverter::convertToK(double value, const string &fromUnits)
{
  double ret = value;
  if (fromUnits == "C") {
    ret =+ 273.15;
  }
  return ret;
}

double TemperatureConverter::convertFromK(double value, const string &toUnits)
{
  double ret = value;
  if (toUnits == "C") {
    ret =- 273.15;
  }
  return ret;
}
