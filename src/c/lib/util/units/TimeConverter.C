/**
 * @file
 *
 *
 */

#include "util/TimeConverter.H"


/**
 * Constructor.
 * Valid conversion factors are set in this function and nowhere else!!!!!
 */
TimeConverter::TimeConverter()
{
  addConversion( "ps", 1.0);
  addConversion( "ns", 1000.0 );
  addConversion( "ms", 1.0e9 );
  addConversion( "s", 1.0e12 );
  setBaseUnits( "ps" );
}



/**
 * Destructor.
 */
TimeConverter::~TimeConverter()
{
}
