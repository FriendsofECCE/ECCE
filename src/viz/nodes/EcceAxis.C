#include <iostream>
using namespace std;
#include <GL/gl.h>
#include "inv/SbBox.H"
#include "inv/SoPrimitiveVertex.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/elements/SoGLLazyElement.H" 
#include "inv/elements/SoGLTextureCoordinateElement.H"
#include "inv/elements/SoGLTextureEnabledElement.H"
#include "inv/elements/SoMaterialBindingElement.H"
#include "inv/elements/SoViewVolumeElement.H"

#include "inv/misc/SoState.H"
#include "viz/EcceAxis.H"
#include "viz/freeglut_font_data.H"
#include "viz/EcceDispParam.H"

static float axisLength = 1.2;

SO_NODE_SOURCE(EcceAxis);


/**
 * Perform class static initialization.
 * Must be called before class is used.
 */
void EcceAxis::initClass()
{
   // Initialize type id variables
   SO_NODE_INIT_CLASS(EcceAxis, SoLineSet, "LineSet");
}



/**
 * Creates an axis line set
 * At construction time, an axis centered at 0,0,0 is created.
 * However, this code has been changed (glRender) to put the axis
 * in a constant location on the near clip plane. So other than creating
 * the line set, most of this code is OBE.
 */
EcceAxis::EcceAxis()
{
  
   SO_NODE_CONSTRUCTOR(EcceAxis);
   int32_t newValues[3] = { 2,2,2 };
   numVertices.setValues(0,3,newValues);

   line = new SoVertexProperty;

   float xyz[3];

   xyz[0] = -0.5;
   xyz[1] = 0.;
   xyz[2] = 0.;
   line->vertex.set1Value(0, xyz);
   xyz[0] = axisLength;
   xyz[1] = 0.;
   xyz[2] = 0.;
   line->vertex.set1Value(1, xyz);

   xyz[0] = 0.;
   xyz[1] = -0.5;
   xyz[2] = 0.;
   line->vertex.set1Value(2, xyz);
   xyz[0] = 0.;
   xyz[1] = axisLength;
   xyz[2] = 0.;
   line->vertex.set1Value(3, xyz);


   xyz[0] = 0.;
   xyz[1] = 0.;
   xyz[2] = -0.5;
   line->vertex.set1Value(4, xyz);
   
   xyz[0] = 0.;
   xyz[1] = 0.;
   xyz[2] = axisLength;
   line->vertex.set1Value(5, xyz);
   line->vertex.setNum(6);

   vertexProperty.setValue(line);

   //uint32_t color = SbColor(0.0, 1.0, 0.0).getPackedValue();
   line->orderedRGBA.set1Value(0,SbColor(1.0, 0.0, 0.0).getPackedValue());
   line->orderedRGBA.set1Value(1,SbColor(1.0, 0.0, 0.0).getPackedValue());
   line->orderedRGBA.set1Value(2,SbColor(0.0, 1.0, 0.0).getPackedValue());
   line->orderedRGBA.set1Value(3,SbColor(0.0, 1.0, 0.0).getPackedValue());
   line->orderedRGBA.set1Value(4,SbColor(0.0, 0.0, 1.0).getPackedValue());
   line->orderedRGBA.set1Value(5,SbColor(0.0, 0.0, 1.0).getPackedValue());
   line->materialBinding = SoVertexProperty::PER_VERTEX;

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



/**
 * Destructor.
 */
EcceAxis::~EcceAxis()
{
}



/**
 * The original center rendering.
 * See also the GLRenderCorner method which I didn't get working correctly.
 *
 */
void EcceAxis::GLRender(SoGLRenderAction *action)
{
//  float r,g,b;
//  EcceDispParam options;
//  options.getForeground(r,g,b);
//   line->orderedRGBA.setValue(SbColor(r,g,b).getPackedValue());
#ifdef corneraxis
   //NOTE if you turn this on, do the same in VRViewer
   GLRenderCorner(action);
#else
   GLRenderCenter(action);
#endif
}

void EcceAxis::GLRenderCenter(SoGLRenderAction *action)
{
   SoLineSet::GLRender(action);

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
   char p_text[80];
   strcpy(p_text,"x");
   glColor3f(1.0,0.0, 0.0);
   glRasterPos3f(axisLength,0.05,0. ); 
   glPushAttrib (GL_LIST_BIT);
   glListBase(fontOffset);  // fontOffset is static global 
   glCallLists(strlen(p_text), GL_UNSIGNED_BYTE, (GLubyte *)p_text);
   glPopAttrib ();

   glColor3f(0.0, 1.0, 0.0);
   glRasterPos3f(0.05,axisLength,0. );
   strcpy(p_text,"y");
   glPushAttrib (GL_LIST_BIT);
   glListBase(fontOffset);  // fontOffset is static global 
   glCallLists(strlen(p_text), GL_UNSIGNED_BYTE, (GLubyte *)p_text);
   glPopAttrib ();

   glColor3f(0.0, 0.0, 1.0);
   glRasterPos3f(0.,0.05,axisLength );
   strcpy(p_text,"z");
   glPushAttrib (GL_LIST_BIT);
   glListBase(fontOffset);  // fontOffset is static global 
   glCallLists(strlen(p_text), GL_UNSIGNED_BYTE, (GLubyte *)p_text);
   glPopAttrib ();

//   glEnable(GL_LIGHTING);

   // Restore the GL matrix
   glPopMatrix();
}


/**
 * Render the axis in the bottom left corner of viewer.
 * I didn't get the scaling to work correctly so putting old code back
 * but saving this.
 */
void EcceAxis::GLRenderCorner(SoGLRenderAction *action)
{
   const SbViewVolume &vv = SoViewVolumeElement::get(action->getState());

   // The bottom-left is 0,0, the upper right is 1,1
   // We pick a box of a size we like where will will draw the axis.
   const SbVec2f pos(0.030,0.030);  // lower left of box
   const SbVec2f ur(0.090,0.090);   // upper right of box


   float upx =0; float upy =0;float upz =0;
   float x =0;float y =0;float z =0;

   SbVec3f urpp = vv.getPlanePoint(vv.getNearDist()+ 0.1 * vv.getDepth(), ur);
   urpp.getValue(upx,upy,upz);

   SbVec3f pp = vv.getPlanePoint(vv.getNearDist()+ 0.1 * vv.getDepth(), pos);
   pp.getValue(x,y,z);
   float dist = upx - x;

   float negoffset = dist * 0.35; // arbitrary % for neg portion of each line
   axisLength = dist - negoffset;
   float xyz[3];
   //cout << "dist " << dist << " " << negoffset << " " << axisLength << endl;


   xyz[0] = x + - negoffset;
   xyz[1] = y + 0.;
   xyz[2] = z + 0.;
   line->vertex.set1Value(0, xyz);
   xyz[0] = x + axisLength;
   xyz[1] = y + 0.;
   xyz[2] = z + 0.;
   line->vertex.set1Value(1, xyz);

   xyz[0] = x + 0.;
   xyz[1] = y+ -negoffset;
   xyz[2] = z + 0.;
   line->vertex.set1Value(2, xyz);
   xyz[0] = x + 0.;
   xyz[1] = y+ axisLength;
   xyz[2] = z + 0.;
   line->vertex.set1Value(3, xyz);


   xyz[0] = x + 0.;
   xyz[1] = y + 0.;
   xyz[2] = z+ -negoffset;
   line->vertex.set1Value(4, xyz);

   xyz[0] = x;
   xyz[1] = y;
   xyz[2] = z+ axisLength;
   line->vertex.set1Value(5, xyz);
   line->vertex.setNum(6);

   SoLineSet::GLRender(action);

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

   float labelOffset = .01;
   glDisable(GL_LIGHTING);
   char p_text[80];
   strcpy(p_text,"x");
   glRasterPos3f(x+axisLength,y+labelOffset,z+0. ); 
   glPushAttrib (GL_LIST_BIT);
   glListBase(fontOffset);  // fontOffset is static global 
   glCallLists(strlen(p_text), GL_UNSIGNED_BYTE, (GLubyte *)p_text);
   glPopAttrib ();

   glRasterPos3f(x+labelOffset,y+axisLength,z+0. );
   strcpy(p_text,"y");
   glPushAttrib (GL_LIST_BIT);
   glListBase(fontOffset);  // fontOffset is static global 
   glCallLists(strlen(p_text), GL_UNSIGNED_BYTE, (GLubyte *)p_text);
   glPopAttrib ();

   glRasterPos3f(x+0.,y+labelOffset,z+axisLength );
   strcpy(p_text,"z");
   glPushAttrib (GL_LIST_BIT);
   glListBase(fontOffset);  // fontOffset is static global 
   glCallLists(strlen(p_text), GL_UNSIGNED_BYTE, (GLubyte *)p_text);
   glPopAttrib ();


   glEnable(GL_LIGHTING);


   // Restore the GL matrix
   glPopMatrix();
}
