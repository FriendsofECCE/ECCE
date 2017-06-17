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
#include "viz/AtomMeasureTorsion.H"
#include "viz/EcceDispParam.H"
#include "viz/SGFragment.H"
#include "tdat/Measures.H"


SO_NODE_SOURCE(AtomMeasureTorsion);


//=============================================================================
//=============================================================================
void AtomMeasureTorsion::initClass()
{
  // Initialize type id variables
  SO_NODE_INIT_CLASS(AtomMeasureTorsion, SoFaceSet, "FaceSet");
}

//=============================================================================
//=============================================================================
AtomMeasureTorsion::AtomMeasureTorsion(SGFragment *frag, TAtm * at1, 
    TAtm* at2, TAtm* at3 , TAtm* at4, SoSeparator *parent)
{
  p_parent = parent;
  p_frag = frag;

  SO_NODE_CONSTRUCTOR(AtomMeasureTorsion);
  setName("Torsion");

  p_atFirst   = at1->index();     
  p_atSecond  = at2->index();      
  p_atThird = at3->index();      
  p_atFourth = at4->index();   

  char buf[120];
  sprintf(buf,"%d-%d-%d-%d",p_atFirst+1,p_atSecond+1,p_atThird+1,p_atFourth+1);
  p_torsionName = buf;

  int32_t newValues[2] = { 4, 4 };
  numVertices.setValues(0,2,newValues);

  p_planes = new SoVertexProperty;

  vertexProperty.setValue(p_planes);

  // Do this once only or run out of memory real fast
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
AtomMeasureTorsion::AtomMeasureTorsion(int atm1, int atm2, int atm3, int atm4,
    ChemData* cd )
{
  SbVec3f coords1 = cd->getAtomCoordinates(atm1) ;
  SbVec3f coords2 = cd->getAtomCoordinates(atm2) ;
  SbVec3f coords3 = cd->getAtomCoordinates(atm3) ;
  SbVec3f coords4 = cd->getAtomCoordinates(atm4) ;

  //SbString name1 = cd->getAtomName(atm1);
  //SbString name2 = cd->getAtomName(atm2);
  //SbString name3 = cd->getAtomName(atm3);
  //SbString name4 = cd->getAtomName(atm4);

  TAtm *atom1 = new TAtm() ;
  atom1->coordinates( coords1[0], coords1[1], coords1[2] );
  //atom1->atomicSymbol( name1.getString() ) ;
  TAtm *atom2 = new TAtm() ;
  atom2->coordinates( coords2[0], coords2[1], coords2[2] );
  //atom2->atomicSymbol( name2.getString() ) ;
  TAtm *atom3 = new TAtm() ;
  atom3->coordinates( coords3[0], coords3[1], coords3[2] );
  //atom3->atomicSymbol( name3.getString() ) ;
  TAtm *atom4 = new TAtm() ;
  atom4->coordinates( coords4[0], coords4[1], coords4[2] );
  //atom4->atomicSymbol( name4.getString() ) ;

  *this = AtomMeasureTorsion (0, atom1, atom2, atom3, atom4, 0 ) ;

}
//=============================================================================
//  new fragment - used in calcviewer for measures.  We need to get the
//  new coordinates copied over
//=============================================================================
void AtomMeasureTorsion::replaceSGFragment(SGFragment* frag)
{
   p_frag = frag ;

   jump() ;

}


//=============================================================================
//=============================================================================
void  AtomMeasureTorsion::jump()
{
  const double * at1 = p_frag->atomRef(p_atFirst)->coordinates();
  const double * at2 = p_frag->atomRef(p_atSecond)->coordinates();
  const double * at3 = p_frag->atomRef(p_atThird)->coordinates();
  const double * at4 = p_frag->atomRef(p_atFourth)->coordinates();

  SbVec3f FirstPoint(at1[0], at1[1], at1[2]);
  SbVec3f SecondPoint(at2[0], at2[1], at2[2]);
  SbVec3f ThirdPoint(at3[0], at3[1], at3[2]);
  SbVec3f FourthPoint(at4[0], at4[1], at4[2]);


  p_planes->vertex.set1Value(0, FirstPoint.getValue());
  p_planes->vertex.set1Value(1, SecondPoint.getValue());
  p_planes->vertex.set1Value(2, ThirdPoint.getValue());
  p_planes->vertex.set1Value(3, (ThirdPoint + (FirstPoint - SecondPoint)).getValue() );
  p_planes->vertex.set1Value(4, FourthPoint.getValue());
  p_planes->vertex.set1Value(5, ThirdPoint.getValue());
  p_planes->vertex.set1Value(6, SecondPoint.getValue());
  p_planes->vertex.set1Value(7, (SecondPoint + (FourthPoint -ThirdPoint)).getValue() );

  p_planes->vertex.setNum(8);

  // Compute cached torsion.
  p_value = Measures::computeDihedral(atom1(),atom2(),atom3(),atom4());

  touch();

}



//=============================================================================
//=============================================================================
AtomMeasureTorsion::AtomMeasureTorsion()
{
  p_parent = (SoSeparator*)0;
  p_frag = (SGFragment*)0;

  SO_NODE_CONSTRUCTOR(AtomMeasureTorsion);

  int32_t newValues[1] = { 4 };
  numVertices.setValues(0,1,newValues);

  p_planes = new SoVertexProperty;

  float xyz[3];

  xyz[0] = 0.;
  xyz[1] = -1.;
  xyz[2] = 0.;
  p_planes->vertex.set1Value(0, xyz);

  xyz[0] = 0.;
  xyz[1] = 1.;
  xyz[2] = 0.; 
  p_planes->vertex.set1Value(1, xyz);

  xyz[0] = 1.;
  xyz[1] = 1.;
  xyz[2] = 0.;
  p_planes->vertex.set1Value(2, xyz);

  xyz[0] = 1.;
  xyz[1] = -1.;
  xyz[2] = 0.;
  p_planes->vertex.set1Value(3, xyz);
  p_planes->vertex.setNum(4);

  vertexProperty.setValue(p_planes);


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
AtomMeasureTorsion::~AtomMeasureTorsion()
{
}



//=============================================================================
//=============================================================================
void AtomMeasureTorsion::GLRender(SoGLRenderAction *action)
{
  SoFaceSet::GLRender(action);

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

  // Declare a pointer to a GLLazyElement.  This will be used if we
  // send multiple colors.
//#define doweusethis 1
#if doweusethis
  SoGLLazyElement* lazyElt;
#endif



  glPushMatrix();

  // Determine if there's a material bound per part
#if doweusethis
  SoMaterialBindingElement::Binding binding = 
    SoMaterialBindingElement::get(state);
  SbBool materialPerPart =
    (binding == SoMaterialBindingElement::PER_PART ||
     binding == SoMaterialBindingElement::PER_PART_INDEXED);
#endif

  // issue a lazy element send.
  // This send will ensure that all material state in GL is current.

  SoGLLazyElement::sendAllMaterial(state);

  // Render the parts 

#define SEND_VERTEX(vec, s, t)   \
  if (doTextures)                \
    glTexCoord2f(s, t);         \
      glVertex3fv(vec.getValue())


  // Need this to figure out label location only since computation
  // is done as part of Fragment class.
  const double * at1 = p_frag->atomRef(p_atFirst)->coordinates();
  const  double * at2 = p_frag->atomRef(p_atSecond)->coordinates();
  const  double * at3 = p_frag->atomRef(p_atThird)->coordinates();
  const  double * at4 = p_frag->atomRef(p_atFourth)->coordinates();

  SbVec3f FirstPoint(at1[0],at1[1],at1[2]);
  SbVec3f  SecondPoint(at2[0],at2[1],at2[2]);
  SbVec3f ThirdPoint(at3[0],at3[1],at3[2]);
  SbVec3f FourthPoint(at4[0],at4[1],at4[2]);


  SbVec3f  LabelPosition =  0.25 * (FirstPoint + SecondPoint + ThirdPoint + FourthPoint );  




#ifdef READY

  glBegin(GL_LINE_STRIP);
  SEND_VERTEX( ThirdPoint, .25,  0.0);
  SEND_VERTEX( (ThirdPoint + 0.3* (ThirdPoint - SecondPoint)), .25,  0.0);
  glEnd(); 

  glBegin(GL_TRIANGLES);

  if (sendNormals)
    glNormal3fv(n_1.getValue());
  SEND_VERTEX( FirstPoint.getValue(), .25,  0.0);
  SEND_VERTEX( SecondPoint.getValue(), .50,  0.0);
  SEND_VERTEX(ThirdPoint.getValue(), .325, 1.0);



  if (sendNormals)
    glNormal3fv(n_3.getValue());
  SEND_VERTEX(SecondPoint.getValue(), .50,  0.0);
  SEND_VERTEX(ThirdPoint.getValue(), .75,  0.0);
  SEND_VERTEX(FourthPoint.getValue(), .625, 1.0);


  glEnd(); 

#endif

  glPopMatrix();
  // reset the diffuse color, if we sent it twice:
#if doweusethis
  if (materialPerPart) lazyElt->reset(state, SoLazyElement::DIFFUSE_MASK);
#endif



  char buf[32];
  sprintf(buf, "%.3f", p_value);


  // Need to use the color the user specified.
  float r,g,b;
  EcceDispParam options;
  options.getForeground(r,g,b);

  glDisable(GL_LIGHTING);

  glColor3f(r,g,b);


  glRasterPos3fv(LabelPosition.getValue());
  glPushAttrib (GL_LIST_BIT);
  glListBase(fontOffset);
  glCallLists(strlen(buf), GL_UNSIGNED_BYTE, (GLubyte *) buf);
  glPopAttrib ();

  glEnable(GL_LIGHTING);


  // Restore the GL matrix
  glPopMatrix();
}

float AtomMeasureTorsion::getTorsion()
{ return p_value; }

SoSeparator *AtomMeasureTorsion::getParent()
{ return p_parent;}

SGFragment *AtomMeasureTorsion::getFragment()
{ return p_frag;}

TAtm *AtomMeasureTorsion::atom1()
{ return p_frag->atomRef(p_atFirst);}

TAtm *AtomMeasureTorsion::atom2()
{ return p_frag->atomRef(p_atSecond);}

TAtm *AtomMeasureTorsion::atom3()
{ return p_frag->atomRef(p_atThird);}

TAtm *AtomMeasureTorsion::atom4()
{ return p_frag->atomRef(p_atFourth);}

string AtomMeasureTorsion::getTorsionName()
{ return p_torsionName;}
