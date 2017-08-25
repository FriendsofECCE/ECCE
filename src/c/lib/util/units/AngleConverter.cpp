/**
 * @file
 *
 *
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "util/AngleConverter.H"

/**
 * Constructor.
 * Valid conversion factors are set in this function and nowhere else!!!!!
 */
AngleConverter::AngleConverter()
{
   double value = 4.0*atan(1.0);  //pi = 3.141592654
   addConversion( "Radian", value);
   addConversion( "Degree", 180.0);

   setBaseUnits("Degree");
}



/**
 * Destructor.
 */
AngleConverter::~AngleConverter()
{
}



/**
 * Overload string conversion routine so that they only produce a limited
 * number of significant figures if we are converting to degrees.
 */
string AngleConverter::convertTo(const string& value, const string& toUnits)
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
      if (toUnits == "Degree") {
         sprintf(buf,"%.3lf",newValue);
      } else {
         sprintf(buf,"%lf",newValue);
      }
      ret = buf;
   }
   return ret;
}


double AngleConverter::convertTo(double value, const string& toUnits)
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

