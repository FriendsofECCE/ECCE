#include "viz/AtomNodesInit.H"
 
#include "viz/SGFragment.H"

#include "viz/AtomMeasureDist.H"
#include "viz/AtomMeasureAngle.H"
#include "viz/AtomMeasureTorsion.H"
#include "viz/AtomLine.H"
#include "viz/AtomRotDragger.H"
#include "viz/AtomRTDragger.H"
#include "viz/MoveAction.H"
#include "viz/AtomDeleteAction.H"
#include "viz/SGLattice.H"
//#include "viz/AtomCylinderDist.H"


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

