#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoGetBoundingBoxAction.H"
#include "inv/elements/SoGLTextureCoordinateElement.H"
#include "inv/elements/SoGLTextureEnabledElement.H"

#include "inv/actions/SoPickAction.H"

#include "inv/nodes/SoScale.H"
#include "inv/nodes/SoSeparator.H"

#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoCone.H"
#include "inv/nodes/SoCylinder.H"
#include "inv/nodes/SoTranslation.H"
#include "inv/nodes/SoTransform.H"
#include "viz/freeglut_font_data.H"

#include "viz/SGLattice.H"

#include <iostream>
  using std::cout;
  using std::endl;


SO_NODE_SOURCE(SGLattice);

void SGLattice::initClass()
{
   SO_NODE_INIT_CLASS(SGLattice, SoLineSet, "LineSet");
}


SGLattice::SGLattice()
{
  SO_NODE_CONSTRUCTOR(SGLattice);
  constructor();
}


void SGLattice::constructor()
{
   // set up 12 lines (2 points each)
   // This is the default box but of course it changes with replication
   int cnt = 12;
   int32_t newValues[12] = { 2,2,2, 2,2,2, 2,2,2, 2,2,2 };
   numVertices.setValues(0,cnt,newValues);
   p_line = new SoVertexProperty;
   vertexProperty.setValue(p_line);

   // The bulk of the lattice is one color but the primary lattice 
   // vectors each have their own color cause it looks nice.
   p_xcolor = SbColor(1.0, 0.0, 0.0).getPackedValue();
   p_ycolor = SbColor(0.0, 1.0, 0.0).getPackedValue();
   p_zcolor = SbColor(0.0, 0.0, 1.0).getPackedValue();
   p_color = SbColor(0.5, 0.5, 0.5).getPackedValue();


   // Each vertex will have its own color so that we can color 
   // a1, a2, a3 primary lines differently.  See SoMaterialBinding
   p_line->materialBinding = SoVertexProperty::PER_VERTEX;

   updateColors();

   // Don't do the font initialization below because it results in crashes
   // with some 64-bit OpenGL libraries including RHEL 5 using standard
   // Mesa GL.  Since text isn't used with the render area for lattices,
   // I'm guessing this code was a copy/paste from somewhere else since
   // removing the code doesn't cause any issues.
   // GDB 5/2/12
#if 000
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
#endif
}

void SGLattice::GLRender(SoGLRenderAction *action)
{
   SoLineSet::GLRender(action);

   // First see if the object is visible and should be rendered
   // now. This is a method on SoShape that checks for INVISIBLE
   // draw style, BOUNDING_BOX complexity, and delayed
   // transparency.
   if (! shouldGLRender(action))
      return;

   glPushMatrix();

   float offset = .05;

   // OK this is truely an embarrassing hack!
   // There is an off by 1 error indexing into the bitmap.  So
   // instead of using a,b,c below, I use b,c,d to correct it.
   // The code is copied from EcceAxis - can't see what I'm doing wrong!!!!
   // signed anonymous:)

   //glEnable(GL_LIGHTING);
   char text[80];
   strcpy(text,"b");
   glColor3f(1.0,0.0, 0.0);
   glRasterPos3f(p_a1str.x()-offset,p_a1str.y()-offset, p_a1str.z()-offset ); 
   glPushAttrib (GL_LIST_BIT);
   glListBase(fontOffset);  // fontOffset is static global 
   glCallLists(strlen(text), GL_UNSIGNED_BYTE, (GLubyte *)text);
   glPopAttrib ();

   glColor3f(0.0, 1.0, 0.0);
   strcpy(text,"c");
   glRasterPos3f(p_a2str.x()-offset,p_a2str.y()-offset, p_a2str.z()-offset ); 
   glPushAttrib (GL_LIST_BIT);
   glListBase(fontOffset);  // fontOffset is static global 
   glCallLists(strlen(text), GL_UNSIGNED_BYTE, (GLubyte *)text);
   glPopAttrib ();



   glColor3f(0.0, 0.0, 1.0);
   strcpy(text,"d");
   glRasterPos3f(p_a3str.x()-offset,p_a3str.y()-offset, p_a3str.z()-offset ); 
   glPushAttrib (GL_LIST_BIT);
   glListBase(fontOffset);  // fontOffset is static global 
   glCallLists(strlen(text), GL_UNSIGNED_BYTE, (GLubyte *)text);

   // Restore the GL matrix
   glPopMatrix();
}


