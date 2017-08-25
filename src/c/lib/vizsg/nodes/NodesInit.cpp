#include "viz/NodesInit.H"
 
#include "viz/VR3dXVSmouse.H"
#include "viz/RTDraggerNode.H"
#include "viz/EcceAxis.H"
#include "viz/VRPlane.H"
#include "viz/SGPlane.H"
#include "viz/MillerPlane.H"
#include "viz/StyledPlane.H"
#include "viz/VRVector.H"
#include "viz/TranslateRadialDragger.H"
#include "viz/TranslateRadialManip.H"
#include "viz/NearPlaneSep.H"
#include "viz/CustomBBoxNode.H"
#include "viz/Coordinate3Manip.H"


//////////////////////////////////////////////////////////////////////////////
// Description
//   Invoke static class initializer for all nodes in this module.
//////////////////////////////////////////////////////////////////////////////
void NodesInit::initClasses()
{

  RTDraggerNode::initClass();
  TranslateRadialDragger::initClass();
  TranslateRadialManip::initClass();
  VR3dXVSmouse::initClass();
  EcceAxis::initClass();
  VRPlane::initClass();
  VRVector::initClass();
  Coordinate3Manip::initClass();
  NearPlaneSep::initClass();
  CustomBBoxNode::initClass();
  SGPlane::initClass();
  MillerPlane::initClass();
  StyledPlane::initClass();
}

