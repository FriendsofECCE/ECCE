/**
 * @file
 *
 */
#ifndef CGENERGYFUNCTION_HH
#define CGENERGYFUNCTION_HH


#include "util/InvalidException.H"

#include "tdat/ObjectiveFunction.H"
#include "tdat/ForceField.H"

class FFCoordinates;

/**
 * This implements the ObjectiveFunction class on the molecular energy function
 * provided by the UFF. It is used for doing rough geometry optimizations.
 */
class CGEnergyFunction: public ObjectiveFunction
{

public:

/**
  * Constructor
  */
  CGEnergyFunction();

/**
  * Destructor
  */
  virtual ~CGEnergyFunction();

/**
  * Modifiers
  */
   void setForceField(ForceField *ffield);

 /**
  * Methods
  */
   virtual double getValue(bool& canceled, FFCoordinates *coords ) const
                  throw (InvalidException);
   virtual FFCoordinates* getGradient(bool& canceled,
                              const FFCoordinates *coords,
                              FFCoordinates *gradient) const
                          throw (InvalidException);

private:
   ForceField *p_ffield;

};
#endif
