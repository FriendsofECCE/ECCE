/**
 * @file
 *
 */
#ifndef CGALIGNMENTFUNCTION_HH
#define CGALIGNMENTFUNCTION_HH


#include "util/InvalidException.H"

#include "tdat/ObjectiveFunction.H"

class FFCoordinates;

/**
 * This implements the ObjectiveFunction class on an alignment objective
 * function. The goal is to find the rotations and translations that
 * optimize the overlap of two fragments.
 */
class CGAlignmentFunction: public ObjectiveFunction
{

public:

/**
  * Constructor
  */
  CGAlignmentFunction();

/**
  * Destructor
  */
  virtual ~CGAlignmentFunction();

/**
  * Modifiers
  */
   void setTargetCoordinates(FFCoordinates *coords);
   void setInitialFragmentCoordinates(FFCoordinates *coords);
   void getFragmentCoordinates(FFCoordinates *transf, FFCoordinates *coords);
   void setRotation(const FFCoordinates *coords, double rot[3][3]) const;
   void setTranslation(const FFCoordinates *coords, double trans[3]) const;
   void setRotationGrad(const FFCoordinates *coords, double drot[3][3][3]) const;
   void setTranslationGrad(const FFCoordinates *coords, double dtrans[3][3]) const;

 /**
  * Methods
  */
   virtual double getValue(bool& canceled, FFCoordinates *coords) const
                  throw (InvalidException);
   virtual FFCoordinates* getGradient(bool& canceled,
                              const FFCoordinates *coords,
                              FFCoordinates *gradient) const
                          throw (InvalidException);

private:
   FFCoordinates *p_target;
   FFCoordinates *p_fragment;
};
#endif
