/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include <set>
  using std::set;
#include <algorithm>
  using std::find;

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "tdat/LinkCellList.H"
#include "tdat/TPerTab.H"
#include "tdat/TBond.H"
#include "tdat/TPerTab.H"

#include "viz/HydrogenBondsCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/AtomLine.H"
#include "viz/AtomMeasureDist.H"

HydrogenBondsCmd::HydrogenBondsCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
HydrogenBondsCmd::~HydrogenBondsCmd()
{

}



/**
 * This method doesn't define any command parameters.
 * It does define a table of elements and distance (squared) to use
 * for elements that should be considered donors for H bonds.
 * The distance is computed as pow(vanderwalsradius+0.66,2).
 * Bruce and Erich originally arrived at a value of 2.01 for a distance
 * for Hydrogen bonds.  This was based on the assumption that only 
 * N, O, F were relevent.  Since others are interested in more elements, we
 * need to compute the distance.  So we are using
 *    2.01 - the VWR of Oxygen = 0.66
 * to arrive at this hardwired number.  Then add it to the vwr of the
 * atom in question.
 */
void HydrogenBondsCmd::init()
{

   setHelpMessage("HydrogenBondsCmd");

   TPerTab pertab;

   static const float HDIST = 0.66;

   p_donors["B"] = ((pertab.vwr(pertab.atomicNumber("B")) + HDIST),2);
//   p_donors["C"] = ((pertab.vwr(pertab.atomicNumber("C")) + HDIST),2);
   p_donors["N"] = ((pertab.vwr(pertab.atomicNumber("N")) + HDIST),2);
   p_donors["O"] = ((pertab.vwr(pertab.atomicNumber("O")) + HDIST),2);
   p_donors["F"] = ((pertab.vwr(pertab.atomicNumber("F")) + HDIST),2);

   p_donors["Al"] = ((pertab.vwr(pertab.atomicNumber("Al")) + HDIST),2);
   p_donors["Si"] = ((pertab.vwr(pertab.atomicNumber("Si")) + HDIST),2);
   p_donors["P"] = ((pertab.vwr(pertab.atomicNumber("P")) + HDIST),2);
   p_donors["S"] = ((pertab.vwr(pertab.atomicNumber("S")) + HDIST),2);
   p_donors["Cl"] = ((pertab.vwr(pertab.atomicNumber("Cl")) + HDIST),2);
   
   p_donors["Ga"] = ((pertab.vwr(pertab.atomicNumber("Ga")) + HDIST),2);
   p_donors["Ge"] = ((pertab.vwr(pertab.atomicNumber("Ge")) + HDIST),2);
   p_donors["As"] = ((pertab.vwr(pertab.atomicNumber("As")) + HDIST),2);
   p_donors["Se"] = ((pertab.vwr(pertab.atomicNumber("Se")) + HDIST),2);
   p_donors["Br"] = ((pertab.vwr(pertab.atomicNumber("Br")) + HDIST),2);

   addParameter(new CommandParameter("switch", true));
}



