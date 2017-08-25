/**
 * @file
 *
 *
 */

#include <iostream>
  using std::cout;
  using std::endl;
  using std::ends;

#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;

#include "tdat/TAtm.H"
#include "tdat/TChartOfNuclides.H"
#include "tdat/TRefIsotope.H"

#include "wxgui/WindowEvent.H"
  using namespace ecce;

#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

#include "viz/SetMassCmd.H"



SetMassCmd::SetMassCmd(const string& name, Receiver * receiver, const string& sender)
    : FragCloneCmd(name, receiver)
{
  p_sender = sender;
  init();
}


/**
 * Destructor.
 * Deletes undo related data.
 */
SetMassCmd::~SetMassCmd()
{
}


void SetMassCmd::init()
{
  addParameter(new CommandParameter("atomicSymbol", "H"));
  addParameter(new CommandParameter("atomicMass", 1));
  addParameter(new CommandParameter("value", 0.0));
  setHelpMessage("Set mass command");
}


bool SetMassCmd::execute() throw(EcceException)
{
  bool ret = false;

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  // Get parameter values
  string element = "H";
  element = getParameter("atomicSymbol")->getString();
  int atomicMass = 1;
  atomicMass = getParameter("atomicMass")->getInteger();
  double value = 1.0;
  value = getParameter("value")->getDouble();

  double newMassValue = 0.;
  if (atomicMass != -1) {
     TChartOfNuclides chart;
     TRefIsotope const * isotope = chart.isotope(element, atomicMass);
     if (isotope) {
        newMassValue = static_cast<double>(isotope->isotopeWeight());
     } 
     else {
        throw EcceException( "No such isotope found for " + element, WHERE);
     }
  } 
  else {
     newMassValue = value;
  }

  if (newMassValue > 0.) {
     if (frag) {
        ret = true;

        // set up undo
        cloneFragment(frag);

        vector<TAtm*> *atoms = frag->atoms();
        TAtm *atom = 0;
        if (frag->m_atomHighLight.size() > 0) {
           // Change selected atoms
           for (int idx=0; idx<frag->m_atomHighLight.size(); idx++) {
              atom = frag->atomRef(frag->m_atomHighLight[idx]);
              if (atom->atomicSymbol() == element) {
                 atom->setMass(newMassValue);
              }
           }
        } 
        else {
           for (int idx=0; idx<atoms->size(); idx++) {
              atom = frag->atomRef(idx);
              if (atom->atomicSymbol() == element) {
                 atom->setMass(newMassValue);
              }
           }
        }
        delete atoms;

        // Publish change message
        Event event("GeomChange","",p_sender);
        EventDispatcher::getDispatcher().publish(event);
     }
  } 
  return ret;
}
