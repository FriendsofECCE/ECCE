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

#include "tdat/TAtm.H"
#include "tdat/TBond.H"
#include "tdat/TResItem.H"
#include "tdat/TResTab.H"
#include "tdat/FragUtil.H"
#include "tdat/Measures.H"

#include "viz/BondEditCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"



BondEditCmd::BondEditCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
BondEditCmd::~BondEditCmd()
{

}



void BondEditCmd::init()
{
   addParameter(new CommandParameter("order", TBond::Single));
   addParameter(new CommandParameter("orient", false));
   setHelpMessage("Select two atoms to add a bond with the specified "
     "bond order. If you select an existing bond, its order "
     "will be changed.");
}



bool BondEditCmd::execute() throw(EcceException)
{
  bool ret = false;
  setErrorMessage("");

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

  double order = getParameter("order")->getDouble();

  bool orient = getParameter("orient")->getBoolean();

  // First get our fragment object.  Create one if it doesn't exist
  SGFragment *frag = sg->getFragment();

  // Will have to delete frag if command doesn't actually execute
  cloneFragment(frag);

  // If bonds are selected, change them to the specified order.
  // This method should not be called for metal-pi bonds.
  // Have to do this first or adding bonds (loop below) will change
  // index to TBond mapping.

  bool changedOrders = false;
  if (frag->m_bondHighLight.size() > 0) {

    ret = true;

    int cnt = frag->m_bondHighLight.size();
    vector<TBond*> *allBonds = frag->bonds();
    TBond *bond;
    string tag1, tag2;
    for (int idx=0; idx<cnt; idx++) {
      bond = (*allBonds)[frag->m_bondHighLight[idx]];
      tag1 = bond->atom1()->atomicSymbol();
      tag2 = bond->atom2()->atomicSymbol();
      if (tag1 != "Nub" && tag1 != "H" && tag2 != "Nub" && tag2 != "H") {
        bond->order(order);
      }
    }
    delete allBonds;
    changedOrders = true;
  }

  bool addedBond = false;
  // If just two are selected then we might be adding a bond
  if (frag->m_atomHighLight.size() == 2) {

    TAtm *atm1, *atm2;
    int iatm1, iatm2;
    iatm1 = frag->m_atomHighLight[0];
    iatm2 = frag->m_atomHighLight[1];
    atm1 = frag->atomRef(iatm1);
    atm2 = frag->atomRef(iatm2);

    double origOrder = -1;
    TBond *bond = frag->bond(iatm1, iatm2);
    if (bond != (TBond*)0) origOrder = bond->order();

    if (origOrder == TBond::Metal) {
      setErrorMessage("This operation cannot be used to convert "
          "a Metal-Pi bond to a conventional bond.");
    } else if (origOrder > 0) {
      ret = frag->changeBond(atm1,atm2, order);
      addedBond = ret = true;
    } else {
      try {
        if (orient) {
          int nAtoms = frag->numAtoms();
          int nBonds = frag->numBonds();
          vector<bool> atombits(nAtoms, false);
          vector<bool> bondbits(nBonds, false);
          frag->getConnected(atombits,bondbits,atm2,0,0);
          atombits[atm2->index()] = true;

          vector<TAtm*> atoms;
          for (int i=0; i<nAtoms; i++) {
            if (atombits[i]) {
              atoms.push_back(frag->atomRef(i));
            }
          }

          TAtm *parent1=0, *nub1=0, *parent2=0, *nub2=0;

          // First we need to make sure that we have nubs for bonding
          if (atm1->atomicSymbol() == "Nub") {
            nub1 = atm1;
            parent1 = frag->nubParent(nub1);
          } else {
            parent1 = atm1;
            nub1 = frag->childNub(parent1);
          }

          if (atm2->atomicSymbol() == "Nub") {
            nub2 = atm2;
            parent2 = frag->nubParent(nub2);
          } else {
            parent2 = atm2;
            nub2 = frag->childNub(parent2);
          }
          if (nub1 == 0 || nub2 == 0) {
             throw EcceException("You must have nubs to create this type of bond.  Try changing the shape to add more nubs first.",WHERE);
          }

          // Get rid of nubs in list that are eliminated by formation
          // of the new bond
          vector<TAtm*> *children1 = parent1->connectedAtoms();
          vector<TAtm*> *children2 = parent2->connectedAtoms();
          int idx;
          for (idx=children1->size()-1; idx>=0; idx--) {
            if ((*children1)[idx] == nub1)  {
              vector<TAtm*>::iterator it = children1->begin();
              it += idx;
              children1->erase(it);
              break;
            }
          }
          for (idx=children2->size()-1; idx>=0; idx--) {
            if ((*children2)[idx] == nub2)  {
              vector<TAtm*>::iterator it = children2->begin();
              it += idx;
              children2->erase(it);
              break;
            }
          }

          // Determine if bond is a special case (amino acid, DNA, RNA)

          TResItem::Classification type1;
          TResItem::Classification type2;
          TResTab restab;
          if (frag->numResidues() > 0) {
            type1 = restab.verify(parent1->getResidue()->name().c_str());
            type2 = restab.verify(parent2->getResidue()->name().c_str());
          }
          bool AAangle  = false;
          bool DNAangle = false;
          bool RNAangle = false;
          if (type1 == TResItem::AminoAcid && type2 == TResItem::AminoAcid)
            AAangle = true;
          if ((type1 == TResItem::DNA && type2 == TResItem::DNA) ||
              (type1 == TResItem::RNAorDNA && type2 == TResItem::DNA) ||
              (type1 == TResItem::DNA && type2 == TResItem::RNAorDNA))
            DNAangle = true;
          if ((type1 == TResItem::RNA && type2 == TResItem::RNA) ||
              (type1 == TResItem::RNAorDNA && type2 == TResItem::RNA) ||
              (type1 == TResItem::RNA && type2 == TResItem::RNAorDNA))
            RNAangle = true;

          FragUtil fragutil(frag);
          fragutil.bondParts(parent1,nub1,parent2,nub2,1.0,&atoms);
          vector<TAtm*>::iterator it = atoms.begin();
          while (it != atoms.end()) {
            if ((*it) == nub1) {
              // Do we need a delete here?
              atoms.erase(it);
              break;
            }
            it++;
          }
          while (it != atoms.end()) {
            if ((*it) == nub2) {
              // Do we need a delete here?
              atoms.erase(it);
              break;
            }
            it++;
          }

          TAtm* patm = (TAtm*)0;
          TAtm* fatm = (TAtm*)0;
          double newangle = 180.0;

          // Check to see if we have a special case. If we do use,
          // special angle, if not, then go to the default algorithm.

          if (AAangle) {
            if ((parent2->atomName() == " N  " && parent1->atomName() == " C ") ||
                (parent2->atomName() == " C  " && parent1->atomName() == " N  ")) {
              for (idx=children1->size()-1; idx>=0; idx--) {
                if ((*children1)[idx]->atomName() == " CA ") patm = (*children1)[idx];
              }
              for (idx=children2->size()-1; idx>=0; idx--) {
                if ((*children2)[idx]->atomName() == " CA ") fatm = (*children2)[idx];
              }
              if (patm == (TAtm*)0 || fatm == (TAtm*)0) AAangle = false;
            } else {
              AAangle = false;
            }
          }
          if (DNAangle) {
            if (parent2->atomName() == " O3*" && parent1->atomName() == " P  ") {
              for (idx=children1->size()-1; idx>=0; idx--) {
                if ((*children1)[idx]->atomName() == " O5*") patm = (*children1)[idx];
              }
              for (idx=children2->size()-1; idx>=0; idx--) {
                if ((*children2)[idx]->atomName() == " C3*") fatm = (*children2)[idx];
              }
              newangle = -95.2;
              if (patm == (TAtm*)0 || fatm == (TAtm*)0) DNAangle = false;
            } else if (parent2->atomName() == " P  " && parent1->atomName() == " O3*") {
              for (idx=children1->size()-1; idx>=0; idx--) {
                if ((*children1)[idx]->atomName() == " C3*") patm = (*children1)[idx];
              }
              for (idx=children2->size()-1; idx>=0; idx--) {
                if ((*children2)[idx]->atomName() == " O5*") fatm = (*children2)[idx];
              }
              newangle = -95.2;
              if (patm == (TAtm*)0 || fatm == (TAtm*)0) DNAangle = false;
            } else { DNAangle =
              false;
            }
          }
          if (RNAangle) {
            if (parent2->atomName() == " O3*" && parent1->atomName() == " P  ") {
              for (idx=children1->size()-1; idx>=0; idx--) {
                if ((*children1)[idx]->atomName() == " O5*") patm = (*children1)[idx];
              }
              for (idx=children2->size()-1; idx>=0; idx--) {
                if ((*children2)[idx]->atomName() == " C3*") fatm = (*children2)[idx];
              }
              newangle = -73.6;
              if (patm == (TAtm*)0 || fatm == (TAtm*)0) RNAangle = false;
            } else if (parent2->atomName() == " P  " && parent1->atomName() == " O3*") {
              for (idx=children1->size()-1; idx>=0; idx--) {
                if ((*children1)[idx]->atomName() == " C3*") patm = (*children1)[idx];
              }
              for (idx=children2->size()-1; idx>=0; idx--) {
                if ((*children2)[idx]->atomName() == " O5*") fatm = (*children2)[idx];
              }
              newangle = -73.6;
              if (patm == (TAtm*)0 || fatm == (TAtm*)0) RNAangle = false;
            } else {
              RNAangle = false;
            }
          }
          if (!AAangle && !DNAangle && !RNAangle) {
            TPerTab tpt;
            double vwr=0.;
            double tvwr;
            TAtm *atom;
            for (idx=children1->size()-1; idx>=0; idx--) {
              atom = (*children1)[idx];
              tvwr = tpt.vwr(tpt.atomicNumber(atom->atomicSymbol()));
              if (tvwr > vwr) {
                vwr = tvwr;
                patm = atom;
              }
            }
            vwr = 0.;
            for (idx=children2->size()-1; idx>=0; idx--) {
              atom = (*children2)[idx];
              tvwr = tpt.vwr(tpt.atomicNumber(atom->atomicSymbol()));
              if (tvwr > vwr) {
                vwr = tvwr;
                fatm = atom;
              }
            }
          }
          // It could be that there weren't enough objects to make a dihedral
          // in which case we choose to do nothing
          if (patm && parent1 && fatm && parent2) {

            // Didn't use  this because it appears to have a memory problem
            // plus we already know what atoms to move.
            //frag->dihedral(patm,baseparent,fragparent,fatm,newangle);

            double curAngle = Measures::computeDihedral(patm,parent1,parent2,fatm);
            double diff = newangle - curAngle;
            MMatrix rotateM;
            MPoint p1, p2;
            p1.xyz(parent1->coordinates());
            p2.xyz(parent2->coordinates());
            rotateM.rotateLineMatrix(p1,p2,diff);
            frag->transform(atoms,rotateM);
          }
          delete children1;
          delete children2;
        } else {
          frag->bondAtoms(atm1,atm2,order,frag->getMainDisplayStyle());
        }
        addedBond = ret = true;
      } catch (EcceException& ex) {
        setErrorMessage(ex.what());
      }
    }
  }

  // Generate a meaningful undo label
  p_undoLabel = "Add Bond";
  if (changedOrders) {
    p_undoLabel = "Change Bond";
  }

  // Must do this before sending events - they are synchronous
  if (ret) {
    frag->touchNumbers();
    sg->touchChemDisplay();
  } else {
    clearUndoFragment();
  }


  if (addedBond || changedOrders) {
     // Do this before sending any events
     if (addedBond)  frag->clearSelection();

     EventDispatcher::getDispatcher().publish(Event("GeomChange"));
     if (addedBond) {
        EventDispatcher::getDispatcher().publish(Event("SelectionChange"));
     }
  }


  return ret;

}

string BondEditCmd::getUndoLabel()
{
   return p_undoLabel;
}
