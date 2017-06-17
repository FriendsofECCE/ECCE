/**
 * @file
 *
 *
 */

#include <stdlib.h>
#include <ctype.h>  // isdigit
#include <math.h>       // floor()
#include <limits.h>

#include <fstream>
  using std::ifstream;

#include <iomanip>
  using std::ios;
#include <algorithm>
#include <iostream>
  using std::cout;
  using std::endl;
#include <set>
  using std::set;
  using std::find;
  using std::less;
#include <list>
    using std::list;

#ifdef __GNUC__
  #include <ext/hash_map>
  using __gnu_cxx::hash_map;
  using __gnu_cxx::hash;
#endif

#include "util/ETimer.H"
#include "util/Ecce.H"
#include "util/STLUtil.H"
#include "util/Transformations.H"
#include "util/ErrMsg.H"
#include "util/SFile.H"
#include "util/EcceSortedVector.H"
#include "util/STLUtil.H"
#include "util/StringTokenizer.H"
#include "util/TempStorage.H"
#include "util/Diag3.H"
#include "util/EcceException.H"

#include "tdat/LatticeDef.H"
#include "tdat/Fragment.H"
#include "tdat/FragUtil.H"
#include "tdat/TFormula.H"
#include "tdat/TTCPair.H"
#include "tdat/TPerTab.H"
#include "tdat/ShapeData.H"
#include "tdat/TBond.H"
#include "tdat/TAtm.H"
#include "tdat/Residue.H"
#include "tdat/TResTab.H"
#include "tdat/TResItem.H"
#include "tdat/LinkCellList.H"
#include "tdat/TChartOfNuclides.H"
#include "tdat/TRefIsotope.H"
#include "tdat/Measures.H"
#include "tdat/GeomConstraintModel.H"
#include "tdat/MetaPotentialModel.H"
#include "tdat/QMMMModel.H"
#include "tdat/SymmetryOps.H"

#define PI 3.14159265358979323846

//#define DEBUG

/**
 * Constructor.
 * This fragment has no name, atoms, residues, or constraints.
 *
 */
Fragment::Fragment() : p_mainDisplayStyle(DisplayStyle::BALLWIRE)
{
   p_name = "";
   p_charge = 0;
   p_pointGroup = "C1";
   p_useSymmetry = false;
   editResidueMode(false, -1);
   p_constraints = 0;
   p_potentials = 0;
   p_QMMMTerms = 0;
   p_firstSolventAtomIndex = -1;
   p_solventNumAtoms = 0;
   p_lattice = 0;
   p_DoF = 0;
}



/**
 * Constructor.
 * Create fragment from set of atomic symbols and coordinates.
 */
Fragment::Fragment(const string& name,
                   const vector<string>& tags,
                   const double* coordinates,
                   const int numBonds,
                   const int *bonds) 
                   : p_mainDisplayStyle(DisplayStyle::BALLWIRE)
{
   p_name = name;
   p_charge = 0;
   p_pointGroup = "C1";
   p_useSymmetry = false;
   //p_editResidueMode = false ;
   //p_editResidueIndex = -1 ;
   editResidueMode(false, -1);
   int numAtoms = tags.size();
   if ((numAtoms == 0) && (coordinates == (double*)0)) {
      ErrMsg().message("EE_BAD_VALUE", EE_FATAL, WHERE,
            "Atms or Coordinates Illegal to Construct Fragment.");
   } else {
      this->addAtoms(tags, coordinates);                     // Atoms
      this->addBonds(bonds, numBonds);                      // Bonds
   }
   p_constraints = 0;
   p_potentials = 0;
   p_QMMMTerms = 0;
   p_firstSolventAtomIndex = -1;
   p_solventNumAtoms = 0;
   p_lattice = 0;
   p_DoF = 0;
}



/**
 * Constructor.
 * Create fragment from set of atomic symbols and coordinates.
 * coordinates can be null to support the basis tool.
 */
Fragment::Fragment(const string& name,
                   const EcceSortedVector<TTagCountPair,
                                          less<TTagCountPair> >& tags,
                   const double* coordinates,
                   const int numBonds,
                   const int *bonds)
                   : p_mainDisplayStyle(DisplayStyle::BALLWIRE)
{
   p_name = name;
   p_charge = 0;
   p_pointGroup = "C1";
   p_useSymmetry = false;
   int numAtoms = tags.size();
   if (numAtoms == 0) {
      ErrMsg().message("EE_BAD_VALUE", EE_FATAL, WHERE,
            "Atms or Coordinates Illegal to Construct Fragment.");
   } else {
      this->addAtoms(tags, coordinates);                     // Atoms
      this->addBonds(bonds, numBonds);                      // Bonds
   }
   p_constraints = 0;
   p_potentials = 0;
   p_QMMMTerms = 0;
   p_firstSolventAtomIndex = -1;
   p_solventNumAtoms = 0;
   p_lattice = 0;
   p_DoF = 0;
}



Fragment::Fragment( const string& name ) 
                   : p_mainDisplayStyle(DisplayStyle::BALLWIRE)
{
   p_name = name;
   p_charge = 0;
   p_pointGroup = "C1";
   p_useSymmetry = false;
   editResidueMode(false, -1);

   p_constraints = 0;
   p_potentials = 0;
   p_QMMMTerms = 0;
   p_firstSolventAtomIndex = -1;
   p_solventNumAtoms = 0;
   p_lattice = 0;
   p_DoF = 0;
}




/**
 * Copy Constructor.
 */
Fragment::Fragment(const Fragment& frag) 
        : p_mainDisplayStyle(DisplayStyle::BALLWIRE)
{ 
   p_constraints = 0;
   p_potentials = 0;
   p_QMMMTerms = 0;
   p_firstSolventAtomIndex = -1;
   p_solventNumAtoms = 0;
   p_lattice = 0;
   p_DoF = 0;
   *this = frag; 
}



/**
 * Destructor.
 */
Fragment::~Fragment()
{
   clearState();
}



/**
 * Assignment operator.
 */
