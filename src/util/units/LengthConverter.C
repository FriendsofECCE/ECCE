/**
 * @file
 *
 *
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "util/LengthConverter.H"


/**
 * Constructor.
 * Valid conversion factors are set in this function and nowhere else!!!!!
 */
LengthConverter::LengthConverter() : BaseUnitConverter()
{
  addConversion( "Angstrom", 1.0 );
  addConversion( "nm", 0.1 );
  addConversion( "pm", 100.);
  addConversion( "Bohr", 1.889725989 );

  setBaseUnits("Angstrom");
}



/**
 * Destructor.
 */
LengthConverter::~LengthConverter()
{
}

