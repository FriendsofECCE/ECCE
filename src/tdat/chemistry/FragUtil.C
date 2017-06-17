//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: FragUtil.C
//
//
// DESIGN:
//
///////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <iostream>
  using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>  // isdigit
#include <math.h>       // floor()
#include <limits.h>
#include <iomanip>


#include "util/NullPointerException.H"
#include "util/SFile.H"
#include "util/STLUtil.H"
#include "util/TempStorage.H"
#include "util/Transformations.H"

#include "tdat/FragUtil.H"
#include "tdat/Fragment.H"
#include "tdat/TFormula.H"
#include "tdat/TTCPair.H"
#include "tdat/TPerTab.H"
#include "tdat/ShapeData.H"
#include "tdat/TBond.H"
#include "tdat/TAtm.H"
#include "tdat/Residue.H"
#include "tdat/TResTab.H"
#include "tdat/TResItem.H"
#include "tdat/Measures.H"

#define PI 3.14159265358979323846

// File static for convenience
static ShapeData s_shapes;


///////////////////////////////////////////////////////////////////////////////
//  Description:
//    Constructor
///////////////////////////////////////////////////////////////////////////////
FragUtil::FragUtil(Fragment* frag) 
{
  p_frag = frag;
}

///////////////////////////////////////////////////////////////////////////////
//  Description:
//    Destructor
///////////////////////////////////////////////////////////////////////////////
FragUtil::~FragUtil(void)
{
  p_frag = (Fragment*)0;
}
///////////////////////////////////////////////////////////////////////////////
//  Description:
//    Add atom at a given location and maintain residue information.
///////////////////////////////////////////////////////////////////////////////
TAtm *FragUtil::addAtLocation
(
   const char *element,
   const char *geom,
   double atomX, double atomY, double atomZ,
   TAtm *parent
)
{
  Fragment *frag = p_frag;
  double xyz[3];

  // First create the thing and add it in
  xyz[0] = atomX;
  xyz[1] = atomY;
  xyz[2] = atomZ;
  TAtm *atom = new TAtm(element,xyz);
  atom->shapeString(geom);

  frag->addAtom(atom,parent);

  // Now can add the nubs
  // First we have to get the geometries...
  ShapeData shapes;
  int numCoords = shapes.getNumberOfAtoms(geom);
  const double *coords = shapes.getGeometryCoordinates(geom);
  NULLPOINTEREXCEPTION(coords, string("No coordinates from shape class")+geom);

  // Need to get this stuff dynamically
  TPerTab tpt;
  static const float scale = 0.5;  
  int atomNum = tpt.atomicNumber(element);
  float radii = tpt.covalentRadius(atomNum);

  // "Lone" doesn't have any coords - the rest will
  double factor = 1.0;;
  TBond *bond;
  TAtm* nub;
  for (int idx=0; idx<numCoords; idx++) {
    factor = scale * (radii / sqrt(pow(coords[idx*3],2) +
          pow(coords[(idx*3)+1],2) + pow(coords[(idx*3)+2],2)));

    xyz[0] = atomX + factor * coords[idx*3];
    xyz[1] = atomY + factor * coords[(idx*3)+1];
    xyz[2] = atomZ + factor * coords[(idx*3)+2];

    nub = new TAtm("Nub",xyz);
    nub->shape(ShapeData::terminal);
    frag->addAtom(nub,atom);

    bond = new TBond(atom, nub, TBond::Single);
    bond->displayStyle(frag->getMainDisplayStyle()) ;
  }

  return atom;
}

