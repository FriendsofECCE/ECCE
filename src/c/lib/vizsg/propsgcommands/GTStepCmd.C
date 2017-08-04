#include <iostream>
  using std::cout;
  using std::endl;

#include "util/InternalException.H"

#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;

#include "dsm/IPropCalculation.H"

#include "tdat/PropTSVecTable.H"
#include "tdat/LatticeDef.H"


#include "viz/GTStepCmd.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"


GTStepCmd::GTStepCmd(const string& name, Receiver *receiver,
        IPropCalculation *calc)
  : PropCmd(name,receiver,calc)
{
  init();
}


GTStepCmd::~GTStepCmd()
{
}


void GTStepCmd::init()
{
  addParameter(new CommandParameter("Index", 0));
  addParameter(new CommandParameter("PropKey", "GEOMTRACE"));
}


bool GTStepCmd::execute() throw(EcceException)
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *sgfrag = (SGFragment*)sg->getFragment();
  LatticeDef *lattice = sgfrag->getLattice();
  IPropCalculation *calc = getCalculation();

  // parameter info
  int step = getParameter("Index")->getInteger();

  // Get the property 
  string propKey = getParameter("PropKey")->getString();
  PropTSVecTable *trace = (PropTSVecTable*)calc->getProperty(propKey);
  INTERNALEXCEPTION(trace,"trace table pointer is null");

  PropTSVecTable *latticeprop = (PropTSVecTable*)calc->getProperty("LATTICEVEC");
 
  // Could be null if data exists but is corrupted.  So we try not to crash...
  if (trace) {
    // loop through all the atoms and replace with coordinates for this step 
    int natoms = sgfrag->numAtoms() ;
    for (int j = 0; j < natoms; j++) {
      double coords[] = {
        trace->value(step,j,0),
        trace->value(step,j,1),
        trace->value(step,j,2)
      };
      sgfrag->atomRef(j)->coordinates(coords);
    }
    //TODO decide what to do if there is no lattice - create one?
    if (lattice && latticeprop) {
         float x,y,z;
         x = latticeprop->value(step,0,0);
         y = latticeprop->value(step,0,1);
         z = latticeprop->value(step,0,2);

         MPoint a1(x,y,z);
         x = latticeprop->value(step,1,0);
         y = latticeprop->value(step,1,1);
         z = latticeprop->value(step,1,2);
         MPoint a2(x,y,z);
         x = latticeprop->value(step,2,0);
         y = latticeprop->value(step,2,1);
         z = latticeprop->value(step,2,2);
         MPoint a3(x,y,z);
         a1.print("a1");
         a2.print("a2");
         a3.print("a3");
         lattice->assign(a1,a2,a3);
         sg->touchLattice() ;
    }

    //todo why call these functions if nothing was done - restructure...
    sgfrag->touch() ;
    sg->adjustMeasures();
    //TODO send the appropriate message - do we need a latticechange??
    //if not, may need to have a look at StepChange handling
    EventDispatcher::getDispatcher().publish(Event("StepChange"));
  }

  return true;
}

