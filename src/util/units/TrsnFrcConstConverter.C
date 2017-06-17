/**
 * @file
 *
 *
 */

#include "util/TrsnFrcConstConverter.H"


/**
 * Constructor.
 * Valid conversion factors are set in this function and nowhere else!!!!!
 */
TrsnFrcConstConverter::TrsnFrcConstConverter()
{
  addConversion( "kcal/Mole", 1.0 );
  addConversion( "kJoule/Mole", 4.184 );

  setBaseUnits("kcal/Mole");
}



/**
 * Destructor.
 */
TrsnFrcConstConverter::~TrsnFrcConstConverter()
{
}

