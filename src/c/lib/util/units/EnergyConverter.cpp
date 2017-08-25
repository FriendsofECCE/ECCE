/**
 * @file
 *
 *
 */

#include "util/EnergyConverter.H"


/**
 * Constructor.
 * Valid conversion factors are set in this function and nowhere else!!!!!
 */
EnergyConverter::EnergyConverter()
{
  addConversion( "kcal/Mole", 1.0);
  addConversion( "kcal/mol", 1.0);
  addConversion( "kJoule/Mole", 4.184 );
  addConversion( "Hartree", 0.001593601451 );
  addConversion( "eV", 0.0433646 );
  //TODO BJP 3/3/04  addConversion( "cm**-1", ???? );
  //TODO BJP 3/3/04  addConversion( "mDyne-Angstrom", ???? );
  setBaseUnits( "kcal/Mole" );
}



/**
 * Destructor.
 */
EnergyConverter::~EnergyConverter()
{
}
