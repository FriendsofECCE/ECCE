//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: ForceField.C
//
//
// DESIGN:
//
//                 ForceField
//                     <>
//       ----------------------------------------
//       |            |       |         |       |
//     PBond        Angle  Torsion  Inversion NonBond
//
//     A ForceField consists of lists of Bonds, Angles, Torsions, 
//     Inversions, and NonBond pairs. 
//
//     Geometry optimization is carried out using the energy and
//     force functions, via a steepest-descent/conjugate-gradient
//     algorithm implemented in the CGMinimizer class.
//
//     The UFFParams class provides parameters for the energy and force
//     functions from the so-called "Universal ForceField" of
//     Rappe' and co-workers (J. Am. Chem. Soc. 1992, v114, p10024-10035)
//     For now, only molecular systems are supported; R&G have published 
//     extensions, including one for metals, that could be added later.  
///////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <iostream>
  using std::cout;
  using std::endl;

#include "util/ErrMsg.H"
#include "util/ETimer.H"
#include "util/ProgressMonitor.H"
#include "util/CancelException.H"

#include "tdat/TPerTab.H"
#include "tdat/TAtm.H"
#include "tdat/ForceField.H"
#include "tdat/PBonds.H"
#include "tdat/Angles.H"
#include "tdat/Torsions.H"
#include "tdat/Inversions.H"
#include "tdat/NonBond.H"
#include "tdat/UFFParams.H"
#include "tdat/CGMinimizer.H"
#include "tdat/FFRules.H"
#include "tdat/LinkCellList.H"
#include "tdat/CGEnergyFunction.H"

//#define FORCETIMER
//#define ENERGYTIMER
//#define CONSTRUCTIONTIMER
//#define TOTALTIMER

#ifdef TOTALTIMER
ETimer total_timer;
#endif

#ifdef FORCETIMER
ETimer force_timer;
#endif

#ifdef CONSTRUCTIONTIMER
ETimer construction_timer;
#endif

#ifdef ENERGYTIMER
ETimer energy_timer;
#endif

// these macros allow various contributions to the energy and force
// to be turned off.  Left in for further work.
//#ifndef INSTALL  (global make; don't set here)
//#define DEBUG   (turns on couts if not INSTALL - slows way down)
//#define BONDOFF
//#define ANGLEOFF
//#define TORSIONOFF
//#define INVERSIONOFF
//#define NONBONDOFF
// #endif

#define  PI  3.14159265358979323846
#define  DEG * PI / 180.0

// must match order of force field category types in FFCategory.H
enum effTypes { eAtomType=0, eCrossType, eBondType, eAngleType,
                eProperType, eImproperType, effTypeCount}; 
                //  effTypeCount must be the last item in enum list
// Note the ForceField does not do anything about Cross force fields

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------

//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//        Constructor from Fragment. Initialization of all bond, angle lists
//        etc. is buried in constructors for p_bonds, p_angles, etc.
//  TODO: We will eventually need a constructor that takes an additional
//        vector<bool> argument that can be used to mask which atoms are to
//        have their forces calculated.
////////////////////////////////////////////////////////////////////////////// 
ForceField::ForceField(Fragment* chemsys, FFRules *rules,
                       ProgressMonitor* monitor) 
  : p_chemsys(chemsys)
{

#ifdef CONSTRUCTIONTIMER
  construction_timer.start();
#endif

#ifdef TOTALTIMER
  total_timer.start();
#endif

  EE_ASSERT( chemsys, EE_FATAL,
	     "Null Fragment pointer passed to ForceField constructor");

  p_coordinates = new FFCoordinates(chemsys->numAtoms(),3,
                                    chemsys->coordinates());
  
  p_rules = rules;
  p_atmptrs = chemsys->atoms();
  p_bonds = new PBonds(p_atmptrs, p_rules);
  p_angles = new Angles(p_atmptrs, p_rules);
  p_torsions = new Torsions(p_atmptrs, p_rules);
  vector<int> atomRings;
  ETimer timer;
  timer.start();
  atomRings = chemsys->findRingAtoms();  // crashing topol ed, slh 3/22/06
  timer.stop();
  //cout << "Ring timer " << timer.elapsedTime() << " " << timer.CPUTime() << endl;
  //p_inversions = new Inversions(p_atmptrs, p_rules, (vector<int>*)0);
  p_inversions = new Inversions(p_atmptrs, p_rules, &atomRings);
  p_nonbond = new NonBond(p_atmptrs);
  p_nonBondCutoff = defaultCoulCutoff;
  p_monitor = monitor;
  p_timer.start();
  p_interval = 0.0;
}

