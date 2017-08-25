///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: NonBond.C
//
//
// DESIGN:
//              <Discuss overall design/purpose of class.>
//   (See NonBond.H)
//
///////////////////////////////////////////////////////////////////////////////

// system includes
#include <math.h>

// application includes
#include "util/Transformations.H"
#include "util/ErrMsg.H"
#include "tdat/NonBond.H"
#include "tdat/FFCoords.H"
#include "tdat/UFFParams.H"
#include "tdat/TBond.H"
#include "tdat/TAtm.H"
#include "tdat/TPerTab.H"
#include "tdat/LinkCellList.H"

#define PI  3.14159265358979323846
#define DEG * PI / 180.0

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Construct the NonBond object from a list of atom pointers that can be
//      obtained from the geometry fragment. The parameters are obtained from
//      the UFFParams class.
////////////////////////////////////////////////////////////////////////////// 

static const double energyHartreeToKcalMol = 331.84160493;
NonBond::NonBond(vector<TAtm*>* atmptrs)
{
  p_atmptrs = atmptrs;
  p_mask = (vector<bool>*)0;
  p_oldCoords = (FFCoordinates*)0;
  p_halfAtoms = false;
  initExclusionList();
  init3rdNeighborList();
  setAtomParameters();
}

NonBond::NonBond(vector<TAtm*>* atmptrs, vector<bool>* mask,
                 bool halfatoms)
{
  p_atmptrs = atmptrs;
  if (mask->size() > 0) {
    p_mask = mask;
  } else {
    p_mask = (vector<bool>*)0;
  }
  p_oldCoords = (FFCoordinates*)0;
  p_halfAtoms = halfatoms;
  initExclusionList();
  init3rdNeighborList();
  setAtomParameters();
}
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      NonBondcopy constructor.  Uses assignment operator.
////////////////////////////////////////////////////////////////////////////// 
NonBond::NonBond(const NonBond& nonbond)
{ (*this) = nonbond; }

 
// ---------- Virtual Destructor ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Simple destructor.
////////////////////////////////////////////////////////////////////////////// 
NonBond::~NonBond(void)
{
  int i;
  int isize = p_exclsnList.size();
  for (i=0; i<isize; i++) delete p_exclsnList[i];
  isize = p_thrdNgbrList.size();
  for (i=0; i<isize; i++) delete p_thrdNgbrList[i];
  delete p_oldCoords;
}

