#include "viz/AtomNodesInit.hpp"
 
#include "viz/SGFragment.hpp"

#include "viz/AtomMeasureDist.hpp"
#include "viz/AtomMeasureAngle.hpp"
#include "viz/AtomMeasureTorsion.hpp"
#include "viz/AtomLine.hpp"
#include "viz/AtomRotDragger.hpp"
#include "viz/AtomRTDragger.hpp"
#include "viz/MoveAction.hpp"
#include "viz/AtomDeleteAction.hpp"
#include "viz/SGLattice.hpp"
//#include "viz/AtomCylinderDist.hpp"


void AtomNodesInit::initClasses()
{
  AtomRTDragger::initClass();
  AtomRotDragger::initClass();
  
  AtomMeasureDist::initClass();
  //AtomCylinderDist::initClass();
  AtomMeasureAngle::initClass();
  AtomMeasureTorsion::initClass();
  AtomLine::initClass();

  MoveAction::initClass();
  AtomDeleteAction::initClass();

  SGLattice::initClass();
}

