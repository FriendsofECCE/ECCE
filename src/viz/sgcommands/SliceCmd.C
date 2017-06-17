/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "viz/SliceCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/SGLattice.H"

SliceCmd::SliceCmd(const string& name, Receiver * receiver)
  : FragCloneCmd(name, receiver)
{
  init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
SliceCmd::~SliceCmd()
{

}



void SliceCmd::init()
{
  // Basic parameters for slicing operation. The rest will be derived from
  // fragment and lattice objects
  addParameter(new CommandParameter("h",1));
  addParameter(new CommandParameter("k",1));
  addParameter(new CommandParameter("l",1));
  addParameter(new CommandParameter("displacement1",0.0));
  addParameter(new CommandParameter("displacement2",1.0));
}



bool SliceCmd::execute() throw(EcceException)
{
  bool ret = false;

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();
  if (frag) {

    LatticeDef *lattice = frag->getLattice();
    if (lattice) {

      cloneFragment(frag);

      int h = getParameter("h")->getInteger();
      int k = getParameter("k")->getInteger();
      int l = getParameter("l")->getInteger();
      double displ1 = getParameter("displacement1")->getDouble();
      double displ2 = getParameter("displacement2")->getDouble();

      MPoint normal = lattice->getPlaneNormal(h,k,l);
      MPoint origin = lattice->getPlaneOrigin(normal);

      ret = frag->slice(normal,origin,displ1,displ2);

      frag->touchNumbers();
      sg->touchChemDisplay();
      // event notification or vcommand properties
      EventDispatcher::getDispatcher().publish(Event("GeomChange"));
    }

  }
  return ret;
}

/**
 * Undo the command.
 * Not sure if we'll need to override superclass or not....
 */
void SliceCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   //hack - should this be copied over in fragment?
   DisplayStyle ds = sg->getFragment()->getMainDisplayStyle();

   if (p_undoFrag != 0) {
      restoreFrag(new SGFragment(*p_undoFrag), ds);
      SGFragment *sgfrag = sg->getFragment();

      string replicate;
      replicate = "replicate";
      EventDispatcher::getDispatcher().publish(Event("GeomChange",replicate));
      // We could have possibly removed the first add frag, resulting in no
      // residues (blank slate)
      if (sgfrag->numResidues() > 0) {
        EventDispatcher::getDispatcher().publish(Event("ResidueChange"));
      }
      EventDispatcher::getDispatcher().publish(Event("LatticeChange"));
   }
}
