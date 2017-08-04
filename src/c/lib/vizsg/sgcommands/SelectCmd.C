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

#include "wxgui/WindowEvent.H"
  using namespace ecce;

#include "tdat/TBond.H"

#include "viz/SelectCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

SelectCmd::SelectCmd(const string& name, Receiver * receiver, const string& sender)
    : Command(name, receiver)
{
   init();
   p_sender = sender;
}



/**
 * Destructor.
 * Deletes undo related data.
 */
SelectCmd::~SelectCmd()
{
}



void SelectCmd::init()
{
   vector<string> new_list;
   addParameter(new CommandParameter("clear", false));
   addParameter(new CommandParameter("atoms", new_list));
   addParameter(new CommandParameter("element", ""));
   addParameter(new CommandParameter("resstatus", ""));
   addParameter(new CommandParameter("atom_type", ""));
   addParameter(new CommandParameter("atom_name", ""));
   addParameter(new CommandParameter("residue_name", ""));
   addParameter(new CommandParameter("atom_range1", -1));
   addParameter(new CommandParameter("atom_range2", -1));
   addParameter(new CommandParameter("residues", new_list));
   addParameter(new CommandParameter("chains", new_list));
   addParameter(new CommandParameter("molecule", false));
   addParameter(new CommandParameter("backbone", false));
   addParameter(new CommandParameter("sidechains", false));
   addParameter(new CommandParameter("solvent", false));
   addParameter(new CommandParameter("full_residue", -1));
   addParameter(new CommandParameter("reverse", false));
   addParameter(new CommandParameter("NWChemSelection", ""));
   setHelpMessage("Selects atoms and bonds based on command line input");
}



// @todo chain list
// @todo radius
bool SelectCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   // For undo
   if (p_selAtoms.size() == 0 && p_selBonds.size() == 0) {
      p_selAtoms = frag->m_atomHighLight;
      p_selBonds = frag->m_bondHighLight;
   }

   // Find first selected atom for molecule selection.
   int nAtoms = frag->numAtoms();
   int nBonds = frag->numBonds();
   int iMol;
   if (frag->m_atomHighLight.size()>0) {
     iMol = frag->m_atomHighLight[0];
   } else {
     iMol = nAtoms;
   }

   // Clear last selection first if requested
   if (getParameter("clear")->getBoolean() && !getParameter("molecule")->getBoolean()) {
      frag->m_atomHighLight.clear();
      frag->m_bondHighLight.clear();
      if (p_selAtoms.size() > 0 || p_selBonds.size() > 0) ret = true;
   }

   // Setup selection bit vectors
   int idx;
   vector<bool> atombits(nAtoms, false);
   vector<bool> bondbits(nBonds, false);
   int nsize = frag->m_atomHighLight.size();
   for (idx=0; idx<nsize; idx++) {
      atombits[frag->m_atomHighLight[idx]] = true;
   }
   nsize = frag->m_bondHighLight.size();
   for (idx=0; idx<nsize; idx++) {
      bondbits[frag->m_bondHighLight[idx]] = true;
   }


   vector<string> *atoms = getParameter("atoms")->getStringList();
   vector<string> *residues = getParameter("residues")->getStringList();
   vector<string> *chains = getParameter("chains")->getStringList();
   string nwchemSelection = getParameter("NWChemSelection")->getString();
   string resstatus = getParameter("resstatus")->getString();

   if (atoms->size() > 0) {
      frag->getSelectionFromAtomList(atoms, atombits, bondbits);
   } 

   if (frag->numResidues() > 0) {
      if (residues->size() > 0) {
         frag->getSelectionFromResidueList(residues, atombits, bondbits);
      } 

      if (chains->size() > 0) {
         frag->getChainAtoms(*chains,atombits,bondbits);
      }

      if (getParameter("backbone")->getBoolean()) {
         frag->getBackboneAtoms(atombits,bondbits);
      }

      if (getParameter("sidechains")->getBoolean()) {
         frag->getSideChainAtoms(atombits,bondbits);
      }
   }

   if (nwchemSelection != "") {
      frag->getSelectionFromNWChemString(nwchemSelection, atombits, bondbits);
   }

   if (getParameter("molecule")->getBoolean()) {
      TAtm *atm;
      if (iMol <nAtoms) {
        atm = frag->atomRef(iMol);
        frag->getConnected(atombits,bondbits,atm,0,0);
      }
   }

   if (getParameter("element")->getString() != "") {
     string element = getParameter("element")->getString();
     frag->getSelectionFromElement(element,atombits,bondbits);
   }

   if (getParameter("atom_type")->getString() != "") {
     string type = getParameter("atom_type")->getString();
     frag->getSelectionFromAtomType(type,atombits,bondbits);
   }

   if (getParameter("atom_name")->getString() != "") {
     string name = getParameter("atom_name")->getString();
     frag->getSelectionFromAtomName(name,atombits,bondbits);
   }

   if (getParameter("residue_name")->getString() != "") {
     string name = getParameter("residue_name")->getString();
     frag->getSelectionFromResidueName(name,atombits,bondbits);
   }

   if (resstatus != "") {
     frag->getSelectionFromResidueStatus(resstatus,atombits,bondbits);
   }

   if (getParameter("atom_range1")->getInteger() > 0) {
     int istart = getParameter("atom_range1")->getInteger();
     istart--;
     int iend = getParameter("atom_range2")->getInteger();
     int natoms = frag->numAtoms();
     if (iend > natoms) iend = natoms;
     int i, j, nbond;
     for (i=istart; i<iend; i++) {
       atombits[i] = true;
       const vector<TBond*> blist = frag->atomRef(i)->bondList();
       nbond = blist.size();
       for (j=0; j<nbond; j++) {
         bondbits[blist[j]->index()] = true;
       }
     }
   }

   if (getParameter("full_residue")->getInteger() >= 0) {
     int iat = getParameter("full_residue")->getInteger();
     frag->getFullResidue(frag->atomRef(iat),atombits,bondbits);
   }

   if (getParameter("solvent")->getBoolean()) {
     frag->getSolvent(atombits,bondbits);
   }

   if (getParameter("reverse")->getBoolean()) {
      for (idx = 0; idx<nAtoms; idx++) {
         atombits[idx] = !atombits[idx];
      }
      for (idx = 0; idx<nBonds; idx++) {
         bondbits[idx] = !bondbits[idx];
      }
   }

   // Grab selection from bit vectors
   frag->m_atomHighLight.clear();
   for (idx = 0; idx<nAtoms; idx++) {
      if (atombits[idx]) {
         frag->m_atomHighLight.push_back(idx);
      }
   }
   frag->m_bondHighLight.clear();
   for (idx = 0; idx<nBonds; idx++) {
      if (bondbits[idx]) {
         frag->m_bondHighLight.push_back(idx);
      }
   }

  // Really rough guess as to whether selection changed
  if (!ret && (p_selAtoms.size() != frag->m_atomHighLight.size() ||
     p_selBonds.size() != frag->m_bondHighLight.size())) ret = true;

   if (ret) {
      Event event("SelectionChanged","",p_sender);
      EventDispatcher::getDispatcher().publish(event);
   }

   return ret;
}



void SelectCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   frag->m_atomHighLight = p_selAtoms;
   frag->m_bondHighLight = p_selBonds;

}
