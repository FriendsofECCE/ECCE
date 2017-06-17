#include <iostream>
  using namespace std;
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
#include "viz/AtomMeasureDist.H"

#include "viz/freeglut_font_data.H"
#include "viz/EcceDispParam.H"
#include "viz/SGFragment.H"


SO_NODE_SOURCE(AtomMeasureDist);


//=============================================================================
//=============================================================================
void AtomMeasureDist::initClass()
{
  // Initialize type id variables
  SO_NODE_INIT_CLASS(AtomMeasureDist, SoLineSet, "LineSet");
}

//=============================================================================
//=============================================================================
AtomMeasureDist::AtomMeasureDist(SGFragment *frag,TAtm *at1,TAtm* at2,
    SoSeparator *parent)
{
  p_distance = 0.;
  p_parent = parent;
  p_frag = frag;

  SO_NODE_CONSTRUCTOR(AtomMeasureDist);

  setName("Distance");

  p_atFirst   = at1->index();
  p_atSecond  = at2->index();

  char buf[120];
  sprintf(buf,"%d-%d",p_atFirst+1,p_atSecond+1);
  p_distanceName = buf;

  int32_t newValues[1] = { 2 };
  numVertices.setValues(0,1,newValues);

  p_line = new SoVertexProperty;
  vertexProperty.setValue(p_line);

  // Create this font list one time only or you'll blow all your memory
  GLint curAlignment;
  glGetIntegerv(GL_UNPACK_ALIGNMENT,&curAlignment);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  fontOffset = glGenLists (128);
  for (GLuint i = 32; i < 127; i++) {
    glNewList(i+fontOffset, GL_COMPILE);
    glBitmap(8, 13, 0.0, 0.0, 7.0, 0.0, Fixed8x13_Character_Map[i]+1);
    glEndList(); 
  }
  glPixelStorei(GL_UNPACK_ALIGNMENT, curAlignment);

  jump();
}

//=============================================================================
//=============================================================================
AtomMeasureDist::AtomMeasureDist(int atm1, int atm2, ChemData* cd )
{
  SbVec3f coords1 = cd->getAtomCoordinates(atm1) ;
  SbVec3f coords2 = cd->getAtomCoordinates(atm2) ;

  //SbString name1 = cd->getAtomName(atm1) ;
  //SbString name2 = cd->getAtomName(atm2) ;

  TAtm *atom1 = new TAtm() ;
  atom1->coordinates( coords1[0], coords1[1], coords1[2] );
  //atom1->atomicSymbol( name1.getString() ) ;

  TAtm *atom2 = new TAtm() ;
  atom2->coordinates( coords2[0], coords2[1], coords2[2] );
  //atom2->atomicSymbol( name2.getString() ) ;

  *this = AtomMeasureDist (0, atom1, atom2, 0) ;

}

//=============================================================================
//  new fragment - used in calcviewer for measures.  We need to get the
//  new coordinates copied over
//=============================================================================
void AtomMeasureDist::replaceSGFragment(SGFragment* frag)
{
   p_frag = frag ;

   jump() ;

}

/**
 * This method moves the measure line vertices which will then be 
 * rendered correctly next time the rendering occurs.  The cached 
 * distance value is also updated now.
 * This method is usually called
 * after some operation that moves an atom that the line is connected
 * to (for example, when a manipulator moves an atom).
 */
void  AtomMeasureDist::jump()
{
  float xyz[3];
  const double * coord1 = p_frag->atomRef(p_atFirst)->coordinates();

  xyz[0] = (float) coord1[0];
  xyz[1] = (float) coord1[1];
  xyz[2] = (float) coord1[2];
  p_line->vertex.set1Value(0, xyz);

  const double * coord2 = p_frag->atomRef(p_atSecond)->coordinates();
  xyz[0] = (float) coord2[0];
  xyz[1] = (float) coord2[1];
  xyz[2] = (float) coord2[2];
  p_line->vertex.set1Value(1, xyz);

  // Recompute the distance
  MPoint firstPoint(coord1[0],coord1[1],coord1[2]);
  MPoint secondPoint(coord2[0],coord2[1],coord2[2]);
  MPoint diff = firstPoint.subtract1(secondPoint);
  p_distance = diff.length();

  touch();
   //cout << "Jump Distance" << getDistance() << endl;
}

