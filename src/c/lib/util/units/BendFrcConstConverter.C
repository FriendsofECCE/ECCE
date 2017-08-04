/**
 * @file
 *
 *
 */

#include <math.h>

#include "util/BendFrcConstConverter.H"

/**
 * Constructor.
 * Valid conversion factors are set in this function and nowhere else!!!!!
 */
BendFrcConstConverter::BendFrcConstConverter()
{
  double pi = 4.0*atan(1.0);
  addConversion( "(kcal/Mole)/(Radian**2)", 1.0/(pi*pi) );

  double value = 4.184/(pi*pi);
  addConversion( "(kJoule/Mole)/(Radian**2)", value);

  setBaseUnits("(kcal/Mole)/(Radian**2)");
}



/**
 * Destructor.
 */
BendFrcConstConverter::~BendFrcConstConverter()
{
}

