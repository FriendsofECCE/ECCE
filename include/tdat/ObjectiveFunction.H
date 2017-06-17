/**
 * @file
 *
 */
#ifndef OBJECTIVEFUNCTION_HH
#define OBJECTIVEFUNCTION_HH

#include "util/InvalidException.H"

class FFCoordinates;

/**
 * Abstract class that wraps the objective function that is minimized using
 * the conjugate gradient minimizer in the CGMinimizer class
 *
 * This is designed to make it relatively easy to use the CGMinimizer on a
 * variety of different objective functions
 */

class ObjectiveFunction
{

public:

   virtual double getValue(bool& canceled, FFCoordinates *coords) const 
     throw (InvalidException) = 0;
   virtual FFCoordinates* getGradient(bool& canceled, 
                                      const FFCoordinates *coords,
                                      FFCoordinates *gradient) const
     throw (InvalidException) = 0;

private:

};
#endif
