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

#include "viz/AtomMeasureAngle.H"
#include "viz/freeglut_font_data.H"
#include "viz/EcceDispParam.H"
#include "viz/SGFragment.H"
#include "tdat/Measures.H"




SO_NODE_SOURCE(AtomMeasureAngle);


//=============================================================================
//=============================================================================
void AtomMeasureAngle::initClass()
{
  // Initialize type id variables
  SO_NODE_INIT_CLASS(AtomMeasureAngle, SoShape, "Shape");
}

//=============================================================================
//=============================================================================
AtomMeasureAngle::AtomMeasureAngle(SGFragment *frag, TAtm * at1, TAtm* at2, 
    TAtm* at3, SoSeparator *parent)
{
  p_parent = parent;
  p_frag = frag;

  SO_NODE_CONSTRUCTOR(AtomMeasureAngle);
  setName("Angle");

  p_atFirst   = at1->index();     
  p_atSecond  = at2->index();     
  p_atThird   = at3->index();   

  char buf[120];
  sprintf(buf,"%d-%d-%d",p_atFirst+1,p_atSecond+1,p_atThird+1);
  p_angleName = buf;

  int32_t newValues[2] = { 3 ,11};
  numVertices.setValues(0,2,newValues);

  p_line = new SoVertexProperty;

  if (SO_NODE_IS_FIRST_INSTANCE()) { 
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
  }

  jump();

}


//=============================================================================
//=============================================================================
AtomMeasureAngle::AtomMeasureAngle
( int atm1, int atm2, int atm3, ChemData* cd )
{
  // get coordinates
  SbVec3f coords1 = cd->getAtomCoordinates(atm1) ;
  SbVec3f coords2 = cd->getAtomCoordinates(atm2) ;
  SbVec3f coords3 = cd->getAtomCoordinates(atm3) ;

  // get names
  //SbString name1 = cd->getAtomName(atm1) ;
  //SbString name2 = cd->getAtomName(atm2) ;
  //SbString name3 = cd->getAtomName(atm3) ;

  // create atoms
  TAtm *atom1 = new TAtm() ;
  atom1->coordinates( coords1[0], coords1[1], coords1[2] );
  //atom1->atomicSymbol( name1.getString() ) ;
  TAtm *atom2 = new TAtm() ;
  atom2->coordinates( coords2[0], coords2[1], coords2[2] );
  //atom2->atomicSymbol( name2.getString() ) ;
  TAtm* atom3 = new TAtm() ;
  atom3->coordinates( coords3[0], coords3[1], coords3[2] );
  //atom3->atomicSymbol( name3.getString() ) ;

  // call original constructor
  *this = AtomMeasureAngle(0, atom1, atom2, atom3, 0) ;

}

//=============================================================================
//  new fragment - used in calcviewer for measures.  We need to get the
//  new coordinates copied over
//=============================================================================
void AtomMeasureAngle::replaceSGFragment(SGFragment* frag)
{
   p_frag = frag ;

   jump() ;

}

//=============================================================================
//=============================================================================
void  AtomMeasureAngle::jump()
{
  float xyz[3];

  const double * at1;
  at1 = p_frag->atomRef(p_atFirst)->coordinates();

  xyz[0] = (float) at1[0];
  xyz[1] = (float) at1[1];
  xyz[2] = (float) at1[2];
  p_line->vertex.set1Value(0, xyz);

  const double * at2;
  at2 = p_frag->atomRef(p_atSecond)->coordinates();
  xyz[0] = (float) at2[0];
  xyz[1] = (float) at2[1];
  xyz[2] = (float) at2[2];
  p_line->vertex.set1Value(1, xyz);

  const double * at3;
  at3 = p_frag->atomRef(p_atThird)->coordinates();
  xyz[0] = (float) at3[0];
  xyz[1] = (float) at3[1];
  xyz[2] = (float) at3[2];
  p_line->vertex.set1Value(2, xyz);

  SbVec3f FirstPoint(at1[0],at1[1],at1[2]);
  SbVec3f SecondPoint(at2[0],at2[1],at2[2]);
  SbVec3f ThirdPoint(at3[0],at3[1],at3[2]);

  SbVec3f vec_1 = FirstPoint - SecondPoint;
  SbVec3f vec_2 = ThirdPoint - SecondPoint;
  float v1 = vec_1.normalize();
  float v2 = vec_2.normalize();
  float r_ang;
  if (v1 < v2) 
    r_ang = 0.5 *  v1;
  else 
    r_ang = 0.5 * v2;
  SbVec3f axis = vec_1.cross(vec_2);
  float val1 = vec_1.dot(vec_2);
  float ang = acos(val1);

  vec_1 = r_ang * vec_1;
  SbRotation rot;
  int num = 10;
  SbVec3f vec_arc;
  float d_ang = ang/(float) num;

  for (int ii = 0; ii <= num; ii++)
  {
    rot.setValue(axis,ii*d_ang);
    rot.multVec(vec_1,vec_arc);
    p_line->vertex.set1Value(3+ii, (SecondPoint.getValue() + vec_arc).getValue());

  }


  p_line->vertex.setNum(14);

  vertexProperty.setValue(p_line);

  // This is a dupliate computation so that we get a value in double
  // precision.  I was having a hard time trying to eliminate the angle
  // computation from the drawing in the existing code so I left it.
  p_value = Measures::computeAngle(atom1(),atom2(),atom3());

  touch();


}

