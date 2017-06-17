#include <GL/gl.h>
#include "inv/SoDB.H"
#include "inv/nodekits/SoWrapperKit.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoGetBoundingBoxAction.H"
#include "inv/actions/SoPickAction.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoVertexProperty.H"
#include "inv/nodes/SoFaceSet.H"
#include "inv/nodes/SoTranslation.H"
#include "inv/nodes/SoTransform.H"
#include "inv/sensors/SoTimerSensor.H"

#include "inv/actions/SoGLRenderAction.H"

#include "inv/elements/SoGLTextureCoordinateElement.H"
#include "inv/elements/SoGLTextureEnabledElement.H"
#include "inv/misc/SoState.H"
#include "inv/elements/SoGLLazyElement.H"

#include "viz/VRPlane.H"
#include "viz/RTDraggerNode.H"


// Include files for new classes

SO_NODE_SOURCE(VRPlane);

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
void VRPlane::initClass()
{
   SO_NODE_INIT_CLASS(VRPlane, SoGroup,"Group");
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
VRPlane::VRPlane()
{
  SO_NODE_CONSTRUCTOR(VRPlane);
  constructor();
}


/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
void VRPlane::constructor()
{

   SoSeparator * sep = new SoSeparator;
   addChild(sep);

   SoSeparator * sep1 = new SoSeparator();
   sep->addChild(sep1);

     


//     SoScale * scal = new SoScale;
//     sep1->addChild(scal);
//     scal->scaleFactor.setValue(0.2,0.2,0.2);
  
   mickeyDragg =  new RTDraggerNode;
   sep1->addChild(mickeyDragg);

   SoTransform * transf = new SoTransform;
   sep->addChild(transf);
   transf->translation.connectFrom(&mickeyDragg->translation);
   transf->rotation.connectFrom(&mickeyDragg->rotation);

   SoFaceSet * plane = new SoFaceSet;
   sep->addChild(plane);
     
   int32_t newValues[1] = { 4 };
   plane->numVertices.setValues(0,1,newValues);

   SoVertexProperty * geo = new SoVertexProperty;

   float xyz[3];

   xyz[0] = 4.;
   xyz[1] = 4.;
   xyz[2] = 0.;
   geo->vertex.set1Value(0, xyz);

   xyz[0] = 4.;
   xyz[1] = -4.;
   xyz[2] = 0.; 
   geo->vertex.set1Value(1, xyz);

   xyz[0] = -4.;
   xyz[1] = -4.;
   xyz[2] = 0.;
   geo->vertex.set1Value(2, xyz);

   xyz[0] = -4.;
   xyz[1] = 4.;
   xyz[2] = 0.;
   geo->vertex.set1Value(3, xyz);
   geo->vertex.setNum(4);

   plane->vertexProperty.setValue(geo);
}

//=============================================================================
   void VRPlane::GLRender(SoGLRenderAction *action)
//=============================================================================
{
   // First see if the object is visible and should be rendered
   // now. This is a method on SoShape that checks for INVISIBLE
   // draw style, BOUNDING_BOX complexity, and delayed
   // transparency.

   glPushMatrix();
 
   // See if texturing is enabled. If so, we will have to
   // send explicit texture coordinates. The "doTextures" flag
   // will indicate if we care about textures at all.       
   
   // Send the next material if it varies per part. 
   // use SoGLLazyElement::sendDiffuseByIndex().
   // This will also send transparency, so that if transparency type       
   // is not SCREEN_DOOR, there can be a change of transparency across
   // the shape:

   float x,  y,  z;
   x= 0.; y=-1.; z= 0;

   GLdouble eqn[4] = {0.,-1.,0.,10.};
   glClipPlane(GL_CLIP_PLANE0,eqn);
   glEnable(GL_CLIP_PLANE0);
   glEnd();
 
   glDisable(GL_CLIP_PLANE0);
    
   eqn[0]=0.1/x;  eqn[1]=0.1/y; eqn[2]=0.1/z;  eqn[3]=-0.1;       

   glClipPlane(GL_CLIP_PLANE0,eqn);
      
   glEnable(GL_CLIP_PLANE0);

   // Restore the GL matrix
   glPopMatrix();
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
VRPlane::~VRPlane()
{
} 