ForceField::ForceField(Fragment* chemsys, vector<int> *selection, FFRules *rules,
                       ProgressMonitor* monitor) 
  : p_chemsys(chemsys)
{

#ifdef CONSTRUCTIONTIMER
  construction_timer.start();
#endif

#ifdef TOTALTIMER
  total_timer.start();
#endif

  bool halfAtoms=false;
  if (selection->size() != 0) {
    int i, natom;
    natom = p_chemsys->numAtoms();
    if (natom > 0 && ((double)selection->size())/((double)natom)<0.5)
      halfAtoms = true;
    else halfAtoms = false;
    for (i=0; i<natom; i++) p_mask.push_back(false);
    for (i=0; i<selection->size(); i++) p_mask[(*selection)[i]] = true;
  } else {
    int i;
    int acnt = 0;
    int natom = p_chemsys->numAtoms();
    for (i=0; i<natom; i++) {
      if (p_chemsys->atomRef(i)->displayStyle().isDisplayed()) {
        p_mask.push_back(true);
        acnt++;
      } else {
        p_mask.push_back(false);
      }
    }
    if (acnt != 0) {
      if (natom > 0 && ((double)selection->size())/((double)natom)<0.5)
        halfAtoms = true;
      else halfAtoms = false;
    } else {
      p_mask.clear();
    }
  }
  EE_ASSERT( chemsys, EE_FATAL,
	     "Null Fragment pointer passed to ForceField constructor");

  p_coordinates = new FFCoordinates(chemsys->numAtoms(),3,
                                    chemsys->coordinates());
  
  p_atmptrs = chemsys->atoms();
  p_rules = rules;
  if (p_mask.size() > 0) {
    p_bonds = new PBonds(p_atmptrs, &p_mask, p_rules);
    p_angles = new Angles(p_atmptrs, &p_mask, p_rules);
    p_torsions = new Torsions(p_atmptrs, &p_mask, p_rules);
    p_inversions = new Inversions(p_atmptrs, &p_mask, p_rules);
    p_nonbond = new NonBond(p_atmptrs, &p_mask, halfAtoms);
  } else {
    p_bonds = new PBonds(p_atmptrs, p_rules);
    p_angles = new Angles(p_atmptrs, p_rules);
    p_torsions = new Torsions(p_atmptrs, p_rules);
    p_inversions = new Inversions(p_atmptrs, p_rules);
    p_nonbond = new NonBond(p_atmptrs);
  }
  p_nonBondCutoff = defaultCoulCutoff;
  p_monitor = monitor;
  p_timer.start();
  p_interval = 0.0;
}
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Copy constructor.
////////////////////////////////////////////////////////////////////////////// 
ForceField::ForceField( const ForceField& forceField) 
{ *this = forceField; }


//  ---------- Destructor ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      ForceField destructor. Clearing memory is mostly handle in p_bonds,
//      p_angles, etc. destructors.
////////////////////////////////////////////////////////////////////////////// 
ForceField::~ForceField(void)
{
  cleanup();
}

//  ---------- Destructor ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Simple accessors to recover Fragment and atomic coordinates
//////////////////////////////////////////////////////////////////////////////
Fragment* ForceField::chemSys(void) const
{ return p_chemsys; }
const FFCoordinates* ForceField::coordinates(void) const
{ return p_coordinates; }

void ForceField::cleanup()
{
   if (p_bonds) {
      delete p_bonds;
      p_bonds = 0;
   }
   if (p_angles) {
      delete p_angles;
      p_angles = 0;
   }
   if (p_torsions) {
      delete p_torsions;
      p_torsions = 0;
   }
   if (p_inversions) {
      delete p_inversions;
      p_inversions = 0;
   }
   if (p_nonbond) {
      delete p_nonbond;
      p_nonbond = 0;
   }

   // Don't actually destroy the TAtm ptrs, they belong to the Fragment
   p_atmptrs->clear();
   if (p_atmptrs) {
      delete p_atmptrs;
      p_atmptrs = 0;
   }
}


// ---------- General Methods  ----------
/**
 * Returns force vector at the passed-in geometry.  
 * @param canceled set to true if the function terminated early (user 
 *        cancel).  With the fancy function pointers, I found that throwing
 *        and catching exceptions did not work at this level in the code.
 *        I left the InvalidException in place anyway because its a program
 *        debug mechanism.
 */
