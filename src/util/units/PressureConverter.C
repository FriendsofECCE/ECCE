/**
 * @file
 *
 *
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "util/PressureConverter.H"

/**
 * Constructor.
 * Valid conversion factors are set in this function and nowhere else!!!!!
 */
PressureConverter::PressureConverter()
{
   addConversion( "Bar", 1.0);
   addConversion( "N/m**2", 100000.0);
   addConversion( "Atm", 0.986923);
   addConversion( "Torr", 750.062);

   setBaseUnits("Bar");
}



/**
 * Destructor.
 */
PressureConverter::~PressureConverter()
{
}

/**
 * Overload function to control number of significant digits if converting
 * to N/m**2
 */
string PressureConverter::convertTo(const string& value, const string& toUnits)
  throw (InvalidException)
{
  string ret = value;
  if (!checkString(toUnits)) {
    throw InvalidException(getErrorMessage(p_base, toUnits), WHERE);
  }
  if (p_base != toUnits) {
     double rvalue = atof(value.c_str());
     double newValue = BaseUnitConverter::convertTo(rvalue, toUnits);
     char buf[240];
     if (toUnits != "N/m**2") {
       sprintf(buf,"%f",newValue);
     } else {
       sprintf(buf,"%12.1f",newValue);
     }
     ret = buf;
  }
  return ret;
}


double PressureConverter::convertTo(double value, const string& toUnits)
  throw (InvalidException)
{
  double ret = value;
  if (!checkString(toUnits)) {
    throw InvalidException(getErrorMessage(p_base, toUnits), WHERE);
  }
  if (p_base != toUnits) {
     ret = BaseUnitConverter::convertTo(value, toUnits);
  }
  return ret;
}