///////////////////////////////////////////////////////////////////////////////
// Description
//   This method is needed but I don't know why.  I think some of the
//   inventor macros maybe.
///////////////////////////////////////////////////////////////////////////////
AtomMeasureDist::AtomMeasureDist()
{
  p_parent = (SoSeparator*)0;
  p_frag = (SGFragment*)0;

  SO_NODE_CONSTRUCTOR(AtomMeasureDist);
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
AtomMeasureDist::~AtomMeasureDist()
{
}


//=============================================================================
//=============================================================================
void AtomMeasureDist::GLRender(SoGLRenderAction *action)
{
  SoLineSet::GLRender(action);

  // Access the state from the action
  SoState  *state = action->getState();

  // First see if the object is visible and should be rendered
  // now. This is a method on SoShape that checks for INVISIBLE
  // draw style, BOUNDING_BOX complexity, and delayed
  // transparency.
  if (! shouldGLRender(action))
    return;

  static uint32_t lastCacheContext = 0; 


  if (lastCacheContext != action->getCacheContext()) {
    lastCacheContext = action->getCacheContext();
#if (!defined(INSTALL) && defined(DEBUG))
    //cout <<  "#####################" << endl;
    //cout <<  "##    new context  ##" << endl;
    //cout <<  "#####################" << endl;
#endif
    GLuint i;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    fontOffset = glGenLists (128);
    for (i = 32; i < 127; i++) {
      glNewList(i+fontOffset, GL_COMPILE);
      glBitmap(8, 13, 0.0, 0.0, 7.0, 0.0, Fixed8x13_Character_Map[i]+1);
      glEndList(); }
  }

  glPushMatrix();

  // issue a lazy element send.
  // This send will ensure that all material state in GL is current.
  // Copied from Torsion KLS Aug 07 to fix back lighting problem
  SoGLLazyElement::sendAllMaterial(state);

  /*
  // See if texturing is enabled. If so, we will have to
  // send explicit texture coordinates. The "doTextures" flag
  // will indicate if we care about textures at all.

  // Note this has changed slightly in Inventor version 2.1.
  // The texture coordinate type now is either FUNCTION or DEFAULT.
  // Texture coordinates are needed only for DEFAULT textures.

  SbBool doTextures =
    (SoGLTextureEnabledElement::get(state) &&
     SoTextureCoordinateElement::getType(state) !=
     SoTextureCoordinateElement::FUNCTION);
  */


  // Calculate the length
  const  double * at1 = p_frag->atomRef(p_atFirst)->coordinates();
  const  double * at2 = p_frag->atomRef(p_atSecond)->coordinates();
  MPoint firstPoint(at1[0],at1[1],at1[2]);
  MPoint secondPoint(at2[0],at2[1],at2[2]);
  MPoint diff = firstPoint.subtract1(secondPoint);
  /* This is now done strictly in jump().
  float val = diff.length();
  p_distance = val;
  */


  // Make a string out of it
  char buf[32];
  sprintf(buf, "%.3f", p_distance);

  // Figure out where to position the text
  diff.scale(0.5);
  secondPoint.add(diff);
  SbVec3f tmp(secondPoint.x(),secondPoint.y(), secondPoint.z());

  // Need to use the color the user specified.
  float r,g,b;
  EcceDispParam options;
  options.getForeground(r,g,b);


  // Disabling fixed problem where measure 2...n where only lit on one surface.
  // I'm not sure exactly why....My theory is that the default lighting uses
  // generated normals and they work perfectly well.
  glDisable(GL_LIGHTING);
  glColor3f(r,g,b);

  glRasterPos3fv(tmp.getValue());
  glPushAttrib (GL_LIST_BIT);
  glListBase(fontOffset);
  glCallLists(strlen(buf), GL_UNSIGNED_BYTE, (GLubyte *) buf);
  glPopAttrib ();


  // KLS 8/6/07 Breaks color drawing in new builder
  // Note that this code was NOT in place for torsion
  //glColor3f(1.0,1.0,1.0);
  glEnable(GL_LIGHTING);


  // Restore the GL matrix
  glPopMatrix();


}


float AtomMeasureDist::getDistance()
{ return p_distance; }

SoSeparator *AtomMeasureDist::getParent()
{ return p_parent;}

SGFragment *AtomMeasureDist::getFragment()
{ return p_frag;}

TAtm *AtomMeasureDist::atom1()
{ return p_frag->atomRef(p_atFirst);}

TAtm *AtomMeasureDist::atom2()
{ return p_frag->atomRef(p_atSecond);}

string AtomMeasureDist::getDistanceName()
{ return p_distanceName;}