bool HydrogenBondsCmd::execute() throw(EcceException)
{
   bool ret = true;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   sg->getTopHBonds()->removeAllChildren();
   
   // Must have fragment and at least two atoms to start the processing
   if (getParameter("switch")->getBoolean() && frag && frag->numAtoms() > 1) {

      vector<TAtm*> *atoms = frag->atoms();

      int nAtoms = atoms->size();

      vector<bool> atom1List;

      /*
      // Using the selected atoms seemed really confusing so comment out for now
      if (frag->m_atomHighLight.size() > 0) {
      atom1List.resize(nAtoms, false);
      vector<int> *atomHighLight = &(frag->m_atomHighLight);
      for (int i = 0; i < atomHighLight->size(); i++) {
      atom1List[(*atomHighLight)[i]] = true;
      }
      } else {
      atom1List.resize(nAtoms, true);
      }
       */
      atom1List.resize(nAtoms, true);

      // Current list of selected atom indices.  Will iterate over
      // these atoms to perform radius selection.

      double maxR = determineCellSize();

      // Get a spatial partition of atoms (LinkCellList) 
      LinkCellList cellList(atoms, maxR);

      maxR = maxR*maxR;
      const int *link_list = cellList.getLinkCellList();
      const int *header = cellList.getHeader();


      TPerTab tpt;
      int nub = tpt.nubAtom();
      int ghost = 0;

      set<int,less<int> > alreadyBonded;

      vector<int> cells;
      int cellIdx;
      int atomIdx;
      int atNum;
      string atsym;
      MPoint p1, p2;
      int j;

      for (int i = 0; i < atom1List.size(); i++) {

         if ( !atom1List[i]) continue;

         cells.clear();
         alreadyBonded.clear();

         TAtm *atm1 = (*atoms)[i];

         atNum = atm1->atomicNumber();

         // If atom is not hydrogen, skip to next atom.
         if (atNum != 1) continue;

         if (atNum == ghost || atNum == nub) continue;

         if (!atm1->displayStyle().isDisplayed()) continue;

         // If hydrogen is not connected to a potential donor,
         // move on to the next atom
         const vector<TBond*> blist = atm1->bondList();
         bool isDonor = false;
         for (j=0; j<blist.size(); j++) {
            TAtm *atm2 = blist[j]->atom1();
            if (atm2 == atm1) {
               atm2 = blist[j]->atom2();
            }
            alreadyBonded.insert(atm2->index());
            // 0.999 is a way of picking bonds with bond order 1.0 or greater
            // while handling floating point number round off issues.  
            // Effectively this means no metal pi bonds.
            if (blist[j]->order() > 0.99) {
               atsym = atm2->atomicSymbol();
               isDonor = isDonorAtom(atsym);
               if (isDonor) break;
            }
         }
         if (!isDonor) continue;

         // Get list of cells that contain "atm1" and border it
         cellList.getCells(cells, i);

         p1.xyz(atm1->coordinates());

         // Iterate over all atoms in cell list
         for (j = 0; j < cells.size(); j++) {

            // Determine cell index
            cellIdx = cells[j];

            // Use cell index to get first atom from header array 
            atomIdx = header[cellIdx];

            // End of cell list is indicated by -1
            while (atomIdx >= 0) {

               // Only perform search if atom hasn't alreadyBonded been selected
               TAtm *atm2 = (*atoms)[atomIdx];
               atsym = atm2->atomicSymbol();

               if (isDonorAtom(atsym) && atm2->displayStyle().isDisplayed()) {
                  if (alreadyBonded.find(atm2->index()) == alreadyBonded.end()) {

                     p2.xyz(atm2->coordinates());
                     p2.subtract(p1);
                     if (p2.lengthSqr()<=getDonorDistance(atsym)) {

                        // Rumor has it that it shouldn't be considered an H
                        // bond if the angle is too wide
                        // Just take a guess at too wide for now
                        // But Sotiris says leave this out for now

                        /* bool doit = true; */

                        // Seems to give really bad results for simple system
                        // like insulin (a lot of hydrogen bonds between
                        // hydroxyl protons and carbon to which hydroxyl group
                        // is attached). Use angle criteria with a really wide
                        // angle (+/- 120 degrees) to keep this from happening.
                        bool doit = false;
                        vector<TAtm*> *connected = atm1->connectedAtoms();
                        if (connected) {
                           TAtm *parent = (*connected)[0];
                           double angle = fabs(atm1->angleBetween(parent,atm2));
                           if (angle >= 60. && angle <= 300.) doit = true;
                           delete connected;
                        }

                        if (doit) {
                           SoSeparator *sep = new SoSeparator;
                           // Its useful to comment this out to use measure so you can see when bonds
                           // form and break
#define uselines
#ifdef uselines                   
                           AtomLine *dist = new
                              AtomLine(frag, atm1, atm2, sep);
#else
                           AtomMeasureDist *dist= new
                              AtomMeasureDist(frag,atm1,atm2,sep);
#endif
                           sep->addChild(dist);
                           sg->getTopHBonds()->addChild(sep);
                        }

                     }
                  }
               }

               // Get next atom in cell
               atomIdx = link_list[atomIdx]; 
            }
         } 
      }

      delete atoms;
   }

   return ret;
}



bool HydrogenBondsCmd::isUndoable() const
{
   return false;
}



void HydrogenBondsCmd::undo()
{
}


/**
 * Determine if the atom should be considered a donor for H bonds.
 */
bool HydrogenBondsCmd::isDonorAtom(const string& symbol) const
{
   return p_donors.find(symbol) != p_donors.end();
}


/**
 * Get cutoff H bond distance for specified element type.
 * This method assumes that the symbol represents a legitimate donor.
 * The returned value is the distance squared.
 */
float HydrogenBondsCmd::getDonorDistance(const string& symbol) const
{
   map<string, float, less<string> >::const_iterator it = p_donors.find(symbol);
   return it->second * it->second;

}


/**
 * Picks the largest radius from amongst all possible donors.
 * This method could be improved by only considering elements which 
 * actually exist in the user's system.
 */
float HydrogenBondsCmd::determineCellSize() const
{
   float ret = 2.01;  // Our original idea for a correct value
   map<string, float, less<string> >::const_iterator it;
   for (it=p_donors.begin(); it!=p_donors.end(); it++) {
      if (it->second > ret) ret = it->second;
   }
   return ret;
}
