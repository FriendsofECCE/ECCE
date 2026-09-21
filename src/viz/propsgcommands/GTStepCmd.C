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


bool GTStepCmd::execute()
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

    // ChemDisplay nodes hold a raw pointer to their ChemData, not an
    // SoSFNode field, so sgfrag->touch() alone doesn't reach their render
    // cache -- touchChemDisplay() re-assigns the display's own
    // atomIndex/bondIndex fields, which is what actually invalidates it
    // and forces a redraw.
    //
    // Without this, stepping through a geometry trace moved the atoms in
    // the data model and published StepChange (so the atom table and any
    // other pull-model reader followed along perfectly) while the 3-D
    // view never changed at all. GeomTracePropertyPanel::processStep()
    // does call sg.touchChemDisplay(), but only inside its `p_recompute`
    // branch, so with bond recomputation off nothing ever invalidated
    // the display. Confirmed live on an ORCA GeoVib job, 2026-09-21:
    // "table changes but the 3D view stays static".
    //
    // This is the identical fix already applied to the vibration-mode
    // sibling in NModeStepCmd.C, whose own comment even cites geometry-
    // trace stepping as the precedent -- the geometry-trace stepper
    // itself was simply never updated to match.
    sg->touchChemDisplay(sgfrag);
    sgfrag->touch() ;
    sg->adjustMeasures();

    // Diagnostic for issue #74 ("table updates, 3-D view stays static").
    // Adding touchChemDisplay() above did not fix it in live testing, and
    // rather than guess again this reports what actually happens on a
    // step: set ECCE_DEBUG_GEOMTRACE=1 before launching builder.
    //
    // What the output tells you:
    //  - no lines at all when stepping => this command is not running,
    //    so the problem is upstream (the panel's step control isn't
    //    reaching processStep()), not in the redraw at all.
    //  - lines appear with a changing step and changing coords => the
    //    data path and the display invalidation both ran, so the fault
    //    is in the render/repaint layer (suspect SoWxRenderArea's
    //    p_inPaint/p_redrawPending path).
    //  - steps=1 => there is only one frame, so nothing can visibly
    //    move and this is not a bug at all.
    if (getenv("ECCE_DEBUG_GEOMTRACE") != 0) {
      std::cerr << "[GEOMTRACE] step=" << step
           << " steps=" << trace->tables()
           << " natoms=" << natoms
           << " traceRows=" << trace->rows()
           << " atom0=(" << trace->value(step,0,0)
           << "," << trace->value(step,0,1)
           << "," << trace->value(step,0,2) << ")"
           << std::endl;
    }
    //TODO send the appropriate message - do we need a latticechange??
    //if not, may need to have a look at StepChange handling
    EventDispatcher::getDispatcher().publish(Event("StepChange"));
  }

  return true;
}