/////////////////////////////////////////////////////////////////////////////
// Description
//   Change an atom shape.  The difficult part is deciding where to put
//   new nubs.  The easiest way is to start with a new atom and its nubs.
//
//   NOTE: the atom identity cannot be changed by this operation or it
//   will mess up other scene graph objects.
//   After looking through the existing implementation, I noticed that
//   the nub identies do get changed.  This is bad and could potentiall
//   cause crashes.  Since it hasn't so far, I'll just leave it.  A crash
//   would occur if the nub was involved in a measure or one of the other
//   nodes (manipulator, line...).  Doesn't seem all that likely.
//
//   Error checking about the sutibility of this change is assumed to
//   have taken place before this method is called.
/////////////////////////////////////////////////////////////////////////////
void FragUtil::changeGeometry(TAtm *orig_atom, 
    const string& symbol, const string& newGeom)
{
  Fragment *frag = p_frag;
  vector<TAtm*> *catoms = 0;
  vector<TAtm*> nmlist;

  // Step 1. 
  // Delete nubs on the atom;  The shape is temporarily inconsistent
  frag->deleteNubs(orig_atom);

  // Step 2. 
  // Create a new atom of the desired type with its nubs 
  // The addAtLocation actually adds it and its nubs to the frag.
  const double *coords = orig_atom->coordinates();
  double x,y,z;
  x = coords[0];
  y = coords[1];
  z = coords[2];
  TAtm *newAtom = addAtLocation(symbol.c_str(),newGeom.c_str(),x,y,z,NULL);
  TAtm *newNub1 = 0;
  TAtm *newNub2 = 0;

  // Step 3. 
  // Rotate the new atom and nub to lie along old atom and bond (if there
  // is one that isn't a metal-pi bond). If there is more than one bond,
  // just pick the first.
  nonMetalBonds(orig_atom,nmlist);
  if (nmlist.size() >= 1) {

    TAtm *batom = nmlist[0];

    // This must be valid or we wouldn't have gotten this far.
    //newNub1 = frag->atomRef(nidx+1);
    //we've gotta get the first nub on our newAtom
    catoms = newAtom->connectedAtoms();
    newNub1 = (*catoms)[0];

    MPoint oldVector, tmpVector, newVector, shift;
    MMatrix rMatrix, sMatrix;
    oldVector.xyz(orig_atom->coordinates());
    tmpVector.xyz(batom->coordinates());
    oldVector.subtract(tmpVector);

    newVector.xyz(newAtom->coordinates());
    tmpVector.xyz(newNub1->coordinates());
    newVector.subtract(tmpVector);
    rMatrix.vvRotateMatrix(newVector, oldVector);

    //now we can get and effect the rotation matrix.
    shift.xyz(newAtom->coordinates());
    shift.scale(-1);
    sMatrix.shiftMatrix(shift);
    frag->transform(*catoms,sMatrix);
    frag->transform(*catoms,rMatrix);
    shift.scale(-1);
    sMatrix.shiftMatrix(shift);
    frag->transform(*catoms,sMatrix);
    delete catoms;
  }

  // Step 4.
  // rotate the other bonds into place (if any)
  nonMetalBonds(orig_atom,nmlist);
  MPoint oldXYZ, newXYZ;
  double len =0 ,minLen= 999;
  int theAngle;
  if (nmlist.size() > 1) {

    //newNub2 = frag->atomRef(nidx+2); // must be valid
    catoms = newAtom->connectedAtoms();
    newNub2 = (*catoms)[1];

    // now we must rotate along the nub-newAtom bond to find a fit
    //to the second bond
    oldXYZ.xyz(nmlist[1]->coordinates());
    //catoms = newAtom->connectedAtoms();  // won't be metal??

    for (int i=0; i<360 ; i=i+5) {
      //rotate along bond
      frag->rotateAboutBond(newNub1, newAtom, i);
      newXYZ.xyz(newNub2->coordinates());
      //how close is it to the old bond
      newXYZ.subtract(oldXYZ);
      len=newXYZ.length();
      if (len < minLen) {
        minLen=len;
        theAngle=i;
      }
      frag->rotateAboutBond(newNub1, newAtom, -i);
    }
    //now we know where the best position is...
    frag->rotateAboutBond(newNub1, newAtom, theAngle);
  }



   // Step 5.
   // Delete our unwanted nubs from the new atom (ie where-ever we placed
  // a bond.
  nonMetalBonds(orig_atom,nmlist);
  int totalBonds = nmlist.size();
  int i;
  for (i=0; i<totalBonds; i++) {
    TAtm *theNub = 0;
    nonMetalBonds(orig_atom, nmlist);
    oldXYZ.xyz(nmlist[i]->coordinates());
    nonMetalBonds(newAtom, nmlist);
    minLen= 999;
    int ncnt = nmlist.size();
    for (int j=0; j<ncnt; j++) {
      newXYZ.xyz(nmlist[j]->coordinates());
      newXYZ.subtract(oldXYZ);
      len= newXYZ.length();
      if (len < minLen) {
        minLen=len;
        theNub=nmlist[j];
      }
    }
    //now delete this nub
    if (theNub != 0) {
      frag->deleteAtom(theNub);
    }
  }


  // Step 6.
  // Wherever there is a nub on the new new atom, add one to the old
  // atom.
  catoms = newAtom->connectedAtoms();
  int nAtoms = catoms->size();

  TAtm *onub;
  TAtm *nnub;
  for (i=0; i<nAtoms; i++) {
    onub = (*catoms)[i];
    nnub = new TAtm(*onub);
    TBond *bond = new TBond(orig_atom,nnub,1.0);
    bond->displayStyle(frag->getMainDisplayStyle()) ;
    frag->addAtom(nnub,orig_atom);
  }


   // Step 7.
   // Delete our new atom and its nubs
  for (i=0; i<nAtoms; i++) {
    frag->deleteAtom((*catoms)[i]);
  }
  frag->deleteAtom(newAtom);
  delete catoms;
}

