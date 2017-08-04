///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: AtomLine.C
//
//
// Description
//  This silly class just displays a line between two atoms.  It was once
//  suggested as useful with depth perception and it shall live forever
//  more.
///////////////////////////////////////////////////////////////////////////////
#include <GL/gl.h>
#include "inv/SbBox.H"
#include "inv/SoPickedPoint.H"
#include "inv/SoPrimitiveVertex.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoRayPickAction.H"
#include "inv/elements/SoGLLazyElement.H" 
#include "inv/elements/SoGLTextureCoordinateElement.H"
#include "inv/elements/SoGLTextureEnabledElement.H"
#include "inv/elements/SoMaterialBindingElement.H"
#include "inv/elements/SoModelMatrixElement.H"
#include "inv/misc/SoState.H"

#include "viz/freeglut_font_data.H"

#include "viz/SGFragment.H"
#include "viz/AtomLine.H"


SO_NODE_SOURCE(AtomLine);


//=============================================================================
//=============================================================================
void AtomLine::initClass()
{
  // Initialize type id variables
  SO_NODE_INIT_CLASS(AtomLine, SoLineSet, "LineSet");
}

//=============================================================================
//=============================================================================
AtomLine::AtomLine(SGFragment *frag,TAtm * at1,TAtm* at2,SoSeparator *parent)
{
  p_frag = frag;
  p_parent = parent;

  SO_NODE_CONSTRUCTOR(AtomLine);

  p_atFirst   = at1;     
  p_atSecond  = at2;   

  int32_t newValues[1] = { 2 };
  numVertices.setValues(0,1,newValues);

  p_line = new SoVertexProperty;
  vertexProperty.setValue(p_line);
  jump();
}

//=============================================================================
//=============================================================================
void  AtomLine::jump()
{
  float xyz[3];
  const double * coord;
  coord = p_atFirst->coordinates();

  xyz[0] = (float) coord[0];
  xyz[1] = (float) coord[1];
  xyz[2] = (float) coord[2];
  p_line->vertex.set1Value(0, xyz);

  coord = p_atSecond->coordinates();
  xyz[0] = (float) coord[0];
  xyz[1] = (float) coord[1];
  xyz[2] = (float) coord[2];
  p_line->vertex.set1Value(1, xyz);

  touch();
}

//=============================================================================
//=============================================================================
AtomLine::AtomLine()
{
  p_parent = (SoSeparator*)0;
  p_frag = (SGFragment*)0;
  p_atFirst = 0;
  p_atSecond = 0;

  SO_NODE_CONSTRUCTOR(AtomLine);
  int32_t newValues[1] = { 2 };
  numVertices.setValues(0,1,newValues);

  p_line = new SoVertexProperty;

  float xyz[3];

  xyz[0] = 0.;
  xyz[1] = 0.;
  xyz[2] = 0.;
  p_line->vertex.set1Value(0, xyz);

  xyz[0] = 1.;
  xyz[1] = 1.;
  xyz[2] = 1.;
  p_line->vertex.set1Value(1, xyz);
  p_line->vertex.setNum(2);

  vertexProperty.setValue(p_line);

  if (SO_NODE_IS_FIRST_INSTANCE()) { 
    GLuint i;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    fontOffset = glGenLists (128);
    for (i = 32; i < 127; i++) {
      glNewList(i+fontOffset, GL_COMPILE);
      glBitmap(8, 13, 0.0, 0.0, 7.0, 0.0, Fixed8x13_Character_Map[i]+1);
      glEndList(); }
  }

}


//=============================================================================
//=============================================================================
AtomLine::~AtomLine()
{
}

SoSeparator *AtomLine::getParent()
{ return p_parent;}

SGFragment *AtomLine::getFragment()
{ return p_frag;}

TAtm *AtomLine::atom1()
{ return p_atFirst;}

TAtm *AtomLine::atom2()
{ return p_atSecond;}
