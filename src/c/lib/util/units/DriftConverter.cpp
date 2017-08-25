/**
 * @file
 *
 *
 */

#include "util/DriftConverter.H"


/**
 * Constructor.
 * Valid conversion factors are set in this function and nowhere else!!!!!
 */
DriftConverter::DriftConverter()
{
  addConversion( "(kcal/Mole)/ps", 1.0);
  addConversion( "(kcal/mol)/ps", 1.0);
  addConversion( "(kJoule/Mole)/ps", 4.184 );
  addConversion( "Hartree/ps", 0.001593601451 );
  addConversion( "eV/ps", 0.0433646 );
  setBaseUnits( "(kcal/Mole)/ps" );
}



/**
 * Destructor.
 */
DriftConverter::~DriftConverter()
{
}
