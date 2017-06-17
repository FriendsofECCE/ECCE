#include <iostream>
using namespace std;

#include "inv/actions/SoGLRenderAction.H"


#include "viz/MillerPlane.H"


// Include files for new classes

SO_NODE_SOURCE(MillerPlane);

void MillerPlane::initClass()
{
   SO_NODE_INIT_CLASS(MillerPlane, SGPlane, "SGPlane");
}

MillerPlane::MillerPlane()
{
   constructor();
}

MillerPlane::MillerPlane(MPoint c1, MPoint c2, MPoint c3, MPoint c4)
  : SGPlane(c1, c2, c3, c4)
{
  SO_NODE_CONSTRUCTOR(MillerPlane);
  constructor();
  setCorners(c1,c2,c3,c4);
}


void MillerPlane::constructor()
{
   SGPlane::constructor();
}



void MillerPlane::setIndices(int h, int k, int l)
{
   p_h = h;
   p_k = k;
   p_l = l;
}

void MillerPlane::getIndices(int& h, int& k, int& l)
{
   h = p_h;
   k = p_k;
   l = p_l;
}


MillerPlane::~MillerPlane()
{
} 