FFCoordinates* ForceField::force( bool& canceled,
                                  const FFCoordinates* coords,
                                  FFCoordinates* argForce)
{
#ifdef FORCETIMER
    force_timer.start();
#endif
  FFCoordinates* result = (FFCoordinates*)0;

  if ( argForce == (FFCoordinates*)0 ) {
    result = new FFCoordinates(coords->rows(),3);
  } else {
    EE_ASSERT(( argForce->columns() == 3 &&
                argForce->rows() == coords->rows()), EE_FATAL,
                "no. of columns not 3 or rows not num_atoms");
    result = argForce;
  }

  // This may be unnecessary, but it is not expensive.
  result = result->zero();

  ////////////////////////////////////////////////////////////////////////
  // contribution from Bond interactions
  ////////////////////////////////////////////////////////////////////////
#ifndef BONDOFF
  p_bonds->forces(coords, result);
  if (checkInterrupt()) {
     canceled = true;
     return result;
  }
#endif
  ////////////////////////////////////////////////////////////////////////
  // contribution from Angle interactions
  ////////////////////////////////////////////////////////////////////////
#ifndef ANGLEOFF
  p_angles->forces(coords, result);
  if (checkInterrupt()) {
     canceled = true;
     return result;
  }
#endif
  ////////////////////////////////////////////////////////////////////////
  // contribution from Torsion interactions
  ////////////////////////////////////////////////////////////////////////
#ifndef TORSIONOFF
  p_torsions->forces(coords, result);
  if (checkInterrupt()) {
     canceled = true;
     return result;
  }
#endif
  ////////////////////////////////////////////////////////////////////////
  // contribution from Inversion interactions
  ////////////////////////////////////////////////////////////////////////
#ifndef INVERSIONOFF
  p_inversions->forces(coords, result);
  if (checkInterrupt()) {
     canceled = true;
     return result;
  }
#endif
  ////////////////////////////////////////////////////////////////////////
  // contribution from nonbonded pairs 
  ////////////////////////////////////////////////////////////////////////
#ifndef NONBONDOFF
  p_nonbond->forces(coords, result, p_nonBondCutoff);
  if (checkInterrupt()) {
     canceled = true;
     return result;
  }
#endif

#ifdef FORCETIMER
  force_timer.stop();
#endif
  //fdCheck(coords, result, p_nonBondCutoff);
  return result;
}

/**
 * Returns the energy at the passed-in geometry.  
 * If a null geometry pointer is passed (the default), the current 
 * geometry stored in the state is used.
 *
 * @param canceled set to true if the function terminated early (user 
 *        cancel).  With the fancy function pointers, I found that throwing
 *        and catching exceptions did not work at this level in the code.
 *        I left the InvalidException in place anyway because its a program
 *        debug mechanism.
 */
