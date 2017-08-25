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

#include "tdat/FragUtil.H"

#include "viz/MakeResidueCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

MakeResidueCmd::MakeResidueCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
MakeResidueCmd::~MakeResidueCmd()
{

}



void MakeResidueCmd::init()
{
   setHelpMessage("Make structure into residue.");
}



bool MakeResidueCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag->numResidues() == 0 && frag->numAtoms() > 0 ) {
      ret = true;

      cloneFragment(frag);

      FragUtil fragutil(frag);
      fragutil.createResidues();

      frag->touchNumbers();
      sg->touchChemDisplay();

      string ename = "GeomChange";
      if (frag->numResidues() > 0) {
        ename = "ResidueChange";
      }
      Event event(ename);
      EventDispatcher::getDispatcher().publish(event);
   }

   return ret;
}
