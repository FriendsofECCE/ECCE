#include <iostream>
  using std::cout;
  using std::endl;

#include "inv/actions/SoSearchAction.H"

#include "util/InternalException.H"


//#include "tdat/SingleGrid.H"
//#include "tdat/PropVecTable.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"
//#include "dsm/PropFactory.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/PropSGFragment.H"
//#include "viz/VRVector.H"
#include "viz/NModeStepCmd.H"

NModeStepCmd::NModeStepCmd(const string& name,Receiver *receiver,
     IPropCalculation * calc)
  : PropCmd(name,receiver,calc)
{
  init();
}

NModeStepCmd::~NModeStepCmd() 
{
}

void NModeStepCmd::init()
{
   addParameter(new CommandParameter("Index", 0));

}


bool NModeStepCmd::execute()
{
   bool ret = false;
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   // parameter info
   int step     = getParameter("Index")->getInteger();


   SGFragment *baseFrag = (SGFragment*)sg->getFragment();

   // Get the hidden PropSGFragment
   PropSGFragment* propSGFrag = 0;
   SoSearchAction sa;
   sa.setName("HiddenNMFrag");
   sa.setType(PropSGFragment::getClassTypeId());
   sa.setInterest(SoSearchAction::FIRST);
   sa.setSearchingAll(true);
   sa.apply(sg->getNMRoot());
   SoPath *path = sa.getPath();
   if (path != 0) {
      propSGFrag = (PropSGFragment*)path->getTail();
   }


   int natoms = propSGFrag->numAtoms() ;

   // get the table that contains all the step coordinates
   PropTSVecTable* nmtrace = propSGFrag->getTraceTable() ;
   INTERNALEXCEPTION(nmtrace,"trace table pointer is null");

   if (nmtrace) {

      ret = true;

      int table = 0 ;  // we only have one table

      // reset the coordinates in the fragment for each atom
      double coords[3];
      for (int j = 0; j < natoms; j++) {
         coords[0] = nmtrace->value(table, step, j*3);
         coords[1] = nmtrace->value(table, step, (j*3)+1);
         coords[2] = nmtrace->value(table, step, (j*3)+2);

         baseFrag->atomRef(j)->coordinates(coords);
      }
   }

   // ChemDisplay nodes hold a raw pointer to their ChemData, not an
   // SoSFNode field, so baseFrag->touch() alone doesn't reach their
   // render cache -- touchChemDisplay() re-assigns the display's own
   // atomIndex/bondIndex fields, which is what actually invalidates it
   // and forces a redraw. Same idiom ShowFragCmd uses for geometry-trace
   // stepping; without it, only pull-model readers (grid/atom table)
   // see the new step, matching the bug's reported symptom.
   sg->touchChemDisplay(baseFrag);
   baseFrag->touch();
   sg->adjustMeasures();



   return ret;
}
