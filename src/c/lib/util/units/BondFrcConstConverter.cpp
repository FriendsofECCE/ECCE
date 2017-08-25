/**
 * @file
 *
 *
 */

#include "util/BondFrcConstConverter.H"

/**
 * Constructor.
 * Valid conversion factors are set in this function and nowhere else!!!!!
 */
BondFrcConstConverter::BondFrcConstConverter()
{
  addConversion( "(kcal/Mole)/(Angstrom**2)", 1.0 );
  double value = 4.184/(0.1*0.1);
  addConversion( "(kJoule/Mole)/(nm**2)", value);

  setBaseUnits("(kcal/Mole)/(Angstrom**2)");
}


/**
 * Destructor.
 */
BondFrcConstConverter::~BondFrcConstConverter()
{
}

