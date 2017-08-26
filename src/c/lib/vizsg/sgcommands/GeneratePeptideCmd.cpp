/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "util/SFile.hpp"

#include "util/EventDispatcher.hpp"
#include "util/Event.hpp"
  using namespace ecce;

#include "viz/GeneratePeptideCmd.hpp"
#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"

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