// ---------- General Methods  ----------
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Construct a list containing all the excluded atoms around a given atom
//    (atoms that are excluded from the non-bonded neighbor list because they
//    are bonded or closely linked via bonding networks to the center atom)
//    as well as a list of pointers to the last atom in each list of excluded
//    atoms.
////////////////////////////////////////////////////////////////////////////////
void NonBond::initExclusionList(void)
{
  int natoms = p_atmptrs->size();
  TAtm *atm1, *atm2, *atm3;
  int iat1, iat2, iat3, iat4;
  int n2, n3, n4;
  int size2, size3, size4;
  vector<TAtm*> *list2, *list3, *list4;
  for (iat1=0; iat1<natoms; iat1++) {
    set<int,less<int> >* exSet = new set<int,less<int> >;
    p_exclsnList.push_back(exSet);
    if (p_mask == (vector<bool>*)0 || ((*p_mask)[iat1] && p_halfAtoms) ||
        !p_halfAtoms) {
      atm1 = (*p_atmptrs)[iat1];
      list2 = atm1->connectedAtoms();
      size2 = list2->size();
      for (iat2=0; iat2<size2; iat2++) {
        n2 = (*list2)[iat2]->index();
        if (n2 != iat1) {
          exSet->insert(n2);
          atm2 = (*list2)[iat2];
          list3 = atm2->connectedAtoms();
          size3 = list3->size();
          for (iat3=0; iat3<size3; iat3++) {
            n3 = (*list3)[iat3]->index();
            if (n3 != n2 && n3 != iat1) {
              exSet->insert(n3);
              atm3 = (*list3)[iat3];
              list4 = atm3->connectedAtoms();
              size4 = list4->size();
              for (iat4=0; iat4<size4; iat4++) {
                n4 = (*list4)[iat4]->index();
                if (n4 != n3 && n4 != n2 && n4 != iat1) {
                  exSet->insert(n4);
                }
              }
              delete list4;
            }
          }
          delete list3;
        }
      }
      delete list2;
    }
  }
//  cout << "Finished setting up exclusion list" << endl;
//  for (iat1=0; iat1<p_exclsnList.size(); iat1++) {
//    cout << "atom["<<iat1<<"]: "<<p_exclsnList[iat1]->size()<<endl;
//  }
}
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Construct a list containing all the 3rd neighbors around a given atom
//    (atoms separated from the given atom by 3 bonds).
////////////////////////////////////////////////////////////////////////////////
void NonBond::init3rdNeighborList(void)
{
  int natoms = p_atmptrs->size();
  TAtm *atm1, *atm2;
  int iat1, iat2, iat3, iat4, n2, n3, n4;
  int size2, size3, size4;
  vector<TAtm*> *list2, *list3, *list4;
  for (iat1=0; iat1<natoms; iat1++) {
    // set up exclusion list containing everything up to second neighbors
    set<int,less<int> > exSet;
    vector<int> scndNgbr;
    if (p_mask == (vector<bool>*)0 || ((*p_mask)[iat1] && p_halfAtoms) ||
        !p_halfAtoms) {
      atm1 = (*p_atmptrs)[iat1];
      exSet.insert(iat1);
      list2 = atm1->connectedAtoms();
      size2 = list2->size();
      for (iat2=0; iat2<size2; iat2++) {
        atm2 = (*list2)[iat2];
        n2 = atm2->index();
        exSet.insert(n2);
      }
      for (iat2=0; iat2<size2; iat2++) {
        atm2 = (*list2)[iat2];
        n2 = atm2->index();
        list3 = atm2->connectedAtoms();
        size3 = list3->size();
        for (iat3=0; iat3<size3; iat3++) {
          n3 = (*list3)[iat3]->index();
          if (exSet.count(n3) == 0) {
            scndNgbr.push_back(n3);
          }
          exSet.insert(n3);
        }
        delete list3;
      }
      delete list2;
    }
    size3 = scndNgbr.size();
    vector<int>* thrdList = new vector<int>;
    p_thrdNgbrList.push_back(thrdList);
    if (p_mask == (vector<bool>*)0 || ((*p_mask)[iat1] && p_halfAtoms) ||
        !p_halfAtoms) {
      for (iat3=0; iat3<size3; iat3++) {
        list4 = (*p_atmptrs)[scndNgbr[iat3]]->connectedAtoms();
        size4 = list4->size();
        for (iat4=0; iat4<size4; iat4++) {
          n4 = (*list4)[iat4]->index();
          if (exSet.count(n4) == 0) {
          // use this condition to avoid duplicates
            if (iat1 < n4) {
              thrdList->push_back(n4);
            }
          }
        }
        delete list4;
      }
    }
  }
//  cout << "Finished setting up third neighbor list" << endl;
//  for (iat1=0; iat1<p_thrdNgbrList.size(); iat1++) {
//    cout << "atom["<<iat1<<"]:";
//    for (iat2=0; iat2<p_thrdNgbrList[iat1]->size(); iat2++) {
//      cout <<" "<<(*p_thrdNgbrList[iat1])[iat2];
//    }
//    cout << endl;
//  }
}
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Construct the neighbor list for evaluating non-bonded interactions.
//    This uses a linked-cell method to keep from doing an order N^2 scan over
//    all pairs of atoms in the system.
////////////////////////////////////////////////////////////////////////////////
void NonBond::getNeighborList(const FFCoordinates* coords, double cutoff)
{
  double skin = 1.0;
  // Check to see if neighbor list needs to be rebuilt
  bool makelist = false;
  int  nAtoms = p_atmptrs->size();
  int i, j;
  if (p_oldCoords == (FFCoordinates*)0) {
    p_oldCoords = new FFCoordinates(nAtoms,3);
    p_oldCoords = p_oldCoords->zero();
    makelist = true;
  }
  // if coordinates have change signigicantly since last neighbor list update,
  // rebuild neighbor list
  if (!makelist) {
    double skin2 = skin/2.0;
    skin2 = skin2*skin2;
    double r2, dr[3];
    const double *c1, *c2;
    for (i = 0; i < nAtoms; i++) {
      if (p_mask == (vector<bool>*)0 || (*p_mask)[i] ) {
        c1 = p_oldCoords->row(i);
        c2 = coords->row(i);
        r2 = 0.0;
        for (j=0; j<3; j++) dr[j] = c1[j] - c2[j];
        for (j=0; j<3; j++) r2 += dr[j]*dr[j];
        if (r2 > skin2) makelist = true;
      }
    }
  }
  if (!makelist) return;
  if (p_ngbrList.size() > 0) p_ngbrList.clear();
  if (p_ngbrLast.size() > 0) p_ngbrLast.clear();

// If only zero or one atoms, return
  if (nAtoms < 2) return;

  MPoint p1,p2;

  TPerTab tpt;
  int nub = tpt.nubAtom();
  int ghost = 0;
  set<int,less<int> >* exSet;

  double maxR;
  maxR = cutoff+skin;

  LinkCellList cellList(p_atmptrs, maxR);

  const int *link_list = cellList.getLinkCellList();
  const int *header = cellList.getHeader();

  TAtm *atm1, *atm2;

  // Initialize list of last neighbors.
  for (i = 0; i < nAtoms; i++) {
    p_ngbrLast.push_back(-1);
  }

// Construct list of neighbors. Start by looping over all atoms.

  int icnt = -1;
  for (i = 0; i < nAtoms; i++) {
    // Guarantee that p_ngbrLast[i] is pointing to something even
    // if atom has no neighbors
    p_ngbrLast[i] = icnt;
    if (p_mask == (vector<bool>*)0 || ((*p_mask)[i] && p_halfAtoms) ||
        !p_halfAtoms) {
      exSet = p_exclsnList[i];
      atm1 = (*p_atmptrs)[i];

      p1.xyz(atm1->coordinates());

      int itag = atm1->atomicNumber();
      // Ignore ghost atoms and nubs.
      if (itag == ghost || itag == nub) continue;

// Add interaction from atoms in same cell as atom i
 
      if (!p_halfAtoms) {
        // starting j at link_list[i] is to avoid duplicates
        j = link_list[i];
      } else {
        int ix = cellList.xIndex(p1.x());
        int iy = cellList.yIndex(p1.y());
        int iz = cellList.zIndex(p1.z());
        int l = cellList.getCellIndex(ix, iy, iz);

        j = header[l];
      }
      while (j >= 0) {

        atm2 = (*p_atmptrs)[j];
        int jtag = atm2->atomicNumber();
        if (jtag != ghost && jtag != nub && exSet->count(j) == 0) {
          // i<j condition is to avoid duplicates
          if (p_mask == (vector<bool>*)0) {
            p2.xyz(atm2->coordinates());
            p2.subtract(p1);
            if (p2.length() <= maxR) {
              p_ngbrList.push_back(j);
              icnt++;
              p_ngbrLast[i] = icnt;
            }
          } else if (((*p_mask)[i] || (*p_mask)[j]) && i != j) {
            // make sure to not to overcount interaction if both i and j
            // are in highlight vector and p_halfAtoms is true
            if (!p_halfAtoms || ((*p_mask)[i] && (*p_mask)[j] && i<j)) {
              p2.xyz(atm2->coordinates());
              p2.subtract(p1);
              if (p2.length() <= maxR) {
                p_ngbrList.push_back(j);
                icnt++;
                p_ngbrLast[i] = icnt;
              }
            }
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

            if (k > l || (p_halfAtoms && k != l)) {
              j = header[k];
              while (j >= 0) {

                atm2 = (*p_atmptrs)[j];
                int jtag = atm2->atomicNumber();
                if (jtag != ghost && jtag != nub && exSet->count(j) == 0) {
                  if (p_mask == (vector<bool>*)0) {
                    p2.xyz(atm2->coordinates());
                    p2.subtract(p1);
                    if (p2.length() <= maxR) {
                      p_ngbrList.push_back(j);
                      icnt++;
                      p_ngbrLast[i] = icnt;
                    }
                  } else if ((*p_mask)[i] || (*p_mask)[j]) {
                    // make sure to not to overcount interaction if both 
                    // i and j are in highlight vector and p_halfAtoms
                    // is true
                    if (!p_halfAtoms || ((*p_mask)[i] &&
                          (*p_mask)[j] && i<j)) {
                      p2.xyz(atm2->coordinates());
                      p2.subtract(p1);
                      if (p2.length() <= maxR) {
                        p_ngbrList.push_back(j);
                        icnt++;
                        p_ngbrLast[i] = icnt;
                      }
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
  }

  // save values of current coordinates
  (*p_oldCoords) = (*coords);
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Assign parameters for non-bonding interactions to all atoms using the
//    Universal Force Field.
//////////////////////////////////////////////////////////////////////////////
void NonBond::setAtomParameters(void)
{
  UFFParams *parameterserver = new UFFParams(p_atmptrs);
  if (p_epsln.size() > 0) p_epsln.clear();
  if (p_sigma.size() > 0) p_sigma.clear();
  if (p_charge.size() > 0) p_charge.clear();
  int natom = p_atmptrs->size();
  int iat, itype;
  for (iat = 0; iat<natom; iat++) {
    itype = parameterserver->uffIndex(iat);
    p_epsln.push_back(sqrt(parameterserver->vdWEnergy(itype)));
    p_sigma.push_back(sqrt(parameterserver->vdWRadius(itype)));
    p_charge.push_back(parameterserver->partialCharge(iat));
  }
  delete parameterserver;
}
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Returns contribution to the forces from non-bonded interactions.
//       Interactions from 3rd neighbors do no include Coulomb interactions.
//       Note that the Universal Force Field does not use the usual definition
//       of the Lennard-Jones potential.
////////////////////////////////////////////////////////////////////////////// 
void NonBond::forces(const FFCoordinates* coords, FFCoordinates* result,
    double cutoff)
{
  int iat,jat,i,j,ifirst,ilast,natom;
  double r,epsln,sigma,qtot,dr[3],cutoff2,scalar;
  double e1,s1,q1,rinv,risig,r2,r3,r6,r12,ri2;
  double force1[3],force2[3];

  // Evaluate forces from non-bonded interactions
  getNeighborList(coords, cutoff);
  natom = p_atmptrs->size();
  const double *c1;
  const double *c2;
  cutoff2 = cutoff*cutoff;
  for (iat=0; iat<natom; iat++) {
    c1 = coords->row(iat);
    if (iat==0) ifirst=0;
    else ifirst = p_ngbrLast[iat-1]+1;
    ilast= p_ngbrLast[iat];
    e1 = p_epsln[iat];
    s1 = p_sigma[iat];
    q1 = p_charge[iat];
    for (j=ifirst; j<=ilast; j++) {
      jat = p_ngbrList[j];
      if (p_mask == (vector<bool>*)0 ||
         ((*p_mask)[iat] || (*p_mask)[jat])) {
        c2 = coords->row(jat);
        for (i=0; i<3; i++) dr[i] = c1[i]-c2[i];
        r = 0.0;
        for (i=0; i<3; i++) r += dr[i]*dr[i];
        if (r < cutoff2) {
          r = sqrt(r);
          epsln = e1*p_epsln[jat];
          sigma = s1*p_sigma[jat];
          qtot = q1*p_charge[jat];
          rinv = 1.0/r;
          risig = sigma*rinv;
          ri2 = rinv*rinv;
          r2 = risig*risig;
          r3 = r2*risig;
          r6 = r3*r3;
          r12 = r6*r6;
          scalar = 12.0*epsln*(r12-r6)*ri2;
          scalar += energyHartreeToKcalMol*qtot*rinv*rinv*rinv;
          for (i=0; i<3; i++) force1[i] = dr[i]*scalar;
          for (i=0; i<3; i++) force2[i] = -force1[i];
          if (p_mask == (vector<bool>*)0) {
            result->add(iat,force1);
            result->add(jat,force2);
          } else {
            if ((*p_mask)[iat]) result->add(iat,force1);
            if ((*p_mask)[jat]) result->add(jat,force2);
          }
        }
      }
    }
  }
  // Evaluate forces from third neighbor interactions.
  // These are the same as the interactions above except that the
  // Coulomb interactions are set to zero.
  int size;
  for (iat=0; iat<natom; iat++) {
    if (p_halfAtoms && (*p_mask)[iat]) {
      size = p_thrdNgbrList[iat]->size();
      c1 = coords->row(iat);
      e1 = p_epsln[iat];
      s1 = p_sigma[iat];
      for (j=0; j<size; j++) {
        jat = (*p_thrdNgbrList[iat])[j];
        c2 = coords->row(jat);
        for (i=0; i<3; i++) dr[i] = c1[i]-c2[i];
        r = 0.0;
        for (i=0; i<3; i++) r += dr[i]*dr[i];
        if (r < cutoff2) {
          r = sqrt(r);
          epsln = e1*p_epsln[jat];
          sigma = s1*p_sigma[jat];
          rinv = 1.0/r;
          risig = sigma*rinv;
          ri2 = rinv*rinv;
          r2 = risig*risig;
          r3 = r2*risig;
          r6 = r3*r3;
          r12 = r6*r6;
          scalar = 12.0*epsln*(r12-r6)*ri2;
          for (i=0; i<3; i++) force1[i] = dr[i]*scalar;
          for (i=0; i<3; i++) force2[i] = -force1[i];
          if (p_mask == (vector<bool>*)0) {
            result->add(iat,force1);
            result->add(jat,force2);
          } else {
            if ((*p_mask)[iat]) result->add(iat,force1);
            if ((*p_mask)[jat]) result->add(jat,force2);
          }
        }
      }
    }
  }
  return;
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Returns contribution to the energy from non-bonded interactions.
////////////////////////////////////////////////////////////////////////////// 
double NonBond::energy(const FFCoordinates* coords, double cutoff)
{
  int iat,jat,i,j,ifirst,ilast,natom;
  double r,epsln,sigma,qtot,dr[3],cutoff2;
  double e1,s1,q1,rinv,risig,r2,r3,r6,r12;
  double scalar = 0.0;

  // Evaluate energies from non-bonded interactions
  getNeighborList(coords, cutoff);
  natom = p_atmptrs->size();
  const double *c1;
  const double *c2;
  cutoff2 = cutoff*cutoff;
  for (iat=0; iat<natom; iat++) {
    c1 = coords->row(iat);
    if (iat==0) ifirst=0;
    else ifirst = p_ngbrLast[iat-1]+1;
    ilast= p_ngbrLast[iat];
    e1 = p_epsln[iat];
    s1 = p_sigma[iat];
    q1 = p_charge[iat];
    for (j=ifirst; j<=ilast; j++) {
      jat = p_ngbrList[j];
      if (p_mask == (vector<bool>*)0 ||
         ((*p_mask)[iat] || (*p_mask)[jat])) {
        c2 = coords->row(jat);
        for (i=0; i<3; i++) dr[i] = c1[i]-c2[i];
        r = 0.0;
        for (i=0; i<3; i++) r += dr[i]*dr[i];
        if (r<cutoff2) {
          r = sqrt(r);
          epsln = e1*p_epsln[jat];
          sigma = s1*p_sigma[jat];
          qtot = q1*p_charge[jat];
          rinv = 1.0/r;
          risig = sigma*rinv;
          r2 = risig*risig;
          r3 = r2*risig;
          r6 = r3*r3;
          r12 = r6*r6;
          scalar += epsln*(r12-2.0*r6);
          scalar += energyHartreeToKcalMol*qtot*rinv;
        }
      }
    }
  }
  // Evaluate energies from third neighbor interactions.
  // These are the same as the interactions above except that the
  // Coulomb interactions are set to zero.
  int size;
  for (iat=0; iat<natom; iat++) {
    if (p_halfAtoms && (*p_mask)[iat]) {
      size = p_thrdNgbrList[iat]->size();
      c1 = coords->row(iat);
      e1 = p_epsln[iat];
      s1 = p_sigma[iat];
      for (j=0; j<size; j++) {
        jat = (*p_thrdNgbrList[iat])[j];
        c2 = coords->row(jat);
        for (i=0; i<3; i++) dr[i] = c1[i]-c2[i];
        r = 0.0;
        for (i=0; i<3; i++) r += dr[i]*dr[i];
        if (r < cutoff2) {
          r = sqrt(r);
          epsln = e1*p_epsln[jat];
          sigma = s1*p_sigma[jat];
          rinv = 1.0/r;
          risig = sigma*rinv;
          r2 = risig*risig;
          r3 = r2*risig;
          r6 = r3*r3;
          r12 = r6*r6;
          scalar += epsln*(r12-2.0*r6);
        }
      }
    }
  }
  return scalar;
}
