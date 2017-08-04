//#include "inv/SoDB.H"
#include "inv/actions/SoGLRenderAction.H"
//#include "inv/actions/SoCallbackAction.H"
//#include "inv/actions/SoGetBoundingBoxAction.H"
//#include "inv/actions/SoPickAction.H"

//#include "inv/nodes/SoScale.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoShapeHints.H"


#include "inv/nodes/SoMaterial.H"

#include "viz/StyledPlane.H"
#include "viz/SGPlane.H"

#include <iostream>
  using std::cout;
  using std::endl;

SO_NODE_SOURCE(StyledPlane);

void StyledPlane::initClass()
{
   SO_NODE_INIT_CLASS(StyledPlane, SoSwitch, "Switch");
}

StyledPlane::StyledPlane()
{
  SO_NODE_CONSTRUCTOR(StyledPlane);
  constructor();
}


StyledPlane::StyledPlane(SGPlane *plane)
{
  SO_NODE_CONSTRUCTOR(StyledPlane);
  p_plane = plane;
  constructor();
}

StyledPlane::~StyledPlane()
{
}


void StyledPlane::constructor()
{
     SoSeparator * sep = new SoSeparator;
     addChild(sep);

     // If you specify the shape type, you don't get two slide lighting
     // Also, having trouble getting color to show up if I set hints
     SoShapeHints *shapeHints = new SoShapeHints;
     shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
     //shapeHints->vertexOrdering = SoShapeHints::CLOCKWISE;
     shapeHints->faceType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
     //shapeHints->faceType = SoShapeHints::SOLID;
     //shapeHints->faceType = SoShapeHints::CONVEX;

     sep->addChild(shapeHints);
     
     p_mat  = new SoMaterial;
     sep->addChild( p_mat) ;
     setColor(0.8,0.8,0.);

     if (p_plane == 0) {
        p_plane = new SGPlane();
     }
     sep->addChild(p_plane);
     whichChild.setValue(SO_SWITCH_ALL);

}

void StyledPlane::show(bool flag)
{
   if (flag) {
      whichChild.setValue(SO_SWITCH_ALL);
   } else {
      whichChild.setValue(SO_SWITCH_NONE);
   }
}

/**
 * Set the material properties of the vector.
 */
void StyledPlane::setColor(float r, float g ,float b)
{
   p_mat->diffuseColor.setValue(r, g, b);
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
void StyledPlane::getColor(float& r, float& g, float& b)
{
   const float *rgb;
   rgb = p_mat->diffuseColor.getValues(0)->getValue();
   r = rgb[0];
   g = rgb[1];
   b = rgb[2];
}


/**
 * Set transparency setting.  We support only on/off since most values
 * are ugly and we pick one good one.
 */
void StyledPlane::setTransparent(bool transparent)
{
   float t = 0.0;
   if (transparent) t = 0.5;
   p_mat->transparency.setValue(t);
}

/**
 * Get transparency setting.  We support only on/off since most values
 * are ugly and we pick one good one.
 */
bool StyledPlane::isTransparent() const
{
   //TODO
   //return p_mat->transparency.get1Value() != 0.0;
   return false;
}


SoMaterial* StyledPlane::getMaterial()
{
  return p_mat;
}

SGPlane* StyledPlane::getPlane()
{
  return p_plane;
}
