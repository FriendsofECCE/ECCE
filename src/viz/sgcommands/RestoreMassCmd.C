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

#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

#include "viz/RestoreMassCmd.H"



RestoreMassCmd::RestoreMassCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
  init();
}


/**
 * Destructor.
 * Deletes undo related data.
 */
RestoreMassCmd::~RestoreMassCmd()
{
}


void RestoreMassCmd::init()
{
  setHelpMessage("TBD");
}


bool RestoreMassCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();


   if (frag) {
      ret = true;

      // set up undo
      cloneFragment(frag);

      TChartOfNuclides chart;

      vector<TAtm*> *atoms = frag->atoms();
      TAtm *atom = 0;
      if (frag->m_atomHighLight.size() > 0) {
         // Change selected atoms
         for (int idx=0; idx<frag->m_atomHighLight.size(); idx++) {
            atom = frag->atomRef(frag->m_atomHighLight[idx]);

            TRefIsotope const * isotope = 
               chart.mostAbundantIsotope(atom->atomicNumber());
            if (isotope) {
               atom->setMass( static_cast<double>(isotope->isotopeWeight()) );
            } 

         }
      } else {
         // Change all Hydrogens
         for (int idx=0; idx<atoms->size(); idx++) {
            atom = frag->atomRef(idx);
            TRefIsotope const * isotope = 
               chart.mostAbundantIsotope(atom->atomicNumber());
            if (isotope) {
               atom->setMass( static_cast<double>(isotope->isotopeWeight()) );
            } 
         }
      }
      delete atoms;

   }

   EventDispatcher::getDispatcher().publish(Event("GeomChange"));
   return ret;
}
