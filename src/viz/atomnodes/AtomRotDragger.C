//////////////////////////////////////////////////////////////////////////////
// $Source$
//
// Description
//   In the interest of getting this done, the source is copied from
//   what Paulo did.  It seems like this class should handle the creation
//   of the tranform and scale but I think that there may be inventor
//   reasons why this shouldn't be done this way.
//////////////////////////////////////////////////////////////////////////////
#include "inv/nodes/SoRotation.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoTransform.H"
#include "inv/nodes/SoSurroundScale.H"
#include "inv/nodes/SoAntiSquish.H"

#include "viz/AtomRotDragger.H"
#include "viz/SGFragment.H"

SO_NODE_SOURCE(AtomRotDragger);

//  Initializes the type ID for this dragger node. This
//  should be called once after SoInteraction::init().
void AtomRotDragger::initClass()
{
  SO_NODE_INIT_CLASS(AtomRotDragger, SoRotateDiscDragger, "SoRotateDiscDragger");	
}

// This is for inventor - shouldn't be used.
AtomRotDragger::AtomRotDragger()
{
  p_frag = 0;
  p_at1 = 0;
  p_at2 = 0;
  p_parent = 0;
  p_old_ang_rb = 0.;
}

AtomRotDragger::AtomRotDragger(SGFragment *frag, TAtm *at1, TAtm *at2,
    SoSeparator *parent)
{
  p_frag = frag;
  p_at1 = at1;
  p_at2 = at2;
  p_parent = parent;
  p_old_ang_rb = 0.;
}

AtomRotDragger::~AtomRotDragger()
{
}


void AtomRotDragger::jump()
{

  SbVec3f pt1,pt2; 

  float xyz[3];
  const double * coord;

  coord = p_at1->coordinates();
  xyz[0] = (float) coord[0];
  xyz[1] = (float) coord[1];
  xyz[2] = (float) coord[2];
  pt1.setValue( xyz[0],  xyz[1] , xyz[2]);

  coord = p_at2->coordinates();
  xyz[0] = (float) coord[0];
  xyz[1] = (float) coord[1];
  xyz[2] = (float) coord[2];
  pt2.setValue( xyz[0],  xyz[1] , xyz[2]);

  // p_tran has to be set by caller or this will dump.
  p_tran->translation.setValue(0.5 * ( pt1 + pt2));
  p_tran->rotation.setValue(SbRotation (SbVec3f(0.,0.,1.) ,(pt2 - pt1)) );


}

TAtm *AtomRotDragger::atom1()
{ return p_at1; }

TAtm *AtomRotDragger::atom2()
{ return p_at2; }

SGFragment *AtomRotDragger::getFragment()
{ return p_frag; }

SoSeparator *AtomRotDragger::getParent()
{ return p_parent; }

SoTransform *AtomRotDragger::getTransform()
{ return p_tran; }

float AtomRotDragger::getRotAngle()
{ return p_old_ang_rb; }

void AtomRotDragger::setTransform(SoTransform *trans)
{
  p_tran = trans;
}

void AtomRotDragger::setRotAngle(float value)
{ p_old_ang_rb = value; }
