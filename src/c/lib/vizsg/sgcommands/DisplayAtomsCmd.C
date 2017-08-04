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

#include "tdat/TBond.H"

#include "viz/DisplayAtomsCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

DisplayAtomsCmd::DisplayAtomsCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
DisplayAtomsCmd::~DisplayAtomsCmd()
{

}



void DisplayAtomsCmd::init()
{
   /** This means essentially showall since there is no hide all */
   addParameter(new CommandParameter("all", false));

   addParameter(new CommandParameter("selected", true));
   addParameter(new CommandParameter("show", false));
   setHelpMessage("Set the display style.");
}



bool DisplayAtomsCmd::execute() throw(EcceException)
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *sgfrag = sg->getFragment();

  bool all = getParameter("all")->getBoolean();
  bool selected = getParameter("selected")->getBoolean();
  bool show = getParameter("show")->getBoolean();

  DisplayStyle style;



  // create bit vector of bonds to show/hide
  vector<TBond*> *allBonds = sgfrag->bonds();
  int numBonds = allBonds->size();
  vector<bool> bondBits;
  vector<int> *bondHighlight = sgfrag->getBondHighlight();
  int numBondHightlights = bondHighlight->size();

  // create bit vector of atoms to show/hide
  int numAtoms = sgfrag->numAtoms();
  vector<bool> atomBits;
  vector<int> *atomHighlight = sgfrag->getAtomHighlight();
  int numAtomHighlights = atomHighlight->size();
  if (all) {
    atomBits.resize(numAtoms, show);
    bondBits.resize(numBonds, show);
  } else {
     // This is tricky because we want to group nubs with their parent
     // atoms even if they aren't selected
     atomBits.resize(numAtoms, selected);
     TAtm *atm;
     for (int i = 0; i < numAtomHighlights; i++) {
        atomBits[(*atomHighlight)[i]] = !selected;
     }

     bondBits.resize(numBonds, selected);
     for (int i = 0; i < numBondHightlights; i++) {
        bondBits[(*bondHighlight)[i]] = !selected;
     }

     // Handle the nubs here.. So wasteful if there are no nubs...
     vector<TAtm*> *conAtoms = 0;
     TAtm *parent;
     for (int i = 0; i < numAtoms; i++) {
        atm = sgfrag->atomRef(i);
        if (atm->atomicSymbol() == "Nub") {
           conAtoms = atm->connectedAtoms();
           parent =  (*conAtoms)[0];
           atomBits[i] = atomBits[parent->index()];
           delete conAtoms;

           const vector<TBond*>& conbonds = atm->bondList(); // should be 1
           bondBits[conbonds[0]->index()] = atomBits[i];
        }

     }
  }

  //// do the atoms ////
  TAtm *anAtom;
  for (int i = 0; i < numAtoms; i++) {
    anAtom = sgfrag->atomRef(i);
    style = anAtom->displayStyle();
    style.setDisplayed(atomBits[i]);
    anAtom->displayStyle(style);
  }


  //// do the bonds ////
  TBond *aBond;
  for (int i = 0; i < numBonds; i++) {
    aBond = (*allBonds)[i];
    style = aBond->displayStyle();
    style.setDisplayed(bondBits[i]);
    aBond->displayStyle(style);
  }

  // Can't forget about residues
  int numResidues = sgfrag->numResidues();
  vector<bool> resbits;
  if (all) {
    resbits.resize(numResidues, show);
  } else {
   sgfrag->atomsToResidues(sgfrag->m_atomHighLight, resbits);
   if (selected) {
      for (int i=0; i<numResidues; i++) resbits[i] = !resbits[i];
   }
  }
  Residue *aRes = 0;
  for (int i = 0; i < numResidues; i++) {
    aRes = sgfrag->residue(i);
    style = aRes->displayStyle();
    style.setDisplayed(resbits[i]);
    aRes->displayStyle(style);
  }

  sgfrag->touchNumbers();
  sg->touchChemDisplay() ;

  EventDispatcher::getDispatcher().publish(Event("VisibilityChange"));

  // It doesn't make sense to keep the hidden things selected.  May
  // confuse user with commands that operate on selection
  if (selected && !all) {
     sgfrag->m_atomHighLight.clear();
     sgfrag->m_bondHighLight.clear();
     EventDispatcher::getDispatcher().publish(Event("SelectionChange"));
  }

  return true;
}



/**
 * @todo support undo of styles.
 */
void DisplayAtomsCmd::undo()
{
}



bool DisplayAtomsCmd::isUndoable() const
{
   return false;
}

