/**
 * @file
 *
 *
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "util/CompressibilityConverter.H"


/**
 * Constructor.
 * Valid conversion factors are set in this function and nowhere else!!!!!
 */
CompressibilityConverter::CompressibilityConverter()
{
  addConversion( "m**2/N", 1.0);
  setBaseUnits( "m**2/N" );
}



/**
 * Destructor.
 */
CompressibilityConverter::~CompressibilityConverter()
{
}

/**
 * Overload string conversion routine so that it switches to exponential
 * notation for small values of the compressibility.
 */
string CompressibilityConverter::convertTo(const string& value, const string& toUnits)
   throw (InvalidException)
{
   string ret = value;
   if (!checkString(toUnits)) {
      throw InvalidException(getErrorMessage(p_base, toUnits),WHERE);
   }
   if (p_base != toUnits) {
      double rvalue = atof(value.c_str());
      double newValue = BaseUnitConverter::convertTo(rvalue, toUnits);
      char buf[240];
      if (fabs(rvalue) < 1.0e-4 && fabs(rvalue) > 0.0 ) {
         sprintf(buf,"%e",newValue);
      } else {
         sprintf(buf,"%lf",newValue);
      }
      ret = buf;
   }
   return ret;
}


double CompressibilityConverter::convertTo(double value, const string& toUnits)
   throw (InvalidException)
{
   double ret = value;
   if (!checkString(toUnits)) {
      throw InvalidException(getErrorMessage(p_base, toUnits),WHERE);
   }
   if (p_base != toUnits) {
      ret = BaseUnitConverter::convertTo(value, toUnits);
   }
   return ret;
}
