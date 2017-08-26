#include "viz/NodesInit.hpp"
 
#include "viz/VR3dXVSmouse.hpp"
#include "viz/RTDraggerNode.hpp"
#include "viz/EcceAxis.hpp"
#include "viz/VRPlane.hpp"
#include "viz/SGPlane.hpp"
#include "viz/MillerPlane.hpp"
#include "viz/StyledPlane.hpp"
#include "viz/VRVector.hpp"
#include "viz/TranslateRadialDragger.hpp"
#include "viz/TranslateRadialManip.hpp"
#include "viz/NearPlaneSep.hpp"
#include "viz/CustomBBoxNode.hpp"
#include "viz/Coordinate3Manip.hpp"


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