////////////////////////////////////////////////////////////////////////////
// Description
//   Return list of atoms connected to "atom" by a regular (ie non metal)
//   bond.
////////////////////////////////////////////////////////////////////////////
void FragUtil::nonMetalBonds(TAtm *atom, vector<TAtm*>& list)
{
  list.clear();

  vector<TBond*> bonds = atom->bondList();
  TAtm *atm1, *atm2;
  TBond *bond;
  for (int idx=0; idx<bonds.size(); idx++) {
    bond = bonds[idx];
    if (bond->order() != TBond::Metal) {
      atm1 = bond->atom1();
      atm2 = bond->atom2();
      if (atm1 == atom) {
        list.push_back(atm2);
      } else {
        list.push_back(atm1); 
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////
// Description
//   Return list of atoms, excluding nubs, that are connected to "atom" by
//   a regular (ie non metal) bond.
////////////////////////////////////////////////////////////////////////////
void FragUtil::nonMetalnonNubBonds(TAtm *atom, vector<TAtm*>& list)
{
  list.clear();

  vector<TBond*> bonds = atom->bondList();
  TPerTab tpt;
  int nub = tpt.nubAtom();
  TAtm *atm1, *atm2;
  TBond *bond;
  for (int idx=0; idx<bonds.size(); idx++) {
    bond = bonds[idx];
    if (bond->order() != TBond::Metal) {
      atm1 = bond->atom1();
      atm2 = bond->atom2();
      if (atm1 == atom) {
        if (atm2->atomicNumber() != nub) list.push_back(atm2);
      } else {
        if (atm1->atomicNumber() != nub) list.push_back(atm1); 
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// Description
//   Adjust location of nubs on atom so that they correspond to a sensible
//   orientation based on the orientation of existing bonds. This is similar
//   to the changeGeometry method, but assumes that the atom already exists
//   and contains the correct number of nubs. This is designed to be used
//   in the add hydrogens operation in the MD toolkit.
//
/////////////////////////////////////////////////////////////////////////////
void FragUtil::adjustExistingNubs(TAtm *orig_atom)
{
  vector<TAtm*> *catoms = 0;
  vector<TAtm*> nmlist;
  vector<double*> nubxyz;
  double* nub;
  int i,j;

  // Step 1. 
  // Create a list of nub locations. Start by getting atom coordinates.
  const double *atxyz = orig_atom->coordinates();
  double x,y,z;
  x = atxyz[0];
  y = atxyz[1];
  z = atxyz[2];
  // Find default nub coordinates for this atom 
  // First we have to get the geometries...

  string geom = orig_atom->shapeString();
  ShapeData shapes;
  int numCoords = shapes.getNumberOfAtoms(geom.c_str());
  const double *coords = shapes.getGeometryCoordinates(geom.c_str());
  NULLPOINTEREXCEPTION(coords, string("No coordinates from shape class")+geom);

  // Need to get this stuff dynamically
  TPerTab tpt;
  static const float scale = 0.5;  
  int atomNum = orig_atom->atomicNumber();
  float radii = tpt.covalentRadius(atomNum);

  // "Lone" doesn't have any coords - the rest will
  double factor = 1.0;;
  for (int idx=0; idx<numCoords; idx++) {
    factor = scale * (radii / sqrt(pow(coords[idx*3],2) +
          pow(coords[(idx*3)+1],2) + pow(coords[(idx*3)+2],2)));

    nub = new double[3];
    nub[0] = x + factor * coords[idx*3];
    nub[1] = y + factor * coords[(idx*3)+1];
    nub[2] = z + factor * coords[(idx*3)+2];
    nubxyz.push_back(nub);
  }

  // Step 2. 
  // Rotate the nubs so that one of them lies along one of the atom
  // bonds (if there is one that isn't a metal-pi bond). If there is
  // more than one bond, just pick the first.
  nonMetalnonNubBonds(orig_atom,nmlist);
  int nmsize = nmlist.size();
  if (nmsize >= 1) {

    TAtm *batom = nmlist[0];

    MPoint oldVector, tmpVector, newVector, shift;
    MMatrix rMatrix, sMatrix;
    oldVector.xyz(orig_atom->coordinates());
    tmpVector.xyz(batom->coordinates());
    oldVector.subtract(tmpVector);

    newVector.xyz(orig_atom->coordinates());
    tmpVector.xyz(nubxyz[0]);
    newVector.subtract(tmpVector);
    rMatrix.alRotateMatrix(newVector, oldVector);

    //now we can get and effect the rotation matrix.
    shift.xyz(orig_atom->coordinates());
    shift.scale(-1);
    sMatrix.shiftMatrix(shift);
    transform(nubxyz,sMatrix);
    transform(nubxyz,rMatrix);
    shift.scale(-1);
    sMatrix.shiftMatrix(shift);
    transform(nubxyz,sMatrix);
  }

  // Step 3.
  // rotate the other bonds into place (if any)
  MPoint alignedBond, newBond, oldBond, origin;
  if (nmsize > 1) {
    // Get bond vector for bond that was aligned in previous step,
    // bond vector to one of the remaining nubs in the new atom,
    // and bond vector to one of unaligined bonds in the original
    // atom
    origin.xyz(orig_atom->coordinates());
    alignedBond.xyz(nubxyz[0]);
    alignedBond.subtract(origin);
    newBond.xyz(nubxyz[1]);
    newBond.subtract(origin);
    oldBond.xyz(nmlist[1]->coordinates());
    oldBond.subtract(origin);

    // We need to actually transform new atom. Calculate transformation
    // matrices as well as their corresponding inverses (where
    // necessary).
    MPoint xAxis(1.0,0.0,0.0), yAxis(0.0,1.0,0.0);
    MPoint shift = origin;
    MMatrix sMatrix, r1Matrix, r2Matrix, r3Matrix;
    MMatrix siMatrix, ri1Matrix, ri2Matrix;
    shift.scale(-1);
    sMatrix.shiftMatrix(shift);
    shift.scale(-1);
    siMatrix.shiftMatrix(shift);
    r1Matrix.alRotateMatrix(alignedBond,xAxis);
    ri1Matrix.alRotateMatrix(xAxis,alignedBond);
    MPoint newBond1 = r1Matrix.transformPoint(newBond);
    MPoint oldBond1 = r1Matrix.transformPoint(oldBond);
    //rotate old bond about x-axis until it lies xy plane
    MPoint oldBond1_yz(0.0,oldBond1.y(),oldBond1.z());
    r2Matrix.alRotateMatrix(oldBond1_yz,yAxis);
    ri2Matrix.alRotateMatrix(yAxis,oldBond1_yz);
    MPoint newBond2 = r2Matrix.transformPoint(newBond1);
    MPoint oldBond2 = r2Matrix.transformPoint(oldBond1);
    //the old bond is now lying in the xy plane. Calculate
    //the angle necessary to rotate the new bond into the
    //xy plane
    MPoint newBond2_yz(0.0,newBond2.y(),newBond2.z());
    r3Matrix.alRotateMatrix(newBond2_yz,yAxis);
    //apply transformation matrices to nonbonded nubs
    for (i=1; i<nubxyz.size(); i++) {
      MPoint tnub((nubxyz[i])[0],(nubxyz[i])[1],(nubxyz[i])[2]);
      MPoint tnub2 = sMatrix.transformPoint(tnub);
      MPoint tnub3 = r1Matrix.transformPoint(tnub2);
      MPoint tnub4 = r2Matrix.transformPoint(tnub3);
      MPoint tnub5 = r3Matrix.transformPoint(tnub4);
      tnub4 = ri2Matrix.transformPoint(tnub5);
      tnub3 = ri1Matrix.transformPoint(tnub4);
      tnub2 = siMatrix.transformPoint(tnub3);
      (nubxyz[i])[0] = tnub2.x();
      (nubxyz[i])[1] = tnub2.y();
      (nubxyz[i])[2] = tnub2.z();
    }
    // We now have the new nubs oriented in the correct direction. The
    // first two nubs in the new atom correspond to two bonds in the
    // original atom. We now need to see if there are any nubs that
    // correspond to additional bonds in the original atom. Note that
    // although this section contains a couple of double loops, in
    // practically all cases these loops will be small.
    if (nmsize > 2) {
      bool useNub[20];
      useNub[0] = false;
      useNub[1] = false;
      double overlap, ocheck;
      int nsize = nubxyz.size();
      if (nsize > 20) nsize = 20;
      for (i=2; i<nsize; i++)
        useNub[i] = true;
      for (i=2; i<nmsize; i++) {
        oldBond.xyz(nmlist[i]->coordinates());
        oldBond.subtract(origin);
        oldBond.normalize();
        for (j=0; j<nsize; j++) {
          newBond.xyz(nubxyz[j]);
          newBond.subtract(origin);
          newBond.normalize();
        }
        overlap = 0.0;
        int jat = 0;
        for (j=2; j<nsize; j++) {
          newBond.xyz(nubxyz[j]);
          newBond.subtract(origin);
          newBond.normalize();
          ocheck = newBond.dotProduct(oldBond);
          if (ocheck > overlap && useNub[j]) {
            overlap = ocheck;
            jat = j;
          }
        }
        useNub[jat] = false;
      }
      // switch nubs around so that unused nubs are at start of nubxyz
      double saveCoord[3];
      int icnt = 2;
      for (i=2; i<nsize; i++) {
        if (useNub[i] && icnt < nmsize) {
          for (j=i+1; j<nsize; j++) {
            if (useNub[i] && !useNub[j]) {
              saveCoord[0] = (nubxyz[i])[0];
              saveCoord[1] = (nubxyz[i])[1];
              saveCoord[2] = (nubxyz[i])[2];
              (nubxyz[j])[0] = saveCoord[0];
              (nubxyz[j])[1] = saveCoord[1];
              (nubxyz[j])[2] = saveCoord[2];
              icnt++;
              useNub[i] = false;
              useNub[j] = true;
            }
          }
        }
      }
    }
  }

  // Find nubs on original atom and reset the coordinates to new values
  catoms = orig_atom->connectedAtoms();
  for (i=0; i<catoms->size(); i++) {
    if ((*catoms)[i]->atomicNumber() == tpt.nubAtom()) {
      (*catoms)[i]->coordinates(nubxyz[nmsize]);
      nmsize++;
    }
  }

  // make sure to clean up nubs
  nmsize = nubxyz.size();
  for (i=0; i<nmsize; i++) delete [] nubxyz[i];
  nubxyz.clear();
  delete catoms;
}

void FragUtil::transform(vector<double*> list, MMatrix& matrix)
{
  int cnt  = list.size();
  if (cnt == 0) return;
  
  MPoint xyz1, xyz2;

  for (int idx=0; idx<cnt; idx++) {
    xyz1.xyz(list[idx]);
    // transform point
    xyz2 = matrix.transformPoint(xyz1);

    // set coordinates back
    (list[idx])[0] = xyz2.x();
    (list[idx])[1] = xyz2.y();
    (list[idx])[2] = xyz2.z();
  }
}
////////////////////////////////////////////////////////////////////////////
// Description
//   Create residues in a system, if none exist already.
////////////////////////////////////////////////////////////////////////////
void FragUtil::createResidues()
{
  if (p_frag->numResidues() > 0) return;
  char buf[81],atsym[3];
  char* cptr;
  TAtm *atm = 0;
  string rstring;
  int i,iat;

//
//  Fragment has no residues. Try and construct them
//  using molecular connectivity to define residues. Maintain the
//  the same relative order for atoms in the same residue.
//
  vector<bool> foundAtom;
  int natom = p_frag->numAtoms();
  if (natom > 0) {
    int j,k,l,m;
    map<int, TAtm*, less<int> > clusterAtoms;
    map<int, TAtm*, less<int> >::iterator nextAtom;
//
//  Initialize bitmask for determining whether atom has been found
//
    for (i = 0; i < natom; i++) {
      foundAtom.push_back(false);
    }
    int firstUnkn=0;
    int rescnt = 0;
    int atomcnt = 0;
    int acntsav;
    vector<int> newAtoms;
//
//   Create array corresponding to all elements in the periodic table.
//   Use this to come up with atom names for PDB file.
//
    TPerTab pertab;
    vector<int> ptable;
    int nelements =  pertab.numAtoms();
    for (i=0; i<nelements; i++) {
      ptable.push_back(0);
    }
    while (atomcnt < natom) {
//
//  Begin search for a new molecule (a molecule is defined as a bunch
//  of connected atoms).
//
      acntsav = atomcnt;
      newAtoms.clear();
      newAtoms.push_back(firstUnkn);
      atm = p_frag->atomRef(firstUnkn);
      foundAtom[firstUnkn] = true;
      clusterAtoms[firstUnkn] = atm;
      for (i=0; i<nelements; i++) {
        ptable[i] = 0;
      } 
      ptable[atm->atomicNumber()-1]++;
      bool newCluster = false;
      while (newAtoms.size() > 0 && !newCluster) {
        int nnew = newAtoms.size();
//
//  Scan through new atoms and find out what they are connected to.
//
        vector<int> veryNewAtoms;
        for (k = 0; k<nnew; k++) {
          if (newCluster) break;
          vector<TAtm*>* nlist = p_frag->connectedAtoms(newAtoms[k]);
          int nconnected = nlist->size();
          for (l = 0; l<nconnected; l++) {
            if (newCluster) break;
            atm = (*nlist)[l];
            if (!foundAtom[atm->index()]) {
              veryNewAtoms.push_back(atm->index());
              foundAtom[atm->index()] = true;
              atomcnt++;
//
// If atom is not a nub, then print it out to PDB file
//
              iat = atm->atomicNumber();
              ptable[iat-1]++;
//
// If there are more than 100 atoms of any one type in the molecule,
// start a new residue.
//
              if (ptable[iat-1] >= 100 && atm->atomicSymbol() != "Nub") {
                foundAtom[atm->index()] = false;
                atomcnt--;
                newCluster = true;
              } else {
                clusterAtoms[atm->index()] = atm;
              }
            }
          }
          delete nlist;
        }
//
//  Clear newAtoms array and replace it with indices in veryNewAtoms
//  array.
//
        newAtoms.clear();
        nnew = veryNewAtoms.size();
        for (k=0; k<nnew; k++) {
          newAtoms.push_back(veryNewAtoms[k]);
        }
        veryNewAtoms.clear();
      }
//
//  Complete cluster found. Use it to construct a new residue.
//
      atomcnt = acntsav;
      for (m=0; m<nelements; m++) {
         ptable[m] = 0;
      }
      rescnt++;
      if (rescnt >= 10000) rescnt = 1;
      Residue *res = new Residue("UNK ",rescnt);
      int nsize = clusterAtoms.size();
      if (nsize > 0) {
        p_frag->addResidue(res);
        nextAtom = clusterAtoms.find(firstUnkn);
        atm = (*nextAtom).second;
        res->firstAtomPtr(atm);
      }
      int atnum;
      for (i=0; i<nsize; i++) {
        iat = firstUnkn + i;
        nextAtom = clusterAtoms.find(iat);
//
//  If next atom is in residue add it to the cluster,
//  if not, start a new residue
//
        if (nextAtom != clusterAtoms.end()) {
          atm = (*nextAtom).second;
          atomcnt++;
          atnum = atm->atomicNumber();
          atm->atomResidue(res);
          ptable[atnum-1]++;
          atm->atomResidue(res);
          rstring = atm->atomicSymbol();
          cptr = buf;
//
//  Construct a name for this atom by combining the atomic
//  symbol with the current number of atoms of this type
//
          if (rstring.length()==1) {
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
          if (ptable[atnum-1] < 10) {
            sprintf(cptr,"%1d ",ptable[atnum-1]);
            cptr++;
            strncpy(cptr," ",1);
            cptr++;
          } else {
            sprintf(cptr,"%2d",ptable[atnum-1]);
            cptr += 2;
          }
          buf[4] = '\0';
          atm->atomName(buf);
        } else {
          firstUnkn = iat;
          clusterAtoms.clear();
          for (i=iat; i<natom; i++) {
            foundAtom[i] = false;
          }
          break;
        }
      }
//
//   Reset parameter to look for a new cluster
//
      for (m=0; m<nelements; m++) {
        ptable[m] = 0;
      }
      if (rescnt > 9999) rescnt = 0;
      clusterAtoms.clear();
      newCluster = false;
//
// Find index of first atom that has not been assigned to a cluster
//
      for (j=firstUnkn; j<natom; j++) {
        if (!foundAtom[j]) {
          firstUnkn = j;
          break;
        }
      }
    }
  }
}


/**
 * addToNub is part of the basic building capability.
 * It executes the action of creating a new atom connected to an exising
 * nub (or its parent).
 * Moved here from viz code.
 */
TAtm *FragUtil::addToNub( TAtm *nubParent, TAtm *theNub,
                  const char *element, const char *geom, double order)
{

  // Code got moved around - make variable so rest of code doesn't change
  Fragment *frag = p_frag;

  if (strcmp(geom,"Lone")== 0) {
    // When I made this static, lost capability to set specific errors here
    //setErrorMessage("A Lone atoom cannot be bonded to a nub");
    return 0;
  }

  // Just add at the origin
  TAtm *newAtom = this->addAtLocation(element,geom,0,0,0,nubParent);
  double dihedral = s_shapes.getDihedralOffset(geom);

  TAtm *atom2 = newAtom ;
  TAtm *nub2 = frag->childNub(atom2);

  // Generate list of atoms that have to be moved by the bonding process
  vector<TAtm*> *a1list = nubParent->connectedAtoms();
  vector<TAtm*> *a2list = atom2->connectedAtoms();
  a2list->push_back(atom2);

  // Now bond the new thing to the selected one.
  this->bondParts(nubParent,theNub,atom2,nub2,order,a2list);

  // Now set the dihedral angle.  We need to pick any nub connected to
  // atom2 (besides the one that just got deleted by the bonding
  // operation), and on the parent side, we'll pick the one with the
  // largest VDR.

  // First find the atom with greatest VWR on atom1 side
  TAtm *pmaxvdr = NULL;
  // theNub was deleted by bonding
  vector<TAtm*>::iterator it = a1list->begin();
  it = find(a1list->begin(), a1list->end(), theNub);
  if (it != a1list->end()) {
    // do we need an erase in here?
    a1list->erase(it);
  }
  TPerTab tpt;
  double vwr=0.0;
  double tvwr;
  TAtm *atom;
  for (int idx=a1list->size()-1; idx>=0; idx--) {
    atom = (*a1list)[idx];
    tvwr = tpt.vwr(tpt.atomicNumber(atom->atomicSymbol()));
    if (tvwr > vwr) {
      vwr = tvwr;
      pmaxvdr = atom;
    }
  }
  // Now find any nub on atom2 side
  TAtm *nub = NULL;
  // got deleted by bonding operation
  it = a2list->begin();
  while(it != a2list->end()) {
    if ((*it) == nub2) {
      // do we need an erase in here?
      a2list->erase(it);
      break;
    }
    it++;
  }
  if (a2list->size() > 1) {
    // must have a nub and it must be the first thing
    nub = (*a2list)[0];
  }

  // Now finally, if we have 4 atoms, we can set the dihedral
  if (pmaxvdr != 0 && nub != 0) {
    double curAngle = Measures::computeDihedral(pmaxvdr, nubParent, atom2, nub);
    double diff = dihedral - curAngle;
    if (diff != 0) frag->rotateAboutBond(nubParent,atom2,diff);
  }


  // If new atom is hydrogen, see if it can be named
  if (strcmp(element,"H")==0) {
    newAtom->atomName(newAtom->getHydrogenName().c_str());
  }
  delete a1list;
  delete a2list;
  return newAtom;

}



/**
 *  bond two atom groups that already exist within the fragment.
 *
 *  The method behind this call goes something like....
 *  1.  Get the ideal bond length for atom1-atom2 bond which is based on
 *      covalent radii.
 *  2.  Work out the ideal xyz for atom2, along atom1-nub1 vector
 *  3.  Make a shift matrix to move to the origin
 *  4.  Make a rotation matrix to align atom2-nub2 with nub1-atom1 vector
 *  5.  Make a shift matrix to move to ideal xyz2 position
 *  6.  Instantiate all matrices on all the bonded atoms in fragment2
 *  7.  add a bond from atom1 to atom2
 *  8.  delete nub1 and nub2
 */
void FragUtil::bondParts
(
   TAtm* mainAtm1,
   TAtm* nubAtm1,
   TAtm* mainAtm2,
   TAtm* nubAtm2,
   double order,
   vector<TAtm*> *a2list
)
{
  // Code got moved around - make variable so rest of code doesn't change
  Fragment *frag = p_frag;

  TPerTab tpt;

  int atomicNum1 = tpt.atomicNumber(mainAtm1->atomicSymbol());
  int atomicNum2 = tpt.atomicNumber(mainAtm2->atomicSymbol());

  // Step #1 
  double idealBondLength = tpt.covalentRadius(atomicNum1) +
                      tpt.covalentRadius(atomicNum2);
  // Adjust bond length to experimental value in the special case that two DNA
  // residues are being joined
  TResItem::Classification type1, type2;
  if (frag->numResidues() > 0) {
    TResTab restab;
    type1 = restab.verify(mainAtm1->getResidue()->name().c_str());
    type2 = restab.verify(mainAtm2->getResidue()->name().c_str());
    if (type1 == TResItem::DNA && type2 == TResItem::DNA) {
      if ((mainAtm1->atomName() == " P  " && mainAtm2->atomName() == " O3*") ||
          (mainAtm1->atomName() == " O3*" && mainAtm2->atomName() == " P  ")) {
        idealBondLength = 1.5945;
      }
    }
  }

  // Step #2 - what is lenth of main1 ->nub1 bond?
  MPoint  atom1xyz, nub1xyz;
  double bondLength, ratio;

  atom1xyz.xyz(mainAtm1->coordinates());

  nub1xyz.xyz(nubAtm1->coordinates());

  // get the offset of the nub from the central atom, and the length
  nub1xyz.subtract(atom1xyz);
  bondLength = nub1xyz.length();

  // what is the ratio of the existing bond and the new one
  ratio = idealBondLength/bondLength;

  // the new atom offset is thus the nub1xyz offset scaled by
  // this ratio
  nub1xyz.scale(ratio);

  // and the new atom position is the offset plus the
  // original atom position
  nub1xyz.add(atom1xyz);

  // Step #3  Shift new atoms to correct location
  MPoint atom2xyz;
  MMatrix shift;

  // How much do we have to shift by: from atom2xyz to nub1xyz
  atom2xyz.xyz(mainAtm2->coordinates());
  nub1xyz.subtract(atom2xyz);

  // now create a shift matrix and apply it
  shift.shiftMatrix(nub1xyz);
  frag->transform(*a2list, shift);

  // Step #4 - rotation matrix to align a1/n1 with a2/n2
  // We have to work out a vector to vector rotation matrix
  MPoint vector1, vector1n, vector2, sh;
  MPoint tmp;
  MMatrix rotate;

  // vector1 is the atom2-nub2 vector
  vector1.xyz(mainAtm2->coordinates());
  tmp.xyz(nubAtm2->coordinates());
  vector1.subtract(tmp);

  // vector2 is the nub1-atom1 vector
  vector2.xyz(nubAtm1->coordinates());
  tmp.xyz(mainAtm1->coordinates());
  vector2.subtract(tmp);

  rotate.vvRotateMatrix(vector1, vector2);

  // Step #5,6
  sh.xyz(mainAtm2->coordinates());
  sh.scale(-1);
  shift.shiftMatrix(sh);
  frag->transform(*a2list, shift);
  frag->transform(*a2list, rotate);
  sh.scale(-1);
  shift.shiftMatrix(sh);
  frag->transform(*a2list, shift);

  // Step #7 - Create a bond between atom1 and 2
  TBond *bond = new TBond(mainAtm1,mainAtm2,order);
  bond->displayStyle(frag->getMainDisplayStyle()) ;

  // Step #8. Delete nubs 1 and 2
  // must delete high number first, then lower index
  int nub1 = ((Fragment*)frag)->atomIndex(*nubAtm1);
  int nub2 = ((Fragment*)frag)->atomIndex(*nubAtm2);
  if (nub1 > nub2) {
    frag->deleteAtom(nub1);
    frag->deleteAtom(nub2);
  } else {
    frag->deleteAtom(nub2);
    frag->deleteAtom(nub1);
  }

  // Phew, finished !

}


/**
 * Create bond between a fragment and an atom list.
 * @param atoms - atoms to be relocated as part of the bonding action
 * @param basenub
 * @param baseparent
 * @param fragparent
 * @param fragnub
 */
void FragUtil::doBond( vector<TAtm*>& atoms,
      TAtm *basenub, TAtm *baseparent,
      TAtm *fragparent, TAtm *fragnub)
{
   // Code got moved around - make variable so rest of code doesn't change
   Fragment *frag = p_frag;


   // Compute angle
   // Bruce's algorithm for figuring out a reasonable angle is to
   // check all the atoms directly connected to baseparent and
   // fragparent and pick out the ones with the largest VWR for
   // each.  The angle for those 4 atoms should be 180.  It should
   // produce reasonable results in most cases.  Robert also said it was
   // probably a reasonable approach though a localized clean would be
   // better.
   vector<TAtm*> *pchildren = baseparent->connectedAtoms();
   vector<TAtm*> *fchildren = fragparent->connectedAtoms();
   int idx;
   // remove the nubs which will go away
   for (idx=pchildren->size()-1; idx>=0; idx--) {
      if ((*pchildren)[idx] == basenub)  {
         vector<TAtm*>::iterator it = pchildren->begin();
         it += idx;
         pchildren->erase(it);
         break;
      }
   }
   for (idx=fchildren->size()-1; idx>=0; idx--) {
      if ((*fchildren)[idx] == fragnub)  {
         vector<TAtm*>::iterator it = fchildren->begin();
         it += idx;
         fchildren->erase(it);
         break;
      }
   }

   TResItem::Classification ftype;
   TResItem::Classification ptype;
   TResTab restab;
   if (frag->numResidues() > 0) {
      ptype = restab.verify(baseparent->getResidue()->name().c_str());
      ftype = restab.verify(fragparent->getResidue()->name().c_str());
   }
   bool AAangle  = false;
   bool DNAangle = false;
   bool RNAangle = false;
   if (ptype == TResItem::AminoAcid && ftype == TResItem::AminoAcid)
      AAangle = true;
   if ((ptype == TResItem::DNA && ftype == TResItem::DNA) ||
         (ptype == TResItem::RNAorDNA && ftype == TResItem::DNA) ||
         (ptype == TResItem::DNA && ftype == TResItem::RNAorDNA))
      DNAangle = true;
   if ((ptype == TResItem::RNA && ftype == TResItem::RNA) ||
         (ptype == TResItem::RNAorDNA && ftype == TResItem::RNA) ||
         (ptype == TResItem::RNA && ftype == TResItem::RNAorDNA))
      RNAangle = true;

   FragUtil fragutil(frag);
   fragutil.bondParts(baseparent,basenub,fragparent,fragnub,1.0,&atoms);
   vector<TAtm*>::iterator it = atoms.begin();
   while (it != atoms.end()) {
      if ((*it) == fragnub) {
         // Do we need a delete here?
         atoms.erase(it);
         break;
      }
      it++;
   }
   while (it != atoms.end()) {
      if ((*it) == basenub) {
         // Do we need a delete here?
         atoms.erase(it);
         break;
      }
      it++;
   }

   TAtm* patm = (TAtm*)0;
   TAtm* fatm = (TAtm*)0;
   double newangle = 180.0;

   // Check to see if we have a special case. If we do use, special angle;
   // if not, then go to the default algorithm.

   if (AAangle) {
      if ((fragparent->atomName() == " N  " &&
               baseparent->atomName() == " C  ") ||
            (fragparent->atomName() == " C  " &&
             baseparent->atomName() == " N  ")) {
         for (idx=pchildren->size()-1; idx>=0; idx--) {
            if ((*pchildren)[idx]->atomName() == " CA ") patm = (*pchildren)[idx];
         }
         for (idx=fchildren->size()-1; idx>=0; idx--) {
            if ((*fchildren)[idx]->atomName() == " CA ") fatm = (*fchildren)[idx];
         }
         if (patm == (TAtm*)0 || fatm == (TAtm*)0) AAangle = false;
      } else {
         AAangle = false;
      }
   }
   if (DNAangle) {
      if (fragparent->atomName() == " O3*" &&
            baseparent->atomName() == " P  ") {
         for (idx=pchildren->size()-1; idx>=0; idx--) {
            if ((*pchildren)[idx]->atomName() == " O5*") patm = (*pchildren)[idx];
         }
         for (idx=fchildren->size()-1; idx>=0; idx--) {
            if ((*fchildren)[idx]->atomName() == " C3*") fatm = (*fchildren)[idx];
         }
         newangle = -95.2;
         if (patm == (TAtm*)0 || fatm == (TAtm*)0) DNAangle = false;
      } else if (fragparent->atomName() == " P  " &&
            baseparent->atomName() == " O3*") {
         for (idx=pchildren->size()-1; idx>=0; idx--) {
            if ((*pchildren)[idx]->atomName() == " C3*") patm = (*pchildren)[idx];
         }
         for (idx=fchildren->size()-1; idx>=0; idx--) {
            if ((*fchildren)[idx]->atomName() == " O5*") fatm = (*fchildren)[idx];
         }
         newangle = -95.2;
         if (patm == (TAtm*)0 || fatm == (TAtm*)0) DNAangle = false;
      } else {
         DNAangle = false;
      }
   }
   if (RNAangle) {
      if (fragparent->atomName() == " O3*" &&
            baseparent->atomName() == " P  ") {
         for (idx=pchildren->size()-1; idx>=0; idx--) {
            if ((*pchildren)[idx]->atomName() == " O5*") patm = (*pchildren)[idx];
         }
         for (idx=fchildren->size()-1; idx>=0; idx--) {
            if ((*fchildren)[idx]->atomName() == " C3*") fatm = (*fchildren)[idx];
         }
         newangle = -73.6;
         if (patm == (TAtm*)0 || fatm == (TAtm*)0) RNAangle = false;
      } else if (fragparent->atomName() == " P  " &&
            baseparent->atomName() == " O3*") {
         for (idx=pchildren->size()-1; idx>=0; idx--) {
            if ((*pchildren)[idx]->atomName() == " C3*") patm = (*pchildren)[idx];
         }
         for (idx=fchildren->size()-1; idx>=0; idx--) {
            if ((*fchildren)[idx]->atomName() == " O5*") fatm = (*fchildren)[idx];
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
      for (idx=pchildren->size()-1; idx>=0; idx--) {
         atom = (*pchildren)[idx];
         tvwr = tpt.vwr(tpt.atomicNumber(atom->atomicSymbol()));
         if (tvwr > vwr) {
            vwr = tvwr;
            patm = atom;
         }
      }
      vwr = 0.;
      for (idx=fchildren->size()-1; idx>=0; idx--) {
         atom = (*fchildren)[idx];
         tvwr = tpt.vwr(tpt.atomicNumber(atom->atomicSymbol()));
         if (tvwr > vwr) {
            vwr = tvwr;
            fatm = atom;
         }
      }
   }
   // It could be that there weren't enough objects to make a dihedral
   // in which case we choose to do nothing
   if (patm && baseparent && fatm && fragparent) {

      // Didn't use  this because it appears to have a memory problem
      // plus we already know what atoms to move.
      //frag->dihedral(patm,baseparent,fragparent,fatm,newangle);

      double curAngle = Measures::computeDihedral(patm,baseparent,fragparent,fatm);
      double diff = newangle - curAngle;
      MMatrix rotateM;
      MPoint p1, p2;
      p1.xyz(baseparent->coordinates());
      p2.xyz(fragparent->coordinates());
      rotateM.rotateLineMatrix(p1,p2,diff);
      frag->transform(atoms,rotateM);
   }
   delete pchildren;
   delete fchildren;

}

/**
 * Load fragment in one of main supported formats.
 */
bool FragUtil::load( istream& is,
      const string& type,
      double mult,
      bool genBOs,
      int selectMdl,
      const string& selectAlt,
      const string& selectChain)
{
   bool ret = false;

   // Code got moved around - make variable so rest of code doesn't change
   Fragment *frag = p_frag;

   if (STLUtil::compareString(type,string("XYZ"))==0) {
      ret = frag->restoreXYZ(is,mult,genBOs);
   } else if (STLUtil::compareString(type,string("MVM"))==0) {
      ret = frag->restoreMVM(is,mult,genBOs);
   } else if (STLUtil::compareString(type,string("CAR"))==0) {
      ret = frag->restoreCAR(is,mult,genBOs);
   } else if (STLUtil::compareString(type,string("PDB"))==0) {
      bool groupFlg = true;
      bool readBonds = false;

      ret = frag->restorePDB(is,mult,genBOs,selectMdl,selectAlt,groupFlg,
            readBonds, selectChain);
   }
   return ret;

}