double ForceField::energy(bool& canceled, const FFCoordinates* coords )
{
#ifdef ENERGYTIMER
  energy_timer.start();
#endif
  double result = 0.0;
  ////////////////////////////////////////////////////////////////////////////// 
  //  Energy from harmonic bonds
  ////////////////////////////////////////////////////////////////////////////// 
#ifndef BONDOFF
  result += p_bonds->energy(coords);
  if (checkInterrupt()) {
     canceled = true;
     return result;
  }
#endif
  ////////////////////////////////////////////////////////////////////////////// 
  //  Energy from angles
  ////////////////////////////////////////////////////////////////////////////// 
#ifndef ANGLEOFF
  result += p_angles->energy(coords);
  if (checkInterrupt()) {
     canceled = true;
     return result;
  }
#endif
  ////////////////////////////////////////////////////////////////////////////// 
  //  Energy from torsions
  ////////////////////////////////////////////////////////////////////////////// 
#ifndef TORSIONOFF
  result += p_torsions->energy(coords);
  if (checkInterrupt()) {
     canceled = true;
     return result;
  }
#endif
  ////////////////////////////////////////////////////////////////////////////// 
  //  Energy from inversions
  ////////////////////////////////////////////////////////////////////////////// 
#ifndef INVERSIONOFF
  result += p_inversions->energy(coords);
  if (checkInterrupt()) {
     canceled = true;
     return result;
  }
#endif
  ////////////////////////////////////////////////////////////////////////////// 
  //  Energy from non-bonded interactions
  ////////////////////////////////////////////////////////////////////////////// 
#ifndef NONBONDOFF
  result += p_nonbond->energy(coords, p_nonBondCutoff);
  if (checkInterrupt()) {
     canceled = true;
     return result;
  }
#endif

#ifdef ENERGYTIMER
  energy_timer.stop();
#endif
  return result;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//
//      Optimize geometry for nsteps steps, or until the max row norm of the 
//      gradient is less than convForceNormAU.  Note that convForceNormAU is
//      assumed to be in Hartree/Bohr.  Operates on passed-in geometry FFCoordinates.
//
//  Implementation:
//      Uses Simple steepest-descent for a "few" steps; then 
//      conjugate-gradient.  CG works best; SD will probably be removed,
//      and a Hessian update added.
//
////////////////////////////////////////////////////////////////////////////// 
void ForceField::optimize(int  maxSteps,
    double  convForceNormAU,
    double  nonBondCutoff,
    double  lineSearchRange,
    int  maxLineSteps,
    double  lineTol ) throw (CancelException)
{

  // convert from Hartree/Bohr
  double convForceNorm = convForceNormAU * 331.841730348;
  p_nonBondCutoff = nonBondCutoff;
  FFCoordinates* coords = new FFCoordinates(p_chemsys->numAtoms(), 3,
      p_chemsys->coordinates());

#if (!defined(INSTALL) && defined(DEBUG))
  cout << "   maxSteps: " << maxSteps << endl;
  cout << "   ForceField:  convForceNorm: " << convForceNorm << endl;
  cout << "   lineSearchRange: " << lineSearchRange << endl;
  cout << "   maxLineSteps: " << maxLineSteps << endl;
  cout << "   lineTol: " << lineTol << endl;
  cout << "   nonBondCutoff: " << nonBondCutoff << endl;
#endif

  ////////////////////////////////////////////////////////////////////////
  // clean up any superimposed atoms
  ////////////////////////////////////////////////////////////////////////
  moveOverLappingAtoms( coords );

  ////////////////////////////////////////////////////////////////////////
  // conjugate gradient minimization
  ////////////////////////////////////////////////////////////////////////
  if ( maxSteps > 0 ) {
    CGEnergyFunction *eFunction = new CGEnergyFunction;
    eFunction->setForceField(this);
    CGMinimizer cgMin(eFunction,coords);

    coords = cgMin.optimize(convForceNorm,
        maxSteps,
        lineSearchRange,
        lineTol,
        maxLineSteps);
    p_chemsys->coordinates((const double*)coords->coordinates()[0],
          p_chemsys->numAtoms());
    delete eFunction;
  }
#ifdef ENERGYTIMER
  cout << "Total Energy       Time : " << energy_timer.elapsedTime() << endl;
#endif
#ifdef FORCETIMER
  cout << "Total Force        Time : " << force_timer.elapsedTime() << endl;
#endif
#ifdef TOTALTIMER
  total_timer.stop();
  cout << "Total ForceField   Time : " << total_timer.elapsedTime() << endl;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Check to see of a sufficient time has passed since last interrupt. If
//       so call isInterrupt again.
////////////////////////////////////////////////////////////////////////////////
bool ForceField::checkInterrupt()
{
  bool ret=false;
  p_interval += p_timer.elapsedTime();
  if (p_interval > 0.10) {
    if (p_monitor) {
      ret = p_monitor->isInterrupted();
    }
    p_interval = 0.0;
  }
  p_timer.start();
  return ret;
}
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Use link cell method to look for overlapping atoms and move them
//       apart to avoid divide by zeros in the force field calculation.
////////////////////////////////////////////////////////////////////////////////
void ForceField::moveOverLappingAtoms(FFCoordinates* coords)
{
  double maxR = 0.2;
  int  nAtoms = p_atmptrs->size();
  int i, j;
  bool foundOverlaps = true;
  vector<int> ngbrList;
  vector<int> ngbrLast;

  // If only zero or one atoms, return
  if (nAtoms < 2) return;

  while (foundOverlaps) {
    foundOverlaps = false;
    if (ngbrList.size() > 0) ngbrList.clear();
    if (ngbrLast.size() > 0) ngbrLast.clear();
    MPoint p1,p2;

    TPerTab tpt;
    int ghost = 0;
    TAtm *atm1, *atm2;

    LinkCellList cellList(p_atmptrs, maxR);

    const int *link_list = cellList.getLinkCellList();
    const int *header = cellList.getHeader();

    // Initialize last neighbors
    for (i = 0; i < nAtoms; i++) {
      ngbrLast.push_back(-1);
    }

    // Construct list of neighbors. Start by looping over all atoms.

    int icnt = -1;
    for (i = 0; i < nAtoms; i++) {
      // Guarantee that ngbrLast[i] is pointing to something even
      // if atom has no neighbors
      ngbrLast[i] = icnt;
      atm1 = (*p_atmptrs)[i];

      p1.xyz(coords->row(i));

      int itag = atm1->atomicNumber();
      // Ignore ghost atoms.
      if (itag == ghost) continue;

      // Construct bonds to atoms in same cell as atom i

      j = link_list[i];
      while (j >= 0) {

        atm2 = (*p_atmptrs)[j];
        int jtag = atm2->atomicNumber();
        if (jtag != ghost) {
          p2.xyz(coords->row(j));
          p2.subtract(p1);
          if (p2.length() <= maxR) {
            ngbrList.push_back(j);
            icnt++;
            ngbrLast[i] = icnt;
            foundOverlaps = true;
          }
        }
        j = link_list[j];
      }

      // Find neighbors in cells neighboring the cell containing
      // atom i. Start by locating index of cell containing atom i.

      int ix = cellList.xIndex(p1.x());
      int iy = cellList.yIndex(p1.y());
      int iz = cellList.zIndex(p1.z());
      int l = cellList.getCellIndex(ix, iy, iz);

      int kxmin = cellList.ixMin(ix);
      int kxmax = cellList.ixMax(ix);

      for (int kx = kxmin; kx <= kxmax; kx++) {
        int kymin = cellList.iyMin(iy);
        int kymax = cellList.iyMax(iy);
        for (int ky = kymin; ky <= kymax; ky++) {
          int kzmin = cellList.izMin(iz);
          int kzmax = cellList.izMax(iz);
          for (int kz = kzmin; kz <= kzmax; kz++) {
            int k = cellList.getCellIndex(kx, ky, kz);

            // Make sure that we only calculate bonds once. Accomplish this
            // by only constructing bonds between cells for which k > l.

            if (k > l) {
              j = header[k];
              while (j >= 0) {

                atm2 = (*p_atmptrs)[j];
                int jtag = atm2->atomicNumber();
                if (jtag != ghost) {
                  p2.xyz(coords->row(j));
                  p2.subtract(p1);
                  if (p2.length() <= maxR) {
                    ngbrList.push_back(j);
                    icnt++;
                    ngbrLast[i] = icnt;
                    foundOverlaps = true;
                  }
                }
                j = link_list[j];
              }
            }
          }
        }
      }
    }
    if (foundOverlaps) {
      int ifirst, ilast, jat, k;
      double dc[3];
      double rmaxinv = 1.0/(double)RAND_MAX;
      for (i=0; i<nAtoms; i++) {
        if (i==0) ifirst = 0;
        else ifirst = ngbrLast[i-1]+1;
        ilast = ngbrLast[i];
        for (j=ifirst; j<=ilast; j++) {
          jat = ngbrList[j];
          for (k=0; k<3; k++) {
            dc[k] = 2.0*moveScale*(rand()*rmaxinv-0.5);
          }
          coords->add(jat,dc);
        }
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Test forces by finite difference at passed-in geometry.
//      Typical delta values are 1.e-1 to 1.e-8.
//
////////////////////////////////////////////////////////////////////////////// 
void ForceField::fdCheck(const FFCoordinates* coords,
    FFCoordinates* result, double cutoff)
{
  bool canceled = false;
  double energy0 = energy(canceled, coords);
  if (canceled) throw CancelException(WHERE);
  int natoms = p_chemsys->numAtoms();
  int i;
  const double *fa;
  double crd[3];
  double fn[3], delta, energy1;
  delta = 0.000001;
  FFCoordinates *tmpcrd = new FFCoordinates(*coords);
  for (i=0; i<natoms; i++) {
    if (p_mask.size() == 0 || p_mask[i]) {
      fa = result->row(i);
      crd[0] = delta;
      crd[1] = 0.0;
      crd[2] = 0.0;
      tmpcrd->add(i,crd);
      energy1 = energy(canceled, tmpcrd);
      if (canceled) throw CancelException(WHERE);
      crd[0] = -delta;
      tmpcrd->add(i,crd);
      crd[0] = 0.0;
      fn[0] = -(energy1-energy0)/delta;

      crd[1] = delta;
      tmpcrd->add(i,crd);
      energy1 = energy(canceled, tmpcrd);
      if (canceled) throw CancelException(WHERE);
      crd[1] = -delta;
      tmpcrd->add(i,crd);
      crd[1] = 0.0;
      fn[1] = -(energy1-energy0)/delta;

      crd[2] = delta;
      tmpcrd->add(i,crd);
      energy1 = energy(canceled, tmpcrd);
      if (canceled) throw CancelException(WHERE);
      crd[2] = -delta;
      tmpcrd->add(i,crd);
      crd[2] = 0.0;
      fn[2] = -(energy1-energy0)/delta;

      cout<<"atom["<<i<<"]: fa[0] "<<fa[0]<<" fn[0] "<<fn[0]<<endl;
      cout<<"         fa[1] "<<fa[1]<<" fn[1] "<<fn[1]<<endl;
      cout<<"         fa[2] "<<fa[2]<<" fn[2] "<<fn[2]<<endl;
    }
  }
  delete tmpcrd;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns count of items depending on force field category.
//
// Param: catID is the index of a force field category, eg, atom, bond.
//
// Return: count of force field items for the given force field category.
//  
////////////////////////////////////////////////////////////////////////////////
int ForceField::getCount( int catID) const
{
  int count = 0;

  switch (catID)  {
    case eAtomType:     count = p_atmptrs->size();
                        break;
    case eBondType:     count = p_bonds->getCount();
                        break;
    case eAngleType:    count = p_angles->getCount();
                        break;
    case eProperType:   count = p_torsions->getCount();
                        break;
    case eImproperType: count = p_inversions->getCount();
    default:            break;
  }
  return count;
}

////////////////////////////////////////////////////////////////////////////////
//  Description: Returns the force field name specified by the category index 
//               and item index. The name should be deleted by the caller.
//  
// Param: catID is the index of a force field category, eg, atom, bond.
// Param: idx   is an index of an item for the specified force field category.
//
// Return: pointer to character string containing name of item specified by
//         the input category index and item index.
//
////////////////////////////////////////////////////////////////////////////////
const string ForceField::getName( int catID, int idx) const
{
  string name;

  switch (catID)  {
    case eAtomType:     if ( isValidID( eAtomType, idx))
                          name = (*p_atmptrs)[idx]->cAtomName();
                        break;
    case eBondType:     name = p_bonds->getString( idx, PBonds::BOND_NAME);
                        break;
    case eAngleType:    name = p_angles->getName( idx); 
                        break;
    case eProperType:   name = p_torsions->getName( idx); 
                        break;
    case eImproperType: name = p_inversions->getName( idx); 
    default:            break;
  }
  return name;
}

////////////////////////////////////////////////////////////////////////////////
//  Description: Returns the type name specified by the input ff category id, 
//               e.g., atom, bond, ..., and the item index.  If the type id or 
//               item index are invalid, null is returned.
//  
// Param: catID is the index of a force field category, eg, atom, bond.
// Param: idx   is an index of an item for the specified force field category.
//
// Return: type of force field specified.  An empty string is returned if the 
//         category index and/or item index is invalid.
//
////////////////////////////////////////////////////////////////////////////////
const string ForceField::getType( int catID, int idx) const
{
  string type;

  switch ( catID)  {
    case eAtomType:     //if ( isValidID( eAtomType, idx))
      type = (*p_atmptrs)[idx]->cAtomType();
      break;
    case eBondType:     type = p_bonds->getString( idx, PBonds::BOND_TYPE);
                        break;
    case eAngleType:    type = p_angles->getType( idx); 
                        break;
    case eProperType:   type = p_torsions->getType( idx); 
                        break;
    case eImproperType: type = p_inversions->getType( idx); 
    default:            break;
  }
  return type;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Pushes atom index indicated by input index onto the end of the 
//              vector.  The input index must be a valid index into the indices
//              of atoms.
//
// Param: idx is index into array of atom indices.
// Param: atoms is a vector of atom indices
// Return: none
//
////////////////////////////////////////////////////////////////////////////////
void ForceField::pushAtom( int idx, vector<int>& atoms)  {
  if ( isValidID( eAtomType, idx))
    atoms.push_back((*p_atmptrs)[idx]->index());
}

////////////////////////////////////////////////////////////////////////////////
// Description: Pushes atom and bond indices indicated by input bond index 
//              onto the end of the referenced vectors.  The input index must 
//              be a valid bond index.
//
// Param: idx is index into the p_bonds object.
// Param: atoms is a vector of atom indices
// Param: bonds is a vector of bond indices
// Return: none
//
////////////////////////////////////////////////////////////////////////////////
void ForceField::pushBond( int idx, vector<int>& atoms, 
    vector<int>& bonds)  {
  int atom1 = -1, atom2 = -1;

  p_bonds->getAtomIndices( idx, &atom1, &atom2);
  if ( atom1 != -1 && atom2 != -1)  {
    atoms.push_back( atom1);
    atoms.push_back( atom2);

    TBond* tbond = p_chemsys->bond( atom1, atom2);
    bonds.push_back( tbond->index());
  }
}

////////////////////////////////////////////////////////////////////////////////
// Description: Pushes atom and bond indices indicated by input angle index 
//              onto the end of the referenced vectors.  The input index must 
//              be a valid angle index.
//
// Param: idx is index into the p_angles object.
// Param: atoms is a vector of atom indices
// Param: bonds is a vector of bond indices
// Return: none
//
////////////////////////////////////////////////////////////////////////////////
void ForceField::pushAngle( int idx, vector<int>& atoms,
    vector<int>& bonds)  {
  int atom1 = -1, atom2 = -1, atom3 = -1;

  p_angles->getAtomIndices( idx, &atom1, &atom2, &atom3);
  if ( atom1 != -1 && atom2 != -1 && atom3 != -1)  {
    atoms.push_back( atom1);
    atoms.push_back( atom2);
    atoms.push_back( atom3);

    TBond* tbond = p_chemsys->bond( atom1, atom2);
    bonds.push_back( tbond->index());
    tbond = p_chemsys->bond( atom2, atom3);
    bonds.push_back( tbond->index());
  }
}

////////////////////////////////////////////////////////////////////////////////
// Description: Pushes atom and bond indices indicated by input proper index 
//              onto the end of the referenced vectors.  The input index must 
//              be a valid proper index.
//
// Param: idx is index into the p_torsions object.
// Param: atoms is a vector of atom indices
// Param: bonds is a vector of bond indices
// Return: none
//
////////////////////////////////////////////////////////////////////////////////
void ForceField::pushProper( int idx, vector<int>& atoms,
    vector<int>& bonds)  {
  int atom1 = -1, atom2 = -1, atom3 = -1, atom4 = -1;

  p_torsions->getAtomIndices( idx, &atom1, &atom2, &atom3, &atom4);
  if ( atom1 != -1 && atom2 != -1 && atom3 != -1 && atom4 != -1) {
    atoms.push_back( atom1);
    atoms.push_back( atom2);
    atoms.push_back( atom3);
    atoms.push_back( atom4);

    TBond *tbond = p_chemsys->bond( atom1, atom2);
    bonds.push_back( tbond->index());
    tbond = p_chemsys->bond( atom2, atom3);
    bonds.push_back( tbond->index());
    tbond = p_chemsys->bond( atom3, atom4);
    bonds.push_back( tbond->index());
  }
}

////////////////////////////////////////////////////////////////////////////////
// Description: Pushes atom and bond indices indicated by input improper index 
//              onto the end of the referenced vectors.  The input index must 
//              be a valid improper index.
//
// Param: idx is index into the p_inversions object.
// Param: atoms is a vector of atom indices
// Param: bonds is a vector of bond indices
// Return: none
//
////////////////////////////////////////////////////////////////////////////////
void ForceField::pushInversion( int idx, vector<int>& atoms,
    vector<int>& bonds)  {
  int atom1 = -1, atom2 = -1, atom3 = -1, atom4 = -1;

  p_inversions->getAtomIndices( idx, &atom1, &atom2, &atom3, &atom4);
  if ( atom1 != -1 && atom2 != -1 && atom3 != -1 && atom4 != -1) {
    atoms.push_back( atom1);
    atoms.push_back( atom2);
    atoms.push_back( atom3);
    atoms.push_back( atom4);

    TBond *tbond = p_chemsys->bond( atom3, atom4);
    bonds.push_back( tbond->index());
    tbond = p_chemsys->bond( atom3, atom1);
    bonds.push_back( tbond->index());
    tbond = p_chemsys->bond( atom3, atom2);
    bonds.push_back( tbond->index());
  }
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves the atoms and bonds associated with the indexed item
//              in specified force field category.
//
// Param: catID is index into the force field categories
// Param: idx is index into the items of the specified category.
// Param: atoms is a vector of atom indices
// Param: bonds is a vector of bond indices
// Return: none
//
////////////////////////////////////////////////////////////////////////////////
void ForceField::getComponentIndices( int catID, int idx,
    vector<int>& atoms, vector<int>& bonds)
{
  switch ( catID)  {
    case eAtomType:
      pushAtom( idx, atoms);
      break;
    case eBondType:
      pushBond( idx, atoms, bonds);
      break;
    case eAngleType:
      pushAngle( idx, atoms, bonds);
      break;
    case eProperType: 
      pushProper( idx, atoms, bonds);
      break;
    case eImproperType:
      pushInversion( idx, atoms, bonds);
    default:  break;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves the atoms and bonds associated with the indexed items
//              in specified force field category.
//
// Param: catID is index into the force field categories
// Param: idxs is a vector of indices into the items of the specified category.
// Param: atoms is a vector of atom indices
// Param: bonds is a vector of bond indices
// Return: none
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void ForceField::getComponentIndices( int catID, vector<int>& idxs,
    vector<int>& atoms, vector<int>& bonds)
{
  vector<int>::iterator iter = idxs.begin();
  switch ( catID)  {
    case eAtomType:
      while ( iter != idxs.end()) {
        pushAtom( *iter, atoms);
        iter++;
      }
      break;
    case eBondType:
      while ( iter != idxs.end()) {
        pushBond( *iter, atoms, bonds);
        iter++;
      }
      break;
    case eAngleType:
      while ( iter != idxs.end()) {
        pushAngle( *iter, atoms, bonds);
        iter++;
      }
      break;
    case eProperType: 
      while ( iter != idxs.end()) {
        pushProper( *iter, atoms, bonds);
        iter++;
      }
      break;
    case eImproperType:
      while ( iter != idxs.end()) {
        pushInversion( *iter, atoms, bonds);
        iter++;
      }
    default:  break;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the atomic symbol of the indexed atom.  The ff
//              category is assumed to be Atoms. If the index is invalid, the
//              assert forces a fatal error.
//
// Param: idx   is an index of an item for the atom force field category.
//  
// Return: String containing the atomic symbol of the indexed atom.
//
////////////////////////////////////////////////////////////////////////////////
string ForceField::getAtomSymbol( int idx) const
{
  EE_ASSERT( idx >= 0 && idx < p_atmptrs->size(), EE_FATAL, 
      "ForceField:::getAtomSymbol() called with invalid index");

  return (*p_atmptrs)[idx]->atomicSymbol();
}

////////////////////////////////////////////////////////////////////////////////
//  Description: Returns the atomic number specified by the input index.  The
//               ff category is assumed to be Atoms. Fails if the index is 
//               invalid.
//  
// Param: idx   is an index of an item for the atom force field category.
//
// Return: Atomic number of the indexed atom.
//
////////////////////////////////////////////////////////////////////////////////
int ForceField::getAtomicNumber( int idx) const
{
  EE_ASSERT( idx >= 0 && idx < p_atmptrs->size(), EE_FATAL, 
      "ForceField:::getAtomicNumber() called with invalid index");

  return (*p_atmptrs)[idx]->atomicNumber();
}

////////////////////////////////////////////////////////////////////////////////
//  Description: Prints to stderr retrieved atom indices for the first matching
//               force field type name for the specified force field category
//               and type name.
//
// Param: catID     is the index indicating the parameter type, eg, atom, bond.
// Param: className is the name of a force field class name, for an atom type a
//                  sample class name is CT.
// Return: none.
//
////////////////////////////////////////////////////////////////////////////////
void ForceField::printIndices( int catID, const char* className)
{
  EE_ASSERT( className != 0, EE_FATAL, 
      "ForceField::printIndices() received null pointer for className");

  char* itemClass = 0;
  //   const char* thisClass = 0;
  string thisClass;
  int *indices = 0;
  int count = 0;
  int i = 0;
  switch ( catID)  {
    case eAtomType:
      count = p_atmptrs->size();
      for ( i=0;i<count;i++)  {
        if ( ( itemClass = (*p_atmptrs)[i]->cAtomType()) != 0)  {
          if ( strcmp( itemClass, className) == 0)  {
            cout << "Index: " << i << endl;
            delete [] itemClass;
            break;
          }
          delete [] itemClass;
        }
      }
      break;
    case eBondType:  
      count = p_bonds->getCount();
      for ( i=0;i<count;i++) {
        //              if ((thisClass = p_bonds->getString( i,PBonds::BOND_TYPE)) != 0) {
        thisClass = p_bonds->getString( i,PBonds::BOND_TYPE);
        if ( strcmp( thisClass.c_str(), className) == 0) {
          if ( (indices = p_bonds->getAtomOrderedIndices( i)) != 0) {
            cout << "Index1: " << ( indices[0]+1) 
              << ", Index2: " << ( indices[1]+1) << endl;
            delete [] indices;
          }
          break;               // stop search at first match
        }
      }
      break;
        case eAngleType: 
      count = p_angles->getCount();
      for ( i=0;i<count;i++) {
        thisClass = p_angles->getType( i);
        if ( strcmp( thisClass.c_str(), className) == 0) {
          if ( (indices = p_angles->getAtomOrderedIndices(i)) != 0) {
            cout << "Index1: " << ( indices[0]+1) 
              << ", Index2: " << ( indices[1]+1)
              << ", Index3: " << ( indices[2]+1) << endl;
            delete [] indices;
          }
          break;              // stop search at first match
        }
      }
      break;
        case eProperType: 
      count = p_torsions->getCount();
      for ( i=0;i<count;i++) {
        thisClass = p_torsions->getType( i);
        if ( strcmp( thisClass.c_str(), className) == 0) {
          if ( (indices = p_torsions->getAtomOrderedIndices(i)) != 0) {
            cout << "Index1: "   << (indices[0] + 1)
              << ", Index2: " << (indices[1] + 1) 
              << ", Index3: " << (indices[2] + 1)
              << ", Index4: " << (indices[3] + 1) << endl;
            delete [] indices;
          }
          break;
        }
      }
      break;
        case eImproperType: 
      count = p_inversions->getCount();
      for ( i=0;i<count;i++) {
        thisClass = p_inversions->getType( i);
        if ( strcmp( thisClass.c_str(), className) == 0)  {
          if ( (indices = p_inversions->getAtomOrderedIndices(i)) != 0) {
            cout << "Index1: "   << ( indices[0]+1) 
              << ", Index2: " << ( indices[1]+1) 
              << ", Index3: " << ( indices[2]+1) 
              << ", Index4: " << ( indices[3]+1) << endl;
            delete [] indices;
          }
          break;
        }
      }
        default: break;
      }
  }

  // ----------------Private Methods ---------------------------------------------
  ////////////////////////////////////////////////////////////////////////////////
  //  Description: Returns true if the input idx is a valid index for the input 
  //               type.
  //  
  ////////////////////////////////////////////////////////////////////////////////
  bool ForceField::isValidID( int type, int idx) const
  {
    if ( idx >= 0 && idx < getCount( type))
      return true;
    return false;
  }