void SGLattice::setLattice(LatticeDef *lattice)
{

   float xyz[3];

   if (lattice) {
      int na1, na2, na3;
      lattice->getReplicationFactors(na1, na2, na3);

      vector<MPoint> *edges = lattice->toLines();


      int totPoints = 0;
      int nlines = edges->size();

      // Get the vectors that tell us the increment in each direction
      vector<MPoint> *vecs = lattice->toVectors();
      MPoint a1 = (*vecs)[0];
      MPoint a2 = (*vecs)[1];
      MPoint a3 = (*vecs)[2];
      delete vecs;

      // Define the base location of the axis labels.
      p_a1str = (*edges)[1];
      p_a2str = (*edges)[3];
      p_a3str = (*edges)[5];


      p_line->vertex.startEditing();
      double xt, yt, zt;
      for (int a=0; a<na1; a++) {
         for (int aa=0; aa<na2; aa++) {
            for (int aaa=0; aaa<na3; aaa++) {
               xt = a1.x() * a + a2.x() * aa + a3.x() * aaa;
               yt = a1.y() * a + a2.y() * aa + a3.y() * aaa;
               zt = a1.z() * a + a2.z() * aa + a3.z() * aaa;
               //cout << "deltas" << xt << " " << yt << " " << zt << endl;
               for (int idx=0; idx<nlines; idx+=2) {
                  MPoint s = (*edges)[idx];
                  //s.print("start ");
                  xyz[0] = s.x() + xt;
                  xyz[1] = s.y() + yt;
                  xyz[2] = s.z() + zt;
                  //cout << "start " << xyz[0] << " " << xyz[1] << " " << xyz[2] << endl;
                  p_line->vertex.set1Value(totPoints++, xyz);

                  MPoint e = (*edges)[idx+1];
                  //e.print("end ");
                  xyz[0] = e.x() + xt;
                  xyz[1] = e.y() + yt;
                  xyz[2] = e.z() + zt;
                  //cout << "end " << xyz[0] << " " << xyz[1] << " " << xyz[2] << endl;
                  p_line->vertex.set1Value(totPoints++, xyz);
               }
            }
         }
      }
      p_line->vertex.finishEditing();

      // Update the counts and the definition of the lines
      // Two points per line
      numVertices.setNum(totPoints/2); // num lines
      numVertices.startEditing();
      for (int idx=0; idx<totPoints/2; idx++) {
         numVertices.set1Value(idx,2);
      }
      numVertices.finishEditing();
      p_line->vertex.setNum(totPoints);

      // Update the color binding array
      updateColors();

      delete edges;

   } else {
      p_line->vertex.setNum(0);
      numVertices.setNum(0);
   }
   touch();

   /*
   cout << "SGLattice lines " << endl;
   int num = p_line->vertex.getNum();
   for (int x=0; x<num; x++) {
      SbVec3f f = p_line->vertex[x];
      cout << "line " << x << " " << f[0] << "," << f[1] << "," << f[2] << endl;
   }
   */
}

/**
 * Set the color array so that:
 *    the first line is red,
 *    the second line is green,
 *    the third line is blue,
 *    the remainder are gray.
 */
void SGLattice::updateColors()
{
   int cnt =  p_line->vertex.getNum();
   p_line->orderedRGBA.set1Value(0,p_xcolor);
   p_line->orderedRGBA.set1Value(1,p_xcolor);
   p_line->orderedRGBA.set1Value(2,p_ycolor);
   p_line->orderedRGBA.set1Value(3,p_ycolor);
   p_line->orderedRGBA.set1Value(4,p_zcolor);
   p_line->orderedRGBA.set1Value(5,p_zcolor);
   for (int idx=6; idx<cnt; idx++) {
      p_line->orderedRGBA.set1Value(idx,p_color);
   }
}


SGLattice::~SGLattice()
{
}

