/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "util/SFile.H"

#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;

#include "viz/GeneratePeptideCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

GeneratePeptideCmd::GeneratePeptideCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
GeneratePeptideCmd::~GeneratePeptideCmd()
{

}



void GeneratePeptideCmd::init()
{
   addParameter(new CommandParameter("sequence", ""));
   setHelpMessage("Click Generate to create Peptide fragment in builder");
}



bool GeneratePeptideCmd::execute() throw(EcceException)
{
  bool ret = true;

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  string sequence = getParameter("sequence")->getString();

  // Set up undo data
  cloneFragment(frag);

  frag->generatePeptide(sequence);

  frag->touchNumbers();
  sg->touchChemDisplay();

  // event notification or vcommand properties
  string ename = "GeomChange";
  if (frag->numResidues() > 0) {
    ename = "ResidueChange";
  }
  Event event(ename);
  EventDispatcher::getDispatcher().publish(event);


  return ret;
}
