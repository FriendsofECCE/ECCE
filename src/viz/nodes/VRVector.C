#include "inv/SoDB.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoGetBoundingBoxAction.H"
#include "inv/actions/SoPickAction.H"

#include "inv/nodes/SoScale.H"
#include "inv/nodes/SoSeparator.H"

#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoCone.H"
#include "inv/nodes/SoCylinder.H"
#include "inv/nodes/SoTranslation.H"
#include "inv/nodes/SoTransform.H"

#include "viz/VRVector.H"

#include <iostream>
  using std::cout;
  using std::endl;

SO_NODE_SOURCE(VRVector);

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
void VRVector::initClass()
{
   SO_NODE_INIT_CLASS(VRVector, SoGroup,"Group");
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
void VRVector::direction(double x, double y ,double z)
{
   p_direction[0] = x;
   p_direction[1] = y;
   p_direction[2] = z;
   float norm = sqrt( x*x + y*y + z*z);
   if (p_fixed) {
      // Sizes in Angstrom, independent of the arrow's length; the head
      // takes at most half of a short arrow.
      const float tip = 1.075f*norm;
      const float hl = (0.18f < 0.5f*tip) ? 0.18f : 0.5f*tip;
      const float sl = tip - hl;
      const bool none = (norm <= 0.f);
      transf->scaleFactor.setValue(1,1,1);
      scal->scaleFactor.setValue(1,1,1);
      shaftTrans->translation.setValue(0, sl/2, 0);
      shaft->height = sl;
      shaft->radius = none ? 0.f : 0.035f;
      headTrans->translation.setValue(0, sl/2 + hl/2, 0);
      head->height = hl;
      head->bottomRadius = none ? 0.f : 0.09f;
   } else {
      transf->scaleFactor.setValue(norm,norm,norm);
   }

   SbVec3f vec((float) x,(float) y,(float) z);
   SbRotation rot( SbVec3f(0.,1.,0.), vec);
   
   transf->rotation = rot;
   place();
}

/**
 * Translation = centre + startRadius along the (unit) direction.
 */
void VRVector::place()
{
   double len = sqrt(p_direction[0]*p_direction[0] + p_direction[1]*p_direction[1]
                     + p_direction[2]*p_direction[2]);
   double k = (len > 0.0) ? p_startRadius/len : 0.0;
   for (int i = 0; i < 3; i++) p_position[i] = p_center[i] + k*p_direction[i];
   transf->translation.setValue((float)p_position[0], (float)p_position[1],
                                (float)p_position[2]);
}

void VRVector::fixedThickness(bool on)
{
   p_fixed = on;
   direction(p_direction[0], p_direction[1], p_direction[2]);
}

void VRVector::startRadius(double r)
{
   p_startRadius = r;
   place();
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
void VRVector::position(double x, double y ,double z)
{
   
   p_center[0] = x;
   p_center[1] = y;
   p_center[2] = z;
   place();
  
}


/**
 * Set the material properties of the vector.
 */
void VRVector::setColor(float R, float G ,float B)
{
   mat->diffuseColor.setValue(R, G, B);
   //mat->shininess = 1.0;
   /*
   original code - flat looking 
   mat->ambientColor.setValue(R,G,B);
   mat->diffuseColor.setValue(0.5*R,0.5*G,0.5*B);
   mat->specularColor.setValue(R,G,B);
   mat->emissiveColor.setValue(0.5*R,0.5*G,0.5*B);
   */
}

/**
 * Get the material properties of the vector.
 */
void VRVector::getColor(float *RGB)
{
   const float *rgb;
   rgb = mat->diffuseColor.getValues(0)->getValue();
   RGB[0] = rgb[0];
   RGB[1] = rgb[1];
   RGB[2] = rgb[2];
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
void VRVector::scaleFactor(float x, float y, float z)
{
  //float ssc = 1./6.;
  scal->scaleFactor.setValue(x, y, z);
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
VRVector::VRVector()
{
  SO_NODE_CONSTRUCTOR(VRVector);
  constructor();
}


/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
void VRVector::constructor()
{
     p_fixed = false;
     p_startRadius = 0.0;
     for (int i = 0; i < 3; i++) p_direction[i] = p_center[i] = p_position[i] = 0.0;
     SoSeparator * sep = new SoSeparator;
     addChild(sep);
     
     transf = new SoTransform;
     sep->addChild(transf);

     SoSeparator * sep1 = new SoSeparator;
     sep->addChild(sep1);     
     
     mat  = new SoMaterial;
     sep1->addChild(mat) ;
     setColor(0.8,0.8,0.);

     scal = new SoScale; 
     sep1->addChild(scal); 
     float ssc = 1./4.;
     scal->scaleFactor.setValue(ssc,ssc,ssc);
     //float ssc = 1./6.;
     //scal->scaleFactor.setValue(ssc,1,ssc);
     SoTranslation * trans = new SoTranslation;
     sep1->addChild(trans);
     shaftTrans = trans;

     trans->translation.setValue(0.,2.,0.);
     SoCylinder *  cylinder = new SoCylinder;
     shaft = cylinder;
     sep1->addChild(cylinder);
     cylinder->height.setValue(4.);
     //cylinder->height.setValue(1.);
     cylinder->radius.setValue(0.15);

     trans = new SoTranslation;
     sep1->addChild(trans);
     headTrans = trans;
     trans->translation.setValue(0.,2.,0.);
     SoCone *  cone = new SoCone;
     head = cone;
     cone->height = .6 ;
     cone->bottomRadius = .4 ;
     sep1->addChild(cone);

}



/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
VRVector::~VRVector()
{
}
/////////////////////////////////////////////////////////////////////////////
// Description
//    Return a pointer to SoTransform associated with vector
/////////////////////////////////////////////////////////////////////////////
SoTransform* VRVector::getTransform()
{
  return transf;
}
/////////////////////////////////////////////////////////////////////////////
// Description
//    Return a pointer to SoScale associated with vector
/////////////////////////////////////////////////////////////////////////////
SoScale* VRVector::getScale()
{
  return scal;
}

/**
 *  Return a pointer to the current direction
 */
const double* VRVector::getDirection()
{
  return p_direction;
}

/**
 *  Return a pointer to the current position
 */
const double* VRVector::getPosition()
{
  return p_position;
}
