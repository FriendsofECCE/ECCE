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
#include "tdat/LinkCellList.H"

#include "viz/SelectRadiusCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

SelectRadiusCmd::SelectRadiusCmd(const string& name, Receiver * receiver, const string& sender)
    : Command(name, receiver)
{
   init();
   p_sender = sender;
}



/**
 * Destructor.
 * Deletes undo related data.
 */
SelectRadiusCmd::~SelectRadiusCmd()
{
}



void SelectRadiusCmd::init()
{
   vector<string> new_list;
   addParameter(new CommandParameter("clear", true));
   addParameter(new CommandParameter("residues", false));
   addParameter(new CommandParameter("min_radius", 0.0));
   addParameter(new CommandParameter("max_radius", 0.0));
   setHelpMessage("Selects atoms and bonds based on distance from current selection");
}



// @todo chain list
// @todo radius
bool SelectRadiusCmd::execute() throw(EcceException)
{

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  // For undo
  if (p_selAtoms.size() == 0 && p_selBonds.size() == 0) {
    p_selAtoms = frag->m_atomHighLight;
    p_selBonds = frag->m_bondHighLight;
  }

  bool selChanged = false;

  if (frag) {

    int nAtoms = frag->numAtoms();
    int nBonds = frag->numBonds();

    vector<TAtm*> *atoms = frag->atoms();
    int i, j, k;

    if (nAtoms > 0) {
      // Search distance
      double min_radius, max_radius;
      min_radius = getParameter("min_radius")->getDouble();
      max_radius = getParameter("max_radius")->getDouble();

      // Determine if residues should be included
      bool includeResidues = getParameter("residues")->getBoolean();

      // Bit mask indicating which atoms have been included
      // as part of the radius selection.  Initially set to currently selected
      // atoms.
      vector<bool> newSelectedAtoms(nAtoms, false);
      vector<bool> oldSelectedAtoms(nAtoms, false);
      vector<bool> shortAtoms(nAtoms, false);

      int nsize = frag->m_atomHighLight.size();
      for (i=0; i<nsize; i++) {
        oldSelectedAtoms[(frag->m_atomHighLight)[i]] = true;
      }

      // Bit mask indicating which bonds have been included
      // as part of the radius selection.  Initially set to currently selected
      // bonds.
      vector<bool> selectedBonds(nBonds, false);

      // Get a spatial partition of atoms (LinkCellList) to facilitate radius selection
      LinkCellList cellList(atoms, max_radius);
      const int *link_list = cellList.getLinkCellList();
      const int *header = cellList.getHeader();


      // --------------------------
      // Perform radius selection
      // --------------------------
      vector<int> cells;
      int cellIdx;
      int atomIdx;
      for (i = 0; i < nsize; i++) {
        TAtm *atm1 = (*atoms)[(frag->m_atomHighLight)[i]];
        cells.clear();

        // Get list of cells that contain "atm1" and border it
        cellList.getCells(cells, (frag->m_atomHighLight)[i]);

        // Iterate over all atoms in cell list
        for (j = 0; j < cells.size(); j++) {

          // Determine cell index
          cellIdx = cells[j];

          // Use cell index to get first atom from header array 
          atomIdx = header[cellIdx];

          // End of cell list is indicated by -1
          while (atomIdx >= 0) {

            // Only perform search if atom hasn't already been selected
            if (!newSelectedAtoms[atomIdx]) {
              TAtm *atm2 = (*atoms)[atomIdx];

              // Calculate the distance 
              double distance = atm1->distanceFrom(atm2);

              // Include atom - if it falls within radius selection criteria
              if (distance >= min_radius && distance <= max_radius &&
                  !oldSelectedAtoms[atomIdx]) {
                newSelectedAtoms[atomIdx] = true;

                // Include complete residues of selected atom - if selected
                if (includeResidues) {
                  Residue *residue = atm2->getResidue();
                  int resIdx = ((Fragment*)frag)->getResidueIndex(residue);
                  if (residue != 0) {
                    vector<int> residueAtoms = frag->findResidueAtomIndices(resIdx);
                    for (k = 0; k < residueAtoms.size(); k++) {
                      newSelectedAtoms[residueAtoms[k]] = true;
                    }
                  } 
                }
              } else if (distance < min_radius) {
                shortAtoms[atomIdx] = true;
              }
            }
            // Get next atom in cell
            atomIdx = link_list[atomIdx]; 
          }
        } 
      }
      delete atoms;
      int nbond;
      for (i=0; i<nAtoms; i++) {
        if (newSelectedAtoms[i] && !shortAtoms[i]) {
          const vector<TBond*> blist = frag->atomRef(i)->bondList();
          nbond = blist.size();
          for (j=0; j<nbond; j++) {
            selectedBonds[blist[j]->index()] = true;
          }
        }
      }

      if (getParameter("clear")->getBoolean()) {
        // Clear last selection first if requested
        frag->m_atomHighLight.clear();
        frag->m_bondHighLight.clear();
      }
      for (i=0; i<nAtoms; i++) {
        if (newSelectedAtoms[i]) {
          frag->m_atomHighLight.push_back(i);
        }
      }
      for (i=0; i<nBonds; i++) {
        if (selectedBonds[i]) {
          frag->m_bondHighLight.push_back(i);
        }
      }
    }
  }
  Event event("SelectionChanged","",p_sender);
  EventDispatcher::getDispatcher().publish(event);

  return selChanged;
}


void SelectRadiusCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   frag->m_atomHighLight = p_selAtoms;
   frag->m_bondHighLight = p_selBonds;

}