//=============================================================================
//=============================================================================
AtomMeasureAngle::AtomMeasureAngle()
{
  p_parent = (SoSeparator*)0;
  p_frag = (SGFragment*)0;

  SO_NODE_CONSTRUCTOR(AtomMeasureAngle);

  int32_t newValues[1] = { 3 };
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


  xyz[0] = -1.;
  xyz[1] = -1.;
  xyz[2] = -1.;
  p_line->vertex.set1Value(2, xyz);
  p_line->vertex.setNum(3);

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
AtomMeasureAngle::~AtomMeasureAngle()
{
}

//=============================================================================
//=============================================================================
void AtomMeasureAngle::GLRender(SoGLRenderAction *action)
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

  static uint32_t lastCacheContext;

  if (lastCacheContext != action->getCacheContext()) {
    lastCacheContext = action->getCacheContext();

#ifdef DEBUG
    cout <<  "#####################" << endl;
    cout <<  "##    new context  ##" << endl;
    cout <<  "#####################" << endl;
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
  // Copied from Torsion to fix backlighting problem
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



  const double * at1 = p_frag->atomRef(p_atFirst)->coordinates();
  const double * at2 = p_frag->atomRef(p_atSecond)->coordinates();
  const double * at3 = p_frag->atomRef(p_atThird)->coordinates();

  SbVec3f FirstPoint(at1[0],at1[1],at1[2]);
  SbVec3f  SecondPoint(at2[0],at2[1],at2[2]);
  SbVec3f  ThirdPoint(at3[0],at3[1],at3[2]);


  SbVec3f vec_1 = FirstPoint - SecondPoint;
  SbVec3f vec_2 = ThirdPoint - SecondPoint;
  float v1 = vec_1.normalize();
  float v2 = vec_2.normalize();
  float r_ang;
  if (v1 < v2) r_ang = 0.5 *  v1;
  else r_ang = 0.5 * v2;
  SbVec3f axis = vec_1.cross(vec_2);
  float val1 = vec_1.dot(vec_2);
  float ang = acos(val1);


  vec_1 = r_ang * vec_1;
  SbRotation rot;
  SbVec3f vec_arc;
  //float val = 180./3.141516 * ang;

  // Need to use the color the user specified.
  float r,g,b;
  EcceDispParam options;
  options.getForeground(r,g,b);

#ifdef READY
  int num = 10;

  float d_ang = ang/(float) num;


  glBegin(GL_LINE_STRIP);
  for (int ii = 0; ii <= num; ii++)
  {
    rot.setValue(axis,ii*d_ang);
    rot.multVec(vec_1,vec_arc);
    glVertex3fv((SecondPoint.getValue() + vec_arc).getValue());
  }
  glEnd(); 
#endif

  // LABEL
  rot.setValue(axis,0.5*ang);
  rot.multVec(vec_1,vec_arc);
  SbVec3f LabelPosition = SecondPoint + vec_arc;


  // MOVED TO JUMP 9/25/06
  // This is a dupliate computation so that we get a value in double
  // precision.  I was having a hard time trying to eliminate the angle
  // computation from the drawing in the existing code so I left it.
  //p_value = Measures::computeAngle(atom1(),atom2(),atom3());

  char buf[32];
  sprintf(buf, "%.3lf", p_value);



  // Disabling fixed problem where measure 2...n where only lit on one surface.
  // I'm not sure exactly why....My theory is that the default lighting uses
  // generated normals and they work perfectly well.
  glDisable(GL_LIGHTING);
  glColor3f(r,g,b);

  glRasterPos3fv(LabelPosition.getValue());
  glPushAttrib (GL_LIST_BIT);
  glListBase(fontOffset);
  glCallLists(strlen(buf), GL_UNSIGNED_BYTE, (GLubyte *) buf);
  glPopAttrib ();

  glEnable(GL_LIGHTING);

  // Restore white color of lines.  Not sure why but necessary so that
  // 2nd, 3rd, 4th, etc angle use white lines.
  // KLS 8/6/07 Breaks color drawing in new builder
  glColor3f(1.0, 1.0, 1.0);


  // Restore the GL matrix
  glPopMatrix();
}

float AtomMeasureAngle::getAngle()
{ return p_value; }

SoSeparator *AtomMeasureAngle::getParent()
{ return p_parent;}

SGFragment *AtomMeasureAngle::getFragment()
{ return p_frag;}

TAtm *AtomMeasureAngle::atom1()
{ return p_frag->atomRef(p_atFirst);}

TAtm *AtomMeasureAngle::atom2()
{ return p_frag->atomRef(p_atSecond);}

TAtm *AtomMeasureAngle::atom3()
{ return p_frag->atomRef(p_atThird);}
string AtomMeasureAngle::getAngleName()
{ return p_angleName;}
