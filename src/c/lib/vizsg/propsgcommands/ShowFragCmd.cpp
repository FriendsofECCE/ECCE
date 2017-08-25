#include <iostream>
  using std::cout;
  using std::endl;


#include "util/InternalException.H"

#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;


#include "dsm/IPropCalculation.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/PropSGFragment.H"
#include "viz/ShowFragCmd.H"

ShowFragCmd::ShowFragCmd(const string& name,Receiver *receiver,
     IPropCalculation * calc)
  : PropCmd(name,receiver,calc)
{
  init();
}

ShowFragCmd::~ShowFragCmd() 
{
}

void ShowFragCmd::init()
{
   // The step index; -1 implies the last one
   addParameter(new CommandParameter("index", -1));
}


bool ShowFragCmd::execute() throw(EcceException)
{
   bool ret = true;
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   // parameter info
   int step     = getParameter("index")->getInteger();


   // This is assumed to be a valid pointer though possibly empty.
   SGFragment *baseFrag = (SGFragment*)sg->getFragment();
   IPropCalculation *calc = getCalculation();
   INTERNALEXCEPTION(calc, "No calculation object!");

   if (baseFrag) {

      bool bondsChanged = false;
      // If it hasn't been loaded yet, load from the calc
      // The base coords (last step) are cached for easy swapping.
      if (calc) {

         // Nothing is loaded yet.  In this case load up the original
         // molecule 
         // This is probably horrendously wasteful but...
         if ( baseFrag->numAtoms() == 0 )  {
            baseFrag->clear();
            calc->getFragment(*baseFrag);
            bondsChanged = true;
         }
         // Not load the requested step
         // Note that -2 means to stick with the original (no step)
         if (step >= -1) {
           calc->getFragmentStep(baseFrag, step); 
         }


         if (bondsChanged) baseFrag->touchNumbers() ;
         sg->touchChemDisplay();
         sg->adjustMeasures();

         sg->touchLattice();


         EventDispatcher::getDispatcher().publish(Event("StepChange"));
      }
   }

   return ret;
}
