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

#include "viz/TransformCellCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/SGLattice.H"

TransformCellCmd::TransformCellCmd(const string& name, 
                                   Receiver * receiver)
  : FragCloneCmd(name, receiver)
{
  init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
TransformCellCmd::~TransformCellCmd()
{

}



void TransformCellCmd::init()
{
  // Basic parameters for slicing operation. The rest will be derived from
  // fragment and lattice objects
  addParameter(new CommandParameter("h",1));
  addParameter(new CommandParameter("k",1));
  addParameter(new CommandParameter("l",1));
}



bool TransformCellCmd::execute() throw(EcceException)
{
  bool ret = false;
  //cout << "TRANSFORM\n" << endl;

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();
  if (frag) {
    LatticeDef *lattice = frag->getLattice();

    if (lattice) {
      cloneFragment(frag);

      int h = getParameter("h")->getInteger();
      int k = getParameter("k")->getInteger();
      int l = getParameter("l")->getInteger();

      int h1,k1,l1,h2,k2,l2;

      ret = lattice->getLatticePlane(h,k,l,h1,k1,l1,h2,k2,l2);
      //cout<<"\nNumber of atoms after transform: "<<frag->numAtoms()<<endl;

      if (ret) {
        // Create a large chunk of periodic lattice based on the values
        // hkl, h1k1l1, h2k2l2
        int hmin = h;
        if (h1<hmin) hmin = h1;
        if (h2<hmin) hmin = h2;
        int hmax = h;
        if (h1>hmax) hmax = h1;
        if (h2>hmax) hmax = h2;
        int kmin = k;
        if (k1<kmin) kmin = k1;
        if (k2<kmin) kmin = k2;
        int kmax = k;
        if (k1>kmax) kmax = k1;
        if (k2>kmax) kmax = k2;
        int lmin = l;
        if (l1<lmin) lmin = l1;
        if (l2<lmin) lmin = l2;
        int lmax = l;
        if (l1>lmax) lmax = l1;
        if (l2>lmax) lmax = l2;
        //cout<< "h k l: "<<h<<" "<<k<<" "<<l<<endl;
        //cout<< "h1 k1 l1: "<<h1<<" "<<k1<<" "<<l1<<endl;
        //cout<< "h2 k2 l2: "<<h2<<" "<<k2<<" "<<l2<<endl;
        if (hmin<0) hmin--;
        if (kmin<0) kmin--;
        if (lmin<0) lmin--;
        //cout<<"hmin: "<<hmin<<" hmax: "<<hmax<<endl;
        //cout<<"kmin: "<<kmin<<" kmax: "<<kmax<<endl;
        //cout<<"lmin: "<<lmin<<" lmax: "<<lmax<<endl;

        // replicate lattice
        vector<MPoint> *basis = lattice->toVectors();
        int natoms = frag->numAtoms();
        vector<TAtm*> *atoms = frag->atoms();
        vector<TAtm*> base_atoms;
        int i,j,m,n;

        // Copy current atoms in fragment
        for (n=0; n<natoms; n++) {
          base_atoms.push_back((*atoms)[n]);
        }

        frag->deleteAllBonds();
        MPoint newCoord;
        MPoint offset, offseti, offsetj, offsetm;
        // loop over all replicas
        for (i=hmin; i<=hmax; i++) { 
          offseti = (*basis)[0];
          offseti.scale((double)i);
          for (j=kmin; j<=kmax; j++) { 
            offsetj = (*basis)[1];
            offsetj.scale((double)j);
            for (m=lmin; m<=lmax; m++) { 
              if (i != 0 || j != 0 || m != 0) {
                offsetm = (*basis)[2];
                offsetm.scale((double)m);
                offset = offseti;
                offset.add(offsetj);
                offset.add(offsetm);
                for (n=0; n<natoms; n++) {
                  const double *coord = base_atoms[n]->coordinates();
                  TAtm *atom = new TAtm;
                  *atom = *base_atoms[n];
                  newCoord.xyz(coord[0],coord[1],coord[2]);
                  newCoord.add(offset);
                  atom->coordinates(newCoord.x(),newCoord.y(),newCoord.z());
                  frag->addAtom(atom);
                }
              }
            }
          }
        }
        //cout<<"\nNumber of atoms after replication: "<<frag->numAtoms()<<endl;

        MPoint origin;
        origin = lattice->getLatticeCorner();

        // Transform to new unit cell
        lattice->transformCell(h,k,l,h1,k1,l1,h2,k2,l2);

        //Slice out extraneous atoms
        basis = lattice->toVectors();
        double zero = 0.0;
        double one = 1.0;
        MPoint normal;
        normal = (*basis)[1].crossProduct1((*basis)[2]);
        normal.normalize();
        if (normal.dotProduct((*basis)[0])<0.0) normal.scale(-1.0);
        frag->slice(normal,origin,zero,one);
        normal = (*basis)[0].crossProduct1((*basis)[2]);
        if (normal.dotProduct((*basis)[1])<0.0) normal.scale(-1.0);
        normal.normalize();
        frag->slice(normal,origin,zero,one);
        normal = (*basis)[0].crossProduct1((*basis)[1]);
        if (normal.dotProduct((*basis)[2])<0.0) normal.scale(-1.0);
        normal.normalize();
        frag->slice(normal,origin,zero,one);

        // Remove extra atoms. Start by finding fractional coordinates
        // of all atoms in the same unit cell
        vector<MPoint> *reciprocal = lattice->toReciprocalVectors();
        frag->convertToFractionalCoords(false);
        natoms = frag->numAtoms();
        vector<double> f1(natoms);
        vector<double> f2(natoms);
        vector<double> f3(natoms);
        TAtm *atm;
        const double *crd;
        for (i=0; i<natoms; i++) {
          atm = frag->atomRef(i);
          crd = atm->coordinates();
          f1[i] = crd[0];
          f2[i] = crd[1];
          f3[i] = crd[2];
          f1[i] = f1[i]-(double)(int)f1[i];
          if (f1[i]<0.0) f1[i] += 1.0;
          f2[i] = f2[i]-(double)(int)f2[i];
          if (f2[i]<0.0) f2[i] += 1.0;
          f3[i] = f3[i]-(double)(int)f3[i];
          if (f3[i]<0.0) f3[i] += 1.0;
        }

        // Compare fractional coordinates, looking for duplicates
        vector<bool> check(natoms,true);
        double eps = 1.0e-10;
        double dx, dy, dz, dr;
        for (i=0; i<natoms; i++) {
          if (check[i]) {
            for (j=i+1; j<natoms; j++) {
              dx = f1[i] - f1[j];
              dy = f2[i] - f2[j];
              dz = f3[i] - f3[j];
              dr = sqrt(dx*dx + dy*dy + dz*dz);
              if (dr < eps) check[j] = false;
            }
          }
        }

        // delete duplicate atoms and reset coordinates of remaining atoms
        // using fractional coordinates
        for (i=natoms-1; i>=0; i--) {
          if (!check[i]) {
            frag->deleteAtom(i);
          } else {
            atm = frag->atomRef(i);
            atm->coordinates(f1[i],f2[i],f3[i]);
          }
        }
        frag->convertFromFractionalCoords();
        //cout<<"Number of atoms after reduction: "<<frag->numAtoms()<<endl;

        // Construct a rotation such that the 001 plane in the transformed
        // unit cell is perpendicular to the z axis and the lattice
        // vector a is lying in the xz plane.

        // Get normal vector to 001 plane
        MPoint c = (*reciprocal)[2];
        c.normalize();

        MPoint zn(0.0,0.0,1.0);

        double cz = c.dotProduct(zn);
        double pi = 4.0*atan(1.0);
        MPoint c_prll, c_perp;
        c_prll.xyz(0.0,0.0,cz);
        c_perp.xyz(c.x(),c.y(),0.0);
        c_perp.normalize();
        // First rotate c_perp about the z axis until it is in the xz plane
        double theta = acos(c_perp.x());
        if (c_perp.y() < 0.0) theta = 2.0*pi - theta;
        double r1[3][3];
        r1[0][0] = cos(theta);
        r1[0][1] = sin(theta);
        r1[0][2] = 0.0;
        r1[1][0] = -sin(theta);
        r1[1][1] = cos(theta);
        r1[1][2] = 0.0;
        r1[2][0] = 0.0;
        r1[2][1] = 0.0;
        r1[2][2] = 1.0;
        // rotate c_prll about the y axis until it lies along the z axis
        theta = acos(c_prll.z());
        double r2[3][3];
        r2[0][0] = cos(theta);
        r2[0][1] = 0.0;
        r2[0][2] = -sin(theta);
        r2[1][0] = 0.0;
        r2[1][1] = 1.0;
        r2[1][2] = 0.0;
        r2[2][0] = sin(theta);
        r2[2][1] = 0.0;
        r2[2][2] = cos(theta);
        // Construct compound rotation;
        double r3[3][3];
        for (i=0; i<3; i++) {
          for (j=0; j<3; j++) {
            r3[i][j] = 0.0;
            for (k=0; k<3; k++) {
              r3[i][j] += r2[i][k]*r1[k][j];
            }
          }
        }

        // rotate system about z axis so that basis vector a is in the xz plane 
        MPoint a,b;
        a.x(r3[0][0]*((*basis)[0]).x()+r3[0][1]*((*basis)[0]).y()+r3[0][2]*((*basis)[0]).z());
        a.y(r3[1][0]*((*basis)[0]).x()+r3[1][1]*((*basis)[0]).y()+r3[1][2]*((*basis)[0]).z());
        a.z(0.0);
        a.normalize();
        theta = acos(a.x());
        if (a.y() < 0.0) theta = 2.0*pi - theta;
        r1[0][0] = cos(theta);
        r1[0][1] = sin(theta);
        r1[0][2] = 0.0;
        r1[1][0] = -sin(theta);
        r1[1][1] = cos(theta);
        r1[1][2] = 0.0;
        r1[2][0] = 0.0;
        r1[2][1] = 0.0;
        r1[2][2] = 1.0;

        // Construct complete rotation
        double r[3][3];
        for (i=0; i<3; i++) {
          for (j=0; j<3; j++) {
            r[i][j] = 0.0;
            for (k=0; k<3; k++) {
              r[i][j] += r1[i][k]*r3[k][j];
            }
          }
        }

        // Transform basis vectors and atom coordinates
        a = (*basis)[0];
        b = (*basis)[1];
        c = (*basis)[2];
        
        a.x(r[0][0]*((*basis)[0]).x()+r[0][1]*((*basis)[0]).y()+r[0][2]*((*basis)[0]).z());
        a.y(r[1][0]*((*basis)[0]).x()+r[1][1]*((*basis)[0]).y()+r[1][2]*((*basis)[0]).z());
        a.z(r[2][0]*((*basis)[0]).x()+r[2][1]*((*basis)[0]).y()+r[2][2]*((*basis)[0]).z());
        b.x(r[0][0]*((*basis)[1]).x()+r[0][1]*((*basis)[1]).y()+r[0][2]*((*basis)[1]).z());
        b.y(r[1][0]*((*basis)[1]).x()+r[1][1]*((*basis)[1]).y()+r[1][2]*((*basis)[1]).z());
        b.z(r[2][0]*((*basis)[1]).x()+r[2][1]*((*basis)[1]).y()+r[2][2]*((*basis)[1]).z());
        c.x(r[0][0]*((*basis)[2]).x()+r[0][1]*((*basis)[2]).y()+r[0][2]*((*basis)[2]).z());
        c.y(r[1][0]*((*basis)[2]).x()+r[1][1]*((*basis)[2]).y()+r[1][2]*((*basis)[2]).z());
        c.z(r[2][0]*((*basis)[2]).x()+r[2][1]*((*basis)[2]).y()+r[2][2]*((*basis)[2]).z());
        lattice->assign(a, b, c);

        natoms = frag->numAtoms();

        double new_coord[3];
        for (k=0; k<natoms; k++) {
          atm = frag->atomRef(k);
          crd = atm->coordinates();
          for (i=0; i<3; i++) {
            new_coord[i] = 0.0;
            for (j=0; j<3; j++) {
              new_coord[i] += r[i][j]*crd[j];
            }
          }
          atm->coordinates(new_coord[0],new_coord[1],new_coord[2]);
        }
        delete reciprocal;
        delete basis;

        frag->deleteAllBonds();
        frag->addCovalentBonds();
      
        frag->touchNumbers();
        sg->touchChemDisplay();
        sg->touchLattice();
        // event notification or vcommand properties
        EventDispatcher::getDispatcher().publish(Event("GeomChange"));
        EventDispatcher::getDispatcher().publish(Event("LatticeChange"));
      }
    }

  }
  return ret;
}

/**
 * Undo the command.
 * Not sure if we'll need to override superclass or not....
 */
void TransformCellCmd::undo()
{
   //FragCloneCmd::undo();
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   //hack - should this be copied over in fragment?
   DisplayStyle ds = sg->getFragment()->getMainDisplayStyle();

   if (p_undoFrag != 0) {
      restoreFrag(new SGFragment(*p_undoFrag), ds);
      SGFragment *sgfrag = sg->getFragment();

      string replicate;
      replicate = "replicate";
      EventDispatcher::getDispatcher().publish(Event("GeomChange",replicate));
      // We could have possibly removed the first add frag, resulting in no
      // residues (blank slate)
      if (sgfrag->numResidues() > 0) {
        EventDispatcher::getDispatcher().publish(Event("ResidueChange"));
      }
      EventDispatcher::getDispatcher().publish(Event("LatticeChange"));
   }
}
