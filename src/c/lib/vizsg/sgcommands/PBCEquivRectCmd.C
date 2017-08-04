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

#include "viz/PBCEquivRectCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/SGLattice.H"

PBCEquivRectCmd::PBCEquivRectCmd(const string& name, Receiver * receiver, SGFragment *base)
    : PBCCmd(name, receiver, base)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
PBCEquivRectCmd::~PBCEquivRectCmd()
{

}



void PBCEquivRectCmd::init()
{
}



bool PBCEquivRectCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   if (frag) {

     // Check for valid LatticeDef object
     LatticeDef *lattice = frag->getLattice();
     if (lattice) {
       ret = true;

       MPoint a, b, c;
       // Get basis vectors for unit cell.
       vector<MPoint> *basis = lattice->toVectors();
       a.xyz((*(basis))[0]);
       b.xyz((*(basis))[1]);
       c.xyz((*(basis))[2]);
       delete basis;

       // Construct new value of c vector
       MPoint c_new = a.crossProduct1(b);
       c_new.normalize();
       double rdot = c_new.dotProduct(c);
       c_new.scale(rdot);

       // Construct new value of b vector
       MPoint b_new = a.crossProduct1(c_new);
       b_new.normalize();
       rdot = b_new.dotProduct(b);
       b_new.scale(rdot);

       b = b_new;
       c = c_new;

       lattice->assign(a,b,c);

       frag->convertToFractionalCoords(false);
      
       const double *crd;
       double f1, f2, f3;
       TAtm *atm;
       int i, j, k;

       int natoms = frag->numAtoms();

       for (i=0; i<natoms; i++) {
         atm = frag->atomRef(i);
         crd = atm->coordinates();

         f1 = crd[0];
         f2 = crd[1];
         f3 = crd[2];

         f1 = f1-(double)(int)f1;
         if (f1<0.0) f1 += 1.0;
         f2 = f2-(double)(int)f2;
         if (f2<0.0) f2 += 1.0;
         f3 = f3-(double)(int)f3;
         if (f3<0.0) f3 += 1.0;

         atm->coordinates(f1,f2,f3);
       }
       frag->convertFromFractionalCoords();

       // Construct a rotation such that the c axis in the transformed
       // unit cell is parallel to the z axis and the lattice
       // vector a is lying  along the x axis.

       MPoint zn(0.0,0.0,1.0);
       c_new = c;
       c_new.normalize();

       double cz = c_new.dotProduct(zn);
       double pi = 4.0*atan(1.0);
       MPoint c_prll, c_perp;
       c_prll.xyz(0.0,0.0,cz);
       c_perp.xyz(c_new.x(),c_new.y(),0.0);
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
       MPoint a_new;
       a_new.x(r3[0][0]*a.x()+r3[0][1]*a.y()+r3[0][2]*a.z());
       a_new.y(r3[1][0]*a.x()+r3[1][1]*a.y()+r3[1][2]*a.z());
       a_new.z(0.0);
       a_new.normalize();
       theta = acos(a_new.x());
       if (a_new.y() < 0.0) theta = 2.0*pi - theta;
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
       a_new.x(r[0][0]*a.x()+r[0][1]*a.y()+r[0][2]*a.z());
       a_new.y(r[1][0]*a.x()+r[1][1]*a.y()+r[1][2]*a.z());
       a_new.z(r[2][0]*a.x()+r[2][1]*a.y()+r[2][2]*a.z());
       b_new.x(r[0][0]*b.x()+r[0][1]*b.y()+r[0][2]*b.z());
       b_new.y(r[1][0]*b.x()+r[1][1]*b.y()+r[1][2]*b.z());
       b_new.z(r[2][0]*b.x()+r[2][1]*b.y()+r[2][2]*b.z());
       c_new.x(r[0][0]*c.x()+r[0][1]*c.y()+r[0][2]*c.z());
       c_new.y(r[1][0]*c.x()+r[1][1]*c.y()+r[1][2]*c.z());
       c_new.z(r[2][0]*c.x()+r[2][1]*c.y()+r[2][2]*c.z());
       lattice->assign(a, b, c);

#if 1
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

#endif
       frag->deleteAllBonds();
       frag->addCovalentBonds();
       frag->touchNumbers();

       sg->touchChemDisplay();
       sg->touchLattice();
       EventDispatcher::getDispatcher().publish(Event("GeomChange"));
     }
   }
   return ret;
}

