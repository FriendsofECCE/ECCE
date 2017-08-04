/**
 * @file
 *
 *
 */

#include "util/MassConverter.H"


/**
 * Constructor.
 * Valid conversion factors are set in this function and nowhere else!!!!!
 */
MassConverter::MassConverter()
{
  addConversion( "amu", 1.0);
  setBaseUnits( "amu" );
}



/**
 * Destructor.
 */
MassConverter::~MassConverter()
{
}