Fragment& Fragment::operator=(const Fragment& frag)
{
   if (this != &frag) {

      clearState();  // clears *current* atom and residue lists

      name(frag.name());
      charge(frag.charge());
      pointGroup(frag.pointGroup());
      useSymmetry(frag.useSymmetry());
      setMainDisplayStyle(frag.getMainDisplayStyle());


      copyAtoms(frag.p_atoms);    // straight copy of Atoms

      IBondList* bondIndices = frag.bondIndices();               // Bonds
      addBonds(bondIndices->matomIndices,
            bondIndices->mnumBonds,
            bondIndices->mbondOrders,bondIndices->mstyle);

      // we also need a copy of the residue information.
      editResidueMode(frag.editResidueMode(), frag.editResidueIndex());
      vector<Residue*> res = frag.p_residueSequence;
      addResidues(res);

      // Need to assign first atom pointers to residues and residue
      // pointers to atoms, if residues are being used.

      int resNum = res.size();
      if (resNum > 0) {
         int natoms = frag.p_atoms.size();
         int rescnt = 0;
         // Inititialize residues with first residue in old fragment
         Residue* newres;
         TAtm * atm;
         int i;
         for (i=0; i<natoms; i++) {
            atm = frag.p_atoms[i];
            newres = atm->getResidue();
            // Skip through residues until one for current atom is found
            while (rescnt < resNum &&
                  newres != frag.p_residueSequence[rescnt]) {
               rescnt++;
            }
            p_atoms[i]->atomResidue(p_residueSequence[rescnt]);
         }
         rescnt = res.size();
         int iat;
         for (i=0; i<rescnt; i++) {
            if (frag.p_residueSequence[i]->firstAtomPtr() != (TAtm*)0) {
               iat = res[i]->firstAtomPtr()->index();
               p_residueSequence[i]->firstAtomPtr(p_atoms[iat]);
            } else {
               p_residueSequence[i]->firstAtomPtr((TAtm*)0);
            }
         }
      }
      delete bondIndices;
      p_chains = frag.p_chains;

      pointGroup(frag.pointGroup());

      p_constraints = frag.p_constraints;
      p_potentials = frag.p_potentials;
      p_QMMMTerms = frag.p_QMMMTerms;
      p_firstSolventAtomIndex = frag.p_firstSolventAtomIndex;
      p_solventNumAtoms = frag.p_solventNumAtoms;
      p_DoF = frag.p_DoF;
      if (frag.p_lattice) {
         p_lattice = new LatticeDef(*frag.p_lattice);
      }
   }

   return *this;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    These are the Object Equality Operators for the Fragment Class.
//
///////////////////////////////////////////////////////////////////////////////
bool Fragment::operator==(const Fragment& frag) const
{ return (this == &frag); }

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    These are the Object Inequality Operators for the Fragment Class.
//
///////////////////////////////////////////////////////////////////////////////
bool Fragment::operator!=(const Fragment& frag) const
{ return (this != &frag); }


/**
 * Clear the Fragment atom sorting.
 * The sort order will revert to index ordering based on atom position within
 * the atom list.
 */
void Fragment::clearAtomSortOrder()
{
   p_atomSortOrder.clear();
}

/**
 * Reset the atom sort order to the specified ordering.
 * It is not required that all atoms be represented in the order
 * at this time.  Atoms not in the sort order will no be processed
 * by code that uses the sort order.
 * @param - new atom order - does not have to account for all atoms
 */
void Fragment::setAtomSortOrder(const SortOrder& order)
{
   p_atomSortOrder = order;
}


/**
 * Get the atom sort oder.
 */
const SortOrder& Fragment::getAtomSortOrder() const
{
   return p_atomSortOrder;
}




///////////////////////////////////////////////////////////////////////////////
// Description
//    Loop through atoms and reassign their index according to their
//    location in the vector.
///////////////////////////////////////////////////////////////////////////////
void Fragment::renumberAtoms(const int& start)
{
  int cnt = p_atoms.size();
  for (int idx=start; idx<cnt; idx++) {
    p_atoms[idx]->index(idx);
  }
}

bool Fragment::operator<(const Fragment& rhs) const
{
  int thisWeight = numElectrons();
  int rhsWeight = rhs.numElectrons();
  bool ret = thisWeight < rhsWeight;
  if (thisWeight == rhsWeight) {
    ret = name() < rhs.name();
  }
  return ret;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Add the state of the argument to the current Fragment object.
//
//  Implementation
//    We assume the argument contains only valid atoms (as others
//    assume that we do) so we don't check them against the periodic
//    table.
//    For the bonds, we must bump the atom numbers for all newly
//    added bonds so they are consistent with their new atom
//    numbers.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::merge(const Fragment& frag)
{
  int numAtoms = this->numAtoms();              // Save For the Bond Offset

  addAtoms(frag.p_atoms);

  IBondList* bondIndices = frag.bondIndices();                   // Bonds
  int* atomIdx = bondIndices->matomIndices;
  for (int index = (bondIndices->mnumBonds * 2); index-- > 0; ) {
    (*atomIdx++) += numAtoms;
  }
  addBonds(bondIndices->matomIndices,
	   bondIndices->mnumBonds,
	   bondIndices->mbondOrders,bondIndices->mstyle);
  delete bondIndices;
}

/**
 * Take the atoms and bonds from frag and merge them into this.  If
 * specified, clear the original frag.  This is much faster than merge
 * since it doesn't have to copy objects and determinine indices etc.
 */
void Fragment::absorbAtoms(Fragment& frag, bool clear)
{

  frag.fixBondStyles(frag.getMainDisplayStyle());

  int cnt = frag.p_atoms.size();
  for (int idx=0; idx<cnt; idx++) {
    addAtom(frag.p_atoms[idx], (TAtm*)0);
  }

  if (clear) {
    frag.p_atoms.clear();
  }

}

/**
 * Take the atoms, bonds, residues etc from frag and merge them into this.  If
 * specified, clear the original frag.  This is much faster than merge
 * since it doesn't have to copy objects and determinine indices etc.
 */
void Fragment::absorb(Fragment& frag, bool clear)
{

  frag.fixBondStyles(frag.getMainDisplayStyle());

  charge(frag.charge());
  pointGroup(frag.pointGroup());
  name(frag.name());

  int idx;
  int cnt = frag.numResidues();

  for (idx=0; idx<cnt; idx++) {
    p_residueSequence.push_back(frag.findResidue(idx));
  }

  cnt = frag.p_atoms.size();
  for (idx=0; idx<cnt; idx++) {
    addAtom(frag.p_atoms[idx]);
  }
  p_chains = frag.p_chains;

  if (clear) {
    frag.p_atoms.clear();
    frag.p_residueSequence.clear();
  }

}



/**
 * Absorb tmpfrag objects into current fragment.
 * @param at - if not null, frag atom we are bonding to
 * @param normalOrder - if true, residues inserted in order, else at the front
 */
void Fragment::absorb(Fragment& tmpfrag,
                      TAtm *at, bool normalOrder)
{
  tmpfrag.fixBondStyles(getMainDisplayStyle());

// Non-null pointer indicates that tmpfrag is being bonded to something in
// fragment

  if (at != (TAtm*)0) {

// Find residue that is being bonded to.

    int newresidx;
    if (normalOrder) {
      newresidx = insertResidue(at);
    } else {
      newresidx = prependResidue(at);
    }
    Residue *fragres = findResidue(newresidx);
    char fchainID = at->chain();

// get first residue in tmpfrag

    Residue *oldres = tmpfrag.findResidue(0);
    Residue *curres = oldres;
    char tchainID = oldres->chain();
    char oldchainID = tchainID;

// Now change fragment residue name to the tmpfrag residue name

    fragres->name(curres->name().c_str());
    fragres->couplingInfo(curres->couplingInfo());
    fragres->chain(fchainID);

    editResidueMode(true,newresidx);
    int atcount=0;
    if (normalOrder) {
      atcount = at->index();
    } else {
      if (newresidx == 0) {
        at = (TAtm*)0;
      } else {
        // Find last atom of previous residue. Next residue must
        // exist since we are prepending residue to it.
        atcount = findResidue(newresidx+1)->firstAtomPtr()->index()-1;
        if (atcount >=0) {
          atcount = at->index();
          at = atomRef(atcount);
        } else {
          // Fragment has several empty residues before residue that
          // is being prepended.
          atcount = 0;
          at = (TAtm*)0;
        }
      }
    }

// Start moving atoms from tmpfrag to frag

    int numAtoms = tmpfrag.numAtoms();
    TAtm* tfatm;
    for (int idx=0; idx<numAtoms; idx++) {
      tfatm = tmpfrag.atomRef(idx);
      curres = tfatm->getResidue();
      if (oldres != curres) {
        tchainID = curres->chain();
        if (tchainID != oldchainID) {
          if (tchainID != ' ') {
            fchainID = getChainIDs()->getNewChainID();
          } else {
            fchainID = ' ';
          }
          oldchainID = tchainID;
        }
        at = atomRef(atcount);
        newresidx = insertResidue(at);
        fragres = findResidue(newresidx);
        editResidueMode(true,newresidx);
        fragres->name(curres->name().c_str());
        fragres->couplingInfo(curres->couplingInfo());
        fragres->segID(curres->segID().c_str());
        fragres->chain(fchainID);
        oldres = curres;
      }
      addAtom(tmpfrag.atomRef(idx),at);
      if (normalOrder) {
        atcount++;
      } else {
        if (at != (TAtm*)0) {
          atcount++;
        } else {
          atcount = 0;
          at = atomRef(0);
        }
      }
    }

  } else {

// tmpfrag is being added to free space. Append it to the end of fragment
// as a new set of residues.

    int fnumAtoms = numAtoms();
    int tnumAtoms = tmpfrag.numAtoms();
    TAtm* at;
    if (fnumAtoms > 0) {
      at = atomRef(fnumAtoms-1);
    } else {
      at = (TAtm*)0; 
    }
    int newresidx = insertResidue(at);
    Residue* oldres = tmpfrag.findResidue(0);
    Residue* curres;
    Residue* fragres = findResidue(newresidx);
    char chainID,oldchainID;
    editResidueMode(true,newresidx);
    fragres->name(oldres->name().c_str());
    fragres->couplingInfo(oldres->couplingInfo());
    fragres->segID(oldres->segID().c_str());
    if (oldres->chain() != ' ') {
      chainID = getChainIDs()->getNewChainID();
      oldchainID = oldres->chain();
      fragres->chain(chainID);
    }
    for  (int i=0; i<tnumAtoms; i++) {
      at = tmpfrag.atomRef(i);
      curres = tmpfrag.findResidue(at);

// New residue encountered in tmpfrag. Add a new residue at the end
// of fragment

      if (curres != oldres) {
        fnumAtoms = numAtoms();
        at = atomRef(fnumAtoms-1);
        newresidx = insertResidue(at);
        editResidueMode(true,newresidx);
        fragres = findResidue(newresidx);
        oldres = curres;
        fragres->name(curres->name().c_str());
        fragres->couplingInfo(curres->couplingInfo());
        fragres->segID(curres->segID().c_str());
        if (curres->chain() != oldchainID) {
          if (curres->chain() != ' ') {
            chainID = getChainIDs()->getNewChainID();
          } else {
            chainID = ' ';
          }
          oldchainID = curres->chain();
        }
        fragres->chain(chainID);
      }

// Add atoms to fragment

      addAtom(tmpfrag.atomRef(i), NULL);
    }
  }

  // remove atoms from tmpfrag

  int numAtoms = tmpfrag.numAtoms();
  for (int idx=numAtoms-1; idx>=0; idx--) {
    tmpfrag.removeAtom(idx);
  }

  editResidueMode(false,-1);
}


///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Add the state of the argument to the current Fragment object.
//
///////////////////////////////////////////////////////////////////////////////
Fragment* Fragment::clone() const
{
  return new Fragment(*this);
}

string Fragment::generateStructureString(void)
{
  EE_RT_ASSERT(false, EE_WARNING,
               "Not implemented - Fragment::generateStructureString"); 
  string ret;
  return ret;
}

string Fragment::generateEmpiricalFormula(void)
{
  string ret;
  vector<TTagCountPair>* tc = tagCounts();
  ret =  TFormula::empiricalFormula(*tc,charge());
  delete tc;
  return ret;
}


//
// --------- Modifiers ---------
void Fragment::name(const string& value)
{ p_name = value; }

string Fragment::name(void) const
{ return p_name; }

void Fragment::charge(const int& value)
{ p_charge = value; }

///////////////////////////////////////////////////////////////////////////////
//  Description
//    Set charge to nearest integer value
///////////////////////////////////////////////////////////////////////////////
void Fragment::charge(const double& value)
{
  int sign = 1;
  if (value < 0.0)
    sign = -1;
  double absValue = fabs(value);
  // Round off to nearest positive integer
  int iabs = (int)(absValue+0.49999);
  p_charge = sign*iabs;
}

int Fragment::charge(void) const
{ return p_charge; }

///////////////////////////////////////////////////////////////////////////////
//  Description
//    Return residue chain ID object
///////////////////////////////////////////////////////////////////////////////
TResChain* Fragment::getChainIDs(void)
{ return &p_chains; }


/**
 * Returns the main display style.
 */
DisplayStyle Fragment::getMainDisplayStyle() const
{
   return p_mainDisplayStyle ;
}

///////////////////////////////////////////////////////////////////////////////
//  Description
//    Set residue chain ID object
///////////////////////////////////////////////////////////////////////////////
void Fragment::setChainIDs(TResChain* rchain)
{ p_chains = *rchain; }

void Fragment::editResidueMode(int on_off, int index)
{
   p_editResidueMode = on_off ;
   p_editResidueIndex = index ;
}

void Fragment::setMainDisplayStyle(DisplayStyle mainStyle)
{
   p_mainDisplayStyle = mainStyle;
}

int Fragment::editResidueMode()
{
   return p_editResidueMode;
}

int Fragment::editResidueMode() const
{
   return p_editResidueMode;
}

int Fragment::editResidueIndex()
{
   return p_editResidueIndex;
}

int Fragment::editResidueIndex() const
{
   return p_editResidueIndex;
}

//
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Set the Fragments's coordinates to new values.  Argument num_coords
//    is the number of atoms in the molecule (so there are 3*num_coords
//    coordinates total).
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::coordinates(const double *coordinates, size_t num_coords)
{
  if ((numAtoms() == num_coords) && (coordinates != (double*)0)) {
    double* iter = (double*)coordinates;
    for (int idx=0; idx<num_coords; idx++, iter +=3) {
      p_atoms[idx]->coordinates(iter);
    }
  } else {                                         // Incorrect Number Passed
    ErrMsg().message("EE_ARG_MISMATCH", EE_WARNING, WHERE,
		   "# Atoms != # Coordinates. No Changes Made");
  }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Add a single atom and coordinate.
//
//  Implementation
//    Again Atm's constructor validates the tag.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::addAtom(const string& tag, const double* newCoords,
                       const string& suffix)
{
  addAtom( new TAtm(tag.c_str(), newCoords, suffix));

}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Add a single atom and coordinate.
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::addAtom(TAtm* atom)
{
  atom->index(p_atoms.size());
  p_atoms.push_back(atom);
}

//////////////////////////////////////////////////////////////////////////////
// addAtom
//  Add atom at appropriate place.  The psuedo code rules are:
//    if no residues exist
//      add at end regardless of parent
//    else 
//      get current residue
//      if parent is null
//        append to currrent reside
//      else 
//        get residue of parent
//        if parentresidue == currentresidue
//          insert after parent
//        else
//          insert at end (or beginning) of curres
//
//   Perhaps sibling is a better term than parent just in case this
//   confuses anyone.
//
//  Sorry this is hideous code.  The tricky part is finding the start and
//  end atoms of a residue.  We do this by using the firstAtomPtr.  Since
//  we don't have a lastAtmPointer, we have to figure it out and take
//  into account the fact that there may be follow-on residues or not, if
//  there are any they may be empty...
//////////////////////////////////////////////////////////////////////////////
void Fragment::addAtom(TAtm* atom, TAtm *parent)
{
  int nResidues = numResidues();
  
  if (nResidues == 0) {
    // If we don't have any residues just do normal append.  We could get
    // fancy and support add to parent but for now don't bother.
    atom->index(p_atoms.size());
    p_atoms.push_back(atom);

  } else {
    int curResIndex = editResidueIndex();
    if (curResIndex < 0 && parent == NULL) {
      // For now this is bad.  Later we'll start a new residue perhaps.
      EE_ASSERT((curResIndex < 0 && parent == NULL),EE_FATAL,
          "Should be a residue to edit or parent to add to");
    } else {

      // Figure out the beginning and ending atom index of the current
      // residue. This is an ugly job.
      Residue *curres = (Residue*)0;
      Residue *parentres = (Residue*)0;

      if (parent) parentres = findResidue(parent);

      if (curResIndex >= 0) {
        // Use current residue if atom is NOT a nub
        if (atom->atomicSymbol() != "Nub") {
          curres = findResidue(curResIndex);
        } else {
          if (parentres) {
            curres = parentres;
          } else {
            curres = findResidue(curResIndex);
          }
        }
      } else {
        // use parent's
        curres = parentres;
      }
      // OK, at this point we have identified the residue that the new atom
      // will belong to. We next need to find the indices of either
      //      The first atom in the residue (if it exists)
      //    or
      //      The first atom before the residue in the fragment
      //  and the indices of either
      //      The last atom in the residue (if it exists)
      //    or
      //      The first atom in the next residue that contains any atoms

      int startAtomIndex, endAtomIndex;
      int idx;
      TAtm *curFirst = curres->firstAtomPtr();
      if (curFirst == (TAtm*)0) {
        // The current residue is empty. Check residues before this one to see
        // if they have any atoms
        if (curResIndex > 0) {
          // Find first non-empty residue prior to this one
          for (idx=curResIndex-1; idx >= 0; idx--) {
            curFirst = findResidue(idx)->firstAtomPtr();
            if (curFirst != (TAtm*)0) {
              break;
            }
          }
          // Found a residue with some atoms. Now scan forward through all
          // atoms in the residue till we find the last one. We will add
          // our new atom after this one.
          if (curFirst != (TAtm*)0) {
            idx = curFirst->index();
            int natoms = numAtoms();
            Residue *res = curFirst->getResidue();
            while (idx < natoms && p_atoms[idx]->getResidue() == res ) {
              curFirst = atomRef(idx);
              idx++;
            }
            startAtomIndex = curFirst->index();
          } else {
            // No atoms in system prior to this residue
            startAtomIndex = -1;
          }
        } else {
          // First residue in system is empty
          startAtomIndex = -1;
        }
        // Need to find first atom after this residue. Check residues after this one
        // to see if they have any atoms.
        endAtomIndex = numAtoms();
        if (curResIndex < nResidues-1) {
          for (idx=curResIndex+1; idx < nResidues; idx++) {
            curFirst = findResidue(idx)->firstAtomPtr();
            if (curFirst != (TAtm*)0) {
              endAtomIndex = curFirst->index();
              break;
            }
          }
        }
      } else {
        // Current residue is not empty. If atom is attached to
        // another atom in residue then put it right after that
        // atom. Otherwise, put it at the end of the residue.
        if (parent != (TAtm*)0 && parent->getResidue() == curres) {
          startAtomIndex = parent->index();
          endAtomIndex = startAtomIndex+1;
        } else {
          // Find last atom in residue
          idx = curFirst->index();
          int natoms = numAtoms();
          while (idx < natoms && findResidue(idx) == curres) {
            curFirst = atomRef(idx);
            idx++;
          }
          startAtomIndex = curFirst->index();
          endAtomIndex = startAtomIndex+1;
        }
      }

      // assign from curres
      atom->atomResidue(curres);

      atom->index(endAtomIndex);
      if (endAtomIndex < numAtoms()) {
        vector<TAtm*>::iterator it = p_atoms.begin();
        it += endAtomIndex;
        p_atoms.insert(it,atom);
      } else {
        p_atoms.push_back(atom);
      }

      // If we were inserting into an empty one, then we better set the ptr
      if (curres->firstAtomPtr() == (TAtm*)0) {
        curres->firstAtomPtr(atom);
      }
      renumberAtoms(endAtomIndex);
    }
  }
}


/**
 * Sets the lattice.  
 * Can be used to clear the lattice object.
 * Assumes control over lattice object.
 */
void Fragment::setLattice(LatticeDef *lattice)
{
   if (p_lattice != 0) delete p_lattice;
   p_lattice = lattice;
}

/**
 * return lattice object.
 * Returned object should not be deleted.
 */
LatticeDef *Fragment::getLattice() const
{
   return p_lattice;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Add a single residue
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::addResidue(Residue* residue)
{
  // Update residue index anytime residues change
  residue->index(p_residueSequence.size());

  p_residueSequence.push_back(residue);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Delete a single residue from the list
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::deleteResidue(int index)
{
  vector<Residue*>::iterator it = p_residueSequence.begin();
  it += index;
  delete p_residueSequence[index];
  p_residueSequence.erase(it);

  reIndexResidues(index);
}

/**
 * Delete residue atoms.
 * Delete all atoms and bonds in the specified residues.  The tricky part
 * comes in when an atom in a residue that is to be deleted is connected
 * to an atom in another residue that is not to be delted.
 * Algorithm
 *   1. make bit vector which is set for all atoms to be deleted.
 *   2. loop through bit vector.
 *     2a. if atom marked, get connected atoms.
 *     2b. if connected atom is marked, delete bond
 *         else do fancy nub replacement
 *   3. delete the marked atoms.
 *   4. delete the residues themselves
 *   5. renumber all the atoms
 *
 * Note that I make a bitmap to quickly check if connected atoms need to
 * be "nubbed".  In order for this to work, it is critical NOT to add any
 * of the nubs to the frag until we are don't breaking all bonds.
 *
 * @param resList - the unique residue indices; non-const so it can be sorted.
 */
void Fragment::deleteResidues(vector<int>& resList)
{
   // Sort so we can process in order.  The numbers are the index into
   // the residue list, not the residue numbers which are not guaranteed
   // to be unique.  Each chain might will typically start counting
   // residue number at 1...
   sort(resList.begin(),resList.end(),ltint());

   int curEditResIndex = editResidueIndex();
   int curMode = editResidueMode();
   editResidueMode(false,-1);

   int idx;
   TPerTab tpt;

   //  1. create bit vector of all atoms to be deleted
   int nAtoms = this->numAtoms();
   int numResidues = resList.size();
   vector<bool> atomsToDelete(nAtoms);
   for (idx=0; idx<numResidues; idx++) {
      Residue *res = findResidue(resList[idx]);
      vector<TAtm*> atomptrs = findResidueAtoms(res);
      int numRAtoms = atomptrs.size();
      for (int adx=0; adx<numRAtoms; adx++) {
         atomsToDelete[atomptrs[adx]->index()] = true;
      }
   }

   // 1a. make list of atom pointers to delete
   vector<TAtm*> ptrsToDelete;
   for (idx=0; idx<nAtoms; idx++) {
      if (atomsToDelete[idx] == true) {
         ptrsToDelete.push_back(atomRef(idx));
      }
   }


   // 2.
   vector<TAtm*> *conAtoms;
   vector<TAtm*> nubList;
   vector<TAtm*> nubParentList;
   TAtm *src;
   int numToDelete = ptrsToDelete.size();
   for (idx=0; idx<numToDelete; idx++) {
      src = ptrsToDelete[idx];

      // get src's connected atoms
      conAtoms = src->connectedAtoms();
      int numConnectedAtoms = conAtoms->size();

      // loop through all the connected atoms
      TAtm *conatm, *nub;
      for (int cdx = 0; cdx < numConnectedAtoms; cdx++) {

         conatm = (*conAtoms)[cdx];
         // TODO this is too slow - need order(1) lookup
         if (atomsToDelete[conatm->index()] == true) {
            // The bond between these atoms can be eliminated
            // do nothing - bonds go away with atom later.
         } else {
            // The bond between these atoms must be exchanged for a
            // nub/atom bond on the conatm.
            // create a nub
            nub = new TAtm("Nub",src->coordinates());
            nub->shape(ShapeData::terminal);

            // give it a display style so the bond is created correctly
            nub->displayStyle(conatm->displayStyle());

            // create a bond and set atom1 and atom2 ptrs
            new TBond(conatm,nub);

            // add the atom to our fragment
            if (conatm->residueName() == "" ) {
               nubList.push_back(nub);
               nubParentList.push_back(conatm);
            } else {
               // copy all the residue info from the atom to the nub since
               // we want to make sure the nub has the same properties as
               // its atom
               nub->copyResidueInfo(conatm);

               nubList.push_back(nub);
               nubParentList.push_back(conatm);
            }

         }
      }
      delete conAtoms;
   }

   int numNubs = nubList.size();
   TAtm *nub;
   for (idx=0; idx<numNubs; idx++) {
      nub = nubList[idx];
      if (nub->residueName() == "") {
         // use the Fragment's method to skip over the display
         // style setting in SGFragment
         Fragment::addAtom(nub,nubParentList[idx]);
      } else {
         // insert our new nub with its other residue atoms
         Fragment::addAtom(nub,nubParentList[idx]);
      }

   }

   // 3. delete atoms
   list<int> delAtomIndices ;
   for (int j = 0; j < ptrsToDelete.size(); j++) {
      delAtomIndices.push_back(ptrsToDelete[j]->index());
   }
   // Do all the deletes at the end to avoid messing up index orders.
   delAtomIndices.sort() ;

   // TODO This is too slow
   list<int>::reverse_iterator delAtom_riter = delAtomIndices.rbegin();
   TAtm *delatom;
   while (delAtom_riter != delAtomIndices.rend()) {
      // renumber when we are done deleting
      // This is slow.  We can just delete the atoms ourselves in this
      // case because we know we will be deleting the residues next
      //frag->deleteAtom(*delAtom_riter, false);
      delatom = atomRef(*delAtom_riter);
      removeAtom(*delAtom_riter);
      delete delatom;
      delAtom_riter++;
   }

   // 4. delete residues
   for (idx=numResidues-1; idx>=0; idx--) {
      deleteResidue(resList[idx]) ;
   }

   // 5. done deleting.  renumber
   renumberAtoms(0);

   // KLS 6/07 Won't this set to bad values sometimes???
   editResidueMode(curMode,curEditResIndex);

}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Add a vecotor of residues
// Here's the painful part: we have to go through and reassign all the 
// first atom pointers in the residue list (since the atoms have moved
// throught the operator= command.  I don't know that anyone else
// should use this call.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::addResidues(vector<Residue*>& residues)
{
   int cnt = residues.size();
   for ( int r = 0; r < cnt; r++)
   {
      Residue* newres = new Residue( *(residues[r]) );
      addResidue(newres);
   }
}

//////////////////////////////////////////////////////////////////////////////
// findResidue
//   return residue to which atom belongs - null if there are no residues
//   or its not found for some buggy reason.
//
//   Currently we have to do some work to find it.  Hopefully later we
//   will keep a back pointer.  We rely on correct atom indices for this
//   function to work.
//////////////////////////////////////////////////////////////////////////////
Residue* Fragment::findResidue(const TAtm *atom) const
{
  return atom->getResidue();
}

//////////////////////////////////////////////////////////////////////////////
//  Description
//    Return a pointer to a Residue object given the residue index
//
////////////////////////////////////////////////////////////////////////////
Residue* Fragment::findResidue(const int index) const
{
  return p_residueSequence[index];
}

int Fragment::getResidueIndex(Residue *residue) const
{
   int ret = 0;
   for (int idx=0; idx<p_residueSequence.size(); idx++) {
      if (p_residueSequence[idx] == residue) {
         ret = idx;
         break;
      }
   }
   return ret;
}


/////////////////////////////////////////////////////////////////////////////
//  Description
//    Insert a single residue before location ires
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::insertResidue(int ires, Residue* residue)
{
  vector<Residue*>::iterator it = p_residueSequence.begin();
  it += ires;
  p_residueSequence.insert(it,residue);
  reIndexResidues(ires);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return a pointer to a Residue object given the residue name, number,
//    insertion code, and segment name
//
///////////////////////////////////////////////////////////////////////////////
Residue* Fragment::findResidue(const char *name, const int ires,
                               const char icode, const char ichain) const
{
  int Nres = p_residueSequence.size();
  int i;
  int itres;
  char itchain, itcode;
  string tname;
  Residue *res;
  for (i = 0; i<Nres; i++) {
    res = p_residueSequence[i];
    tname = res->name();
    itres = res->number();
    itchain = res->chain();
    itcode = res->insertCode();
    
    if (strncmp(tname.c_str(),name,3) == 0 && itres == ires &&
        itcode == icode && itchain == ichain) {
      return res;
    }
    
  }
  return NULL;
}
Residue* Fragment::findResidue(const int ires,const char icode,
                               const char iseg) const
{
  int Nres = p_residueSequence.size();
  int i;
  int itres;
  char itseg, itcode;
  Residue *res;
  for (i = 0; i<Nres; i++) {
    res = p_residueSequence[i];
    itres = res->number();
    itseg = res->chain();
    itcode = res->insertCode();
    if (itres == ires && itcode == icode && itseg == iseg) {
      return res;
    }
   
  }
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Provide a list of pointers to the atoms contained within a residue,
//    given a pointer to a particular residue object
//
///////////////////////////////////////////////////////////////////////////////
vector<TAtm*> Fragment::findResidueAtoms(const Residue *res) const
{
  int Natoms = numAtoms();
  string name = res->name();
  int ires = res->number();
  char icode = res->insertCode();
  char iseg = res->chain();
  vector<TAtm*> atomvec;

  if (res->firstAtomPtr() != (TAtm*)0) {
    int index = res->firstAtomPtr()->index();
    string aname;
    int ares;
    char acode,aseg;
    if (index > 0) {
      TAtm* tatm = p_atoms[index-1];
      aname = tatm->residueName();
      ares = tatm->residueNumber();
      acode = tatm->icode();
      aseg = tatm->chain();
      EE_RT_ASSERT(!(strncmp(aname.c_str(),name.c_str(),3)==0 && ares==ires &&
                   acode==icode && aseg==iseg), EE_WARNING,
                   "firstAtomPtr is not first atom in residue!!!!");
    }
    TAtm * atm;
    bool endlist = false;
    while (index < Natoms && !endlist) {
      atm = p_atoms[index];
      aname = atm->residueName();
      ares = atm->residueNumber();
      acode = atm->icode();
      aseg = atm->chain();
      if (strncmp(aname.c_str(),name.c_str(),3) == 0 && ares == ires &&
          acode == icode && aseg == iseg ) {
        atomvec.push_back(atm);
      } else {
        endlist = true;
      }
      index++;
    }
  }
  return atomvec;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Provide a list of pointers to the atoms contained within a residue,
//    given a pointer to a particular residue object
//
///////////////////////////////////////////////////////////////////////////////
vector<int> Fragment::findResidueAtomIndices(int index) 
{
   Residue *res = findResidue(index);
   // need these to find atoms
   string name = res->name();
   int ires = res->number();
   char icode = res->insertCode();
   char iseg = res->chain();

   vector<int> atomvec;

   TAtm * atm = res->firstAtomPtr() ;
   if ( atm != NULL )
   {
      string aname = "";
      int ares;
      char acode,aseg;

      for (int i = atm->index(); i < numAtoms(); i++) 
      {
         atm = p_atoms[i];
         aname = atm->residueName();
         ares = atm->residueNumber();
         acode = atm->icode();
         aseg = atm->chain();
         if (strncmp(aname.c_str(),name.c_str(),3) == 0 && ares == ires &&
               acode == icode && aseg == iseg ) 
         {
            atomvec.push_back(atm->index());
         }
         else
            break ;
      }
   }

  return atomvec;
}

///////////////////////////////////////////////////////////////////////////////
//
// Description
//    Change the bond list.
//
// Implementation
//    We validate that the bond indices are good for this fragment.
//    The old bonds are deleted and new ones added using the passed
//    bond indices.
//    This code is compiled away if an INSTALL build is being done.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::addBonds(const int* bonds, int numBonds, 
    const double* orders, const DisplayStyle *styles)
{
  if (numBonds != 0 ) {
    const int *bondPtr = bonds;
    int index;
#ifndef INSTALL
    bool error = false;
    int numAtoms = this->numAtoms();
    for (index = 0; index < (numBonds * 2); index++, bondPtr++) {
      if ((*bondPtr) >= numAtoms) {
        ErrMsg().message("EE_ARG_MISMATCH", EE_WARNING, WHERE,
            "Illegal Bond Index");
        error = true;
        break;
      }
    }
    if (!error) {
      bondPtr = bonds;
#endif
      TAtm *katm1, *katm2;
      TBond *bond;
      if (orders != (const double*)0) {
        for (index = 0; index < numBonds; index++) {
          katm1 = p_atoms[*bondPtr++];
          katm2 = p_atoms[*bondPtr++];
	       bond = new  TBond(katm1,katm2,(*orders++));
          if (styles)  {
            bond->displayStyle(*styles++); 
          }
	    }
      } else {
        for (index = 0; index < numBonds; index++) {
          katm1 = p_atoms[*bondPtr++];
          katm2 = p_atoms[*bondPtr++];
          bond = new TBond(katm1,katm2);
          if (styles)  {
            bond->displayStyle(*styles++); 
          }
        }
      }
#ifndef INSTALL
    }
#endif
  }
}

///////////////////////////////////////////////////////////////////////////////
// Description
//   Get all atoms connected to theAtom.  On the first call, visited
//   should include the index of theAtom and lastAtom should equal
//   firstAtom and be the atom FROM the direction you want to traverse.
//   (the bond in question).
///////////////////////////////////////////////////////////////////////////////
bool Fragment::getConnected(vector<bool>& visited,
	      const TAtm* theAtom,
	      const TAtm* lastAtom,
	      const TAtm* firstAtom) const
{
  bool foundCycle = false;
  const vector<TBond*>& bonds = theAtom->bondList();
  int cnt = bonds.size();
  TBond *theBond = (TBond*)0;
  for (int idx=0; idx<cnt; idx++) {
    theBond = bonds[idx];
    const TAtm* nextAtom;
    if ((theBond->atom1()) == theAtom) {
      nextAtom = theBond->atom2();
    } else { 
      nextAtom = theBond->atom1();
    }
    if (nextAtom != lastAtom) {
      if (nextAtom == firstAtom) {
        foundCycle = true;
        break;
      } else if (!(visited[nextAtom->index()])) {
        visited[nextAtom->index()] = true;
        if ((foundCycle = getConnected(visited, nextAtom, theAtom, firstAtom)))
          break;
      }
    }
  }

  return foundCycle;
}

///////////////////////////////////////////////////////////////////////////////
// Description
//   Same as above except this one returns list of bonds too.
///////////////////////////////////////////////////////////////////////////////
bool Fragment::getConnected(vector<bool>& vatoms, vector<bool>& vbonds,
	      const TAtm* theAtom,
	      const TAtm* lastAtom,
	      const TAtm* firstAtom) const
{
  bool foundCycle = false;
  const vector<TBond*>& bonds = theAtom->bondList();
  int cnt = bonds.size();
  TBond *theBond = (TBond*)0;
  for (int idx=0; idx<cnt; idx++) {
    theBond = bonds[idx];
    const TAtm* nextAtom;
    if ((theBond->atom1()) == theAtom) {
      nextAtom = theBond->atom2();
    } else { 
      nextAtom = theBond->atom1();
    }
    if (nextAtom != lastAtom) {
        vbonds[theBond->index()] = true;
      if (nextAtom == firstAtom) {
        foundCycle = true;
        break;
      } else if (!(vatoms[nextAtom->index()])) {
        vatoms[nextAtom->index()] = true;
        if ((foundCycle=getConnected(vatoms,vbonds,nextAtom,theAtom,firstAtom)))
          break;
      }
    }
  }

  return foundCycle;
}

/**
 * Simply check distance between atoms to see if less than
 * some scale of the sum of the two atom's radii.
 *   1. sum covalent radii of two atoms
 *   2. check this vs. the distance between the atoms.
 *   There are lots of magic numbers here that should be fixed!
 *
 * NOTE: This must be implemented carefully or the performance degrades
 *       very quickly for large systems.  Don't make assumptions about
 *       what should be faster - I found that actually using a timer
 *       showed unexpected results.
 *
 *
 *     The outer loop index 'i' is AFTER the inner loop 'j': 'i'
 *     leads 'j' in the list: since hydrogens traditionally follow
 *     the heavy atom they're bonded to, this makes it easy to quit
 *     bonding to hydrogens after one bond is made by breaking out of
 *     the 'j' loop when 'i' is a hydrogen and we make a bond to it.
 *     Working backwards like this makes it easy to find the heavy
 *     atom that came 'just before' the Hydrogen. mp
 *
 *     NOTE: don't even think about comparing atomic symbols inside the
 *     loop.  Way way too slow.  Use numbers.
 *
 * @param overrideisdisplayed - true indicates that the bonding algorithm 
 *        will consider the atom even if it is not displayed.
 */
void Fragment::addCovalentBonds(bool overrideIsDisplayed) 
{
  int  nAtoms = numAtoms();

// If only zero or one atoms, return

  if (nAtoms < 2) return;

  double   distance;
  double* covalentRadii = new double[nAtoms];

  TPerTab tpt;
  int nub = tpt.nubAtom();
  int ghost = 0;
  int i, j, k;
  double maxCR = 0.;

  for (int idx = 0; idx < nAtoms; idx++) {
    int num = tpt.atomicNumber(p_atoms[idx]->atomicSymbol());
    covalentRadii[idx] = tpt.covalentRadius(num);
    covalentRadii[idx] *= tpt.covalentRadiusTolerance(num);
    if (covalentRadii[idx] > maxCR) maxCR = covalentRadii[idx];
  }
  maxCR = 2.0 * maxCR;
  MPoint p1,p2;

  vector<TAtm*> *atoms = this->atoms();
  LinkCellList cellList(atoms, maxCR);

  const int *link_list = cellList.getLinkCellList();
  const int *header = cellList.getHeader();

// Construct list of bonds. Start by looping over all atoms

  int itag, jtag;
  TAtm *atm1, *atm2;
  for (i = 0; i < nAtoms; i++) {
    atm1 = p_atoms[i];
    if (!overrideIsDisplayed && !atm1->displayStyle().isDisplayed()) continue;

    p1.xyz(atm1->coordinates());

    itag = atm1->atomicNumber();
    if (itag == ghost || itag == nub) continue;

// Construct bonds to atoms in same cell as atom i
 
    j = link_list[i];
    while (j >= 0) {

      atm2 = p_atoms[j];
      jtag = atm2->atomicNumber();
      if (jtag != ghost && jtag != nub) {
 
     // Never bond Hydrogens to each other

        if (itag != 1 || jtag != 1) { 

          distance = covalentRadii[i] + covalentRadii[j];
          distance *= distance;

          p2.xyz(atm2->coordinates());
          p2.subtract(p1);
          if (p2.lengthSqr()<=distance && bond(j,i)==(TBond*)0)
            addBond(j, i, 1.0);
        }
      }
      j = link_list[j];
    }

// Find bonds to atoms in cells neighboring the cell containing
// atom i. Start by locating index of cell containing atom i.

    int ix = cellList.xIndex(p1.x());
    int iy = cellList.yIndex(p1.y());
    int iz = cellList.zIndex(p1.z());
    int l = cellList.getCellIndex(ix, iy, iz);

    int kxmin, kymin, kzmin, kxmax, kymax, kzmax;
    int kx, ky, kz;

    kxmin = cellList.ixMin(ix);
    kxmax = cellList.ixMax(ix);
    for (kx = kxmin; kx <= kxmax; kx++) {

      kymin = cellList.iyMin(iy);
      kymax = cellList.iyMax(iy);
      for (ky = kymin; ky <= kymax; ky++) {

        kzmin = cellList.izMin(iz);
        kzmax = cellList.izMax(iz);
        for (kz = kzmin; kz <= kzmax; kz++) {
          k = cellList.getCellIndex(kx, ky, kz);

// Make sure that we only calculate bonds once. Accomplish this
// by only constructing bonds between cells for which k > l.

          if (k > l) {
            j = header[k];
            while (j >= 0) {

              atm2 = p_atoms[j];
              jtag = atm2->atomicNumber();
              if (jtag != ghost && jtag != nub) {

             // Never bond Hydrogens to each other

                if (itag != 1 || jtag != 1) {

                  distance = covalentRadii[i] + covalentRadii[j];
                  distance *= distance;

                  p2.xyz(atm2->coordinates());
                  p2.subtract(p1);
                  if (p2.lengthSqr() <= distance) {
                    if (bond(j,i) == (TBond*)0) addBond(j, i, 1.0);
                  }
                }
              }
              j = link_list[j];
            }
          }
        }
      }
    }
  }
  atoms->clear();
  delete atoms;
  delete[] covalentRadii;
 // Scan atoms to look for hydrogens with more than one bond. Delete
 // all bonds except the shortest if any are found. Only do this if system
 // contains residues.
  int ichk;
  double blen, tmplen;
  vector<int> index1, index2;
  if (numResidues() > 0) {
    for (i=0; i<nAtoms; i++) {
      if (p_atoms[i]->atomicNumber() == 1) {
        const vector<TBond*>& bonds = p_atoms[i]->bondList();
        int nbonds = bonds.size();
        if (nbonds>1) {
          vector<int> index;
          TAtm *atm;
          for (j=0; j<nbonds; j++) { 
            atm = bonds[j]->atom1();
            if (atm->index() == i) {
              atm = bonds[j]->atom2();
            }
            if (p_atoms[i]->getResidue() == atm->getResidue()) {
              index.push_back(j);
            }
          }
          if (index.size() > 0) {
            ichk = index[0];
            blen = bonds[index[0]]->length();
            for (j=1; j<index.size(); j++) { 
              tmplen = bonds[index[j]]->length();
              if (tmplen < blen) {
                ichk = index[j];
                blen = tmplen;
              }
            }
          } else {
            ichk = 0;
            blen = bonds[0]->length();
            for (j=1; j<nbonds; j++) { 
              tmplen = bonds[j]->length();
              if (tmplen < blen) {
                ichk = j;
                blen = tmplen;
              }
            }
          }
          for (j=0; j<nbonds; j++) {
            if (j != ichk) {
              index1.push_back(bonds[j]->atom1()->index());
              index2.push_back(bonds[j]->atom2()->index());
            }
          }
          for (j=0; j<nbonds-1; j++) {
            deleteBond(index1[j],index2[j]);
          }
        }
        index1.clear();
        index2.clear();
      }
    }
  }
//cout << "bond generation done " << timer.elapsedTime() << endl;
//cout << "total number of bonds computed " << bondcheck << endl;
  generateShapes();
  generateBondOrders();
}

////////////////////////////////////////////////////////////////////////////////
// Description:
//   Finds all hydrogen bonds. Donors can be a hydrogen atom attached to
//   oxygen, nitrogen, or carbon and acceptors can be an oxygen, nitrogen, or
//   fluorine atom.
////////////////////////////////////////////////////////////////////////////////
void Fragment::addHydrogenBonds(void)
{
  int  nAtoms = numAtoms();

// If only zero or one atoms, return

  if (nAtoms < 2) return;

  string atsym;

  TPerTab tpt;
  int nub = tpt.nubAtom();
  int ghost = 0;
  int i, j, k;
  double maxR = 2.01;

  set<int,less<int> > exSet;

  MPoint p1,p2;

  vector<TAtm*> *atoms = this->atoms();
  LinkCellList cellList(atoms, maxR);

  maxR = maxR*maxR;

  const int *link_list = cellList.getLinkCellList();
  const int *header = cellList.getHeader();

// Construct list of bonds. Start by looping over all atoms

  int itag;
  TAtm *atm1, *atm2;
  for (i = 0; i < nAtoms; i++) {
    atm1 = p_atoms[i];
    if (!atm1->displayStyle().isDisplayed()) continue;
    // If atom is not hydrogen, skip to next atom.
    if (atm1->atomicNumber() != 1) continue;
    // If hydrogen is not connected to Oxygen, Nitrogen, or Fluorine,
    // continue.
    const vector<TBond*> blist = atm1->bondList();
    bool isDonor = false;
    exSet.clear();
    for (j=0; j<blist.size(); j++) {
      atm2 = blist[j]->atom1();
      if (atm2 == atm1) {
        atm2 = blist[j]->atom2();
      }
      exSet.insert(atm2->index());
      if (blist[j]->order() > 0.99) {
        atsym = atm2->atomicSymbol();
        if (atsym == "N" || atsym == "O" || atsym == "F") {
          isDonor = true;
        }
      }
    }
    if (!isDonor) continue;

    p1.xyz(atm1->coordinates());

    itag = atm1->atomicNumber();
    if (itag == ghost || itag == nub) continue;

// Construct hydrogen bonds to atoms in same cell as atom i that are not
// already covalently linked to i
 
    j = link_list[i];
    while (j >= 0) {

      atm2 = p_atoms[j];
      // If atm2 is already bonded to atm1, continue;
      if (exSet.find(atm2->index()) != exSet.end()) continue;
      atsym = atm2->atomicSymbol();
      if (atsym != "N" && atsym != "O" && atsym != "F") continue;

      p2.xyz(atm2->coordinates());
      p2.subtract(p1);
      if (p2.lengthSqr()<=maxR && bond(j,i)==(TBond*)0)
        addBond(j, i, 0.25);
      j = link_list[j];
    }

// Find bonds to atoms in cells neighboring the cell containing
// atom i. Start by locating index of cell containing atom i.

    int ix = cellList.xIndex(p1.x());
    int iy = cellList.yIndex(p1.y());
    int iz = cellList.zIndex(p1.z());
    int l = cellList.getCellIndex(ix, iy, iz);

    int kxmin, kymin, kzmin, kxmax, kymax, kzmax;
    int kx, ky, kz;

    kxmin = cellList.ixMin(ix);
    kxmax = cellList.ixMax(ix);
    for (kx = kxmin; kx <= kxmax; kx++) {

      kymin = cellList.iyMin(iy);
      kymax = cellList.iyMax(iy);
      for (ky = kymin; ky <= kymax; ky++) {

        kzmin = cellList.izMin(iz);
        kzmax = cellList.izMax(iz);
        for (kz = kzmin; kz <= kzmax; kz++) {
          k = cellList.getCellIndex(kx, ky, kz);

// Make sure that we only calculate bonds once. Accomplish this
// by only constructing bonds between cells for which k > l.

          if (k > l) {
            j = header[k];
            while (j >= 0) {

              atm2 = p_atoms[j];
              // If atm2 is already bonded to atm1, continue;
              if (exSet.find(atm2->index()) != exSet.end()) continue;
              atsym = atm2->atomicSymbol();
              if (atsym != "N" && atsym != "O" && atsym != "F") continue;

              p2.xyz(atm2->coordinates());
              p2.subtract(p1);
              if (p2.lengthSqr() <= maxR) {
                if (bond(j,i) == (TBond*)0) addBond(j, i, 0.25);
              }
              j = link_list[j];
            }
          }
        }
      }
    }
  }
  atoms->clear();
  delete atoms;
}



/** 
 * Change a bond order or add the bond if it doesn't exist.
 */
bool Fragment::changeBond(TAtm *atm1, TAtm *atm2, float order)
{
   bool ret = false;

   //is any atom a nub ?
   if (atm1->atomicSymbol() != "Nub" &&
         atm2->atomicSymbol() != "Nub") {

      int iatm1 = atomIndex(*atm1);
      int iatm2 = atomIndex(*atm2);
      TBond* tbond = bond(iatm1, iatm2);
      if (tbond != (TBond*)0) {
         tbond->order(order);
      } else {
         addBond(iatm1, iatm2, order);
      }
      ret = true;
   }
   return ret;
}


/**
 * Bond two atoms that have nubs and remove nubs as needed.
 * An exception is thrown for user type errors such as bonding and atom
 * to itself or no nubs are available.
 */
void Fragment::bondAtoms(TAtm *atm1, TAtm *atm2, float order,
      const DisplayStyle& style) throw (EcceException)
{
   TAtm *parent1=0, *nub1=0, *parent2=0, *nub2=0;

   // First we need to make sure that we have nubs for bonding
   if (atm1->atomicSymbol() == "Nub") {
      nub1 = atm1;
      parent1 = nubParent(nub1);
   } else {
      parent1 = atm1;
      nub1 = childNub(parent1);
   }

   if (atm2->atomicSymbol() == "Nub") {
      nub2 = atm2;
      parent2 = nubParent(nub2);
   } else {
      parent2 = atm2;
      nub2 = childNub(parent2);
   }

   if (parent1 == 0 || nub1 == 0 || parent2 == 0 || nub2 == 0) {
      throw EcceException("No bond sites are available to create this bond.", WHERE);
   }

   //check to see that we are not bonding to the same atom
   if (parent1 == parent2 ) {
      throw EcceException("Cannot bond an atom to itself.",WHERE);
   }


   // if the 2 parents are directly bonded, cannot make a new one
   // The caller may have checked the original atoms but either of them
   // may have been a nub so we have to check again.
   int iatm1 = atomIndex(*parent1);
   int iatm2 = atomIndex(*parent2);
   TBond *tbond = bond(iatm1, iatm2);
   if (tbond != (TBond*)0) {
      throw EcceException("The atoms are already bonded.",WHERE);
   }

   new TBond(parent1, parent2, style, order);
   if (nub1 > nub2) {
      deleteAtom(atomIndex(*nub1));
      deleteAtom(atomIndex(*nub2));
   } else {
      deleteAtom(atomIndex(*nub2));
      deleteAtom(atomIndex(*nub1));
   }

}



////////////////////////////////////////////////////////////////////////////////
//
// Description
//   Generate shapes for all atoms in the fragment
//
// Implementation
//   Shape is assigned based on the number of bonds the atom has.
//   If an atom has 2 or 3 bonds, then check the angle between the bonds
//   and assign other shape if necessary.
//   Atom with 2 bonds is linear unless the angle is not 180 +- 8 degrees
//   Atom with 3 bonds is trigonalPlanar unless the angle is not 120 +- 8
//   degrees. (angles are arbitrary ranges supplied by Hugh).
//
////////////////////////////////////////////////////////////////////////////////
void Fragment::generateShapes(void)
{
  bool shapeOk = true;
  int atmCntr;
  TAtm *currAtm;
  do {
    shapeOk = true;
    for (atmCntr = 0; atmCntr < p_atoms.size(); atmCntr++) {
       currAtm = p_atoms[atmCntr];
       shapeOk = generateShape(currAtm);
    }
  } while (shapeOk==false);
}



/**
 * Determines and sets the shape for the specified atom.
 * @param atm the atom to determine shape for.
 * @return true if a known shape can be identified.
 */
bool Fragment::generateShape(TAtm *atm)
{
   bool ret = true;
   ShapeData::Shape atmShape = ShapeData::unknown;
   TAtm *atom1;
   TAtm *atom2;
   TAtm *atom3;
   double angle;

   // This totally ignores metal-pi bonds!!!!
   // Maybe not a big problem since this is only called after
   // reading in a molecule which probably doesn't have metal-pi
   // bonds?  This is a question not a statement.
   switch (atm->bondList().size())
   {
      case 0:
         atmShape = ShapeData::lone;
         break;
      case 1:
         atmShape = ShapeData::terminal;
         break;
      case 2:
         atmShape = ShapeData::linear;
         atom1 = (atm->bondList()[0])->atom1();
         if (atom1 == atm)
            atom1 = (atm->bondList()[0])->atom2();
         atom2 = (atm->bondList()[1])->atom1();
         if (atom2 == atm)
            atom2 = (atm->bondList()[1])->atom2();
         angle = atm->angleBetween(atom1,atom2);
         if (angle < 172 || angle > 188)
            atmShape = ShapeData::bent;
         break;
      case 3:
         atmShape = ShapeData::trigonalPlanar;
         if ( atm->atomicSymbol() != "C" ) { // 3-coordinate C always planar
            atom1 = (atm->bondList()[0])->atom1();
            if (atom1 == atm)
               atom1 = (atm->bondList()[0])->atom2();
            atom2 = (atm->bondList()[1])->atom1();
            if (atom2 == atm)
               atom2 = (atm->bondList()[1])->atom2();
            atom3 = (atm->bondList()[2])->atom1();
            if (atom3 == atm)
               atom3 = (atm->bondList()[2])->atom2();
            double angle12, angle23, angle13;
            angle12 = atm->angleBetween(atom1,atom2);
            angle23 = atm->angleBetween(atom2,atom3);
            angle13 = atm->angleBetween(atom1,atom3);
            angle = angle12 + angle23 + angle13;
            if (angle < 355)
               atmShape = ShapeData::pyramidal;
         }
         break;
      case 4:
         atmShape = ShapeData::tetrahedral;
         break;
      case 5:
         atmShape = ShapeData::trigonalBipyramidal;
         break;
      case 6:
         atmShape = ShapeData::octahedral;
         break;
      case 7:
         atmShape = ShapeData::pentagonalBipyramidal;
         break;
      case 8:
         atmShape = ShapeData::cubic;
         break;
      case 9:
         atmShape = ShapeData::tricappedTrigonalPrismD3h;
         break;
      case 10:
         atmShape = ShapeData::decahedral;
         break;
      default:
         // If we don't understand the shape,  we are just going to delete
         // all the atom's bonds and make the user do it manually.
         EE_ASSERT(0,EE_INFO,"Unknown shape - deleting all bonds.")
            atm->removeBonds();
         ret=false;
         atmShape = ShapeData::unknown;
         break;
   }
   atm->shape(atmShape);

   return ret;
}




////////////////////////////////////////////////////////////////////////////////
//
// Description
//   Generate bond orders for all bonds in the fragment
//
// Implementation
//   Assume all bonds are single unless one of the following:
//    Double:
//     C-C bond XX long +- YY
//     C-N bond XX long +- YY
//     C-O bond XX long +- YY
//     C-S bond XX long +- YY
//     N-N bond XX long +- YY
//     N-O bond XX long +- YY
//     N-S bond XX long +- YY
//     O-S bond XX long +- YY
//    Triple:
//     C-C bond XX long +- YY
//     N-N bond XX long +- YY
//     C-N bond XX long +- YY
//  Bond lengths are from CRC Chem-Physics handbook, with fudge factors
//  provided by Hugh Taylor. (Lengths are in angstroms)
//  Step through all bonds in the Fragment and check to see if it meets
//  this criteria. 
//
////////////////////////////////////////////////////////////////////////////////
void Fragment::generateBondOrders(void)
{
  // This is to optimize the loop as much as possible.  Don't do strcmp.
  TPerTab tpt;
  int C = tpt.atomicNumber("C");
  int N = tpt.atomicNumber("N");
  int O = tpt.atomicNumber("O");
  int S = tpt.atomicNumber("S");

  TAtm* atom1;
  TAtm* atom2;
  double distance;
  vector<TBond*>* const bonds = this->bonds();
  int cnt = bonds->size();
  TBond* bond;
  int anum1, anum2;
  for (int idx=0; idx<cnt; idx++) {
    bond = (*bonds)[idx];
    atom1 = bond->atom1();
    anum1 = atom1->atomicNumber();
    atom2 = bond->atom2();
    anum2 = atom2->atomicNumber();
    bond->order(TBond::strToOrder("Single"));
    if ((anum1 == C) && (anum2 == C)) {
      distance = atom1->distanceFrom(atom2);
      if (distance < 1.26) {
        bond->order(TBond::strToOrder("Triple"));
      } else if (distance < 1.37) {
        bond->order(TBond::strToOrder("Double"));
      } else if (distance < 1.49) {
        bond->order(TBond::strToOrder("Aromatic"));
      }
    } else if ((anum1 == N) && (anum2 == N)) {
      distance = atom1->distanceFrom(atom2);
      if (distance <1.15) {
        bond->order(TBond::strToOrder("Triple"));
      } else if (distance < 1.28) {
        bond->order(TBond::strToOrder("Double"));
      } else if (distance < 1.34) {
        bond->order(TBond::strToOrder("Aromatic"));
      }
    } else if ((anum1 == C) && (anum2 == N)) {
      distance = atom1->distanceFrom(atom2);
      if (distance < 1.25) {
        bond->order(TBond::strToOrder("Triple"));
      } else if (distance < 1.45) {
        bond->order(TBond::strToOrder("Aromatic"));
      }
    } else if (((anum1 == C) && (anum2 == O)) ||
             ((anum1 == O) && (anum2 == C))) {
      distance = atom1->distanceFrom(atom2);
      if (distance < 1.30) {
        bond->order(TBond::strToOrder("Double"));
      }
    } else if (((anum1 == N) && (anum2 == O)) ||
             ((anum1 == O) && (anum2 == N))) {
      distance = atom1->distanceFrom(atom2);
      if (distance < 1.30) {
        bond->order(TBond::strToOrder("Double"));
      }
    } else if (((anum1 == C) && (anum2 == S)) ||
             ((anum1 == S) && (anum2 == C))) {
      distance = atom1->distanceFrom(atom2);
      if (distance < 1.71) {
        bond->order(TBond::strToOrder("Double"));
      }
    }
  }
  delete bonds;
}


//
// --------- Builder Modifiers ---------
vector<string> Fragment::generateLabels(int start)
{
  vector<string> ret;
  int cnt = start;
  char buf[12];
  int numa = p_atoms.size();
  TAtm *atom;
  for (int idx=0; idx<=numa; idx++) {
    atom = p_atoms[idx];
    sprintf(buf,"%s%d",atom->atomicSymbol().c_str(),cnt++);
    atom->atomName(buf);
    ret.push_back(buf);
  }
  return ret;
}


bool Fragment::containsNubs()
{
  bool ret = false;

  int cnt = p_atoms.size();
  for (int idx=0; idx<cnt; idx++)
    if (p_atoms[idx]->atomicSymbol() == "Nub") {
      ret = true;
      break;
    }

  return ret;
}


// caller responsible for deleting the returned vector.
vector<int> * Fragment::getNubsIndex()
{
  vector<int> * ret = new vector<int>;
  int cnt = p_atoms.size();
  for (int idx=0; idx<cnt; idx++)
    if (p_atoms[idx]->atomicSymbol() == "Nub")
      ret->push_back(idx);
  return ret;
}


void Fragment::deleteNubs(TAtm *atom)
{
  vector<TAtm*>* catoms = atom->connectedAtoms();
  int cnt = catoms->size();
  for (int idx=0; idx<cnt; idx++) {
    if ((*catoms)[idx]->atomicSymbol() == "Nub") {
      deleteAtom((*catoms)[idx]);
    }
  }
  delete catoms;
  renumberAtoms(0);  // maybe provide better start index later?
}

/**
 * If there is a lattice and it has replication factors > 1,
 * resize to the units cell.
 */
void Fragment::resizeToUnitCell()
{
   LatticeDef *lattice = this->getLattice();
   int x,y,z = 1; // lattice replication factors

   lattice->getReplicationFactors(x,y,z);
   if (lattice && (x*y*z) > 1) {
      this->resize(numAtoms() / (x * y * z));
   }
}


/**
 * Resize fragment to the first n atoms.
 * Why?  To unreplicate for example.
 * Currently deletes all bonds and regenerates them.  On large systms where
 * only a few atoms are removed, this might be really a bad choice.
 */
void Fragment::resize(int n)
{
   int nAtoms = this->numAtoms();
   if (n < nAtoms) {
      this->deleteAllBonds();
      for (int idx=nAtoms-1; idx>=n; idx--) {
         this->deleteAtom(idx, false);
      }
      this->addCovalentBonds();

   }
}

///////////////////////////////////////////////////////////////////////////////
//
// Description
//   Delete the specified atom which is assumed to have an accurate
//   index so we shouldn't need to find it.
///////////////////////////////////////////////////////////////////////////////
void Fragment::deleteAtom(TAtm *atom)
{
  deleteAtom(atom->index());
//  p_atoms.remove(atom);
//  delete atom;
}

///////////////////////////////////////////////////////////////////////////////
//
// Description
//    Delete Specified Atom.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::deleteAtom(int index, bool renumber)
{ 
  EE_ASSERT(index <= numAtoms(), EE_FATAL, "Range Error on Atom Index");
  TAtm* atom = p_atoms[index];

  // special case:  is this atom the first atom ptr in a residue?
  Residue* res = atom->getResidue();

  if (p_residueSequence.size() > 0) {
    string errMsg = "No residue associated with atom index ";
    char buf[80];
    sprintf(buf, "%d", index);
    errMsg += buf;
    
    EE_RT_ASSERT(res != 0, EE_FATAL, errMsg);

    TAtm* firstatom =res->firstAtomPtr();
    if (firstatom == atom)  // move ptr
    {
      //cout << p_atoms[index+1]->residueName() << " "
      //   << atom->residueName() << endl ;
      //cout << p_atoms.size() << " " << index+1 << endl ;
      //cout << "residue: " << res->residueNumber() << endl ;
      //cout << "  atom: " << atom->index() ;
      // if we are not at the end of the list &&
      // the next residue is the same as the one we're deleting
      int nextatom = index+1;
      int natoms = p_atoms.size();
      if (nextatom < natoms)
      {
        while (nextatom < natoms &&
               p_atoms[nextatom]->getResidue() == atom->getResidue()) {
          if (p_atoms[nextatom]->atomicSymbol() != "Nub") {
            break;
          } else {
            nextatom++;
          }
        }
        if (nextatom < natoms &&
            p_atoms[nextatom]->getResidue() == atom->getResidue()) {
          // assign the next atom as the firstAtomPtr
          res->firstAtomPtr(p_atoms[nextatom]);
        }
        // otherwise the next atom in the list belongs to a different
        // residue, and we don't have any more atoms that belong to
        // this residue
        else
          res->firstAtomPtr((TAtm*)0);
      }
      // otherwise, we are at the end of the list.  No more atoms
      // exist.  Our Residue doesn't have anything to point to
      else
      {
        res->firstAtomPtr((TAtm*)0);
        //editResidueMode(editResidueMode(), -1); why?
      }
    }
  }

  vector<TAtm*>::iterator it = p_atoms.begin();
  it += index;
  p_atoms.erase(it);
  delete atom;

  if (true == renumber) renumberAtoms(index);
}

///////////////////////////////////////////////////////////////////////////////
//
// Description
//    Change Element for Specified Center.
//
// TODO
//    Make this more efficient by merely replacing pieces of the old
//    atom with only those new pieces needed.  For instance,
//    don't change the shape or coordinates, just the atomic Number,
//    and mass.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::changeTag(int index, const string& tag)
{
  EE_ASSERT(index < numAtoms(), EE_FATAL, "Range Error on Atom Index");
  TAtm* oldAtom = p_atoms[index];

  // create new atom with this tag and coordinates
  TAtm changedAtom(tag.c_str(), oldAtom->coordinates());

  // copy display style info over
  changedAtom.displayStyle(oldAtom->displayStyle());

  // copy residue information over
  changedAtom.atomResidue(oldAtom->getResidue());

  // copy miscellaneous atom information over
  if (oldAtom->atomicSymbol() == changedAtom.atomicSymbol()) {
    changedAtom.atomName(oldAtom->atomName().c_str());
    changedAtom.atomType(oldAtom->atomType().c_str());
    changedAtom.partialCharge(oldAtom->partialCharge());
    changedAtom.chargeGroup(oldAtom->chargeGroup());
    changedAtom.polarizationGroup(oldAtom->polarizationGroup());
    changedAtom.connectionIndex(oldAtom->connectionIndex());
  }

  *oldAtom = changedAtom;
}

///////////////////////////////////////////////////////////////////////////////
//
// Description
//    Modify the Coordinates of the Specified Atom.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::changeCoords(int index, const double* newCoords)
{
  EE_ASSERT((index < numAtoms()) && (newCoords != (const double*)0),
	    EE_FATAL, "Range Error on Atom Index or Null Coordiantes");
  p_atoms[index]->coordinates(newCoords);
}

///////////////////////////////////////////////////////////////////////////////
//
// Description
//    Add a Bond Between the Specified Atoms.
//    The bond pointer is returned.
//
///////////////////////////////////////////////////////////////////////////////
TBond *Fragment::addBond(int index1, int index2, double bondOrder)
{
  EE_ASSERT((index1 < numAtoms()) && (index2 < numAtoms()),
	    EE_FATAL, "Range Error on Atom Index");
  return new TBond(p_atoms[index1], p_atoms[index2], bondOrder);
}

void Fragment::removeAtom(int index)
{
  vector<TAtm*>::iterator it = p_atoms.begin();
  it += index;
  p_atoms.erase(it);
}

///////////////////////////////////////////////////////////////////////////////
//
// Description
//    Delete Bond Between Specified Atoms.
//
// Implementation
//    The Fragment::bond() accessors verifies index ranges.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::deleteBond(int index1, int index2)
{
  TBond* const bond = this->bond(index1, index2);
  if (bond != (TBond*)0) {
    delete bond;
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Description
//    Delete all bonds.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::deleteAllBonds(void)
{
  TBond *bond = 0;
  vector<TBond*> *bonds = this->bonds();
  for (int idx=bonds->size()-1; idx>=0; idx--) {
    bond = (*bonds)[idx];
    if (bond->atom1()->atomicSymbol() == "Nub" ||
        bond->atom2()->atomicSymbol() == "Nub") continue;
    bonds->pop_back();
    delete bond; 
  }
  delete bonds;
}



//
// --------- Accessors ---------
//

bool Fragment::contains(TAtm *atom) const
{
  for (int i=0; i<p_atoms.size();i++) {
    if (p_atoms[i] == atom) return true;
  }
  return false;
}


TAtm *Fragment::nubParent(TAtm *nub) const
{
  TAtm *ret=0;
  vector<TAtm*> *bondedAtoms;
  bondedAtoms = nub->connectedAtoms();
  if (bondedAtoms->size() >= 1) ret = (*bondedAtoms)[0];
  delete bondedAtoms;
  return ret;
}

// Description
//  Return ANY nub directly connected to the atom
TAtm *Fragment::childNub(TAtm *atom) const
{
  TAtm *ret=0;
  vector<TAtm*> *bondedAtoms;
  bondedAtoms = atom->connectedAtoms();
  int cnt = bondedAtoms->size();
  for (int idx=0; idx<cnt; idx++) {
    if ((*bondedAtoms)[idx]->atomicSymbol() == "Nub") {
      ret = (*bondedAtoms)[idx];
      break;
    }
  }

  delete bondedAtoms;
  return ret;
}


/**
 * Return the current value of the firstSolventAtomIndex. This is the
 * residue index corresponding to the first identified solvent
 * residue. A value of -1 indicates that no solvent exists or has
 * been identified
 */
int Fragment::getFirstSolventAtomIndex() const
{
  return p_firstSolventAtomIndex;
}

/**
 * Return the number of atoms in a solvent molecule. This returns 0 if the
 * solvent molecule size has not be set
 */
int Fragment::getSolventNumAtoms() const
{
  return p_solventNumAtoms;
}

/**
 * Return the number of degrees of freedom, if they have been stored in the
 * fragment
 */

int Fragment::getNumDoF() const
{
  return p_DoF;
}
////////////////////////////////////////////////////////////////////////////
// Description
//   Modify the bond length between atom1 and atom2.  Atom1 remains fixed
//   and atom2 is moved away.  If a cycle is
//   found, FALSE is returned and the change is not made.
////////////////////////////////////////////////////////////////////////////
bool Fragment::bondLength(TAtm& atom1, TAtm& atom2, double length,
               bool moveConnected)
{
  bool ret = false;
  MMatrix matrix;

  MPoint v1,v2,tmp;
  double ratio;
  const double *coords = atom2.coordinates();
  v1.xyz(coords[0], coords[1], coords[2]);

  coords = atom1.coordinates();
  tmp.xyz(coords[0], coords[1], coords[2]);
  v1.subtract(tmp);

  ratio = length / v1.length();
  v2.xyz(v1);
  v2.scale(ratio);

  v2.subtract(v1);
  matrix.shiftMatrix(v2);

  // create vector of atoms that are to be transformed
  vector<TAtm*> atoms;
  atoms.push_back(&atom2);
  if (moveConnected) {
     if (!atom2.traverse(atoms,&atom1,&atom1)) {
        transform(atoms,matrix);
        ret = true;
     }
  } else {
     transform(atoms,matrix);
     ret = true;
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////
// Description
//   Set the angle between three atoms to the input angle (assumed in
//   degrees). If a cycle is
//   found, FALSE is returned and the change is not made.
//   atom2 is the center atom of the angle; atom3 is the one moved.
////////////////////////////////////////////////////////////////////////////
bool Fragment::angle(TAtm& atom1,TAtm& atom2,TAtm& atom3,double angle,
                     bool moveConnected)
{
  bool ret = false;

  ////////////////////////////////////////////////////////////////////////
  // construct 3-vectors from atom2 to each of atom1 and atom3
  ////////////////////////////////////////////////////////////////////////
  const double* coord1 = atom1.coordinates();
  const double* coord2 = atom2.coordinates();
  const double* coord3 = atom3.coordinates();
  MPoint vec21(coord1[0]-coord2[0],coord1[1]-coord2[1],coord1[2]-coord2[2]);
  MPoint vec23(coord3[0]-coord2[0],coord3[1]-coord2[1],coord3[2]-coord2[2]);

  ////////////////////////////////////////////////////////////////////////
  // compute the current angle, desired change
  ////////////////////////////////////////////////////////////////////////
  double angleNow =  // radians
    ( vec21.dotProduct(vec23) / (vec21.length() * vec23.length()) );

  ////////////////////////////////////////////////////////////////////////
  // acos function is unique only on interval [0,PI). It maps -1 to
  // 0 instead of 180 degrees
  ////////////////////////////////////////////////////////////////////////

  if (fabs(angleNow+1.0)<1.0e-12) {
    angleNow = PI;
  } else {
    angleNow = acos(angleNow);
  }

  double angleChange = angle*PI/180.0 - angleNow;
  angleChange = angleChange *180./PI;

  ////////////////////////////////////////////////////////////////////////
  // construct the axis of rotation.  It is oriented normal to the
  // plane of the three atoms and passes through atom2.
  ////////////////////////////////////////////////////////////////////////

  MPoint vec2(coord2[0],coord2[1],coord2[2]);
  MPoint axis = vec21.crossProduct1(vec23);

  ////////////////////////////////////////////////////////////////////////
  // If the axis is all 0's, it didn't find a good plane to rotate in.
  // This happens if the three atoms are in a line. Construct a rotation
  // axis perpendicular to the line of atoms. Start by picking a vector
  // along one of the coordinate axes (assuming the three atoms are not
  // parallel to that axis). Then take cross product of that vector and
  // one of the original difference vectors to get vector perpendicular
  // to three atoms.
  ////////////////////////////////////////////////////////////////////////

  MPoint tmpaxis(0.0,0.0,0.0);
  if (axis.length() < 1.e-6) {
    if (vec21.x() != 0 || vec21.y() != 0) {
      tmpaxis.z(1.);
    } else {
      tmpaxis.x(1.);
    }
    axis = tmpaxis.crossProduct1(vec21);
  }

  ////////////////////////////////////////////////////////////////////////
  // construct rotation matrix, then construct general transformation
  // matrix to do that rotation centered on atom2
  ////////////////////////////////////////////////////////////////////////

  MMatrix rotate;
  MPoint vecz(0,0,1);
  rotate.vvRotateMatrix(axis,vecz);

  ////////////////////////////////////////////////////////////////////////
  // Need to find out if it is necessary to change sign on angleChange
  // (do we need to rotate in clockwise or counterclockwise direction?)
  ////////////////////////////////////////////////////////////////////////

  vec21 = rotate.transformPoint(vec21);
  vec23 = rotate.transformPoint(vec23);
  tmpaxis = vec21.crossProduct1(vec23);
  if (tmpaxis.z() < 0) {
    angleChange = -angleChange;
  }
  MMatrix rotatez;
  rotatez.rotateMatrix(angleChange,ZAXIS);

  MMatrix rotatei;
  rotatei.vvRotateMatrix(axis,vecz);
  rotatei.invertMatrix();

  rotatez.multiplyMatrix(rotate);
  rotatei.multiplyMatrix(rotatez);


  ////////////////////////////////////////////////////////////////////////
  // create vector of atoms that are to be transformed 
  ////////////////////////////////////////////////////////////////////////
  vector<TAtm*> atoms;
  atoms.push_back(&atom3);

  ////////////////////////////////////////////////////////////////////////
  // traverse will return a list of atoms connected to atom2, ensuring
  // that no cycle back to atom1 exists (returns foundCycle).
  ////////////////////////////////////////////////////////////////////////

  bool doit = true;
  if (moveConnected) {
    doit = !atom3.traverse(atoms,&atom2,&atom2);
  }


  if (doit) {
    MPoint rotVec;
    double rotCoords[3];

    int cnt = atoms.size();
    TAtm *at;
    for ( int iatom=0; iatom<cnt; iatom++ ) {
      at = atoms[iatom];
      const double* coords = at->coordinates();
      MPoint vec((float)coords[0],(float)coords[1],(float)coords[2]);
      //      rotateAroundAtom2.multMatrixVec(vec,rotVec);
      vec.subtract(vec2);
      rotVec = rotatei.transformPoint(vec);
      rotVec.add(vec2);
      rotCoords[0] = (double)rotVec.x();
      rotCoords[1] = (double)rotVec.y();
      rotCoords[2] = (double)rotVec.z();
      at->coordinates(rotCoords);
    }
    ret = true;
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////
// Description
//   Set the dihedral angle.  The rotation will be about a2 and a3.
//
//   The traverse operation doesn't work in all cases.  Consider a C-4
//   atom with hydrogens added.  If you try to change the dihedral with
//   atoms 3&4 being two of the hydrogens, it will think there is a cycle
//   because it will come back to a3 via the carbon.  The only way I can
//   think to handle this is to see if there is a bond between a3,a4.  IF
//   not, try a2,a4.  If not try a1,a4.  If not use 0.  This would be the
//   arguments to traverse.
/////////////////////////////////////////////////////////////////////////
bool Fragment::dihedral(TAtm *a1, TAtm* a2, TAtm *a3, TAtm
    *a4,double angle)
{
  bool ret = true;
  double curAngle = Measures::computeDihedral(a1,a2,a3,a4);
  double diff = angle - curAngle;
  if (diff != 0) {
    vector<TAtm*> atoms;
    atoms.push_back(a4);
    if ((ret = a4->traverse(atoms,a3,a3)) == false) {
      // ok to rotate  because we didn't find a cycle
      MMatrix rotateM;
      MPoint p1, p2;
      p1.xyz(a2->coordinates());
      p2.xyz(a3->coordinates());
      rotateM.rotateLineMatrix(p1,p2,diff);
      transform(atoms,rotateM);
      ret = true;
    }
  }
  return ret;
}

bool Fragment::dihedralAll(TAtm *a1, TAtm* a2, TAtm *a3, TAtm *a4,double torsion)
{
   bool ret = false;
   double curAngle = Measures::computeDihedral(a1,a2,a3,a4);
   double diff = torsion - curAngle;
   if (diff != 0) {
      int cnt = numAtoms();
      vector<TAtm*> atoms;

      // Get all atoms connected to atom3 (except atom 2).  We will
      // need to traverse to all the connected atoms of each of
      // these.
      vector<bool> bitmask(cnt, false);
      int idx;

      // If there is no bond between 3 and 4, then we want to move
      // everything connected to 4.  Otherwise move everything
      // connected to 4 from the 3 bond direction.
      vector<TAtm*> *connected = a3->connectedAtoms();
      if (find(connected->begin(), connected->end(), a4) !=
            connected->end()) {
         bitmask[a3->index()] = true;
         ret |= getConnected(bitmask,a3,a2,a2);
      } else {
         ret |= getConnected(bitmask,a4,0,0);
      }
      delete connected;

      if (!bitmask[a4->index()]) atoms.push_back(a4);
      int num = 0;
      for (idx=0; idx<cnt; idx++) {
         if (bitmask[idx]) {
            num++;
            atoms.push_back(atomRef(idx));
         }
      }
      if (!ret) {
         MMatrix rotateM;
         MPoint p1, p2;
         p1.xyz(a2->coordinates());
         p2.xyz(a3->coordinates());
         rotateM.rotateLineMatrix(p1,p2,diff);
         transform(atoms,rotateM);
      }
   }
   return ret;
}

/////////////////////////////////////////////////////////////////////////
// Description
//   Rotate around the a1/a2 vector; a bond isn't really required.
/////////////////////////////////////////////////////////////////////////
bool Fragment::rotateAboutBond(TAtm *a1, TAtm *a2, double angle)
{
  bool ret = false;
  vector<TAtm*> atoms;
  atoms.push_back(a2);
  if (!a2->traverse(atoms,a1,a1)) {
    // ok to rotate 
    MMatrix rotateM;
    MPoint p1, p2;
    p1.xyz(a1->coordinates());
    p2.xyz(a2->coordinates());
    rotateM.rotateLineMatrix(p1,p2,angle);
    transform(atoms,rotateM);
    ret = true;
  }

  return ret;
}

void Fragment::transform(const vector<TAtm*>& list, MMatrix& matrix)
{
  if (list.size() == 0) return;

  MPoint xyz1, xyz2;

  int cnt = list.size();
  TAtm *atm;
  for (int idx=0; idx<cnt; idx++) {
    atm = list[idx];

    xyz1.xyz(atm->coordinates());

    //now transform point
    xyz2 = matrix.transformPoint(xyz1);

    // and set the coords back
    atm->coordinates(xyz2.x(),xyz2.y(),xyz2.z());
  }
}



///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return a String Containing the Formula of this Object.
//
//  Implementation
//    We assume that the formula hasn't been set in the outside
//    ChemicalSystem so we must construct one from scratch.  We
//    construct a formula by using the tagCounts vector.
//
///////////////////////////////////////////////////////////////////////////////
string Fragment::formula(void) const
{
  string ret;
  vector<TTagCountPair>* tc = tagCounts();
  ret =  TFormula::formula(*tc,charge());
  delete tc;
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return Pairs of the Tag and Count of the Atoms.
//    The Caller Must Deallocate the Vector that is Returned.
//
///////////////////////////////////////////////////////////////////////////////
vector<TTagCountPair>* Fragment::tagCounts(void) const
{
  vector<TTagCountPair> *result = new vector<TTagCountPair>;
  int cnt = p_atoms.size();
  TTagCountPair lookup;               // Reference TagCountPair
  TAtm *atom=0;
  for (int idx=0; idx<cnt; idx++) {

    atom = p_atoms[idx];
    lookup.tag = atom->atomicSymbol();
    vector<TTagCountPair>::iterator it;
    it = find(result->begin(), result->end(), lookup);
    if (it != result->end()) {
      TTagCountPair &tcpair = *it;
      tcpair.count += 1;
    } else {
      lookup.count = 1;
      result->push_back(lookup);
    }
  }
  return result;
}


/**
 * Return Pairs of the Tag and Count of the Atoms.
 * The Caller Must Deallocate the map that is Returned.
 * Point Charges will not be included in the count.
 */
TagCountMap* Fragment::tagCountsSTL(void) const
{
  TagCountMap *result = new TagCountMap;

  TAtm *atom=0;
  TagCountMap::iterator it;

  // for all the atoms in the chemical system
  for (int idx=0; idx < p_atoms.size(); idx++)
  {
    atom = p_atoms[idx]; // element tag to look up
    bool isPointCharge = false;
    if (atom->getBehaviorType() == TAtm::POINT) {
      isPointCharge = true;
    }

    it = result->find(atom->atomicSymbol().c_str());
    if (it != result->end()) {  // if element tag found
      if (!isPointCharge) {
        (*it).second += 1;
      }
    }
    else {
      if (!isPointCharge) {
        (*result)[atom->atomicSymbol().c_str()] = 1;
      }
      else {
        (*result)[atom->atomicSymbol().c_str()] = 0;
      }
    }

  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return the Number of Atoms in this Object.
//
///////////////////////////////////////////////////////////////////////////////
size_t Fragment::numAtoms(void) const
{ return p_atoms.size(); }


/**
 * Counts the number of atoms excluding nubs.
 */
size_t Fragment::numNonNubs() const
{
   size_t ret = 0;

   size_t total = numAtoms();

   for (size_t idx=0; idx<total; idx++) {
      if (!p_atoms[idx]->isNub()) {
         ret++;
      }
   }
   return ret;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return the Number of Residues in this Object.
//
///////////////////////////////////////////////////////////////////////////////
size_t Fragment::numResidues(void) const
{ return p_residueSequence.size(); }

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return the Number of Bonds in this Fragment.
//    This is Currently a Time Expensive Call - Use Sparingly.
//
///////////////////////////////////////////////////////////////////////////////
size_t Fragment::numBonds(void) const
{
  size_t result = 0;
  vector<TBond*>* const bonds = this->bonds();
  result = bonds->size();
  delete bonds;
  return result;
}

///////////////////////////////////////////////////////////////////////////////
// Description
//   I took this from the old ChemSys class.  Seems like there might be a
//   problem with this since the charge is a float and the nuclear charge
//   is just an int.  Perhaps nuclear charge should be using partial
//   charges if available?  I guess we'll wait and see if anybody
//   notices.
///////////////////////////////////////////////////////////////////////////////
int Fragment::numElectrons(void) const
{
  return (int)nuclearCharge() - charge();
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return the Nuclear Charge on this Fragment.
//
///////////////////////////////////////////////////////////////////////////////
int Fragment::nuclearCharge(void) const
{
  int ncharge = 0;
  for (int idx=p_atoms.size()-1; idx>=0; idx--) {
    if (p_atoms[idx]->getBehaviorType()==TAtm::POINT ||
        p_atoms[idx]->getBehaviorType()==TAtm::BQX) {
    } else if (p_atoms[idx]->atomicSymbol() != "Nub") {
      ncharge += p_atoms[idx]->atomicNumber();
    } else {
      ncharge += 1;  // assume it will be a H
    }
  }
  return ncharge;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return the Number of Atoms for the Specified Tag.
//
///////////////////////////////////////////////////////////////////////////////
int Fragment::count(const string& tag) const
{
  int count = 0;
  for (int idx=p_atoms.size()-1; idx>=0; idx--) {
    if (p_atoms[idx]->atomicSymbol() == tag) count++;
  }
  return count;
}  

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return a vector of coordinates in row-major order.
//    The caller must deallocate the memory with delete[].
//    The local argument is ignore since coordinates are always local.
//
///////////////////////////////////////////////////////////////////////////////
double* Fragment::coordinates(void) const
{
  double* result = new double[numAtoms() * 3];
  double* iter = result;
  int cnt = p_atoms.size();
  for (int idx=0; idx<cnt; idx++) {
    const double* coords = p_atoms[idx]->coordinates();
    (*iter++) = (*coords++); 
    (*iter++) = (*coords++); 
    (*iter++) = (*coords++);
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    This is the accessor for the atoms member of the Fragment class.
//    The caller must deallocate the vector that is returned.
//
///////////////////////////////////////////////////////////////////////////////
vector<TAtm*>* Fragment::atoms(void) const
{
  vector<TAtm*>* ret = new vector<TAtm*>(p_atoms.size());
  *ret  = p_atoms;
  return ret;
}

const vector<Residue*>& Fragment::residues(void)
{
  return p_residueSequence;
}

Residue* Fragment::residue(const int ires) const
{
  if (p_residueSequence.size() > 0 && ires < p_residueSequence.size()) {
    return p_residueSequence[ires];
  } else {
    return (Residue*)0;
  }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    This is the accessor for the atoms member with a specific tag. 
//    The Caller Must Deallocate the Vector that is Returned.
//    (added by liz)
//
///////////////////////////////////////////////////////////////////////////////
vector<TAtm*>* Fragment::atoms(const string& tag) const
{
  vector<TAtm*>* atomlist = new vector<TAtm*>;

  int cnt = p_atoms.size();
  TAtm *saveatom ;
  for (int idx=0; idx<cnt; idx++) { 
    if ( p_atoms[idx]->atomicSymbol() == tag ) {
      saveatom = new TAtm( *(p_atoms[idx]) );  
      atomlist->push_back(saveatom);
    }
  }

  return atomlist ;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return a Vector of the Unique Tags Contained in this SimpleFrag.
//    The Caller Must Deallocate the Vector that is Returned.
//
///////////////////////////////////////////////////////////////////////////////
vector<string>* Fragment::uniqueTags(void) const
{
  vector<string>* tags = new vector<string>;
  int cnt = p_atoms.size();
  vector<string>::iterator it;
  for (int idx=0;idx<cnt; idx++) {
    const string& symbol = p_atoms[idx]->atomicSymbol();
    it = find(tags->begin(), tags->end(), symbol);
    if (it == tags->end()) tags->push_back(symbol);
  }
  return tags;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return a string of the unique tags contained in this Fragment.
//    This is used for TGBSConfig methods and the Basis Tool.
//
///////////////////////////////////////////////////////////////////////////////
string Fragment::uniqueTagStr(void) const
{
  string tagStr;
  vector<string>* tags = uniqueTags();
 
  for(int i=0; i < tags->size(); i++)
  {
    if(i != 0) tagStr += " ";
    tagStr += (*tags)[i];
  }
  delete tags;
  return tagStr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return bond objects.
//
/////////////////////////////////////////////////////////////////////////////
vector<TBond*>* Fragment::bonds(void) const
{
  //ETimer timer;
  //timer.start();
  vector<TBond*> *ret = new vector<TBond*>;
  int cnt = p_atoms.size();
  TBond *bond;

#if 111
  // Initialize a map to 10% larger than numAtoms  - just a guess
  hash_map<unsigned long, TBond*, hash<unsigned long>, equint> mymap((int)(cnt * 1.1));
  for (int idx=0; idx<cnt; idx++) {
    const vector<TBond*>& bonds = p_atoms[idx]->bondList();
    int bcnt = bonds.size();
    for (int jdx=0; jdx<bcnt; jdx++) {
      bond = bonds[jdx];
      if (mymap.find((unsigned long)bond) == mymap.end()) {
        mymap[(unsigned long)bond] = bond;
        ret->push_back(bond);
      }
    }
  }
  //cout << "bonds() map time " << timer.elapsedTime() << " " << ret->size() << endl;;
#else
  // Left this in case we need it - logrithmic performance.

  set<TBond*, ltTBondPtr > bset;
  for (int idx=0; idx<cnt; idx++) {
    const vector<TBond*>& bonds = p_atoms[idx]->bondList();
    int bcnt = bonds.size();
    for (int jdx=0; jdx<bcnt; jdx++) {
      bond = bonds[jdx];
      if (bset.insert(bond).second == true) {
        // insert succeeded (ie not a duplicate)
        ret->push_back(bond);
      }
    }
  }
  //cout << "bonds() set time " << timer.elapsedTime() << " " << ret->size() << endl;;
#endif
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return a c-style Vector Copy of the bonds.
//    The caller must deallocate the memory.
//    Also reset the bond indices.
//
///////////////////////////////////////////////////////////////////////////////
IBondList* Fragment::bondIndices(void) const
{
  vector<TBond*> *const bonds = this->bonds();
  int numBonds = bonds->size();
//RWTimer t;
//t.start();

  IBondList* result = new IBondList(numBonds);
  int* atomIdx = result->matomIndices;
  double* orderIdx = result->mbondOrders;
  DisplayStyle *style = result->mstyle;

  TBond *tempBond;
  for (int index = 0; index < numBonds; index++) {
    tempBond = (*bonds)[index];
    tempBond->index(index);
    (*atomIdx++) = tempBond->atom1()->index();
    (*atomIdx++) = tempBond->atom2()->index();
    (*orderIdx++) = tempBond->order();
    (*style++) = tempBond->displayStyle();
  }
  delete bonds;

//t.stop();
//cout << "bonds indices done " << t.elapsedTime() << endl;

  return result;
}


//
// --------- Builder Accessors ---------
//
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return Const Pointer to Bond Identified by a Pair of Indices.
//    If No Such Bond Exists, a Null Pointer is Returned.
//
///////////////////////////////////////////////////////////////////////////////
TBond* Fragment::bond(int index1, int index2) const
{
  EE_ASSERT((index1 < numAtoms()) && (index2 < numAtoms()),
	    EE_FATAL, "Range Error on Atom Index");
  TBond* result = (TBond*)0;
  Fragment* nonConstThis = (Fragment*)this;
  TAtm* atom2 = nonConstThis->p_atoms[index2];

  const vector<TBond*>& bonds = p_atoms[index1]->bondList();
  int cnt = bonds.size();
  for (int idx=0; idx<cnt; idx++) {
    TBond *bond = bonds[idx];
    if ((bond->atom1() == atom2) || (bond->atom2() == atom2)) {
      result = bond;
      break;
    }
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return Dynamically Allocated Vector of Atm Pointers for Connected Atoms.
//
//  Implementation
//    Forward the call to the Atm class.
//
///////////////////////////////////////////////////////////////////////////////
vector<TAtm*>* Fragment::connectedAtoms(int index) const
{
  EE_ASSERT(index < numAtoms(), EE_FATAL, "Range Error on Atom Index");
  return p_atoms[index]->connectedAtoms();
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return Pointer to Atm for Specified Index.
//    It is a Fatal Error to Request an Illegal Atom Number.
//
///////////////////////////////////////////////////////////////////////////////
TAtm* Fragment::atomRef(int index) const
{
  EE_ASSERT(index < numAtoms(), EE_FATAL, "Bad Atom Index.");
  return p_atoms[index];
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return Index of Specified Atom (or MAXINT if not Found).
//
///////////////////////////////////////////////////////////////////////////////
int Fragment::atomIndex(const TAtm& atom) const
{
  int result = INT_MAX;
  int cnt = p_atoms.size();
  for (int idx=0; idx<cnt; idx++) {
    if (*(p_atoms[idx]) == atom) {
      result = idx;
      break;
    }
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return Const Pointer to Specified Atom's Coordinates.
//
///////////////////////////////////////////////////////////////////////////////
const double* Fragment::atomCoords(int index) const
{
  EE_ASSERT(index < numAtoms(), EE_FATAL, "Range Error on Atom Index");
  return (((Fragment*)this)->p_atoms[index])->coordinates();
}



//
// --------- Aggregate Information ---------
//
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Return a Dynamically Allocated, c-style, Ordered Array of crs.
//    The Caller must Deallocate the Array Memory.
//
///////////////////////////////////////////////////////////////////////////////
double* Fragment::covalentRadii(void) const
{
  TPerTab tpt;
  double* result = new double[numAtoms()];
  double* iter = result;

  int cnt = p_atoms.size();
  for (int idx=0; idx<cnt; idx++) {
    (*iter) = tpt.vwr(tpt.atomicNumber(p_atoms[idx]->atomicSymbol()));
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
//
//  DESCRIPTION
//    Return a Dynamically Alocated, c-style, Ordered Array of Atom Numbers.
//    The Caller must Deallocate the Array Memory.
//
///////////////////////////////////////////////////////////////////////////////
int* Fragment::atomicNumbers(void) const
{
  int* result = new int[numAtoms()];
  int* iter = result;
  int cnt = p_atoms.size();
  for (int idx=0; idx<cnt; idx++) {
    (*iter++) = p_atoms[idx]->atomicNumber();
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
// Description
//   Public version of clearState().
///////////////////////////////////////////////////////////////////////////////
void Fragment::clear() 
{ clearState(); }

void Fragment::pointGroup(const string& group)
{ p_pointGroup = group; }

string Fragment::pointGroup() const
{ return p_pointGroup; }

void Fragment::useSymmetry(const bool& flag)
{ p_useSymmetry = flag; }

bool Fragment::useSymmetry() const
{ return p_useSymmetry; }



///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Private Atoms Modifier Using c-style char* Array.
//
//  Implementation
//    If we can determine that the number of atoms passed as an
//    argument doesn't match the length of the char* array we
//    signal a warning.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::addAtoms(const char** tags, const double* coords, int numAtoms)
{
  EE_ASSERT((tags != (const char**)0) && (coords != (double*)0),
	    EE_FATAL, "Bad arguments to atom() modifier!");
  double* iter = (double*)coords;
  for (int index = 0; index < numAtoms; index++, iter += 3) {
    EE_ASSERT(tags[index] != (char*)0, EE_FATAL,
	      "Bad arguments to atom() modifier!");
    addAtom(new TAtm(tags[index], iter));
  }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Private Atoms Modifier using RogueWave Vectors.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::addAtoms(const vector<string>& tags, const double* coords)
{
  EE_ASSERT(coords != (double*)0, EE_FATAL,
	    "Bad arguments to atom() modifier!");
  int numAtoms = tags.size();
  double* iter = (double*)coords;
  for (int index = 0; index < numAtoms; index++, iter += 3) {
    addAtom(new TAtm(tags[index].c_str(), iter));
  }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Private Atoms Modifier using RogueWave Vectors.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::addAtoms(const EcceSortedVector<TTagCountPair,
                        less<TTagCountPair> >& tags,
                        const double* coords)
{
  int numAtoms = tags.size();
  double* iter = (double*)coords;
  for (int index = 0; index < numAtoms; index++) {
    EE_ASSERT(!tags[index].tag.empty(), EE_FATAL,
              "Bad arguments to atom() modifier!");
    for (int count = 0; count < tags[index].count; count++ ) {
      addAtom(new TAtm(tags[index].tag.c_str(), iter));
    }
    if (coords != (const double*)0) iter += 3;
  }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Private Atoms Modifier Using c-style char* Array.
//
//  Implementation
//    If we can determine that the number of atoms passed as an
//    argument doesn't match the length of the char* array we
//    signal a warning.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::addAtoms(const vector<TAtm*>& atoms)
{
  int cnt = atoms.size();
  for (int idx=0; idx<cnt; idx++) {
    addAtom(new TAtm(*(atoms[idx])));
  }
}

///////////////////////////////////////////////////////////////////////////////
//  Description
//    Private Atom copy 
//
//  Implementation
//    used in operator= to pass over the logic in addAtom
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::copyAtoms(const vector<TAtm*>& atoms)
{
   if( p_atoms.size() > 0) p_atoms.clear();

   int cnt = atoms.size();
   for (int idx = 0; idx < cnt; idx++) 
   {
      TAtm *cp_atom = new TAtm(*(atoms[idx]));
      cp_atom->index(p_atoms.size());
      p_atoms.push_back(cp_atom);
   }
}
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Safely Delete all the State and Memory that we've Allocated.
//
//  Implementation
//    We don't need to explicitly delete the Bonds because deleting
//    their atoms propagates the delete.
//
///////////////////////////////////////////////////////////////////////////////
void Fragment::clearState()
{
  int idx;
  for (idx=p_atoms.size()-1; idx>=0; idx--) {
    delete p_atoms[idx];
    p_atoms.pop_back();
  }
  for (idx=p_residueSequence.size()-1; idx>=0; idx--) {
    delete p_residueSequence[idx];
    p_residueSequence.pop_back();
  }

  editResidueMode(false, -1);
  p_chains.clearChainID();
  p_name = "";
  p_charge = 0;
  p_pointGroup = "C1";
  p_useSymmetry = false;
  clearFirstSolventAtomIndex();
  p_DoF = 0;

  /* Not responsible for this pointer; struggling with design
  if (p_constraints != 0) {
     delete p_constraints;
  }
  */
#ifdef DEBUG
  //cout << "set lattice to zero " << endl;
#endif
  setLattice((LatticeDef*)0);

}



/**
 * Set the constraint model for this fragment.
 * The Constraint models back pointer to Fragment is automatically
 * updated.
 */
void Fragment::setConstraints(GeomConstraintModel *constraints)
{
   if (p_constraints != 0) {
      delete p_constraints;
   }
   p_constraints = constraints;
   p_constraints->setFragment(*this);
}


GeomConstraintModel *Fragment::getConstraints() const
{
   return p_constraints;
}

/**
 * Set the meta-potential model for this fragment.
 * The potential models back pointer to Fragment is automatically
 * updated.
 */
void Fragment::setPotentials(MetaPotentialModel *potentials)
{
   
   if (p_potentials != 0) {
      delete p_potentials;
   }
   p_potentials = potentials;
   p_potentials->setFragment(*this);
}


/**
 * Get the meta-potential model for this fragment.
 */
MetaPotentialModel *Fragment::getPotentials() const
{
   return p_potentials;
}


/**
 * Set the QMMM model for this fragment.
 * The QMMM models back pointer to Fragment is automatically
 * updated.
 */
void Fragment::setQMMMModel(QMMMModel *terms)
{
   if (p_QMMMTerms != 0) {
      delete p_QMMMTerms;
   }
   p_QMMMTerms = terms;
   p_QMMMTerms->setFragment(*this);
}


/**
 * Get the meta-potential model for this fragment.
 */
QMMMModel *Fragment::getQMMMModel() const
{
   return p_QMMMTerms;
}


/**
 * Check to see if at least some fixed charges in system are non-zero.
 * Returns true if there is one or more fixed charges in fragment and
 * at least one of them is non-zero, returns false otherwise.
 */
bool Fragment::checkFixedCharges() const
{
  bool fixedAtoms = false;
  bool nonZero = false; 
  int nAtom = numAtoms();
  for (int i=0; i<nAtom; i++) {
    if (p_atoms[i]->getBehaviorType() == TAtm::POINT) {
      fixedAtoms = true;
      if (p_atoms[i]->partialCharge() != 0.0) {
        nonZero = true;
      }
    }
  }
  return fixedAtoms && nonZero;
}

/**
 * Remove all fixed charges from the system that have zero charge
 */
vector<int> Fragment::getZeroCharges()
{
  vector<int> ret;
  int nAtom = numAtoms();
  for (int i=0; i<nAtom; i++) {
    if (p_atoms[i]->getBehaviorType() == TAtm::POINT) {
      if (p_atoms[i]->partialCharge() == 0.0) {
        ret.push_back(i);
      }
    }
  }
  return ret;
}

/**
 * Reset atom behavior to default for entire system
 */
void Fragment::clearAtomBehavior()
{
  int nAtom = numAtoms();
  for (int i=0; i<nAtom; i++) {
    p_atoms[i]->setBehaviorType(TAtm::UNSPECIFIED);
  }
}

/**
 * Prescan atoms in PDB file looking for multiple models and
 * alternative locations.
 * @param groupFlgVec - not sure; something to toss?
 * @param chainVec set of chains, one per model; each character
 *     in a given chain string represents a unique chain.
 */
bool Fragment::prescanPDB(istream& infile, int& numModels,
                          vector<string>& altLocVec,
                          vector<bool>& groupFlgVec,
                          vector<string>& chainVec,
                          int &TotAtoms)
{
  bool ret = true;
  char* cptr;
  char buf[1025];
  char record[7];
  char altloc[2];
  char chain[2];
  int i,ichk;

  record[6] = '\0';
  altloc[1] = '\0';
  chain[1] = '\0';

  if (!infile.good() || infile.eof()) return false;

// Prescan file looking for multiple models, alternate location
// indicators, and TER cards. Also, check and make sure that file
// is a PDB file (i.e. it has at least one ATOM or HETATM record
// in it).

  int ialt = 0;
  int ipdb = 0;
  numModels = 0;
  TotAtoms = 0;
  int numAssemblies = 1;
  string altLocKey;
  string chainKey;
  bool addOne;
  altLocVec.clear();
  chainVec.clear();
  while (infile.getline(buf,1024)) {
    cptr = buf;
    strncpy(record,cptr,6);
    if (strncmp(record,"ATOM  ",6) == 0 ||
        strncmp(record,"HETATM",6) == 0) {

// If no models found so far, then add one.

      addOne = true;
      if (numModels == 0) {
        altLocKey = "";
        chainKey = "";
        numModels++;
      }
      if (numModels > 1) addOne = false;

      cptr += 16;
      strncpy(altloc,cptr,1);
      cptr += 5;
      strncpy(chain,cptr,1);
      ipdb++;

// Check to see if alternate location key contains a non-whitespace
// character.

      if (isgraph(altloc[0]) != 0) {

// Compare character with previously found characters.

        if (ialt == 0) {
          altLocKey += altloc;
          ialt++;
        } else {
          ichk = 0;
          for (i=0; i<ialt; i++) {
            if (altloc[0] == altLocKey[i]) {
              ichk = 1;
              if (i > 0) addOne = false;
            }
          }
          if (ichk == 0) {
            altLocKey += altloc;
            ialt++;
          }
        }
      }

// Check to see if chain key contains a non-whitespace
// character.

      if (isgraph(chain[0]) != 0) {

// Compare character with previously found characters.

        if (chainKey.find(chain) == string::npos) {
          chainKey += chain;
        }
      }
      if (addOne) TotAtoms++;
    } else if (strncmp(record,"MODEL",5) == 0) {
      if (numModels > 0) {
        altLocVec.push_back(altLocKey);
        chainVec.push_back(chainKey);
        groupFlgVec.push_back(numAssemblies > 1);
      }
      numAssemblies = 1;
      altLocKey = "";
      chainKey = "";
      ialt = 0;
      numModels++;
    } else if (strncmp(record,"TER",3) == 0) {
      numAssemblies++;
    }
  }
  
  // Add alternate locations etc. for final model
  altLocVec.push_back(altLocKey);
  chainVec.push_back(chainKey);
  groupFlgVec.push_back(numAssemblies>1);
#ifdef DEBUG  
  //cout << "numModels = " << numModels << endl;
#endif
  if (ipdb == 0) return false;
  return ret;
}
//////////////////////////////////////////////////////////////////////////
// Description
//   Prescan XYZ file to determine total number of atoms.
//////////////////////////////////////////////////////////////////////////
bool Fragment::prescanXYZ(istream &infile, int &TotAtoms)
{
  static const int BUFSIZE = 240;
  char lbuf[BUFSIZE];  // line buf - none should be longer
  TotAtoms = 0;

  if (!infile.good() || infile.eof()) return false;

  infile.getline(lbuf,BUFSIZE-1,'\n');
  sscanf(lbuf,"%d",&TotAtoms);
  return true;
}
//////////////////////////////////////////////////////////////////////////
// Description
//   Prescan MVM file to determine total number of atoms.
//////////////////////////////////////////////////////////////////////////
bool Fragment::prescanMVM(istream &infile, int &TotAtoms)
{
  char buf[1024];
  while (!infile.eof() && infile.good()) {
    string line;
    infile.getline(buf, 1023);
    line = buf;
    // Add validation here for correct format number
    STLUtil::removeCommentAndSpaces(line);
    if (line != "") {
      string key = STLUtil::stripKey(line);
      if (STLUtil::compareString(key,"num_atoms") == 0) {
        TotAtoms = atoi(line.c_str());
        return true;
      }
    }
  }
  return false;
}

void Fragment::adjustCoordinates(double mult)
{
  if (mult != 1.0) {
    int nAtoms = numAtoms();
    TAtm *atom = NULL;
    for (int idx=0; idx<nAtoms; idx++) {
      atom = atomRef(idx);
      const double *coords = atom->coordinates();
      atom->coordinates(coords[0]*mult, coords[1]*mult, coords[2]*mult);
    }
    

  }
}


/**
 * See  http://hincklab.uthscsa.edu/~ahinck/html/soft_packs/msi_docs/insight980/insight/a_FF-CLASSIC.fcs.html
 */
bool Fragment::restoreCAR(istream& infile, double mult, bool genBonds)
{
  bool ret = true;
  bool pbc = false;
  string sg;
  infile.seekg(0, ios::beg);
  infile.clear();
  if (!infile.good() || infile.eof()) return false;

  // First line: !BIOSYM archive 3
  const int MAXLINE = 255;
  char buf[MAXLINE+1];
  if (infile.getline(buf,MAXLINE) && strstr(buf,"!BIOSYM") >= 0) {
     string line;
     vector<string> tlist;

     // Second line: PBC=ON or PBC=OFF
     // Be careful here.  These may be dos files with returns to get in the
     // way of your string compares.
     infile.getline(buf,MAXLINE);
     string tmp = buf;
     if (tmp.find("pbc=on") == 0 || tmp.find("PBC=ON") == 0) pbc = true;

     //Third line: 1-64 Title for the system.
     infile.getline(buf,MAXLINE);
     name(buf);

     //Fourth line: !date ##/##/## time: ##:##:## (if any; otherwise blank)
     infile.getline(buf,MAXLINE);

     //Fifth line: (see manual)
     //PBC information IF present:
     //1-3 `PBC'
     //4-13 a in angstroms
     //14-23 b in angstroms
     //24-33 c in angstroms
     //34-43 alpha in degrees
     //44-53 beta in degrees
     //54-63 gamma in degrees
     //64-80 space group
     infile.getline(buf,MAXLINE);
     if (pbc) {
        double a,b,c,alpha,beta,gamma;
        StringTokenizer tokenizer(buf);
        tlist = tokenizer.tokenize(" \t");
        sscanf(tlist[1].c_str(),"%lf",&a);
        sscanf(tlist[2].c_str(),"%lf",&b);
        sscanf(tlist[3].c_str(),"%lf",&c);
        sscanf(tlist[4].c_str(),"%lf",&alpha);
        sscanf(tlist[5].c_str(),"%lf",&beta);
        sscanf(tlist[6].c_str(),"%lf",&gamma);
        if (tlist.size() > 7) {
           string sgstr;
           for (int j=7; j<tlist.size(); j++) {
              sgstr += tlist[j];
           }

           // The format of the space group may vary but from the samples
           // I've seen, they look like (FD-3M)
           for (int idx=0; idx<sgstr.size(); idx++) {
              if (sgstr[idx] == '(') continue;
              if (sgstr[idx] == ')') break;
              if (sgstr[idx] == ' ') continue;
              if (sg.size() == 0) {
                 // assume first char is good to add
                 sg += sgstr[idx];
              } else {
                 if (isalpha(sgstr[idx])) {
                    sg += tolower(sgstr[idx]);
                 } else {
                    sg += sgstr[idx];
                 }
              }
           }
        }
        //cout << "lattice: " << *lattice << endl;
        LatticeDef  *lattice = new LatticeDef(a,b,c,alpha,beta,gamma);
        /*
        vector<MPoint> * vecs = lattice->toVectors();
        cout << "vecs " << vecs->size();
        for (int x=0; x<vecs->size(); x++) {
           (*vecs)[x].print("xx ");
        }
        delete vecs;
        */
        setLattice(lattice);
     }

     // end denotes the end of a molecule.  wow do we have to care?
     // hopefully our bond generator will take care of it.
     TAtm *atom; 
     string sym;
     double xyz[3];
     double x,y,z;
     while (infile.getline(buf,MAXLINE)) {
        if (strncmp(buf,"end",3) == 0) continue;
        //cout << "car atom " << buf << "!" << endl;
        line = buf;
        StringTokenizer tokenizer(buf);
        tlist = tokenizer.tokenize(" \t");
        sym = tlist[7];
        sscanf(tlist[1].c_str(),"%lf",&x);
        sscanf(tlist[2].c_str(),"%lf",&y);
        sscanf(tlist[3].c_str(),"%lf",&z);
        xyz[0] = x; xyz[1] = y; xyz[2] = z;
        atom = new TAtm(sym.c_str(),xyz);
        atom->atomName(tlist[0].c_str());
        addAtom(atom);
     }
     if (mult != 1.0) adjustCoordinates(mult);
     if (sg != "") {
        // Generate atoms...
        //cout << "generate space groups: " << sg << endl;
        // rearrange lattice vectors and atoms so that they
        // match NWChem conventions

        vector<MPoint> * vecs = getLattice()->toVectors();
//        cout<<"oA[1:]: "<<(*vecs)[0].x()<<" "<<(*vecs)[0].y()<<" "<<(*vecs)[0].z()<<endl;
//        cout<<"oA[2:]: "<<(*vecs)[1].x()<<" "<<(*vecs)[1].y()<<" "<<(*vecs)[1].z()<<endl;
//        cout<<"oA[3:]: "<<(*vecs)[2].x()<<" "<<(*vecs)[2].y()<<" "<<(*vecs)[2].z()<<endl;
        // exchange first two rows

        // according to Kurt Glaeseman and what is done by NWChem for periodic
        // system symmetry, these steps are no longer needed
/*
        sav_val = (*vecs)[0].x();
        (*vecs)[0].x((*vecs)[1].x());
        (*vecs)[1].x(sav_val);

        sav_val = (*vecs)[0].y();
        (*vecs)[0].y((*vecs)[1].y());
        (*vecs)[1].y(sav_val);

        sav_val = (*vecs)[0].z();
        (*vecs)[0].z((*vecs)[1].z());
        (*vecs)[1].z(sav_val);
*/

        // exchange first two columns
/*
        for (i=0; i<3; i++) {
          sav_val = (*vecs)[i].x();
          (*vecs)[i].x((*vecs)[i].y());
          (*vecs)[i].y(sav_val);
        }
*/
//        cout<<"iA[1:]: "<<(*vecs)[0].x()<<" "<<(*vecs)[0].y()<<" "<<(*vecs)[0].z()<<endl;
//        cout<<"iA[2:]: "<<(*vecs)[1].x()<<" "<<(*vecs)[1].y()<<" "<<(*vecs)[1].z()<<endl;
//        cout<<"iA[3:]: "<<(*vecs)[2].x()<<" "<<(*vecs)[2].y()<<" "<<(*vecs)[2].z()<<endl;

        // transpose lattice vectors
        vector<MPoint> new_vecs(3);
        new_vecs[0].x((*vecs)[0].x()); 
        new_vecs[1].x((*vecs)[0].y()); 
        new_vecs[2].x((*vecs)[0].z()); 
        new_vecs[0].y((*vecs)[1].x()); 
        new_vecs[1].y((*vecs)[1].y()); 
        new_vecs[2].y((*vecs)[1].z()); 
        new_vecs[0].z((*vecs)[2].x()); 
        new_vecs[1].z((*vecs)[2].y()); 
        new_vecs[2].z((*vecs)[2].z()); 
        getLattice()->assign(new_vecs[0], new_vecs[1], new_vecs[2]);
//        cout<<"fA[1:]: "<<new_vecs[0].x()<<" "<<new_vecs[0].y()<<" "<<new_vecs[0].z()<<endl;
//        cout<<"fA[2:]: "<<new_vecs[1].x()<<" "<<new_vecs[1].y()<<" "<<new_vecs[1].z()<<endl;
//        cout<<"fA[3:]: "<<new_vecs[2].x()<<" "<<new_vecs[2].y()<<" "<<new_vecs[2].z()<<endl;
//        getLattice()->assign((*vecs)[0], (*vecs)[1], (*vecs)[2]);
        delete vecs;


        // switch x and y values of fractional coordinates
/*
        for (iat=0; iat<natoms; iat++) {
          atm = atomRef(iat);
          const double *coords = atm->coordinates();
          x = coords[0];
          y = coords[1];
          z = coords[2];
          atm->coordinates(y, x, z);
        }
*/

        try {
          //TODO good threshold
          pointGroup(sg);
          SymmetryOps::generateLatticeFragment(*this, 0.1);
        } catch (EcceException& ex) {
           cerr << "Warning; unable to process space group: " << sg << endl;
        }

     }
     if (genBonds) {
        addCovalentBonds(true);
        setEnvIndices();
     }

  } else {
     ret = false;  // not the right file type?
  }
  return ret;
}

/**
 * Read in atoms from a PDB file.
 * @param selectAlt - alternative location selection; " " means ??
 * @param chainID - chain to process; currently interpreted as a string
 *   of one character which specifies the chain. The value " " means
 *   all chains.
 */
bool Fragment::restorePDB(istream& infile, double mult, bool genBonds,
    const int& selectMdl, const string& selectAlt, const bool& groupFlg,
    bool readBonds, const string& chainID)
{
  bool ret = true;
  TPerTab pertable;
  TResTab restable;
  char* cptr;
  char buf[1025];
  char record[7];
  char i5[6];
  char i4[5];
  char d8[9];
  char d6[7];
  char atnam[5];
  string altloc;
  char resnam[5];
  string chainid;
  char insertres[2];
  char segid[5];
  char atsym[3];
  double x,y,z;
  double occupancy,bfactor;
  int ires,imdl;
  int i,j,ichk,ilen,pcnt;
  int rescnt=0;
  int iresold = 0;
  char insresold = ' ';
  char resnamold[5] = "   ";
  char chainold = ' ';
  bool useChain = false;

  record[6] = '\0';
  i5[5] = '\0';
  i4[4] = '\0';
  d8[8] = '\0';
  d6[6] = '\0';
  atnam[4] = '\0';
  resnam[4] = '\0';
  insertres[1] = '\0';
  segid[4] = '\0';
  atsym[2] = '\0';

  infile.seekg(0, ios::beg);
  infile.clear();
  if (!infile.good() || infile.eof()) return false;
 
  if (chainID != " ") {
    useChain = true;
  }
  p_chains.clearChainID();
  imdl = 0;
  while (infile.getline(buf,1025)) {
    cptr = buf;
    strncpy(record,cptr,6);
    cptr += 6;

// Check to see if first six characters correspond to any of the records
// currently being supported (ATOM, HETATM)

    if (strncmp(record,"ATOM  ",6) == 0 ||
        strncmp(record,"HETATM",6) == 0) {

      if (imdl == 0 && (strncmp(record,"ATOM  ",6) == 0 ||
          strncmp(record,"HETATM",6) == 0)) {
        imdl++;
      }
      if (imdl == selectMdl && (strncmp(record,"ATOM  ",6) == 0 ||
          strncmp(record,"HETATM",6) == 0)) {
   

//  Copy remaining fields into variables and string buffers

        strncpy(i5,cptr,5);
        cptr += 6;
        strncpy(atnam,cptr,4);
        cptr += 4;

//  Replace any single quotes that may be present in atom
//  name with asterisks
        pcnt = 0;
        for (j=0; j<4; j++) {
          if (atnam[j] == '\'') {
            pcnt++;
            atnam[j] = '*';
          }
        }
//  If two single quotes are found, check to see if first
//  character is an H and then shift over one space.
        if (pcnt == 2 && atnam[0] == 'H') {
          for (j=2; j>=0; j--) {
            atnam[j+1] = atnam[j];
          }
          atnam[0] = '2';
        }

//        strncpy(altloc,cptr,1);
        altloc = cptr[0];
        cptr += 1;
        strncpy(resnam,cptr,4);
        cptr += 4;
        chainid = cptr[0];
        // Check to see if chainid matches chainID, assuming
        // that only one chain is being imported
        if (useChain && chainid != chainID)
          continue;
        cptr += 1;
        strncpy(i4,cptr,4);
        ires = atoi(i4);
        cptr += 4;
        strncpy(insertres,cptr,1);
        cptr += 4;
        strncpy(d8,cptr,8);
        x = atof(d8);
        cptr += 8;
        strncpy(d8,cptr,8);
        y = atof(d8);
        cptr += 8;
        strncpy(d8,cptr,8);
        z = atof(d8);
        cptr += 8;

//  The occupancy, B-factor, segment identifier, element symbol
//  and charge fields are not used reliably. Will need to do
//  additional checking to make sure that parsing these fields does
//  not mess up the input.

        ilen = strlen(buf);
        if (ilen >= 60) {
          strncpy(d6,cptr,6);
          ichk = 0;
          for (i=0; i<6; i++) {
            if (isdigit(d6[i])) {ichk = 1;}
          }
          if (ichk == 1) {
            occupancy = atof(d6);
          } else {
            occupancy = 1.0;
          }
        } else {
          occupancy = 1.0;
        }
        cptr += 6;
        if (ilen >= 66) {
          strncpy(d6,cptr,6);
          ichk = 0;
          for (i=0; i<6; i++) {
            if (isdigit(d6[i])) {ichk = 1;}
          }
          if (ichk == 1) {
            bfactor = atof(d6);
          } else {
            bfactor = 0.0;
          }
        } else {
          bfactor = 0.0;
        }
        cptr += 12;
        if (ilen >= 76) {
          strncpy(segid,cptr,4);
        } else {
          for (i = 0; i < 4; i++) {
            segid[i] = ' ';
          }
        }
// For now, we will not bother parsing the atomic symbol (this is redundant
// if valid atom names are being used) and the charge (these are not
// particularly useful).
//        cptr += 4;
//        strncpy(atsym,cptr,2);
//        cptr += 2;
//        strncpy(ichrg,cptr,2);

// Add atom attributes to fragment. If alternate stuctures
// were dectected on initial scan, only add selected alternates
// to fragment.

        if (altloc == selectAlt || altloc == " ") {

// Determine atomic symbol from atom name. Check to see if atom symbol
// is only a single letter.

          if (isdigit(atnam[0]) != 0 || isspace(atnam[0]) != 0) {
            atsym[0] = toupper(atnam[1]);
            atsym[1] = '\0';
          } else {
            atsym[0] = toupper(atnam[0]);
            atsym[1] = tolower(atnam[1]);
            atsym[2] = '\0';
          }
          double xyz[3];
          xyz[0] = x;
          xyz[1] = y;
          xyz[2] = z;
          TAtm *atom = new TAtm(atsym,xyz);

          atom->atomName(atnam);

          atom->occupancy(occupancy);
          atom->bFactor(bfactor);
          addAtom(atom);

// Add residue to residue list. Only perform this operation if a new
// residue has been detected.

          Residue* res;
          if (strncmp(resnam,resnamold,4) != 0 ||
              chainold != chainid[0] || iresold != ires ||
              insresold != insertres[0]) {
            rescnt++;
            Residue *residue = new Residue(resnam,ires);
            res = residue;
            addResidue(residue);
            residue->name(resnam);
            residue->number(ires);
            residue->chain(chainid[0]);
            residue->insertCode(insertres[0]);
            residue->segID(segid);
            residue->firstAtomPtr(atom);
            p_chains.addChainID(chainid[0]);
             

// Reset current values of residue name, residue number,
// insertion code, and chain ID

            strcpy(resnamold,resnam);
            iresold = ires;
            chainold = chainid[0];
            insresold = insertres[0];
          }
          atom->atomResidue(res);
        }
      }
    } else if (strncmp(record,"MODEL",5) == 0) {
      imdl++;
    } else if (strncmp(record,"CONECT",6) == 0 && readBonds) {
      if (strlen(cptr) >= 12) {
        strncpy(d6,cptr,6);
        i = atoi(d6) - 1;
        cptr += 6;
        strncpy(d6,cptr,6);
        j = atoi(d6) - 1;
        if (bond(i,j) == (TBond*)0) addBond(i, j, 1.0);
      }
    }
  }

// Set bonds, etc.

  if (mult != 1.0) adjustCoordinates(mult);
  if (genBonds && !readBonds) addCovalentBonds(true);
  if (genBonds) setEnvIndices();

// PDB file has now been completely parsed, need to assign coupling
// info to residue list.

  Residue *res1, *res2, *res3;
  char cid1, cid2, cid3;

  int size = p_residueSequence.size();

// Evaluate special cases first. Start with the case of only one
// residue
  if (size == 1) {
    res1 = p_residueSequence[0];
    res1->couplingInfo(Residue::MONOMER);
  } else if (size > 1) {

// Evaluate first residue on the list

    res1 = p_residueSequence[0];
    res2 = p_residueSequence[1];
    cid1 = res1->chain();
    cid2 = res2->chain();
    if (restable.verify(res1->name().c_str()) != TResItem::Other) {
      if (cid1 != cid2) {
        res1->couplingInfo(Residue::MONOMER);
      } else {
        res1->couplingInfo(Residue::HEAD);
      }
    } else {
      res1->couplingInfo(Residue::MONOMER);
    }

// Evaluate remaining residues, except for the last one

    for (i = 1; i<size-1; i++) {

// Determine coupling on a case by case basis.

      res1 = p_residueSequence[i-1];
      res2 = p_residueSequence[i];
      res3 = p_residueSequence[i+1];
      cid1 = res1->chain();
      cid2 = res2->chain();
      cid3 = res3->chain();
      if (restable.verify(res2->name().c_str()) != TResItem::Other) {
        /**
         * Compare chain IDs to identify beginning and ends of chains
         */
        if (cid1 != cid2 && cid2 != cid3) {
          res2->couplingInfo(Residue::MONOMER);
        } else if (cid1 != cid2 && cid2 == cid3) {
          res2->couplingInfo(Residue::HEAD);
        } else if (cid1 == cid2 && cid2 != cid3) {
          res2->couplingInfo(Residue::TAIL);
        } else {
          if (restable.verify(res1->name().c_str()) != TResItem::Other &&
              restable.verify(res3->name().c_str()) != TResItem::Other) {
            /**
             * No chain ID available. Check atoms to determine whether
             * residue is at end of chain
             */
            int istart = res2->firstAtomPtr()->index();
            int iend = res3->firstAtomPtr()->index();
            int idx, jdx;
            bool isHead = true;
            bool isTail = true;
            for (idx = istart; idx < iend; idx++) {
              vector<TAtm*>* connected = p_atoms[idx]->connectedAtoms();
              int nconnect = connected->size();
              for (jdx=0; jdx<nconnect; jdx++) {
                if ((*connected)[jdx]->getResidue() == res1) {
                  isHead = false;
                } else if ((*connected)[jdx]->getResidue() == res3) {
                  isTail = false;
                }
              }
              delete connected;
            }
            if (isHead) {
              res2->couplingInfo(Residue::HEAD);
            } else if (isTail) {
              res2->couplingInfo(Residue::TAIL);
            } else {
              res2->couplingInfo(Residue::INTERIOR);
            }
          } else if (restable.verify(res1->name().c_str()) !=
                    TResItem::Other) {
            res2->couplingInfo(Residue::TAIL);
          } else if (restable.verify(res3->name().c_str()) !=
                    TResItem::Other) {
            res2->couplingInfo(Residue::HEAD);
          } else {
            res2->couplingInfo(Residue::MONOMER);
          }
        }
      } else {
        res2->couplingInfo(Residue::MONOMER);
      }
      // Do a final check to see if residue is attached to a non-standard
      // residue
      if (res2->couplingInfo() == Residue::TAIL) {
        int ibeg = res2->firstAtomPtr()->index();
        int iend = res3->firstAtomPtr()->index();
        for (j=ibeg; j<iend; j++) {
          if (p_atoms[j]->atomName() == " C  ") {
            vector<TAtm*> *connected = p_atoms[j]->connectedAtoms();
            int k;
            int ksize = connected->size();
            for (k=0; k<ksize; k++) {
              if ((*connected)[k]->getResidue() != res2) {
                res2->couplingInfo(Residue::INTERIOR);
              }
            }
            delete connected;
            break;
          }
        }
      } else if (res2->couplingInfo() == Residue::HEAD) {
        int ibeg = res2->firstAtomPtr()->index();
        int iend = res3->firstAtomPtr()->index()-1;
        for (j=ibeg; j<iend; j++) {
          if (p_atoms[j]->atomName() == " N  ") {
            vector<TAtm*> *connected = p_atoms[j]->connectedAtoms();
            int k;
            int ksize = connected->size();
            for (k=0; k<ksize; k++) {
              if ((*connected)[k]->getResidue() != res2) {
                res2->couplingInfo(Residue::INTERIOR);
              }
            }
            delete connected;
            break;
          }
        }
      }
    }

// Evaluate last residue on the list

    res2 = p_residueSequence[size-2];
    res3 = p_residueSequence[size-1];
    cid2 = res2->chain();
    cid3 = res3->chain();
    if (restable.verify(res3->name().c_str()) != TResItem::Other) {
      if (cid2 != cid3) {
        res3->couplingInfo(Residue::MONOMER);
      } else {
        res3->couplingInfo(Residue::TAIL);
      }
    } else {
      res3->couplingInfo(Residue::MONOMER);
    }
  }

  findFirstSolventAtomIndex();
  return ret;
}

//////////////////////////////////////////////////////////////////////////
// Description
//  There are lots of definitions of what xyz is.  This parser expects
//  a well defined syntax:
//
// line1: numAtoms
// line2: title
// line3: atom1 x y z
// ...
// linen: atomn+2 x y z
//
//   where:
//    . numAtoms > 0
//    . atom? can be an atomic symbol or number.  Anything after a
//      valid symbol starting with "_" is put into a member named
//      suffixHack which is passed on to the NWChem input file geometry.
//      Mixed case is supported.
//    . atomic symbols and numbers are range checked
//    . numAtoms should exactly match the actual number of atoms provided.
//      extra atoms are ignored.  Too few is a fatal error.
//    . missing coordinates (i.e. no z value provided) are not handled
//      correctly at this time.  A tokenizing strategy is required for
//      this to be fixed.
//    . extra fields at the end of a line are safely ignored.
//
// For large systems get() is sooo much faster than string.getLine().
//
// Avoid use of string for performance reasons.
//////////////////////////////////////////////////////////////////////////
bool Fragment::restoreXYZ(istream& infile, double mult, bool genBonds)
{
  int tmp = 0;
  static const int BUFSIZE = 240;
  TPerTab tpt;
  p_name = "XYZ import ";
  bool error = false;
  char lbuf[BUFSIZE];  // line buf - none should be longer
  int numAtoms = 0;

  p_name = "";

  if (!infile.good() || infile.eof()) return false;

  infile.getline(lbuf,BUFSIZE-1,'\n');
  tmp++;
  sscanf(lbuf,"%d",&numAtoms);

  if (numAtoms <= 0 || !infile.good() || infile.eof()) return false;

  infile.getline(lbuf,BUFSIZE-1,'\n');
  tmp++;

  if (!infile.good() || infile.eof()) return false;
  if (strlen(lbuf) != 0) p_name = lbuf;


  int curAtom=0;
  double coords[3];
  string tag;
  // resize just once
  int last, len, idx;
  char *symp;
  char *shouldbecoord;
//  p_atoms.resize(p_atoms.size() + numAtoms);
  while (curAtom < numAtoms && !error && infile.good()) {
    if (infile.getline(lbuf,BUFSIZE-1)) {
      tmp++;
      symp = strtok(lbuf," ");
      shouldbecoord = strtok(0," ");
      if (shouldbecoord == NULL) {
        error = true;
        break;
      }
      coords[0] = atof(shouldbecoord);

      shouldbecoord = strtok(0," ");
      if (shouldbecoord == NULL) {
        error = true;
        break;
      }
      coords[1]=atof(shouldbecoord);
      shouldbecoord = strtok(0," ");
      if (shouldbecoord == NULL) {
        error = true;
        break;
      }
      coords[2]=atof(shouldbecoord);

      if (isdigit(symp[0])) {
        tag = tpt.atomicSymbol(atoi(symp));
        strcpy(symp,tag.c_str());
      }

      // Some files have these goofey signs at the end.  This doesn't take
      // too long to check but maybe we consider it an error for speed
      // later.
      last = strlen(symp) - 1;
      if (symp[last] == '+' || symp[last] == '-') symp[last] = '\0';

      // stuff anything beginning with _ under the suffixHack member and
      // then delete it from the end of the symbol
      char* under = strchr(symp, '_');
      string suffixHack = "";
      if (under != NULL) {
        suffixHack = under;
        *under = '\0';
      }

      len = strlen(symp);
      if (len == 1) {
        symp[0] = toupper(symp[0]);
      } else if (len > 1) {
        symp[0] = toupper(symp[0]);
        for (idx=1; idx<len; idx++) symp[idx] = tolower(symp[idx]);
      }

      if (!tpt.isValid(symp)) {
        // Handle the special case of Bq atoms by converting them to a
        // ghost atom
        if (strlen(symp) > 1 && symp[0] == 'B' && symp[1] == 'q') {
          symp[0] = 'X';
          symp[1] = '\0';
        } else {
          error = true; break;
        }
      }
      curAtom++;
      addAtom(symp,coords,suffixHack);
    } else {
      error = true;
    }
  }
#ifdef DEBUG
  //cout << "restoreXYZ read " << tmp << endl;
#endif
//t.stop();
//cout << "restore xyz " << t.elapsedTime() << endl;
  if (mult != 1.0) adjustCoordinates(mult);
  if (genBonds) {
    addCovalentBonds(true);
    setEnvIndices();
  }


  return ((curAtom+1!=numAtoms) && !error );
}

//////////////////////////////////////////////////////////////////////////
//  Routine for generating a PDB file
//////////////////////////////////////////////////////////////////////////
bool Fragment::dumpPDB(ostream& outfile, bool writeBonds)
{
  bool ret = false;
  char buf[81];
  char* cptr;
  char atsym[3];
  char segid[5];
  int icnt = 0;
  const double *coords;
  //bool set_residues = false;
  double rnum;
  TAtm *atm = 0;
  Residue* res = 0;
  TResTab restab;
  string rstring;

  atsym[2] = '\0';
  segid[4] = '\0';

  if (numResidues() == 0) {
    FragUtil fragutil(this);
    fragutil.createResidues();
    //set_residues = true;
  }
  if (outfile) {
    ret = true;
    int i, rcnt = numResidues();
    for (i = 0; i < rcnt; i++) {
      res = p_residueSequence[i];
      vector<TAtm*> atomList = this->findResidueAtoms(res);
      for (int j = 0; j < atomList.size(); j++) {
        atm = atomList[j];
        if (atm->atomicSymbol() != "Nub") {
          icnt++;
          cptr = buf;

          if (restab.verify(res->name().c_str()) != TResItem::Other) {
            strncpy(cptr,"ATOM  ",6);
          } else {
            strncpy(cptr,"HETATM",6);
          }
          cptr += 6;

          sprintf(cptr,"%5d",icnt);
          cptr += 5;

          strncpy(cptr," ",1);
          cptr++;

// Print out atom name. If no name available, use atomic
// symbol.

          rstring = atm->atomName();
          if (rstring.length() == 4) {
            strncpy(cptr,rstring.c_str(),4);
            cptr += 4;
          } else {
            rstring = atm->atomicSymbol();
            if (rstring.length() ==1) {
              strncpy(cptr," ",1);
              cptr++;
              strncpy(cptr,rstring.c_str(),1);
              cptr++;
            } else {
              strncpy(atsym,rstring.c_str(),2);
              atsym[1] = toupper(atsym[1]);
              strncpy(cptr,atsym,2);
              cptr += 2;
            }
            strncpy(cptr,"  ",2);
            cptr += 2;
          }

          strncpy(cptr," ",1);
          cptr++;

// Print out residue name, chain ID, number, and insertion code

          rstring = res->name();
          if (rstring.length() == 4) {
            if (rstring != "    ") {
              strncpy(cptr,rstring.c_str(),4);
            } else {
              strncpy(cptr,"UNK ",4);
            }
          } else {
            strncpy(cptr,"UNK ",4);
          }
          cptr += 4;

          if (res->chain() != '\0') { 
            *cptr = res->chain();
          } else {
            *cptr = ' ';
          }
          cptr++;

          sprintf(cptr,"%4d",res->number());
          cptr += 4;

          if (res->insertCode() != '\0') { 
            *cptr = res->insertCode();
          } else {
            *cptr = ' ';
          }
          cptr++;

          strncpy(cptr,"   ",3);
          cptr += 3;

          coords = atm->coordinates();
          sprintf(cptr,"%8.3lf",coords[0]);
          cptr += 8;
          sprintf(cptr,"%8.3lf",coords[1]);
          cptr += 8;
          sprintf(cptr,"%8.3lf",coords[2]);
          cptr += 8;

          rnum = atm->occupancy();
          if (rnum == 0.0) {
            rnum = 1.0;
          }
          sprintf(cptr,"%6.2f",rnum);
          cptr += 6;
      
          rnum = atm->bFactor();
          sprintf(cptr,"%6.2f",rnum);
          cptr += 6;

          strncpy(cptr,"      ",6);
          cptr += 6;

// Check to make sure if segment label is left justified. Remove
// white space at the front of the label if it is not.

          rstring = atm->segment();
          if (rstring.length() == 4) {
            if (rstring != "    ") {
              strncpy(segid,rstring.c_str(),4);
              while (segid[0] == ' ') {
                for (int ic = 0; ic < 3; ic++) {
                  segid[ic] = segid[ic+1];
                }
                segid[3] = ' ';
              }
              strncpy(cptr,segid,4);
            } else {
              strncpy(cptr,rstring.c_str(),4);
            }
          } else {
            strncpy(cptr,"    ",4);
          }
          cptr += 4;

          rstring = atm->atomicSymbol();
          if (rstring.length() == 1) {
            strncpy(cptr," ",1);
            cptr++;
            strncpy(cptr,rstring.c_str(),1);
            cptr++;
          } else {
            strncpy(atsym,rstring.c_str(),2);
            atsym[1] = toupper(atsym[1]);
            strncpy(cptr,atsym,2);
            cptr += 2;
          }

          strncpy(cptr,"  ",2);
          cptr += 2;
          *cptr = '\0';
  
          outfile << buf << endl;
        }
      }
    }
    if (writeBonds) {
      vector<TBond*>* bondlist = bonds();
      int size = bondlist->size();
      int idx, i, j;
      TBond *bond;
      for (idx = 0; idx < size; idx++) {
        cptr = buf;
        strncpy(cptr,"CONECT",6);
        cptr += 6;
        bond = (*bondlist)[idx]; 
        i = bond->atom1()->index();
        j = bond->atom2()->index();
        i++;
        j++;
        sprintf(cptr,"%6d",i);
        cptr += 6;
        sprintf(cptr,"%6d",j);
        outfile << buf << endl;
      }
      delete bondlist;
    }
    outfile.flush();
  }
//  if (set_residues) {
//    clearResidueInfo();
//  }
  return ret;
}


/**
 * Dump in mvm format.
 * Currently dumps lattice in vector format and geometry in cartesian only.
 */
bool Fragment::dumpMVM(ostream& outfile, bool dumpAll)
{
  bool ret = false;

  if (outfile) {
    ret = true;
    int idx,cnt;
    TAtm *atm=0;
    const double *coords;
    bool writePDB = false;
    bool writeAttr = false;
    bool writeSuffix = false;
    bool writeType = false;
    bool writeMass = false;
    bool writeBehavior = false;

    TChartOfNuclides ichart;
    const TRefIsotope *isotope;

    cnt = numAtoms();
    outfile.setf(ios::showpoint);
    outfile << "title: " << p_name << "\n";
    outfile << "type: molecule\n";

// Check to see if fragment attribute list should be printed.

    bool writeFragAttr = false;
    if (p_pointGroup != "C1") writeFragAttr = true;
    if (p_charge != 0.0) writeFragAttr = true;
    if (p_firstSolventAtomIndex != -1) writeFragAttr = true;
    if (p_DoF != 0) writeFragAttr = true;
    if (writeFragAttr) {
      outfile << "fragment_attributes:\n";
      if (p_pointGroup != "C1") outfile << "  point_group: "
        << p_pointGroup << "\n";
      if (p_charge != 0.0) outfile << "  charge: "
        << (int)p_charge << "\n";
      if (p_firstSolventAtomIndex != -1)
        outfile << " first_solvent_atom_index: "
        << p_firstSolventAtomIndex << "\n";
      if (p_solventNumAtoms != 0)
        outfile << " solvent_num_atoms: "
        << p_solventNumAtoms << "\n";
      if (p_DoF != 0)
        outfile << " fragment_dof: " << p_DoF << "\n";
      outfile << "end_fragment_attributes:\n";
    }

// Print out the lattice info if it exists
    if (p_lattice) {
       outfile << "lattice_vectors: \n"; 
       char lbuf[80];
       vector<MPoint> *vecs = p_lattice->toVectors();
       for (int idx=0; idx<3; idx++) {
          MPoint p = (*vecs)[idx];
          sprintf(lbuf, "%10.6lf %10.6lf %10.6lf\n",p.x(), p.y(), p.z());
          outfile << lbuf;
       }
       delete vecs;
    }
    outfile << "num_atoms: " << cnt << "\n";
    outfile << "atom_info: symbol cart\n";

    outfile << "atom_list: \n"; 

// Print out atom list

    for (idx=0; idx<cnt; idx++) {
      atm = p_atoms[idx];
      coords = atm->coordinates();
      outfile << atm->atomicSymbol() << " ";
      outfile << coords[0] << " " << coords[1] << " " << coords[2] << "\n";
    }

// Print out PDB list

    char* cptr;
    string astring;
    char buf[120];
    int ires;
    char a1[2];
    char segid[5];
    a1[1] = '\0';
    segid[4] = '\0';

    if (p_residueSequence.size() > 0) writePDB = true;
    if (writePDB) {
      outfile << "pdb_list: \n"; 
      for (idx=0; idx<cnt; idx++) {
        cptr = buf;
        atm = p_atoms[idx];
        Residue::Coupling coupling = atm->getResidue()->couplingInfo();
        if (coupling == Residue::MONOMER) {
          cptr[0] = 'M';
        } else if (coupling == Residue::TAIL) {
          cptr[0] = 'T';
        } else if (coupling == Residue::HEAD) {
          cptr[0] = 'H';
        } else if (coupling == Residue::INTERIOR) {
          cptr[0] = 'I';
        }
        cptr++;
        strncpy(cptr," ",1);
        cptr++;
        astring = atm->atomName();
        if (astring.length()==4) {
          strncpy(cptr,astring.c_str(),4);
        } else {
          strncpy(cptr,"    ",4);
        }
        cptr += 4;
        strncpy(cptr," ",1);
        cptr++;
        astring = atm->residueName();
        if (astring.length()==4) {
          strncpy(cptr,astring.c_str(),4);
        } else {
          strncpy(cptr,"    ",4);
        }
        cptr += 4;
        ires = atm->residueNumber();
        sprintf(cptr,"%6d",ires);
        cptr += 6;
        strncpy(cptr," ",1);
        cptr++;
        a1[0] = atm->icode();
        strncpy(cptr,a1,1);
        cptr++;
        strncpy(cptr," ",1);
        cptr++;

// Check to make sure if segment label is left justified. Remove
// white space at the front of the label if it is not.

        astring = atm->segment();
        if (astring.length() == 4) {
          if (astring != "    ") {
            strncpy(segid,astring.c_str(),4);
            while (segid[0] == ' ') {
              for (int ic = 0; ic < 3; ic++) {
                segid[ic] = segid[ic+1];
              }
              segid[3] = ' ';
            }
            strncpy(cptr,segid,4);
          } else {
            strncpy(cptr,astring.c_str(),4);
          }
        } else {
          strncpy(cptr,"    ",4);
        }

        cptr += 4;
        strncpy(cptr," ",1);
        cptr++;

        a1[0] = atm->chain();
        strncpy(cptr,a1,1);
        cptr++;
        *cptr = '\0';
        outfile << buf << endl;
      }

    }

    int igrp;
    double rnum;

// Check to see if either attribute or type information should be
// written out.

    for (idx=0; idx<cnt; idx++) {
      atm = p_atoms[idx];
      rnum = atm->partialCharge();
      if (rnum != 0.0) writeAttr = true;
      igrp = (int)atm->chargeGroup();
      if (igrp != 0) writeAttr = true;
      igrp = (int)atm->polarizationGroup();
      if (igrp != 0) writeAttr = true;
      igrp = (int)atm->connectionIndex();
      if (igrp != 0) writeAttr = true;
      igrp = (int)atm->dihedralIndex();
      if (igrp != 0) writeAttr = true;
      isotope = ichart.mostAbundantIsotope(atm->atomicNumber());
      if (isotope != (TRefIsotope*)0) {
        rnum = (double)isotope->isotopeWeight();
      } else {
        rnum = 0.0;
      }
      if (rnum != atm->getMass()) writeMass = true;

      if (TAtm::UNSPECIFIED != atm->getBehaviorType()) writeBehavior = true;

      if (dumpAll) {
        writeType = true;
      } else {
        astring = atm->atomType();
        int i;
        for (i=0; i<astring.length(); i++) {
          if (astring[i] != ' ') writeType = true;
        }
      }
    }

// Print out attribute list

    if (writeAttr) {
      outfile << "attr_list:" << endl;
      for (idx=0; idx<cnt; idx++) {
        atm = p_atoms[idx];
        cptr = buf;
        rnum = atm->partialCharge();
        sprintf(cptr,"%9.6f",rnum);
        cptr += 9;
        igrp = (int)atm->chargeGroup();
        sprintf(cptr,"%4d",igrp);
        cptr += 4;
        igrp = (int)atm->polarizationGroup();
        sprintf(cptr,"%4d",igrp);
        cptr += 4;
        igrp = (int)atm->connectionIndex();
        sprintf(cptr,"%4d",igrp);
        cptr += 4;
        igrp = (int)atm->dihedralIndex();
        sprintf(cptr,"%4d",igrp);
        cptr += 4;
        *cptr = '\0';
        outfile << buf << endl;
      }
    }

// Print out atom type list

    if (writeType) {
      outfile << "atom_type_list:" << endl;
      for (idx=0; idx<cnt; idx++) {
        atm = p_atoms[idx];
        cptr = buf;
        astring = atm->atomType();
        int i;
        if (astring.length() <= 5) {
          for (i=0; i<astring.length(); i++) {
            *cptr = astring[i];
            cptr++;
          }
          for (i=astring.length(); i<5; i++) {
            *cptr = ' ';
            cptr++;
          }
        } else {
          strncpy(cptr,astring.c_str(),5);
          cptr += 5;
        }
        if (dumpAll) {
          strncpy(cptr," ",1);
          cptr++;
          igrp = (int)atm->shape();
          sprintf(cptr,"%3d",igrp);
          cptr += 3;
          strncpy(cptr," ",1);
          cptr++;
          igrp = (int)atm->displayStyle().getStyle();
          sprintf(cptr,"%3d",igrp);
          cptr += 3;
        }
        *cptr = '\0';
        outfile << buf << endl;
      }
    }

// Check to see if suffixes should be written
    for (idx=0; idx<cnt; idx++) {
      atm = p_atoms[idx];
      if (atm->suffixHack() != "") {
        writeSuffix = true;
        break;
      }
    }
    if (writeSuffix) {
      outfile << "suffix_list:\n"; 

// Print out suffix list

      for (idx=0; idx<cnt; idx++) {
        atm = p_atoms[idx];
        coords = atm->coordinates();
        outfile << atm->suffixHack() << endl;
      }
    }

// Print out atomic mass list

    if (writeMass) {
      outfile << "atom_mass_list:\n";
      for (idx = 0; idx<cnt; idx++) {
        atm = p_atoms[idx];
        outfile << atm->getMass() << endl;
      }
    }

// Print out atomic behavior list

    if (writeBehavior) {
      outfile << "atom_behavior_list:\n";
      for (idx = 0; idx<cnt; idx++) {
        atm = p_atoms[idx];
        if (atm->getBehaviorType() == TAtm::QUANTUM) {
          outfile << "quantum" << endl;
        } else if (atm->getBehaviorType() == TAtm::POINT) {
          outfile << "point" << endl;
        } else if (atm->getBehaviorType() == TAtm::BQ) {
          outfile << "bq" << endl;
        } else if (atm->getBehaviorType() == TAtm::BQX) {
          outfile << "bqx" << endl;
        } else {
          outfile << "unspecified" << endl;
        }
      }
    }

// Print out bond list

    vector<TBond*>* const mybonds = bonds();
    cnt = mybonds->size();
    outfile << "num_bonds: " << cnt << "\n";
    if (cnt > 0) {
      TBond *bond;
      outfile << "bond_list: \n"; 
      for (idx=0; idx<cnt; idx++) {
        bond = (*mybonds)[idx];
        outfile << atomIndex(*bond->atom1())+1 << " ";
        if (dumpAll) {
          outfile << atomIndex(*bond->atom2())+1 << " ";
          outfile << bond->order() << " ";
          outfile << (int)(bond->displayStyle().getStyle()) << "\n";
        } else {
          outfile << atomIndex(*bond->atom2())+1 << " ";
          outfile << bond->order() << "\n";
        }
      }
    }

    // failure to flush pending output results in file getting truncated
    // (missing bond information)
    outfile.flush();

    delete mybonds;
  }

  return ret;
}


/**
 * Dump fragment as a car formatted file.
 * See  http://hincklab.uthscsa.edu/~ahinck/html/soft_packs/msi_docs/insight980/insight/a_FF-CLASSIC.fcs.html
 * @param outfile the output stream where data will be written
 * @return true if succesful, false if a stream error occurs.
 */
bool Fragment::dumpCAR(ostream& outfile)
{
  bool ret = false;
  if (outfile) {
    ret = true;
    int idx;
    TAtm *atm=0;
    char buf[82]; // need extra for newline and endstring 
    const double *coords;

    // Prescan list to eliminate nubs from total count
    // because we don't export them.
    int atot = 0;
    int cnt=0;
    int numSortedAtoms = p_atomSortOrder.size();

    if (numSortedAtoms > 0) {
       atot = numSortedAtoms;
       for (idx=0; idx<atot; idx++) {
          atm = p_atoms[p_atomSortOrder[idx]];
          if (atm->atomicSymbol() != "Nub" && 
              atm->displayStyle().isDisplayed()) {
             cnt++;
          }
       }
    } else {
       atot = p_atoms.size();
       for (idx=0; idx<atot; idx++) {
          atm = p_atoms[idx];
          if (atm->atomicSymbol() != "Nub" && 
              atm->displayStyle().isDisplayed()) {
             cnt++;
          }
       }
    }

    // bail if there are no vectors (i.e. a regular/non-periodic system)
    // without this we seg fault and that's not a good thing
    if (getLattice() == (LatticeDef*)0)
      return false;

    vector<MPoint> *old_vecs = getLattice()->toVectors();
    // transpose lattice vectors
    vector<MPoint> vecs(3);
    vecs[0].x((*old_vecs)[0].x());
    vecs[1].x((*old_vecs)[0].y());
    vecs[2].x((*old_vecs)[0].z());
    vecs[0].y((*old_vecs)[1].x());
    vecs[1].y((*old_vecs)[1].y());
    vecs[2].y((*old_vecs)[1].z());
    vecs[0].z((*old_vecs)[2].x());
    vecs[1].z((*old_vecs)[2].y());
    vecs[2].z((*old_vecs)[2].z());
    delete old_vecs; 

    //exchange first two rows
    double sav_val;
    sav_val = vecs[0].x();
    vecs[0].x(vecs[1].x());
    vecs[1].x(sav_val);
    sav_val = vecs[0].y();
    vecs[0].y(vecs[1].y());
    vecs[1].y(sav_val);
    sav_val = vecs[0].z();
    vecs[0].z(vecs[1].z());
    vecs[1].z(sav_val);

    //exchange first two columns
    int i;
    for (i=0; i<3; i++) {
       sav_val = vecs[i].x();
       vecs[i].x(vecs[i].y());
       vecs[i].y(sav_val);
    }

    //switch x and y values of fractional coordinates
    int iat, natoms;
    double x, y, z;
    natoms = numAtoms();
//    convertToFractionalCoords();
    for (iat = 0; iat<natoms; iat++) {
      atm = atomRef(iat);
      const double *coords = atm->coordinates();
      x = coords[0];
      y = coords[1];
      z = coords[2];
      atm->coordinates(y, x, z);
    }
    getLattice()->assign(vecs[0],vecs[1],vecs[2]);
//    convertFromFractionalCoords();

    // 1st line !BIOSYM archive 3
    outfile << "!BIOSYM archive 3\n";

    // 2nd line is PBC on or off
    outfile << "PBC=";
    if (p_lattice) {
       outfile << "ON\n";
    } else {
       outfile << "OFF\n";
    }

    // 3rd line is title - we don't really have one
    outfile << "ECCE generated CAR File \n";

    // 4th line - !date ##/##/## time: ##:##:## (if any; otherwise blank)
    // For now, don't bother.
    outfile << "DATE " << TDateTime().toString() << "\n";

    //Fifth line:
    //PBC information IF present:
    //1-3 `PBC'
    //4-13 a in angstroms
    //14-23 b in angstroms
    //24-33 c in angstroms
    //34-43 alpha in degrees
    //44-53 beta in degrees
    //54-63 gamma in degrees
    //64-80 space group
    if (p_lattice) {
       double a,b,c,alpha,beta,gamma;
       p_lattice->getLattice(a,b,c,alpha,beta,gamma);
       sprintf(buf,"PBC%12.6lf%12.6lf%12.6lf%12.6lf%12.6lf%12.6lf",
             a,b,c,alpha,beta,gamma);
       outfile << buf << endl;
    } else {
       outfile << "\n";
    }
    

    // Then the atoms
    // Sixth line - Nth line:
    // 1-4 Atom name.
    // 6-20 x Cartesian coordinate for the atom (angstrom).
    // 21-35 y Cartesian coordinate for the atom (angstrom).
    // 36-50 z Cartesian coordinate for the atom (angstrom).
    // 52-55 Name of residue containing atom.
    // 56-60 Residue sequence number relative to the
    // beginningof the current molecule.
    // 62-65 Potential function atom type (left justified)
    // (ignored; see Molecular Data File).
    // 71-72 Element type.
    // 74-79 Partial charge on the atom.
    for (idx=0; idx<atot; idx++) {
       if (numSortedAtoms > 0) {
          atm = p_atoms[p_atomSortOrder[idx]];
       } else {
          atm = p_atoms[idx];
       }
       if (atm->atomicSymbol() != "Nub" && 
           atm->displayStyle().isDisplayed()) {
          coords = atm->coordinates();
          if (atm->atomicSymbol().length() == 1) {
             outfile << atm->atomicSymbol() << atm->suffixHack() << "    ";
          } else {
             outfile << atm->atomicSymbol() << atm->suffixHack() << "   ";
          }
          sprintf(buf,"%15.9lf%15.9lf%15.9lf RESN %-4d %4s%5s%3s%6.3f",
                 coords[0],coords[1],coords[2],1,"xx"," ",
                 atm->atomicSymbol().c_str(),atm->partialCharge());
          outfile << buf << endl;
       }
    }

    // Final line for a given molecule:
    // 1-3 The word end.
    outfile << "end\n";

    outfile.flush();
  }
  return ret;
}

/**
 * Dump fragment as an xyz formatted file.
 * @param outfile the output stream where data will be written
 * @return true if succesful, false if a stream error occurs.
 */
bool Fragment::dumpXYZ(ostream& outfile)
{
  bool ret = false;
  if (outfile) {
    ret = true;
    int idx;
    TAtm *atm=0;
    char buf[80];
    const double *coords;

    // Prescan list to eliminate nubs from total count
    // because we don't export them.
    int atot = 0;
    int cnt=0;
    int numSortedAtoms = p_atomSortOrder.size();

    if (numSortedAtoms > 0) {
       atot = numSortedAtoms;
       for (idx=0; idx<atot; idx++) {
          atm = p_atoms[p_atomSortOrder[idx]];
          if (atm->atomicSymbol() != "Nub" && 
              atm->displayStyle().isDisplayed()) {
             cnt++;
          }
       }
    } else {
       atot = p_atoms.size();
       for (idx=0; idx<atot; idx++) {
          atm = p_atoms[idx];
          if (atm->atomicSymbol() != "Nub" && 
              atm->displayStyle().isDisplayed()) {
             cnt++;
          }
       }
    }

    // 1st line is number of atoms
    outfile << cnt << "\n";

    // 2nd line is title
    outfile << p_name << "\n";

    // Then the atoms
    for (idx=0; idx<atot; idx++) {
       if (numSortedAtoms > 0) {
          atm = p_atoms[p_atomSortOrder[idx]];
       } else {
          atm = p_atoms[idx];
       }
       if (atm->atomicSymbol() != "Nub" && 
           atm->displayStyle().isDisplayed()) {
          coords = atm->coordinates();
          if (atm->atomicSymbol().length() == 1) {
             outfile << atm->atomicSymbol() << atm->suffixHack() << "     ";
          } else {
             outfile << atm->atomicSymbol() << atm->suffixHack() << "    ";
          }
          sprintf(buf,"%15.5f   %15.5f   %15.5f",coords[0],coords[1],coords[2]);
          outfile << buf << endl;
       }
    }
    outfile.flush();
  }
  return ret;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    parse data from an MVM file and fill some temporary data structures
//    which can then be used to create Fragment objects.
//
//  Implementation:
//    I've added some code to make this more robust but it can
//    probably still be crashed if you try hard.  The major changes
//    are:
//       . make sure error gets set if an error gets put on the queue
//       . check for infile.good() in the loop
//       . check for fail() (conversion error) while reading atom and bond
//         lists. 
//    I also fixed some memory leaks where memory was not cleaned up
//    if the import failed.  There are probably more of these.  For example,
//    shouldn't coordinates be deleted at the end?
//    
///////////////////////////////////////////////////////////////////////////////
bool Fragment::restoreMVM(istream& infile, double mult, bool genBonds)
{
  TPerTab tpt;
  int index;
  p_name = "mvm import";
  bool cartesian = false;                       // Includes Coordinates
  bool fractional = false;
  bool numbers = false;
  bool has_indexes = false;
  bool error = false;
  int atomNum, atmCnt, ijunk1;
  int numBonds=0;
  int offset = numAtoms();
  char buf[1024];
  bool setEnvInd = true;
  LatticeDef *lattice = 0;
  while (!infile.eof() && !error && infile.good()) {
    string line;
    infile.getline(buf, 1023);
    line = buf;
    // Add validation here for correct format number
    STLUtil::removeCommentAndSpaces(line);
    if (line != "") {
      string key = STLUtil::stripKey(line);
      if (STLUtil::compareString(key,"title") == 0) {
        p_name = line;
//      } else if (key.compareTo("charge", string::ignoreCase) == 0) {
//        p_charge = atof(line.c_str());
      } else if (STLUtil::compareString(key,"lattice_vectors") == 0) {
         lattice = new LatticeDef(1.,1.,1.,90.,90.,90.);
         double x,y,z;
         MPoint p[3];
         for (int adx=0; adx<3 && !error && infile.good();adx++) {
            if (infile.getline(buf,120)) {
               sscanf(buf,"%lf %lf %lf",&x,&y,&z);
               p[adx].xyz(x,y,z);
            }
         }
         if (infile.good()) {
            lattice->assign(p[0], p[1], p[2]);
            setLattice(lattice);
         } else {
            delete lattice;
         }
      } else if (STLUtil::compareString(key,"lattice") == 0) {
         lattice = new LatticeDef(1.,1.,1.,90.,90.,90.);
         double a,b,c, alpha,beta,gamma;
         if (infile.getline(buf,120)) {
            sscanf(buf,"%lf %lf %lf %lf %lf %lf",&a,&b,&c,&alpha,&beta,&gamma);
         }
         if (infile.good()) {
            lattice->assign(a,b,c,alpha,beta,gamma);
            setLattice(lattice);
         } else {
            delete lattice;
         }
      } else if (STLUtil::compareString(key,"num_atoms") == 0) {
        atmCnt = atoi(line.c_str());
      } else if (STLUtil::compareString(key,"atom_info") == 0) {
        StringTokenizer next(line);
        string token;
        while (!((token = next.next()).empty())) {
          if (STLUtil::compareString(token,"symbol") == 0) {
            ;
          } else if (STLUtil::compareString(token,"cart") == 0) {
            cartesian = true;
          } else if (STLUtil::compareString(token,"fractional") == 0) {
            fractional = true;
          } else if (STLUtil::compareString(token,"at_number") == 0) {
            numbers= true;
          } else if (STLUtil::compareString(token,"index") ==0) {
            has_indexes= true;
          } else {
            ErrMsg().message("EE_PARSE_ERROR",EE_WARNING, WHERE,line.c_str());
            error = true;
          }
        }
      } else if (STLUtil::compareString(key,"fragment_attributes") == 0) {
      // Get fragment attributes if available
        char buf[1024];
        infile.getline(buf, 1023);
        line = buf;
        key = STLUtil::stripKey(line);
        while(STLUtil::compareString(key,"end_fragment_attributes") !=0) {
          if (STLUtil::containsString(key,"charge")) {
            p_charge = atoi(line.c_str());
          }
          if (STLUtil::containsString(key,"first_solvent_atom_index")) {
            p_firstSolventAtomIndex = atoi(line.c_str());
          }
          if (STLUtil::containsString(key,"fragment_dof")) {
            p_DoF = atoi(line.c_str());
          }
          if (STLUtil::containsString(key,"solvent_num_atoms")) {
            p_solventNumAtoms = atoi(line.c_str());
          }
          if (STLUtil::containsString(key,"point_group")) {
            char tmpgrp[120];
            int i, j;
            j=0;
            for (i=0; i<line.length(); i++) {
              if (line[i] != ' ') {
                tmpgrp[j] = line[i];
                j++;
              }
            }
            tmpgrp[j] ='\0';
            p_pointGroup = tmpgrp;
          }
          infile.getline(buf, 1023);
          line = buf;
          key = STLUtil::stripKey(line);
        }
      } else if (STLUtil::compareString(key, "atom_list") == 0) {
        // Get atoms from atom list
        string tag;
        string symbolBuffer;
        double coords[3];
        for (index=0; index<atmCnt && !error && infile.good();index++) {
          if (has_indexes) infile >> ijunk1;
          if (numbers) {
            infile >> atomNum;
            symbolBuffer = tpt.atomicSymbol(atomNum);
          } else {
            char buf[12];
            infile >> buf;
            symbolBuffer = buf;
            // Due to xlation process it might still be a #
            if (symbolBuffer.size() > 0 && isdigit(symbolBuffer[0])) {
              symbolBuffer = tpt.atomicSymbol(atoi(symbolBuffer.c_str()));
            } 
          }
          tag = TPerTab::mixedCase(symbolBuffer);;
          int idx = tag.find_last_of('-');
          if (idx != string::npos) tag.erase(idx,tag.size()-idx+1);
          idx = tag.find_last_of('+');
          if (idx != string::npos) tag.erase(idx,tag.size()-idx+1);
          if (cartesian || fractional) {
            infile >> coords[0] >> coords[1] >> coords[2];
            error = infile.fail();
            addAtom(tag,coords);
          }
        }
        if (fractional) {
           convertFromFractionalCoords();
        }
      } else if (STLUtil::compareString(key,"pdb_list") == 0) {
        // Read in PDB information
        char buf[121];
        char* cptr;
        char resnam[5];
        char segnam[5];
        char a4[5];
        char i6[7];
        char icode;
        char chain;
        char coupling;
        int ires;
        char oldres[5];
        int oldires = -1;
        char oldchain = ' ';
        char oldinsert = ' ';
        strncpy(oldres,"    ",4);
        resnam[4] = '\0';
        oldres[4] = '\0';
        segnam[4] = '\0';
        a4[4] = '\0';
        i6[6] = '\0';
        TAtm* atm;
        Residue* res = (Residue*)0;
        p_chains.clearChainID();
        for (index=0; index<atmCnt && !error && infile.good();index++) {
          if (infile.getline(buf,120)) {
            cptr = buf;
            atm = p_atoms[index];
            coupling = cptr[0];
            cptr += 2;
            strncpy(a4,cptr,4);
            atm->atomName(a4);
            cptr +=5;
            strncpy(resnam,cptr,4);
            cptr += 4;
            strncpy(i6,cptr,6);
            ires = atoi(i6);
            cptr += 7;
            icode = cptr[0];
            cptr += 2;
            strncpy(segnam,cptr,4);
            cptr += 5;
            chain = cptr[0];

// Check to see if residue is new. If so, add a new residue to residue
// list.            

            if (strncmp(oldres,resnam,4) != 0 || ires != oldires ||
                icode != oldinsert || chain != oldchain) {
              Residue* residue = new Residue(resnam,ires);
              res = residue;
              addResidue(residue);
              residue->name(resnam);
              residue->number(ires);
              residue->chain(chain);
              residue->insertCode(icode);
              residue->firstAtomPtr(atm);
              residue->segID(segnam);
              p_chains.addChainID(chain);
              if (coupling == 'M') {
                residue->couplingInfo(Residue::MONOMER);
              } else if (coupling == 'H') {
                residue->couplingInfo(Residue::HEAD);
              } else if (coupling == 'T') {
                residue->couplingInfo(Residue::TAIL);
              } else if (coupling == 'I') {
                residue->couplingInfo(Residue::INTERIOR);
              } else {
                residue->couplingInfo(Residue::MONOMER);
              }
              strcpy(oldres,resnam);
              oldires = ires;
              oldinsert = icode;
              oldchain = chain;
            }
            atm->atomResidue(res);
          }
        }
      } else if (STLUtil::compareString(key,"suffix_list") == 0) {
        // Read in attributes
        TAtm* atm;
        char buf[121];
        for (index=0; index<atmCnt && !error && infile.good();index++) {
          if (infile.getline(buf,120)) {
            atm = p_atoms[index];
            atm->suffixHack(buf);
          }
        }
      } else if (STLUtil::compareString(key,"attr_list") == 0) {
        // Read in attributes
        TAtm* atm;
        char buf[121];
        char* cptr;
        char a9[10];
        char i4[5];
        a9[9] = '\0';
        i4[4] = '\0';
        for (index=0; index<atmCnt && !error && infile.good();index++) {
          if (infile.getline(buf,120)) {
            cptr = buf;
            atm = p_atoms[index];
            if (strlen(cptr) >= 9) {
              strncpy(a9,cptr,9);
              atm->partialCharge(atof(a9));
              cptr += 9;
            }
            if (strlen(cptr) >= 4) {
              strncpy(i4,cptr,4);
              atm->chargeGroup(atoi(i4));
              cptr += 4;
            }
            if (strlen(cptr) >= 4) {
              strncpy(i4,cptr,4);
              atm->polarizationGroup(atoi(i4));
              cptr += 4;
            }
            if (strlen(cptr) >= 4) {
              strncpy(i4,cptr,4);
              atm->connectionIndex(atoi(i4));
              cptr += 4;
            }
            if (strlen(cptr) >= 4) {
              strncpy(i4,cptr,4);
              atm->dihedralIndex(atoi(i4));
              cptr += 4;
              setEnvInd = false;
            }
          }
        }
      } else if (STLUtil::compareString(key,"atom_type_list") == 0) {
        // Read in atom types
        char buf[121];
        char* cptr;
        char a5[6];
        char i3[4];
        a5[5] = '\0';
        i3[3] = '\0';
        TAtm* atm;
        for (index=0; index<atmCnt && !error && infile.good();index++) {
          if (infile.getline(buf,120)) {
            cptr = buf;
            atm = p_atoms[index];
            strncpy(a5,cptr,5);
            atm->atomType(a5);
            cptr += 5;
            if (strlen(cptr) >= 4) {
              cptr++;
              strncpy(i3,cptr,3);
              atm->shape((ShapeData::Shape)atoi(i3));
              cptr += 3;
            }
            if (strlen(cptr) >= 4) {
              cptr++;
              strncpy(i3,cptr,3);
              // this line allows us to load the display style saved in
              // the MVM file.  But we want to use the one set in the
              // builder and the calcviewer under "Display>All" .  So
              // this line is commented out. 
              //atm->displayStyle((DisplayStyle)atoi(i3));
              cptr += 3;
            }
          }
        }
      } else if (STLUtil::compareString(key,"atom_mass_list") == 0) {
        char buf[121];
        TChartOfNuclides ichart;
        TAtm* atm;
        for (index=0; index<atmCnt && !error && infile.good();index++) {
          if (infile.getline(buf,120)) {
            atm = p_atoms[index];
            atm->setMass(atof(buf));
          }
        }
      } else if (STLUtil::compareString(key,"atom_behavior_list") == 0) {
        char buf[121];
        TAtm* atm;
        string tbuf;
        for (index=0; index<atmCnt && !error && infile.good();index++) {
          if (infile.getline(buf,120)) {
            atm = p_atoms[index];
            tbuf = buf;
            if (STLUtil::compareString(tbuf,"quantum") == 0) {
              atm->setBehaviorType(TAtm::QUANTUM);
            } else if (STLUtil::compareString(tbuf,"point") == 0) {
              atm->setBehaviorType(TAtm::POINT);
            } else if (STLUtil::compareString(tbuf,"bq") == 0) {
              atm->setBehaviorType(TAtm::BQ);
            } else if (STLUtil::compareString(tbuf,"bqx") == 0) {
              atm->setBehaviorType(TAtm::BQX);
            } else {
              atm->setBehaviorType(TAtm::UNSPECIFIED);
            }
          }
        }
      } else if (STLUtil::compareString(key,"num_bonds") == 0) {
        numBonds = atoi(line.c_str());
      } else if (STLUtil::compareString(key,"bond_info") == 0) {
        StringTokenizer next(line);
        string token;
        while (!((token = next.next()).empty())) {
          if (STLUtil::compareString(token,"order") != 0) {
            ErrMsg().message("EE_PARSE_ERROR",EE_WARNING, WHERE,line.c_str());
            error = true;
          }
        }
      } else if (STLUtil::compareString(key,"bond_list") == 0) {
        if (numBonds <= 0) {
          ErrMsg().message("EE_PARSE_ERROR",EE_WARNING, WHERE,line.c_str());
          error = true;
        } else {
          // Read in bonds
          for (index = 0;index < numBonds && !error && infile.good(); index++) {
            int atom1 = 0, atom2 = 0;
            double order = 1.0;
            int display = -1;
            infile >> atom1 >> atom2;
            if (infile.peek() != '\n') {
              infile >> order;
            }
            order = (order==0.0 ? 1.0 : order);
            if (infile.peek() != '\n') {
              infile >> display;
            }
            // Although this seems like a bizarre thing to do (throw away a
            // pointer to newly allocated memory), it's actually needed.  Bonds
            // are not created within the atoms referenced without calling the
            // TBond constructor, but the bond returned isn't used so casting
            // to void avoids a compiler warning.  GDB 1/5/2009
            (void)new TBond(atomRef(atom1+offset-1),
                            atomRef(atom2+offset-1),order);
            // Again... no need to set the bond display style with the
            // one saved in this file.  We'll use the mainDisplayStyle
            // set in the builder.
            //if (display > -1) bond->displayStyle((DisplayStyle)display);
            error = infile.fail();
          }
        }
      } else if ((STLUtil::compareString(key,"type") == 0) || 
          (STLUtil::compareString(key,"cell_units") == 0) ) {
        continue;
      } else {
#if 000
        cerr << "Ignoring Key \"" << key << " " << line << "\"" << endl;
#endif
      }
    }
  }
  generateShapes();
  if (mult != 1.0) adjustCoordinates(mult);
  if (genBonds) generateBondOrders();
  if (setEnvInd) setEnvIndices();

  return (!error && numAtoms() > 0);
}

void Fragment::generateFullMolecule(void)
{
  string symmetry = pointGroup();
  string symmorig = symmetry;
  for (int i=0; i<symmetry.size(); i++) {
    symmetry[i] = toupper(symmetry[i]);
  }

  if (symmetry != "C1") {
    string thename = name();
    int thecharge = charge();
    bool symmetryFlag = useSymmetry();

    // Create input file
    SFile* inFile = TempStorage::getTempFile();
    FILE* fp = fopen((char*)inFile->path().c_str(), "w");

    if (fp != (FILE*)0) {
      double threshold = 0.01;
      fprintf(fp, "%zu\n%G\n%-8s\n", numAtoms(), threshold, symmetry.c_str());

      double* coords = coordinates();
      TAtm* aref;
      for (int index=0; index<numAtoms(); index++) {
        aref = atomRef(index);
        fprintf(fp, "%-16s\n%d %G %G %G\n",
                (char*)aref->atomicSymbol().c_str(),
                aref->atomicNumber(), coords[3*index],
                coords[3*index+1], coords[3*index+2]);
      }
      fclose(fp);
      delete [] coords;

      SFile* outFile = TempStorage::getTempFile();

      // create command line
      string cmd = "./genmol <" + inFile->path() + " >" +
                   outFile->path();
      int istatus = system((char*)cmd.c_str()) ;  // execute command
      istatus = istatus >> 8;
      inFile->remove();

      if (istatus == 0) {
        // read in new file
        ifstream ifs(outFile->path().c_str());
        if (ifs) {
          // delete current fragment
          clear();

          bool status = restoreXYZ(ifs, 1.0, false);
          if (status) {
            addCovalentBonds(true);

            pointGroup(symmorig);
            name(thename);
            charge(thecharge);
            useSymmetry(symmetryFlag);
          }

          ifs.close();
          outFile->remove();
        }
      }
    }
  }
}

void Fragment::generateIrreducibleFragment(void)
{
  // Create input file
  SFile* inFile = TempStorage::getTempFile();
  FILE* fp = fopen((char*)inFile->path().c_str(), "w");

  if (fp != (FILE*)0) {
    double threshold = 0.01;
    int natoms = numAtoms();
    int index;

    string symmetry = pointGroup();
    for (int i = 0; i<symmetry.size(); i++) {
      symmetry[i] = toupper(symmetry[i]);
    }

    fprintf(fp, "%d\n%G\n%-8s\n", natoms, threshold, symmetry.c_str());

    // create list of atom indices
    vector<int> listofatoms;
    for (index=0; index<natoms; index++)
      listofatoms.push_back(index);

    double* coords = coordinates();
    TAtm* aref;
    for (index=0; index<numAtoms(); index++) {
      aref = atomRef(index);
      fprintf(fp, "%-16s\n%d %G %G %G\n",
              (char*)aref->atomicSymbol().c_str(),
              aref->atomicNumber(), coords[3*index],
              coords[3*index+1], coords[3*index+2]);
    }
    fclose(fp);
    delete [] coords;

    SFile* outFile = TempStorage::getTempFile();

    // create command line
    string cmd = "./getfrag <" + inFile->path() + " >" +
                 outFile->path();
    int istatus = system((char*)cmd.c_str()) ;  // execute command
    istatus = istatus >> 8;
    inFile->remove();

    if (istatus == 0) {
      int nind = 0;

      // read output file.
      // File contains a list of the atoms that should remain in the
      // system.  The atoms remaining in the listofatoms are the ones
      // removed from the fragment
      fp = fopen((char*)outFile->path().c_str(), "r");
      if (fp != (FILE*)0) {
        char buf[256];
        (void)fgets(buf, 256, fp);
        nind = (int)strtol(buf, NULL, 10);

        int ind;
        vector<int>::iterator it;
        for (index=0; index<nind; index++) {
          (void)fgets(buf, 256, fp);
          ind = (int)strtol(buf, NULL, 10) - 1;
          it = find(listofatoms.begin(),listofatoms.end(), ind);
          listofatoms.erase(it);
        }
        fclose(fp);
        outFile->remove();

        // Delete all atoms except the irreducible indices
        for ( index=listofatoms.size()-1; index>-1; index--)
          deleteAtom(listofatoms[index]);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Renumber residue sequence starting at 1. Remove all insertion indices
//    and resequence chain ID's starting at A.
//
////////////////////////////////////////////////////////////////////////////////
void Fragment::renumberResidues(void)
{
  int nresidues = p_residueSequence.size();
  if (nresidues == 0) return;

// Free up all characters for use as chain IDs

  p_chains.clearChainID();
  Residue* res=p_residueSequence[0];

  char oldchainID = res->chain();
  char newchainID = ' ';
  if (oldchainID != ' ') {
    newchainID = p_chains.getNewChainID();
  }
  
  char chainID;
  for (int i=0; i<nresidues; i++) {
    res=p_residueSequence[i];
    chainID = res->chain();
    if (chainID != oldchainID) {
      if (chainID != ' ') {
        newchainID = p_chains.getNewChainID();
      } else {
        newchainID = ' ';
      }
      oldchainID = chainID;
    }
    res->number(i+1);
    res->chain(newchainID);
    res->insertCode(' ');
  }
}


/**
 * UPdate residue indices.
 */
void Fragment::reIndexResidues(int start)
{
   int nresidues = p_residueSequence.size();
   for (int idx=start; idx<nresidues; idx++) {
      p_residueSequence[idx]->index(idx);
   }
}


/**
 * Compute center of mass.
 */
void Fragment::centerOfMass(double& cmx, double& cmy, double& cmz)
{
  TPerTab pertab;
  int natoms = numAtoms();
  cmx = 0.0;
  cmy = 0.0;
  cmz = 0.0;
  double mtot = 0.0;
  const double *coords;
  double mass;
  TAtm* atom;
  for (int i=0; i<natoms; i++) {
    atom = atomRef(i);
    if (atom->atomicNumber()>0) {
      mass = (double)pertab.atomicWeight(atom->atomicNumber());
      mtot += mass;
      coords = atom->coordinates();
      cmx += mass*coords[0];
      cmy += mass*coords[1];
      cmz += mass*coords[2];
    }
  }
  cmx = cmx/mtot;
  cmy = cmy/mtot;
  cmz = cmz/mtot;
}
/**
 * Compute center of mass of residue whose index is idx.
 */
void Fragment::residueCenterOfMass(int idx, double& x, double& y, double &z)
{
  TPerTab pertab;

  x = 0.0;
  y = 0.0;
  z = 0.0;
  double mass = 0.0;
  float tmass;

  Residue *res = p_residueSequence[idx];
  vector<TAtm*> atoms = findResidueAtoms(res);
  int natoms = atoms.size();
  int i;
  for (i=0; i<natoms; i++) {
    const double *coords = atoms[i]->coordinates(); 
    tmass = pertab.atomicWeight(atoms[i]->atomicNumber());
    x += ((double)tmass)*coords[0];
    y += ((double)tmass)*coords[1];
    z += ((double)tmass)*coords[2];
    mass += (double)tmass;
  }
  x = x/mass;
  y = y/mass;
  z = z/mass;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Translate fragment so that center of mass is at the origin
//    The parameter iopt allows user to specify that only some components of
//    the center of mass are translated to the origin.
//
////////////////////////////////////////////////////////////////////////////////
void Fragment::recenterFrag(int iopt)
{
  
  int natoms = numAtoms();
  double cmx = 0.0;
  double cmy = 0.0;
  double cmz = 0.0;

  centerOfMass(cmx, cmy, cmz);

  if (iopt == 1) {
    cmy = 0.0;
    cmz = 0.0;
  } else if (iopt == 2) {
    cmx = 0.0;
    cmz = 0.0;
  } else if (iopt == 3) {
    cmx = 0.0;
    cmy = 0.0;
  }
  for (int i=0; i<natoms; i++) {
    TAtm *atom = atomRef(i);
    const double *coords = atom->coordinates();
    const double x = coords[0] - cmx;
    const double y = coords[1] - cmy;
    const double z = coords[2] - cmz;
    atom->coordinates(x,y,z);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Description:
//    Translate and rotate fragment so that the center of mass is at the origin
//    and the principle components of the moment of inertia are along the axes
//
////////////////////////////////////////////////////////////////////////////////
void Fragment::orientFrag()
{
  recenterFrag(0);
  double inertia[3][3];
  int natoms = numAtoms();
  int n,i,j;

  // Initialize moments of inertia to zero
  for (i=0; i<3; i++) {
    for (j=0; j<3; j++) {
      inertia[i][j] = 0.0;
    }
  }

  TAtm *atm;
  const double *coords;
  double mass, r2;
  // Evaluate moments of inertia tensor
  for (n=0; n<natoms; n++) {
    atm = atomRef(n);
    coords = atm->coordinates();
    mass = atm->getMass();
    r2 = coords[0]*coords[0]+coords[1]*coords[1]+coords[2]*coords[2];
    for (i=0; i<3; i++) {
      inertia[i][i] += mass*r2;
    }
    for (i=0; i<3; i++) {
      for (j=0; j<3; j++) {
        inertia[i][j] += -mass*coords[i]*coords[j];
      }
    }
  }
  // Diagonalize moments of inertia
  Diag3 diag(inertia[0][0],inertia[0][1],inertia[0][2],
             inertia[1][1],inertia[1][2],inertia[2][2]);
  double ew[3],e1[3],e2[3],e3[3];
  diag.calculate(ew, e1, e2, e3);

  // Evaluate transformation matrix that diagonalizes inertia tensor
  double r[3][3];
  for (i=0; i<3; i++) {
    r[0][i] = e3[i];
    r[1][i] = e2[i];
    r[2][i] = e1[i];
  } 
  
  // transform coordinates
  for (n=0; n<natoms; n++) {
    atm = atomRef(n);
    // copy coordinates to new array
    coords = atm->coordinates();
    for (i=0; i<3; i++) {
      ew[i] = coords[i];
    }
    // transform coordinates
    for (i=0; i<3; i++) {
      e1[i] = 0.0;
      for (j=0; j<3; j++) {
        e1[i] += r[i][j]*ew[j];
      }
    }
    // reset atom
    atm->coordinates(e1);
  }
}
////////////////////////////////////////////////////////////////////////////////
//
// Description:
//   Determine if fragment is linear. Single atoms are not considered linear.
//
////////////////////////////////////////////////////////////////////////////////
bool Fragment::isLinear()
{
  double com[3];
  double inertia[3][3];
  int natoms = numAtoms();
  int n,i,j;
  bool ret = false;

  if (natoms > 2) {
    // Initialize moments of inertia to zero
    for (i=0; i<3; i++) {
      for (j=0; j<3; j++) {
        inertia[i][j] = 0.0;
      }
    }

    // Find center of mass
    centerOfMass(com[0], com[1], com[2]);

    TAtm *atm;
    const double *coords;
    double mass, r2;
    double crd[3];
    // Evaluate moments of inertia tensor
    for (n=0; n<natoms; n++) {
      atm = atomRef(n);
      coords = atm->coordinates();
      mass = atm->getMass();
      crd[0] = coords[0] - com[0];
      crd[1] = coords[1] - com[1];
      crd[2] = coords[2] - com[2];
      r2 = crd[0]*crd[0]+crd[1]*crd[1]+crd[2]*crd[2];
      for (i=0; i<3; i++) {
        inertia[i][i] += mass*r2;
      }
      for (i=0; i<3; i++) {
        for (j=0; j<3; j++) {
          inertia[i][j] += -mass*crd[i]*crd[j];
        }
      }
    }
    // Diagonalize moments of inertia
    Diag3 diag(inertia[0][0],inertia[0][1],inertia[0][2],
        inertia[1][1],inertia[1][2],inertia[2][2]);
    double ew[3],e1[3],e2[3],e3[3];
    diag.calculate(ew, e1, e2, e3);
    if (fabs(ew[0]) <= 1.0e-10 || fabs(ew[1]) <= 1.0e-10 || fabs(ew[2]) <= 1.0e-10) {
      ret = true;
    }
  } else if (natoms == 2) {
    ret = true;
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
//
// Description:
//    Rotate the system about the origin so that atom 1 is translated to the
//    origin, atom 2 is aligned along the z axis, and atom 3 is aligned along
//    the x=y plane
//
////////////////////////////////////////////////////////////////////////////////
void Fragment::alignFrag(int atm1, int atm2, int atm3)
{
  double r[3][3];
  int natoms = numAtoms();
  int n,i,j;
  TAtm *atm = atomRef(atm1);
  const double *coords = atm->coordinates();
  double disp[3];
  for (i=0; i<3; i++) {
    disp[i] = coords[i];
  }

  // Center atom 1
  double rtmp[3];
  for (n=0; n<natoms; n++) {
    atm = atomRef(n);
    coords = atm->coordinates();
    for (i=0; i<3; i++) {
      rtmp[i] = coords[i]-disp[i];
    }
    atm->coordinates(rtmp);
  }

  // Align atom 2 with z-axis
  atm = atomRef(atm2);
  coords = atm->coordinates();
  for (i=0; i<3; i++) {
    disp[i] = coords[i];
  }
  double pi = 4.0*atan(1.0);
  double rn = sqrt(disp[0]*disp[0]+disp[1]*disp[1]+disp[2]*disp[2]);
  // Calculate angle between projection of coordinates of atom 2 in
  // the xy plane and the x axis
  double theta = acos(disp[0]/rn);
  if (disp[1]<0) theta = 2.0*pi - theta;
  // Calculate rotation matrix about z axis
  r[0][0] = cos(theta);
  r[0][1] = sin(theta);
  r[0][2] = 0.0;
  r[1][0] = -sin(theta);
  r[1][1] = cos(theta);
  r[1][2] = 0.0;
  r[2][0] = 0.0;
  r[2][1] = 0.0;
  r[2][2] = 1.0;
  // Perform rotation
  for (n=0; n<natoms; n++) {
    atm = atomRef(n);
    coords = atm->coordinates();
    for (i=0; i<3; i++) {
      rtmp[i] = 0;
      for (j=0; j<3; j++) {
        rtmp[i] = r[i][j]*coords[j];
      }
    }
    atm->coordinates(rtmp);
  }

  // Do second rotation about y axis to align atom 2 with z axis
  atm = atomRef(atm2);
  coords = atm->coordinates();
  for (i=0; i<3; i++) {
    disp[i] = coords[i];
  }
  rn = sqrt(disp[0]*disp[0]+disp[1]*disp[1]+disp[2]*disp[2]);
  theta = acos(disp[2]/rn);
  if (disp[0]<0) theta = 2.0*pi - theta;
  // Calculate rotation matrix about y axis
  r[0][0] = sin(theta);
  r[0][1] = 0.0;
  r[0][2] = cos(theta);
  r[1][0] = 0.0;
  r[1][1] = 1.0;
  r[1][2] = 0.0;
  r[2][0] = sin(theta);
  r[2][1] = 0.0;
  r[2][2] = cos(theta);
  // Perform rotation
  for (n=0; n<natoms; n++) {
    atm = atomRef(n);
    coords = atm->coordinates();
    for (i=0; i<3; i++) {
      rtmp[i] = 0;
      for (j=0; j<3; j++) {
        rtmp[i] = r[i][j]*coords[j];
      }
    }
    atm->coordinates(rtmp);
  }
  // Align atom 3 with x=y plane
  atm = atomRef(atm3);
  coords = atm->coordinates();
  for (i=0; i<3; i++) {
    disp[i] = coords[i];
  }
  rn = sqrt(disp[0]*disp[0]+disp[1]*disp[1]+disp[2]*disp[2]);
  // Calculate angle between projection of coordinates of atom 3 in
  // the xy plane and the vector (1,1)/sqrt(2)
  theta = acos((disp[0]+disp[1])/(rn*sqrt(2.0)));
  if ((-disp[0]+disp[1])<0) theta = 2.0*pi - theta;
  // Calculate rotation matrix about z axis
  r[0][0] = cos(theta);
  r[0][1] = sin(theta);
  r[0][2] = 0.0;
  r[1][0] = -sin(theta);
  r[1][1] = cos(theta);
  r[1][2] = 0.0;
  r[2][0] = 0.0;
  r[2][1] = 0.0;
  r[2][2] = 1.0;
  // Perform rotation
  for (n=0; n<natoms; n++) {
    atm = atomRef(n);
    coords = atm->coordinates();
    for (i=0; i<3; i++) {
      rtmp[i] = 0;
      for (j=0; j<3; j++) {
        rtmp[i] = r[i][j]*coords[j];
      }
    }
    atm->coordinates(rtmp);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Remove all residue information from fragment.
//
////////////////////////////////////////////////////////////////////////////////
void Fragment::clearResidueInfo(void)
{
  int natoms = p_atoms.size();
  int i;
  TAtm *atm;
  for (i=0; i<natoms; i++) {
    atm = atomRef(i);
    atm->atomResidue((Residue*)0);
  }
  int nres = p_residueSequence.size();
  for (i=0; i<nres; i++) {
    delete p_residueSequence[i];
  }
  p_residueSequence.clear();
  clearFirstSolventAtomIndex();
}
////////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Insert residue into chain AFTER residue containing the TAtm atom.
//    If atom is null, then append residue to the end of the chain.
//    Try and create relatively unique set of indices, etc. for new
//    residue. Return the index of the new residue.
//
////////////////////////////////////////////////////////////////////////////////
int Fragment::insertResidue(TAtm* atom)
{
  if (atom != (TAtm*)0) {
    Residue* residue = findResidue((const TAtm*)atom);
    int ires;
//
// find index of residue containing atom
//
    vector<Residue*>::iterator it = p_residueSequence.begin();
    for (int i = 0; i<p_residueSequence.size(); i++) {
      if (*it == residue) {
        ires = i;
        break;
      } else it++;
    }
//
// Get data from residues ires and the residue after ires.
//
    int idx1,idx2;
    char icode1,icode2;
    char chn1,chn2;
    bool exst2;
    idx1 = residue->number();
    icode1 = residue->insertCode();
    chn1 = residue->chain();

    if (ires+1 < p_residueSequence.size()) {
      idx2 = p_residueSequence[ires+1]->number();
      icode2 = p_residueSequence[ires+1]->insertCode();
      chn2 = p_residueSequence[ires+1]->chain();
      exst2 = true;
    } else {
      exst2 = false;
    }
//
// Figure out number, chain ID, and insertion code for
// new residue.
//
    int newIndex;
    char newChain,newICode;
    if (exst2 && idx2 > idx1+1) {
// There is a gap in the numbering. Just use next integer.
      newIndex = idx1+1;
      newChain = chn1;
      newICode = ' ';
    } else if (exst2 && idx2 == idx1) {
// No gap in the numbering
      if (chn1 != chn2) {
// No gap in the indices but next residue is on a different
// chain. Just use next integer.
        newIndex = idx1+1;
        newChain = chn1;
        newICode = ' ';
      } else {
// Only insertion codes are different. Pick a third insertion
// code.
        newIndex = idx1;
        newChain = chn1;
        if (icode1 != 'A' && icode2 != 'A') {
          newICode = 'A';
        } else if (icode1 != 'B' && icode2 != 'B') {
          newICode = 'B';
        } else {
          newICode = 'C';
        }
      }
    } else if (exst2 && idx2 == idx1+1) {
// If residues on same chain, insert residue with same index as
// ires but use new insertion code. If not on the same chain,
// just increase index.
      newChain = chn1;
      if (chn1 == chn2) {
        newIndex = idx1;
        if (icode1 != 'A') {
          newICode = 'A';
        } else {
          newICode = 'B';
        }
      } else {
        newIndex = idx1+1;
        newICode = ' ';
      }
    } else if (exst2) {
// Residue numbering is fairly screwed up. Just increase index
// and let user figure it out later.
      newIndex = idx1+1;
      newChain = chn1;
      newICode = ' ';
    } else {
// ires is last residue in fragment
      newIndex = idx1+1;
      newChain = chn1;
      newICode = ' ';
    }
//
//  Indices etc. have been determined for new residue. Create new
//  residue and add it to the list.
//
    Residue* newRes = new Residue("UNK ",newIndex);
    newRes->chain(newChain);
    newRes->insertCode(newICode);
    if (exst2) {
      vector<Residue*>::iterator it = p_residueSequence.begin();
      it += (ires+1);
      p_residueSequence.insert(it, newRes);
    } else {
      (void)p_residueSequence.push_back(newRes);
    }
    return ires+1;
  } else {
// Null atom pointer handed in. Just append residue to the end
// of residue list.
    int ires = p_residueSequence.size()-1;
    int newIndex;
    if (ires != -1) {
      newIndex = p_residueSequence[ires]->number()+1;
    } else {
      newIndex = 1;
    }
    char newChain = ' ';
    char newICode = ' ';
    Residue* newRes = new Residue("UNK ",newIndex);
    newRes->chain(newChain);
    newRes->insertCode(newICode);
    (void)p_residueSequence.push_back(newRes);
    return ires+1;
  }
}
////////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Insert residue into chain Before residue containing the TAtm atom.
//    If atom is null, then insert residue at the front end of the chain.
//    Try and create relatively unique set of indices, etc. for new
//    residue. Return the index of the new residue.
//
////////////////////////////////////////////////////////////////////////////////
int Fragment::prependResidue(TAtm* atom)
{
  if (atom != (TAtm*)0) {
    Residue* residue = findResidue((const TAtm*)atom);
    int ires;
//
// find index of residue containing atom
//
    vector<Residue*>::iterator it = p_residueSequence.begin();
    for (int i = 0; i<p_residueSequence.size(); i++) {
      if (*it == residue) {
        ires = i;
        break;
      } else it++;
    }
//
// Get data from residues ires and the residue before ires.
//
    int idx1,idx2;
    char icode1,icode2;
    char chn1,chn2;
    bool exst2;
    idx1 = residue->number();
    icode1 = residue->insertCode();
    chn1 = residue->chain();

    if (ires-1 >= 0) {
      idx2 = p_residueSequence[ires-1]->number();
      icode2 = p_residueSequence[ires-1]->insertCode();
      chn2 = p_residueSequence[ires-1]->chain();
      exst2 = true;
    } else {
      exst2 = false;
    }
//
// Figure out number, chain ID, and insertion code for
// new residue.
//
    int newIndex;
    char newChain,newICode;
    if (exst2 && idx2 < idx1-1) {
// There is a gap in the numbering. Just use next lowest integer.
      newIndex = idx1-1;
      newChain = chn1;
      newICode = ' ';
    } else if (exst2 && idx2 == idx1) {
// No gap in the numbering
      if (chn1 != chn2) {
// No gap in the indices but next residue is on a different
// chain. Just use next lowest integer.
        newIndex = idx1-1;
        newChain = chn1;
        newICode = ' ';
      } else {
// Only insertion codes are different. Pick a third insertion
// code.
        newIndex = idx1;
        newChain = chn1;
        if (icode1 != 'A' && icode2 != 'A') {
          newICode = 'A';
        } else if (icode1 != 'B' && icode2 != 'B') {
          newICode = 'B';
        } else {
          newICode = 'C';
        }
      }
    } else if (exst2 && idx2 == idx1-1) {
// If residues on same chain, insert residue with same index as
// ires-1 but use new insertion code. If not on the same chain,
// just decrease index.
      newChain = chn1;
      if (chn1 == chn2) {
        newIndex = idx1-1;
        if (icode2 != 'A') {
          newICode = 'A';
        } else {
          newICode = 'B';
        }
      } else {
        newIndex = idx1-1;
        newICode = ' ';
      }
    } else if (exst2) {
// Residue numbering is fairly screwed up. Just decrease index
// and let user figure it out later.
      newIndex = idx1-1;
      newChain = chn1;
      newICode = ' ';
    } else {
// ires is first residue in fragment
      if (idx1 > 1) {
        newIndex = idx1-1;
        newICode = ' ';
      } else {
        newIndex = 1;
        if (icode1 != 'A') {
          newICode = 'A';
        } else {
          newICode = 'B';
        }
      }
      newChain = chn1;
    }
//
//  Indices etc. have been determined for new residue. Create new
//  residue and add it to the list.
//
    Residue* newRes = new Residue("UNK ",newIndex);
    newRes->chain(newChain);
    newRes->insertCode(newICode);
    it = p_residueSequence.begin();
    it += ires;
    p_residueSequence.insert(it, newRes);
    reIndexResidues(ires);
    return ires;
  } else {
// Null atom pointer handed in. Just insert residue at the front
// of residue list.
    int newIndex=1;
    char newChain = ' ';
    char newICode = ' ';
    if (p_residueSequence.size() > 0) {
      Residue *firstRes = p_residueSequence[0];
      if (firstRes->number() == 1) {
        if (firstRes->chain() == ' ') {
          if (firstRes->insertCode() == ' ') {
            newICode = 'A';
          } else if (firstRes->insertCode() == 'A') {
            newICode = 'B';
          }
        }
      }
    }
    Residue* newRes = new Residue("UNK ",newIndex);
    newRes->chain(newChain);
    newRes->insertCode(newICode);
    p_residueSequence.insert(p_residueSequence.begin(), newRes);
    reIndexResidues();
    return 0;
  }
}
///////////////////////////////////////////////////////////////////////////////
//  Description
//    This is a utility function to reset the internal atom list with a
//    replacement list
///////////////////////////////////////////////////////////////////////////////
void Fragment::resetAtomList(vector<TAtm*> &list)
{
  int size = list.size();
  int i;
  p_atoms.clear();
  for (i=0; i<size; i++) {
    p_atoms.push_back(list[i]);
    p_atoms[i]->index(i);
  }
  return;
}
/**
 *  Description
 *    This function returns the minimum and maximum values of the system in
 *    angstroms for each of the three coordinate dimensions. The ordering of
 *    the values is:
 *       xmin, xmax, ymin, ymax, zmin, zmax
 *    dims must be allocated to hold at least 6 doubles.
 */
void Fragment::getBoxDimensions(double *dims)
{
  int natoms = numAtoms();
  const double *coords;
  if (natoms > 0) {
    coords = p_atoms[0]->coordinates();
    dims[0] = coords[0];
    dims[1] = coords[0];
    dims[2] = coords[1];
    dims[3] = coords[1];
    dims[4] = coords[2];
    dims[5] = coords[2];
    int i;
    for (i=1; i<natoms; i++) {
      coords = p_atoms[i]->coordinates();
      if (coords[0] < dims[0] ) dims[0] = coords[0];
      if (coords[0] > dims[1] ) dims[1] = coords[0];
      if (coords[1] < dims[2] ) dims[2] = coords[1];
      if (coords[1] > dims[3] ) dims[3] = coords[1];
      if (coords[2] < dims[4] ) dims[4] = coords[2];
      if (coords[2] > dims[5] ) dims[5] = coords[2];
    }
  } else {
    dims[0] = 0.0;
    dims[1] = 0.0;
    dims[2] = 0.0;
    dims[3] = 0.0;
    dims[4] = 0.0;
    dims[5] = 0.0;
  }
  return;
}
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Print to Stream the Object's Local State.
//
//  Implemenation
//    Even though we must have coordinates we still put a check in here
//    to make sure they haven't disappeared.
//
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const Fragment& frag)
{
  int numAtoms = frag.numAtoms();
  int cnt = frag.p_atoms.size();
  os << "charge: " << frag.p_charge << endl;
  os << "numAtoms: " << numAtoms << endl;
  for (int idx=0; idx<cnt; idx++) {
    os << "Atom["<<idx+1<<"] pointer: " << frag.p_atoms[idx] << endl;
    os << "Atom["<<idx+1<<"]: " << *frag.p_atoms[idx] << endl;
  }

  os << "Bonds: " << endl;
  IBondList* bondIndices = frag.bondIndices();
  if (bondIndices->mnumBonds != 0) {
    int* atomIdx = bondIndices->matomIndices;
    double* orderIdx = bondIndices->mbondOrders;
    os << "numBonds: " << bondIndices->mnumBonds << endl;
    for (int index = 0; index < bondIndices->mnumBonds; index++) {
      os << "{" << (*atomIdx++) << ",";
      os <<  (*atomIdx++)<< ",";
      os << (*orderIdx++) << "}" << endl;
    }
  }
  delete bondIndices;
  os << ">" << endl;
  return os;
}

////////////////////////////////////////////////////////////////////////////////
//
// Description
//   Generate dihedral (environment) indices for all atoms in the fragment
//
// Implementation
//   Check shape of atom and atomic symbol and set dihedral (environment)
//   index
//
////////////////////////////////////////////////////////////////////////////////
void Fragment::setEnvIndices(void)
{
  int atmCntr;
  TAtm *currAtm;
  int dihIndx;

  for (atmCntr = 0; atmCntr < p_atoms.size(); atmCntr++) {
    dihIndx = 0;
    currAtm = p_atoms[atmCntr];
    if (currAtm->shape() == ShapeData::trigonalPlanar) {
      dihIndx = 1;
    } else if (currAtm->shape() == ShapeData::pyramidal &&
               currAtm->atomicSymbol() == "N") {
      dihIndx = 1;
    }
    currAtm->dihedralIndex(dihIndx);
  }
}
////////////////////////////////////////////////////////////////////////////////
//
// Description
//   Dummy routine that is used to invoke a method in SGFragment using a
//   Fragment pointer that originates as a SGFragment pointer
//
// Implementation
//   Some useless (but harmless) code
//
////////////////////////////////////////////////////////////////////////////////
vector<int> Fragment::findRingAtoms(void) const
{
  int natoms = numAtoms();
  int i;
  vector<int> ret;
  for (i=0; i<natoms; i++) ret.push_back(0);
  return ret;
}
////////////////////////////////////////////////////////////////////////////////
//
// Description
//   Routine that returns the status of the residues in a fragment. The return
//   code is:
//     -2: no residues in fragment
//     -1: incomplete or unknown residues found
//      0: all residues are generatable or complete
//      1: all residues are complete
//      2: only histidines are unknown
////////////////////////////////////////////////////////////////////////////////
int Fragment::checkResidueStatus(void) const
{
  int nres = p_residueSequence.size();
  if (nres == 0) return -2;
  bool allGeneratable = true;
  bool allComplete = true;
  bool onlyHistidine = true;
  int i;
  Residue::ResidueStatus status;
  for (i=0; i<nres; i++) {
    status = p_residueSequence[i]->residueStatus();
    if (status != Residue::COMPLETE) {
      allComplete = false;
    }
    if (status != Residue::GENERATABLE) {
      allGeneratable = false;
    }
    if (status == Residue::UNKNOWN && p_residueSequence[i]->name() != "HIS ") {
      onlyHistidine = false;
    }
  }
  if (allComplete) return 1;
  if (allGeneratable) return 0;
  if (onlyHistidine) return 2;
  return -1;
}
////////////////////////////////////////////////////////////////////////////////
//
// Description
//   Routine to return the sum of all partial charges in the fragment
////////////////////////////////////////////////////////////////////////////////
double Fragment::getTotalCharge()
{
  int natoms = numAtoms();
  int i;
  double ret = 0.0;
  for (i=0; i<natoms; i++) {
    ret += p_atoms[i]->partialCharge();
  }
  charge(ret);
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
//
// Description
//   Create a new fragment from a list of atoms from an existing fragment
////////////////////////////////////////////////////////////////////////////////
Fragment* Fragment::createSubFragment(vector<int> list, bool complement)
{
  Fragment *ret = new Fragment;
  vector<TAtm*> *atoms = this->atoms();
  int natoms = atoms->size();
  int tatoms = numAtoms();
  int nsize = list.size();
  int i;
  int acnt = 0;
  if (nsize > 0) {
    if (!complement) {
      for (i=0; i<nsize; i++) {
        if (list[i] >= 0 && list[i] <natoms) {
          TAtm* atm = new TAtm(*(*atoms)[list[i]]);
          ret->addAtom(atm);
          acnt++;
        }
      }
    } else {
      set<int> index;

      for (i=0; i<nsize; i++) {
        index.insert(list[i]);
      }
      for (i=0; i<tatoms; i++) {
        if (index.find(i) == index.end()) {
          TAtm* atm = new TAtm(*(*atoms)[i]);
          ret->addAtom(atm);
          acnt++;
        }
      }
    }
  } else {
    delete ret;
    ret = (Fragment*)0;
  }
  if (atoms) {
    delete atoms;
  }
  if (acnt > 0) {
    ret->addCovalentBonds();
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
//   Copy selected atoms AND bonds between selected atoms.
//   I basically copied this from the old ostore version to save time.
//   There could be some real time tradeoffs in how this is implemented
//   if you copy a large set of atoms from a large system vs a small
//   number of atoms from a large system.
/////////////////////////////////////////////////////////////////////////////
Fragment* Fragment::clipFragment(vector<int> atoms)
{
  int numCopyAtoms = atoms.size();


  Fragment *ret = new Fragment;
  if (numCopyAtoms > 0) {
    int idx;

    int atom_index = 0;
    TAtm* atm ;

    int numBond = numBonds();
    int bond_index = 0;
    TBond *bond ;

    // Clone the atoms and construct a map between the atoms in
    // the original fragment and the atoms in the buffer.

    map<int, int, less<int> > fragtobuf;
    map<int, int, less<int> > buftofrag;
    for (idx=0; idx<numCopyAtoms; idx++) 
    {
       atom_index = atoms[idx];
       atm = this->atomRef(atom_index);
       ret->addAtom(atm->clone(),(TAtm*)0);
       fragtobuf[atom_index] = idx;
       buftofrag[idx] = atom_index;
    }

    // Check to see if there are any residues in the original fragment.
    // If there are, then build up corresponding residue list in
    // ret.

    if (this->numResidues() > 0) {
      Residue* oldres = (Residue*)0;
      Residue* curres;
      Residue* bufres;
      for (idx = 0; idx < numCopyAtoms; idx++) {
        curres = this->atomRef(buftofrag[idx])->getResidue();
        if (curres != oldres) {
          Residue* newres = new Residue(*curres);
          ret->addResidue(newres);
          bufres = newres;
          newres->residueStatus(curres->residueStatus());
          newres->couplingInfo(curres->couplingInfo());
          newres->firstAtomPtr(ret->atomRef(idx));
          oldres = curres;
        }
        ret->atomRef(idx)->atomResidue(bufres);
      }
    }

    // Reset all residue pointers in individual atoms so that they point to
    // new residues in ret.
    int numRes = ret->numResidues();
    int natom = ret->numAtoms();
    int ires;
    int istart, iend;
    Residue *res;
    for (ires=0; ires<numRes; ires++) {
      res = ret->residue(ires);
      istart = res->firstAtomPtr()->index();
      if (ires < numRes-1) {
        iend = ret->residue(ires+1)->firstAtomPtr()->index();
      } else {
        iend = natom;
      }
      for (idx=istart; idx<iend; idx++) {
        ret->atomRef(idx)->atomResidue(res);
      }
    }

    //clone the bonds. Only copy bonds from fragment if both atoms
    //at ends of bond are in ret.

    TAtm* atom1;
    TAtm* atom2;
    int iat1,iat2,ibat1,ibat2;
    vector<TBond*> savebond;
    vector<TBond*> *bonds = this->bonds();
    // Just store old atoms and new nubs in temporary vectors so we don't have
    // to worry about updating maps
    vector<TAtm*> newNubs;
    vector<TAtm*> oldAtoms;
    for (idx=0; idx<numBond; idx++)
    {
      bond_index = idx;
      bond = (*bonds)[bond_index];
      atom1 = bond->atom1();
      atom2 = bond->atom2();
      iat1 = atom1->index();
      iat2 = atom2->index();

      // check to see that both iat1 and iat2 are being copied. If so,
      // then create bond in ret. If only one atom is being copied
      // then replace other atom with a nub.

      if (fragtobuf.count(iat1) == 1 && fragtobuf.count(iat2) == 1) {
        ibat1 = fragtobuf[iat1];
        ibat2 = fragtobuf[iat2];
        ret->addBond(ibat1,ibat2,bond->order());
        TBond* bufbond = ret->bond(ibat1,ibat2);
        bufbond->displayStyle(bond->displayStyle());
      } else if (fragtobuf.count(iat1) ==1 && fragtobuf.count(iat2) == 0) {
        // Create nub for atom 1
        TAtm* nub = new TAtm("Nub",atom2->coordinates());
        nub->shape(ShapeData::terminal);
        nub->displayStyle(atom1->displayStyle());
        newNubs.push_back(nub);
        ibat1 = fragtobuf[iat1];
        oldAtoms.push_back(ret->atomRef(ibat1));
      } else if (fragtobuf.count(iat1) ==0 && fragtobuf.count(iat2) == 1) {
        // Create nub for atom 1
        TAtm* nub = new TAtm("Nub",atom1->coordinates());
        nub->shape(ShapeData::terminal);
        nub->displayStyle(atom2->displayStyle());
        newNubs.push_back(nub);
        ibat2 = fragtobuf[iat2];
        oldAtoms.push_back(ret->atomRef(ibat2));
      }
    }
    // Finish creating nubs
    int size = newNubs.size();
    TAtm *atom, *nub;
    TPerTab tpt;
    for (idx=0; idx<size; idx++) {
      atom = oldAtoms[idx];
      nub = newNubs[idx];
      if (numRes > 0) {
        ires = ret->getResidueIndex(atom->getResidue());
        ret->editResidueMode(1, ires);
      }
      ret->addAtom(nub,atom);
      // Fix up the nub. This is essentially the same action as in the
      // SGFragment::fixNub method, but we can't access that function from
      // here.
      float radii = 1.0;
      if (nub->displayStyle().getStyle() == DisplayStyle::CPK) {
        radii = tpt.vwr(atom->atomicNumber());
      } else {
        radii = tpt.covalentRadius(atom->atomicNumber());
      }
      double x,y,z;
      const double *nubCoords = nub->coordinates();
      const double *coords = atom->coordinates();
      x = nubCoords[0] - coords[0];
      y = nubCoords[1] - coords[1];
      z = nubCoords[2] - coords[2];

      float factor = (float)((radii/ sqrt(x*x+y*y+z*z)));
      double newCoords[3];
      newCoords[0] = x * factor + coords[0];
      newCoords[1] = y * factor + coords[1];
      newCoords[2] = z * factor + coords[2];
      nub->coordinates(newCoords);
      iat1 = nub->index();
      iat2 = atom->index();
      ret->addBond(iat1,iat2,1.0);
      if (numRes > 0) {
        ret->editResidueMode(0, -1);
      }
    }
    delete bonds;

// Copy information on chain labels.

    ret->setChainIDs(this->getChainIDs());
  
    LatticeDef  *lat = this->getLattice();
    if (lat) ret->setLattice(new LatticeDef(*lat));
  } else {
    ret = (Fragment*)0;
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
//
// Description:
//   Create a new Fragment by clipping out the residue specified by the index.
//   Include nubs if the residue is connected to other parts of the fragment.
////////////////////////////////////////////////////////////////////////////////
Fragment* Fragment::clipSelectedResidue(int index)
{
  Fragment *ret;
  vector<TAtm*> *atoms = this->atoms();
  vector<int> selection;
  Residue *res = residue(index);
  int nres = numResidues();
  if (index >= nres-1 || index < 0) {
    ret = (Fragment*)0;
  } else {
    int natoms = atoms->size();
    int ifirst = res->firstAtomPtr()->index();
    int ilast = natoms;
    if (index < nres-1) {
      ilast = residue(index+1)->firstAtomPtr()->index();
    }
    // select all indices between ifirst and ilast
    for (int i=ifirst; i<ilast; i++) {
      selection.push_back(i);
    }
    ret = clipFragment(selection);
  }
  if (atoms) {
    delete atoms;
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
//
// Description
//   Create a character string using the atom index and assign it to the atom
//   type field. Used for tracking reactions
////////////////////////////////////////////////////////////////////////////////
void Fragment::setReactionIndices()
{
  int natoms = p_atoms.size();
  int i;
  char buf[120];
  for (i=0; i<natoms; i++) {
    sprintf(buf,"%d",i+1);
    p_atoms[i]->atomType(buf);
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Description
//   Create a character string using the atom index and assign it to the atom
//   type field. Check first if there are any current indices and only do the
//   operation if there aren't.  This keeps from overriding previous values.
////////////////////////////////////////////////////////////////////////////////
void Fragment::setReactionIndicesIfUnset()
{
  int natoms = p_atoms.size();
  int i;
  char buf[120];
  for (i=0; i<natoms &&
       (p_atoms[i]->atomType()=="" || p_atoms[i]->atomType()=="    "); i++);
  if (i == natoms) {
    for (i=0; i<natoms; i++) {
      sprintf(buf,"%d",i+1);
      p_atoms[i]->atomType(buf);
    }
  }
}

/**
 *  Find first solvent atom index by scanning the residue list, marking the
 *  location of the first solvent residue, and finding the corresponding first
 *  atom;
 */
void Fragment::findFirstSolventAtomIndex()
{
  int i, resmax;

  resmax = p_residueSequence.size();
  if (resmax < 1) return;
  p_firstSolventAtomIndex = -1;
  TAtm *atm;
  int ires = -1;
  for (i=0; i<resmax; i++) {
    if (p_residueSequence[i]->isSolvent()) {
      ires = i;
      break;
    }
  }

  if (ires >= 0) {
    atm = p_residueSequence[ires]->firstAtomPtr();
    if (atm) {
      p_firstSolventAtomIndex = atm->index();
    }
  }
}

/**
 *  Set first solvent residue index
 */
void Fragment::setFirstSolventAtomIndex(int idx)
{
  p_firstSolventAtomIndex = idx;
}

/**
 *  Unset first solvent residue index
 */
void Fragment::clearFirstSolventAtomIndex()
{
  p_firstSolventAtomIndex = -1;
}

/**
 *  Set the value of the number of atoms in a solvent molecule
 */
void Fragment::setSolventNumAtoms(int size)
{
  p_solventNumAtoms = size;
}

/**
 *  Set the number of degrees of freedom for the solvent
 */
void Fragment::setNumDoF(int dof)
{
  p_DoF = dof;
}

/**
 * Remove all atoms outside the two slicing planes. This routine 
 * calculates the projection of the vector separation vector between
 * each atom and the fixed point on the two slice planes defined by the
 * origin vector plus the displacement times the normal vector. If the
 * projection has the opposite sign for the two planes then the atom
 * must lie between them, otherwise the atom is outside the two planes.
 */
bool Fragment::slice(MPoint normal, MPoint origin, double displ1, double displ2)
{
  bool ret = false;

  // Check for valid LatticeDef object
  LatticeDef *lattice = getLattice();
  if (lattice) {

    // Find vector pointing in direction indicated by Miller indices
    // h,k,l and the reciprocal lattice vectors
    normal.normalize();

    //get range
    double range = lattice->displacementRange(normal);

    //get points on slicer planes 
    MPoint orig1 = normal;
    orig1.scale(displ1*range);
    orig1.add(origin);
    MPoint orig2 = normal;
    orig2.scale(displ2*range);
    orig2.add(origin);

    // Identify points that are retained by slicing operation
    int natoms = numAtoms();
    vector<bool> save(natoms,false);
    int i;
    TAtm *atom;
    for (i=0; i<natoms; i++) {
      atom = p_atoms[i];
      const double *coords = atom->coordinates();
      MPoint xyz1;
      xyz1.xyz(coords[0],coords[1],coords[2]);
      xyz1.subtract(orig1);
      double cdot1 = xyz1.dotProduct(normal);
      MPoint xyz2;
      xyz2.xyz(coords[0],coords[1],coords[2]);
      xyz2.subtract(orig2);
      double cdot2 = xyz2.dotProduct(normal);
      if ((cdot1 >= 0.0 && cdot2 <= 0.0) ||
          (cdot2 >= 0.0 && cdot1 <= 0.0)) {
        save[i] = true;
      }
    }
    deleteAllBonds();
    for (i=natoms-1; i>=0; i--) {
      if (!save[i]) {
        deleteAtom(i);
      }
    }
    addCovalentBonds();
    ret = true;
  }
  return ret;
}

void Fragment::convertToFractionalCoords(const bool& unshifted)
{
  // Get basis vectors for unit cell.
  MPoint a, b, c;
  vector<MPoint> *basis = getLattice()->toVectors();
  a.xyz((*(basis))[0]);
  b.xyz((*(basis))[1]);
  c.xyz((*(basis))[2]);
  delete basis;

  double volume = fabs((a.crossProduct1(b)).dotProduct(c));

  // Get vectors corresponding to inverse matrix
  MPoint d = b.crossProduct1(c);
  MPoint e = c.crossProduct1(a);
  MPoint f = a.crossProduct1(b);
  //cout<<"Ainv[1:] "<<d.x()/volume<<" "<<d.y()/volume<<" "<<d.z()/volume<<endl;
  //cout<<"Ainv[2:] "<<e.x()/volume<<" "<<e.y()/volume<<" "<<e.z()/volume<<endl;
  //cout<<"Ainv[3:] "<<f.x()/volume<<" "<<f.y()/volume<<" "<<f.z()/volume<<endl;

  // Get origin of lattice
  MPoint origin = getLattice()->getLatticeCorner();

  int natoms = numAtoms();
  const double *crd;
  int i;
  double f1, f2, f3;
  double new_coord[3];
  
  for (i=0; i<natoms; i++) {
    crd = p_atoms[i]->coordinates();

    if (unshifted) {
      new_coord[0] = crd[0];
      new_coord[1] = crd[1];
      new_coord[2] = crd[2];
    } else {
      new_coord[0] = crd[0] - origin.x();
      new_coord[1] = crd[1] - origin.y();
      new_coord[2] = crd[2] - origin.z();
    }

    //calculate fractional coordinates
    f1 = new_coord[0]*d.x() + new_coord[1]*d.y() + new_coord[2]*d.z();
    f2 = new_coord[0]*e.x() + new_coord[1]*e.y() + new_coord[2]*e.z();
    f3 = new_coord[0]*f.x() + new_coord[1]*f.y() + new_coord[2]*f.z();
    f1 /= volume;
    f2 /= volume;
    f3 /= volume;

    p_atoms[i]->coordinates(f1,f2,f3);
  }
}

void Fragment::convertFromFractionalCoords()
{
  // Get basis vectors for unit cell.
  MPoint a, b, c;
  vector<MPoint> *basis = getLattice()->toVectors();
  a.xyz((*(basis))[0]);
  b.xyz((*(basis))[1]);
  c.xyz((*(basis))[2]);
  delete basis;
 
  int natoms = numAtoms();
  const double *crd;
  int i;
  double x, y, z;
  
  for (i=0; i<natoms; i++) {
    // Assume that crd holds fractional coordinates
    crd = p_atoms[i]->coordinates();
    x = crd[0]*a.x()+ crd[1]*b.x()+crd[2]*c.x();
    y = crd[0]*a.y()+ crd[1]*b.y()+crd[2]*c.y();
    z = crd[0]*a.z()+ crd[1]*b.z()+crd[2]*c.z();
    p_atoms[i]->coordinates(x,y,z);
  }
}

/**
 * Make sure all bonds fragment are of the specified display style.
 * Its faster to go through all of the atoms bond lists and hit
 * bonds multiple times than to generate the minimal bond list.
 */
void Fragment::fixBondStyles(const DisplayStyle& style)
{
   int cnt = p_atoms.size();
   TBond *bond;
   int bcnt;
   int jdx;
   for (int idx=0; idx<cnt; idx++) {
      const vector<TBond*>& bonds = (p_atoms)[idx]->bondList();
      bcnt = bonds.size();
      for (jdx=0; jdx<bcnt; jdx++) {
         bond = bonds[jdx];
         bond->displayStyle(style);
      }
   }

}


/**
 * Returns true if this will be a bonding situation.  Currently that
 * means the user has selected an atom in the existing system and the
 * fragment that have nubs and can thus be bonded.  Later we might look
 * at things like the link #.
 *
 * We need to figure out if we are adding to free space or if we are
 * going to be bonding fragments.  We will try to bond if
 *   . conatom != 0 && conatom == Nub
 *   . conatom != 0 and conatom is not a nub but conatom has at least
 *     one nub (we'll grab the first in the list).
 * The equivalilent must be true on the existing fragment
 *   . nSelected = 1 && m_atomLight[0] == Nub
 *   . nSelected = 1 && m_atomLight[0] is not Nub but has at least
 *     one nub (we'll grab the first in the list).
 *
 * In the case that bonding will occur, the four atoms to bond will be
 * returned otherwise they will be left untouched.
 * @param tmpfrag - fragment that we may bond to
 * @param conAtm - atom in tmpfrag we may connect to
 * @param mainNub, mainAtom - nub or atom in this fragment to connect to
 * @param tmpNub, tmpAtom - nub or atom in this tmpfrag to connect to
 */
bool Fragment::bondingAtoms(const Fragment& tmpfrag,
      int tmpfragconatom, int fragconatom,
      TAtm **mainNub, TAtm **mainAtom,
      TAtm **fragAtom, TAtm **fragNub)
{

   bool ret = false;

   if (numAtoms() > 0 && fragconatom >= 0) {
      TAtm *baseparent=0, *basenub=0;
      TAtm *fragparent=0, *fragnub=0;

      TAtm *tmpfragcatm = 0;
      if (tmpfragconatom >= 0) tmpfragcatm = tmpfrag.atomRef(tmpfragconatom);
      // If we can assign fragparent and fragnub, we just might be able
      // to bond two fragments.  Will have to fill in base* too.
      if (tmpfragcatm && tmpfragcatm->atomicSymbol() == "Nub") {
         fragnub = static_cast<TAtm*>(tmpfragcatm);
         fragparent = tmpfrag.nubParent(fragnub);
      } else if (tmpfragcatm) {
         fragparent = static_cast<TAtm*>(tmpfragcatm);
         fragnub = tmpfrag.childNub(fragparent);
      }

      // Now can we get a baseparent and basenub
      TAtm *batm = atomRef(fragconatom);
      if (batm && batm->atomicSymbol() == "Nub") {
         basenub = batm;
         baseparent = nubParent(basenub);
      } else if (batm && batm->atomicSymbol() == "H") {
         TPerTab tpt;
         // Lets just change this atom to a nub 
         batm->atomicSymbol("Nub");
         batm->atomicNumber( tpt.atomicNumber("Nub"));
         basenub = batm;
         baseparent = 0;
         vector<TAtm*> *bondedAtoms = batm->connectedAtoms();
         if (bondedAtoms->size() >= 1) baseparent = (*bondedAtoms)[0];
         delete bondedAtoms;
      } else {
         baseparent = batm;
         basenub = childNub(baseparent);
      }
      if (baseparent && basenub && fragnub && fragparent) {
         ret = true;
         *mainNub = basenub;
         *mainAtom = baseparent;
         *fragNub = fragnub;
         *fragAtom = fragparent;

      }
   }


   return ret;
}





/**
 * Merge the fragment into this fragment.
 * This complicated method takes into accound the current residue situation.
 *
 *   This command has to act differently depending on the state of the
 *   current system and the type of fragment being added.  Key
 *   information includes:
 *     . is there an existing system
 *     . does it have residues
 *     . does the incoming frag have residues
 *     . is there a residue number that is currently being edited.
 * @param tmpfrag - object to be merged in
 * @param conatom - atom to connect to (if connecting, otherwise x,y,z
 * @param x,y,z - location for offsetting a molecule
 */
void Fragment::mergeFragment(Fragment& tmpfrag, 
                   int tmpfragconatom, int fragconatom,
                   double x, double y, double z)
{

   // We'll slect to add a manipulate if the existing frag already has atoms
   // and the new fragment is being added to free space.
   // This is so we can offset the newly loaded atoms if we want to (for
   // continuous past like the original builder.  I'm not sure its a
   // good idea.

   // Cache the atom pointers for later use.  We can't assume they get
   // added at the end anymore.
   vector<TAtm*> *newAtoms = tmpfrag.atoms();

   double cmx = 0.0;
   double cmy = 0.0;
   double cmz = 0.0;

   tmpfrag.centerOfMass(cmx, cmy, cmz);

   if (numAtoms() == 0 && numResidues() == 0) {
      if (x!=0.0 || y!=0.0 || z!=0.0) {
        tmpfrag.translate(x-cmx,y-cmy,z-cmz);
      }
      absorb(tmpfrag,true);
      //    cout << "time to do absorb" << timer.elapsedTime() << endl;
   } else {

      TAtm *basenub, *baseparent, *fragparent, *fragnub;
      basenub = baseparent = fragparent = fragnub = NULL;
      bool canDoBonding = false;
      canDoBonding = bondingAtoms(tmpfrag, tmpfragconatom, fragconatom,  
            &basenub,&baseparent, &fragparent, &fragnub);

      int numExistingResidues = numResidues();
      int numNewResidues = tmpfrag.numResidues();
      int curResIndex = editResidueIndex();

      if (canDoBonding) {
         if (numExistingResidues > 0) {
            if (curResIndex >= 0) {
               tmpfrag.clearResidueInfo();
               //append atoms to current residue
               absorbAtoms(tmpfrag,true);
            } else {
               // Oh boy, no current residue so we have to be smart
               // We don't currently save link# info in atom so skip that
               // logic
               int parentlink = -1, fraglink = -1;
               parentlink = baseparent->connectionIndex();
               fraglink = fragparent->connectionIndex();
               if (parentlink == 1 && fraglink == 2) {
                  // prepend residue before baseparent residue
                  absorb(tmpfrag,baseparent,false);
               } else if (parentlink == 2 && fraglink == 1) {
                  // append residue after baseparent residue
                  absorb(tmpfrag,baseparent,true);
               } else {
                  // If we are linking two amino acids or two DNA or RNA
                  // bases then we will try and figure out the correct order
                  bool linkKnownResidue = false;
                  TResItem::Classification ftype = TResItem::Other;
                  TResItem::Classification ptype = TResItem::Other;
                  TResTab restab;
                  if (numResidues() > 0) {
                     ptype = restab.verify(baseparent->getResidue()->
                           name().c_str());
                     ftype = restab.verify(fragparent->getResidue()->
                           name().c_str());
                  }
                  if ((ftype == TResItem::DNA || ftype == TResItem::RNA ||
                           ftype == TResItem::RNAorDNA) &&
                        (ptype == TResItem::DNA || ptype == TResItem::RNA ||
                         ptype == TResItem::RNAorDNA)) {
                     if (fragparent->atomName() == " P  " &&
                           baseparent->atomName() == " O3*") {
                        absorb(tmpfrag,baseparent,true);
                        linkKnownResidue = true;
                     } else if (fragparent->atomName() == " O3*" &&
                           baseparent->atomName() == " P  ") {
                        absorb(tmpfrag,baseparent,false);
                        linkKnownResidue = true;
                     }
                  } else if (ftype == TResItem::AminoAcid &&
                        ptype == TResItem::AminoAcid) {
                     if (fragparent->atomName() == " N  " &&
                           baseparent->atomName() == " C  ") {
                        absorb(tmpfrag,baseparent,true);
                        linkKnownResidue = true;
                     } else if (fragparent->atomName() == " C  " &&
                           baseparent->atomName() == " N  ") {
                        absorb(tmpfrag,baseparent,false);
                        linkKnownResidue = true;
                     }
                  }
                  if (!linkKnownResidue) {
                     if (numNewResidues > 0) {
                        // add new residues and atoms at end of frag
                        // BUT I THINK ADD TO parentatom
                        // create new residue using Bruces method then 
                        // move atoms into it
                        // delete tmpfrags residues but not atoms/bonds
                        absorb(tmpfrag,baseparent,true);
                     } else {
                        // create new residue at end
                        int newresidx = insertResidue(NULL);
                        editResidueMode(true,newresidx);
                        absorbAtoms(tmpfrag,true);
                        editResidueMode(false,-1);
                     }
                  }
               }
               reIndexResidues();
            }
         } else {
            // No existing residues
            tmpfrag.clearResidueInfo();
            absorb(tmpfrag,true);
         }
         // do bonding - angle determination buried within doBond
         renumberAtoms(0);
         FragUtil fragutil(this);
         fragutil.doBond(*newAtoms,basenub,baseparent,fragparent,fragnub);


      } else {
         tmpfrag.translate(x-cmx,y-cmy,z-cmz);
         // No bonding action - easier
         if (numExistingResidues > 0) {
            if (curResIndex >= 0) {
               tmpfrag.clearResidueInfo();
               // todo append to curResIndex
               absorbAtoms(tmpfrag, true);
            } else {
               if (numNewResidues > 0) {
                  // add residues and their atoms to end of frag
                  absorb(tmpfrag,NULL,true);
               } else {
                  //create new residue at end
                  int newresidx = insertResidue(NULL);
                  editResidueMode(true,newresidx);
                  //add atoms to new residue
                  absorbAtoms(tmpfrag,true);
                  editResidueMode(false,-1);
               }
               reIndexResidues();
            }
         } else { 
            // no existing residues
            tmpfrag.clearResidueInfo();
            absorb(tmpfrag,true);
         }
         renumberAtoms(0);
      }


   }


   findFirstSolventAtomIndex();
   delete newAtoms;
}


/**
 * Translate all atoms by the vector x,y,z
 */
void Fragment::translate(double x, double y, double z)
{
   int natoms = numAtoms();
   for (int i=0; i<natoms; i++) {
      const double *coords = p_atoms[i]->coordinates();
      p_atoms[i]->coordinates(coords[0]+x,coords[1]+y,coords[2]+z);
   }
}

