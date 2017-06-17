#include <iostream>
using namespace std;
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
//#include "inv/nodes/SoQuadMesh.H"
#include "inv/nodes/SoTranslation.H"
#include "inv/nodes/SoTransform.H"
#include "inv/sensors/SoTimerSensor.H"

#include "inv/actions/SoGLRenderAction.H"

#include "inv/elements/SoGLTextureCoordinateElement.H"
#include "inv/elements/SoGLTextureEnabledElement.H"
#include "inv/misc/SoState.H"
#include "inv/elements/SoGLLazyElement.H"

#include "viz/SGPlane.H"
#include "viz/freeglut_font_data.H"


// Include files for new classes

SO_NODE_SOURCE(SGPlane);

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
void SGPlane::initClass()
{
   SO_NODE_INIT_CLASS(SGPlane, SoFaceSet, "FaceSet");
}

SGPlane::SGPlane()
{
   constructor();
}

SGPlane::SGPlane(MPoint c1, MPoint c2, MPoint c3, MPoint c4)
{
  SO_NODE_CONSTRUCTOR(SGPlane);
  constructor();
  setCorners(c1,c2,c3,c4);
}


void SGPlane::constructor()
{
   setColor(1.0, 1.0, 0.0); // yello

   // Some reasonable? default plane at the origin?  Ha Ha its a dot
   MPoint origin;
   setCorners(origin, origin, origin, origin);

   if (SO_NODE_IS_FIRST_INSTANCE()) { 
    GLuint i;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    fontOffset = glGenLists (128);
    for (i = 32; i < 127; i++) {
      glNewList(i+fontOffset, GL_COMPILE);
      glBitmap(8, 13, 0.0, 0.0, 7.0, 0.0, Fixed8x13_Character_Map[i]+1);
      glEndList();
    }
   }
}

void SGPlane::setCorners(MPoint c1, MPoint c2, MPoint c3, MPoint c4)
{
   p_cnrs.clear();
   p_cnrs.push_back(c1);
   p_cnrs.push_back(c2);
   p_cnrs.push_back(c3);
   p_cnrs.push_back(c4);
   regenerate();
}

void SGPlane::setCorners(const vector<MPoint>& corners)
{
   p_cnrs = corners;
   regenerate();
}

void SGPlane::setLabel(const string& label)
{
   p_label = label;
}

string SGPlane::getLabel() const
{
   return p_label;
}

void SGPlane::setColor(float r, float g, float b)
{
   p_r = r;
   p_g = g;
   p_b = b;
}

void SGPlane::regenerate()
{
   int numCorners = p_cnrs.size();
   //for (int idx=0; idx<p_cnrs.size(); idx++) {
   //   p_cnrs[idx].print(" ");;
   //}
     
   // 4 points make up just our one faceset
   int32_t newValues[1];
   newValues[0] = numCorners;
   startIndex = 0;
   numVertices.setValues(0,1,newValues);

   SoVertexProperty * geo = new SoVertexProperty;

   float xyz[3];
   for (int crnr=0; crnr<numCorners; crnr++) {
      xyz[0] = p_cnrs[crnr].x();
      xyz[1] = p_cnrs[crnr].y();
      xyz[2] = p_cnrs[crnr].z();
      geo->vertex.set1Value(crnr, xyz);
   }


   geo->vertex.setNum(numCorners);

   //uint32_t color = SbColor(1.0, 1.0, 0.0).getPackedValue();
   //Note: I did not have any luck forcibly creating the rgba value manually
   // to control transparency.  I guess a material node must be used instead.
   //last part is transparency - 0xaabbggrr
   //color = 0x00ff0000;
//  KLS 9/30/09 Color now set in the material node associated with the plane
//  in SGContainer.  Someday make a group node
//   uint32_t color = SbColor(p_r, p_g, p_b).getPackedValue();
//   geo->orderedRGBA.set1Value(0,color);

   vertexProperty.setValue(geo);
}


void SGPlane::GLRender(SoGLRenderAction *action)
{
   SoFaceSet::GLRender(action);

   // First see if the object is visible and should be rendered
   // now. This is a method on SoShape that checks for INVISIBLE
   // draw style, BOUNDING_BOX complexity, and delayed
   // transparency.
   if (! shouldGLRender(action))
      return;
   glPushMatrix();

   // Note this has changed slightly in Inventor version 2.1.
   // The texture coordinate type now is either FUNCTION or DEFAULT.
   // Texture coordinates are needed only for DEFAULT textures.

//   glDisable(GL_LIGHTING);
   // Place a label at the first corner location
   MPoint lpos = p_cnrs[0];
   char p_text[80];
   strcpy(p_text,p_label.c_str());
   //glColor3f(1.0,0.0, 0.0);
   glColor3f(p_r, p_g, p_b);
   glRasterPos3f(lpos.x(),lpos.y(),lpos.z() ); 
   glPushAttrib (GL_LIST_BIT);
   glListBase(fontOffset);  // fontOffset is static global 
   glCallLists(strlen(p_text), GL_UNSIGNED_BYTE, (GLubyte *)p_text);
   glPopAttrib ();

//   glEnable(GL_LIGHTING);

   // Restore the GL matrix
   glPopMatrix();
}

SGPlane::~SGPlane()
{
} 
